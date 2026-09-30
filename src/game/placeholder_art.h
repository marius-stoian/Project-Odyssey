#pragma once

#include "boundary.h"

#include "core/geometry.h"
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

// Props for the spear demo (US-029): spears pointing 8 ways (row 0 flint, row 1 wooden),
// a straw target on a post (untouched and hit), and a soft shadow.
inline constexpr int kSpearFrameSize = 32;
luna::engine::Image makePropSheet();
odysseus::core::Rect spearFrame(Facing facing, bool flintTip);
inline constexpr odysseus::core::Rect kTargetFrame{0, 64, 32, 48};
inline constexpr odysseus::core::Rect kTargetHitFrame{32, 64, 32, 48};
inline constexpr odysseus::core::Rect kShadowFrame{64, 64, 24, 8};

// The facing whose direction is closest to the vector (x, y), with y pointing down the screen.
Facing facingForVector(double x, double y);

} // namespace odysseus::game
