/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/**
 * @file viewport_light.cpp Day/night lighting of the viewports.
 *
 * The lighting is a purely visual, client side effect: the drawn landscape of a viewport is
 * multiplied with a light colour that slowly cycles through night, dawn, day and dusk. The
 * cycle is driven by the game tick counter so it pauses together with the game, and its
 * length in game days is configurable. The interface and the viewport labels are not lit.
 */

#include "stdafx.h"
#include "viewport_light.h"
#include "settings_type.h"
#include "gfx_func.h"
#include "openttd.h"
#include "timer/timer.h"
#include "timer/timer_window.h"
#include "timer/timer_game_tick.h"
#include "season.h"

#include "safeguards.h"

static constexpr Colour NEUTRAL_LIGHT(0xFF, 0xFF, 0xFF);

/** The light that is currently applied to the viewports. */
static Colour _viewport_light = NEUTRAL_LIGHT;

/** A key frame of the lighting cycle. */
struct LightKeyFrame {
	uint16_t phase; ///< Position in the cycle, in 1/1000th of the cycle length.
	Colour light;   ///< Light colour at that position.
};

/** The lighting cycle; the first key frame must be at phase 0 and the last at phase 1000. */
static const LightKeyFrame _light_key_frames[] = {
	{    0, Colour(118, 132, 205) }, // deep night
	{  190, Colour(118, 132, 205) },
	{  260, Colour(232, 168, 132) }, // dawn
	{  340, NEUTRAL_LIGHT },         // day
	{  690, NEUTRAL_LIGHT },
	{  760, Colour(248, 172, 122) }, // dusk
	{  850, Colour(118, 132, 205) }, // night
	{ 1000, Colour(118, 132, 205) },
};

/**
 * Interpolate the light colour for a position in the cycle.
 * @param phase Position in the cycle, in 1/1000th of the cycle length.
 * @return The light colour.
 */
static Colour ComputeLight(uint phase)
{
	for (size_t i = 1; i < lengthof(_light_key_frames); i++) {
		const LightKeyFrame &from = _light_key_frames[i - 1];
		const LightKeyFrame &to = _light_key_frames[i];
		if (phase > to.phase) continue;

		int span = to.phase - from.phase;
		int pos = phase - from.phase;
		auto mix = [&](uint8_t a, uint8_t b) -> uint8_t {
			int value = a + ((int)b - (int)a) * pos / span;
			/* Quantise to limit the number of full redraws during the transitions. */
			return (uint8_t)std::min(255, (value / 3) * 3);
		};
		return Colour(mix(from.light.r, to.light.r), mix(from.light.g, to.light.g), mix(from.light.b, to.light.b));
	}
	return NEUTRAL_LIGHT;
}

/**
 * Get the light that is currently applied to the viewports.
 * @return The light colour; white when no lighting is applied.
 */
const Colour &GetViewportLight()
{
	return _viewport_light;
}

/**
 * Check whether the current light leaves the viewports unchanged.
 * @return True when no tinting is needed.
 */
bool IsViewportLightNeutral()
{
	return _viewport_light.r == 0xFF && _viewport_light.g == 0xFF && _viewport_light.b == 0xFF;
}

/**
 * Recompute the viewport light from the game time and the settings, and
 * redraw the screen when it changed.
 */
void UpdateViewportLight()
{
	Colour light = NEUTRAL_LIGHT;

	/* The scenario editor needs the true colours of the map; the title screen and the game get the lighting. */
	if (_settings_client.gui.daynight_cycle && _game_mode != GameMode::Editor) {
		uint64_t cycle_ticks = std::max<uint64_t>(1, _settings_client.gui.daynight_cycle_days) * Ticks::DAY_TICKS;
		uint phase = static_cast<uint>((TimerGameTick::counter % cycle_ticks) * 1000 / cycle_ticks);
		light = ComputeLight(phase);
	}

	if (_game_mode != GameMode::Editor) {
		Colour tone = GetSeasonTint();
		light = Colour((light.r * (tone.r + 1)) >> 8, (light.g * (tone.g + 1)) >> 8, (light.b * (tone.b + 1)) >> 8);
	}
	UpdateSeason();

	if (light.r == _viewport_light.r && light.g == _viewport_light.g && light.b == _viewport_light.b) return;

	_viewport_light = light;
	MarkWholeScreenDirty();
}

/** Follow the game time at a modest rate, as every change of the light triggers a full redraw. */
static const IntervalTimer<TimerWindow> _viewport_light_timer(std::chrono::milliseconds(120), [](auto) {
	UpdateViewportLight();
});
