#pragma once

#include "boundary.h"

#include "luna/engine/image.h"

namespace odysseus::game {

// Programmer art, drawn by code until real art is chosen (D-05, M3). No files, no licences.

inline constexpr int kCharacterWidth = 32;  // D-04
inline constexpr int kCharacterHeight = 48; // D-04
inline constexpr int kWalkFrames = 4;
inline constexpr int kTileSize = 32; // D-16

// The 8 facing directions (D-04), in sheet row order.
enum class Facing { South, SouthWest, West, NorthWest, North, NorthEast, East, SouthEast, Count };

// Rows = facing directions, columns = walking frames (column 0 doubles as the idle pose).
luna::engine::Image makeCharacterSheet();

// One row of 32x32 tiles, in this order.
enum class TileKind { Grass, Path, Rock, Water, Count };
luna::engine::Image makeTileSheet();

} // namespace odysseus::game
