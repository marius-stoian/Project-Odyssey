#pragma once

#include "boundary.h"

#include "tile_map.h"

namespace luna::engine {

// An axis-aligned box in world pixels. Fractions allowed: movement is smooth even when a
// character moves 4.8 pixels per tick.
struct Box {
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
};

// Moves `box` by (dx, dy) through `map`, stopping flush against solid tiles (and the map
// edge). Horizontal first, then vertical, so a character sliding along a wall keeps moving
// along it instead of sticking.
Box moveAndCollide(const TileMap& map, Box box, double dx, double dy);

} // namespace luna::engine
