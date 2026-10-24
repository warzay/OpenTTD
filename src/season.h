/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file season.h Visual seasons: winter snow in the temperate climate and a seasonal colour tone. */

#ifndef SEASON_H
#define SEASON_H

#include "gfx_type.h"
#include "tile_cmd.h"

/** Marker for "no seasonal snow at any height". */
static constexpr uint8_t SEASON_NO_SNOW = UINT8_MAX;

uint8_t GetSeasonSnowLine();
bool IsSeasonSnow(const TileInfo *ti);
Colour GetSeasonTint();
void UpdateSeason();

#endif /* SEASON_H */
