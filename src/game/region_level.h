#pragma once

#include "boundary.h"

#include "game/catalogs.h"
#include "game/level.h"
#include "sim/region.h"
#include "sim/region_edits.h"

#include <string>
#include <vector>

namespace odysseus::game {

// A generated region as a level the game can play (US-040, US-041, D-31): the ground from the biomes (steppe and forest as
// grass, rivers and lakes as water, mountains as stone, cave mouths as paths), wood as trees, berries as edible plants, flint
// as a patch of moss, and each herd as one animal standing on the steppe. The hero starts at the region's start, where a fire
// burns; the level is marked as a clan level. The region itself is left untouched.
Level levelFromRegion(sim::Region& region, const Definitions& definitions, const Catalogs& catalogs);

// The same, with the owner's hand edits of the world file laid over it (US-204): the seed things in `edits` as tombstones are left out, and the things, people and named
// places put on the land are added (a person is a character with its class and properties; a place is a named spot, its kind its first tag). Entries come in group
// order (things, people, places) and file order inside a group, so one world file always gives the same level. An entry that cannot be made is named in `problems`.
Level levelFromRegion(sim::Region& region, const Definitions& definitions, const Catalogs& catalogs, const sim::RegionEdits& edits, std::vector<std::string>* problems = nullptr);

} // namespace odysseus::game
