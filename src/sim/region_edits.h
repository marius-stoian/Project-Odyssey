#pragma once

#include "boundary.h"

#include "region.h"

#include <string>
#include <vector>

namespace odysseus::sim {

// The owner's hand edits on top of the seed, as far as regeneration needs to know them (US-201, ADR-010, ADR-020, design M12 sections 3 and 7):
// tiles painted a different biome, and things, people, places, camps and resources put on the land or taken off it. Only differences from the
// seed are held. The file format and the tools that make these edits arrive with US-202 to US-205; this is the model they will fill.
// Edits are never moved or dropped by a change of the generator settings: they stay where they are, and `findConflicts` lists the ones the new land
// no longer suits, so the owner decides.
enum class EditGroup { Thing, Person, Place, Camp, Resource };

struct TileEdit {
    int x = 0;
    int y = 0;
    Biome biome = Biome::Steppe;
    friend bool operator==(const TileEdit&, const TileEdit&) = default;
};

struct PlacedEdit {
    std::string id;     // stable, "t-0001"
    EditGroup group = EditGroup::Thing;
    std::string kind;   // an id from a catalog
    int x = 0;
    int y = 0;
    bool removal = false; // a tombstone: the seed's own thing at (x, y) is taken away
    bool forced = false;  // a camp the owner placed on purpose where the site check fails
    friend bool operator==(const PlacedEdit&, const PlacedEdit&) = default;
};

struct RegionEdits {
    std::vector<TileEdit> tiles;
    std::vector<PlacedEdit> placed;

    bool empty() const { return tiles.empty() && placed.empty(); }
    friend bool operator==(const RegionEdits&, const RegionEdits&) = default;
};

struct EditConflict {
    std::string id;     // the entry's id, or "tile x,y" for a painted tile
    std::string reason; // plain words: what is wrong now
    friend bool operator==(const EditConflict&, const EditConflict&) = default;
};

// The biome a tile has once the edits are laid over the (re)generated land.
Biome effectiveBiome(Region& land, const RegionEdits& edits, int x, int y);

// The edits that the regenerated land does not suit, in the order of the edits. Nothing is changed. Rules:
// - an added thing, person, place or resource on a tile people cannot walk (water, mountain) or outside the region;
// - a camp (not forced) on such a tile, or on a tile that fails the site check (water and food within reach, and a walk to both);
// - a tombstone where the land no longer has anything to take away.
std::vector<EditConflict> findConflicts(Region& land, const RegionEdits& edits);

// What the land as it is now would trouble the owner with (US-203, design M12 section 6; warnings, never a block: the owner may want an island): the start
// has become water or mountain or no longer has water and food within reach, and cave mouths painted into a cliff that nobody can walk up to.
std::vector<std::string> landWarnings(Region& land);

} // namespace odysseus::sim
