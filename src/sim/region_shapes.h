#pragma once

#include "boundary.h"

#include "region.h"

#include <vector>

namespace odysseus::sim {

// The shapes of the water and mountain tools (US-203): whole-number geometry, so the same drag always makes the same tiles (Charter rule 6).

// The tiles on the straight line from `from` to `to`, both ends included, in order. Four-connected: no step is diagonal, so a line of water or
// mountain cannot be squeezed through at a corner by anyone who walks in four directions.
std::vector<Tile> line4(Tile from, Tile to);

// The tiles within `width` of a round stamp laid on each tile of `centre` (width 1: the tile itself; 2: with its four neighbours; 3: a 3 x 3 block;
// the same round brush the paint tools use), inside the region, each once, in tile order.
std::vector<Tile> thicken(const std::vector<Tile>& centre, int width, int regionSize);

// The tiles of a filled disc of the given radius (a radius of 0 is one tile), inside the region, in tile order.
std::vector<Tile> disc(Tile centre, int radius, int regionSize);

} // namespace odysseus::sim
