#pragma once

#include "boundary.h"

#include "luna/engine/tile_map.h"

namespace odysseus::game {

inline constexpr int kTestMapSize = 64; // US-023: a 64 x 64 test map

// The M1 test valley: grass, a path crossing at the centre, a pond, scattered rocks.
// Rocks and water are solid. Always the same map (no randomness).
luna::engine::TileMap makeTestMap();

} // namespace odysseus::game
