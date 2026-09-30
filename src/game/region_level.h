#pragma once

#include "boundary.h"

#include "game/catalogs.h"
#include "game/level.h"
#include "sim/region.h"

namespace odysseus::game {

// A generated region as a level the game can play (US-040, US-041, D-31): the ground from the biomes (steppe and forest as
// grass, rivers and lakes as water, mountains as stone, cave mouths as paths), wood as trees, berries as edible plants, flint
// as a patch of moss, and each herd as one animal standing on the steppe. The hero starts at the region's start, where a fire
// burns; the level is marked as a clan level. The region itself is left untouched.
Level levelFromRegion(sim::Region& region, const Definitions& definitions, const Catalogs& catalogs);

} // namespace odysseus::game
