/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file season.cpp Visual seasons.
 *
 * Two purely visual, client side effects that follow the calendar:
 *  - In the temperate climate the ground, roads, rails, bridges and stations are drawn
 *    with snow in winter. During November the snow line creeps down from the hills to
 *    sea level, during March it retreats again. Only the drawing code looks at this;
 *    the map, the tile loop and the economy are not touched, so it is safe in
 *    multiplayer and for savegames.
 *  - In every climate the viewport light gets a slight seasonal tone: fresh green in
 *    spring, warm in autumn, cold in winter.
 */

#include "stdafx.h"
#include "season.h"
#include "settings_type.h"
#include "gfx_func.h"
#include "tile_type.h"
#include "timer/timer_game_calendar.h"

#include "safeguards.h"

/** Height level above which the ground is snowy at the start of November and the end of March. */
static constexpr int SEASON_SNOW_TOP = 8;

/** The snow line currently used for drawing; #SEASON_NO_SNOW when there is no seasonal snow. */
static uint8_t _season_snow_line = SEASON_NO_SNOW;

/**
 * Compute the seasonal snow line from the calendar.
 * @return Height level at or above which tiles are drawn snowy, or #SEASON_NO_SNOW.
 */
static uint8_t ComputeSeasonSnowLine()
{
	if (!_settings_client.gui.seasons || _settings_game.game_creation.landscape != LandscapeType::Temperate) return SEASON_NO_SNOW;

	TimerGameCalendar::YearMonthDay ymd = TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date);
	switch (ymd.month) {
		case 10: return static_cast<uint8_t>(SEASON_SNOW_TOP - SEASON_SNOW_TOP * (ymd.day - 1) / 29); // November: snow comes down
		case 11:
		case 0:
		case 1: return 0; // December to February: everything is white
		case 2: return static_cast<uint8_t>(SEASON_SNOW_TOP * (ymd.day - 1) / 30); // March: snow retreats
		default: return SEASON_NO_SNOW;
	}
}

/**
 * Get the seasonal snow line used for drawing.
 * @return Height level at or above which tiles are drawn snowy, or #SEASON_NO_SNOW.
 */
uint8_t GetSeasonSnowLine()
{
	return _season_snow_line;
}

/**
 * Check whether a tile should be drawn with seasonal snow.
 * @param ti Tile information of the tile being drawn.
 * @return True when the tile lies on or above the seasonal snow line.
 */
bool IsSeasonSnow(const TileInfo *ti)
{
	if (_season_snow_line == SEASON_NO_SNOW) return false;
	return ti->z >= _season_snow_line * static_cast<int>(TILE_HEIGHT);
}

/** Seasonal light tone per month; the value of a month blends into the next one over the month. */
static const Colour _season_tones[12] = {
	Colour(226, 234, 255), // January: cold
	Colour(226, 234, 255), // February
	Colour(238, 246, 255), // March: thaw
	Colour(240, 255, 236), // April: fresh green
	Colour(240, 255, 236), // May
	Colour(255, 255, 255), // June: neutral
	Colour(255, 255, 255), // July
	Colour(255, 250, 232), // August: late summer
	Colour(255, 240, 214), // September: autumn
	Colour(255, 232, 204), // October
	Colour(238, 236, 236), // November: grey
	Colour(226, 234, 255), // December: cold
};

/**
 * Get the seasonal colour tone of the viewport light.
 * @return Light colour; white when seasons are disabled.
 */
Colour GetSeasonTint()
{
	if (!_settings_client.gui.seasons) return Colour(0xFF, 0xFF, 0xFF);

	TimerGameCalendar::YearMonthDay ymd = TimerGameCalendar::ConvertDateToYMD(TimerGameCalendar::date);
	const Colour &from = _season_tones[ymd.month];
	const Colour &to = _season_tones[(ymd.month + 1) % 12];
	int pos = ymd.day - 1; // 0..30
	auto mix = [&](uint8_t a, uint8_t b) -> uint8_t {
		int value = a + ((int)b - (int)a) * pos / 31;
		return (uint8_t)std::min(255, (value / 3) * 3); // quantised, see viewport_light.cpp
	};
	return Colour(mix(from.r, to.r), mix(from.g, to.g), mix(from.b, to.b));
}

/**
 * Recompute the seasonal snow line and redraw the map when it changed.
 * Called frequently from the viewport light timer; cheap when nothing changed.
 */
void UpdateSeason()
{
	uint8_t line = ComputeSeasonSnowLine();
	if (line == _season_snow_line) return;

	_season_snow_line = line;
	MarkWholeScreenDirty();
}
