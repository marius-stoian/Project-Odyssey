#pragma once

#include "boundary.h"

#include "region.h"
#include "region_edits.h"

#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim {

// Things, people and named places put on the land, taken off it, or moved (US-204, ADR-020, design M12 section 8). They are the `placed` list of RegionEdits;
// these functions are the only way the Editor changes that list, so every change has the same rules and the same way back.
//
// An id is made once ("t-0001" a thing, "p-0001" a person, "l-0001" a place) and stays with the entry through saving, loading, moving and every regeneration
// of the land: the land under an entry may change, the entry is still the same entry. A new id is one more than the largest in use for its group.

// One entry before and after a change; no value means "not in the list". This is a step of Undo for the world tools.
struct PlacedChange {
    std::string id;
    std::optional<PlacedEdit> before;
    std::optional<PlacedEdit> after;
    friend bool operator==(const PlacedChange&, const PlacedChange&) = default;
};

// "t-0001" for the next thing, "p-0001" for the next person and so on.
std::string nextPlacedId(const RegionEdits& edits, EditGroup group);

// Empty when the property may be set on an entry of the group, else the rule it breaks. What each group allows:
//  - person: hp, swordDamage, family (whole numbers), attitude (one word), tags (words separated by commas);
//  - thing: hp (an animal), and "<interaction>.duration" or "<interaction>.delay" (whole seconds) for what a plant offers;
//  - place: tags.
std::string placedPropertyProblem(EditGroup group, const std::string& key, const std::string& value);

// Empty when `entry` may stand where it is put, else what is wrong in plain words: outside the region, on water or mountain, no kind, a place without a unique name,
// something of its group already on the tile, a property that breaks its rule. `edits` is the list it would join (its own earlier version is ignored).
std::string placementProblem(Region& land, const RegionEdits& edits, const PlacedEdit& entry);

const PlacedEdit* findPlaced(const RegionEdits& edits, const std::string& id);
// The entry of the group standing on a tile (a tombstone is not standing); nothing when there is none.
const PlacedEdit* placedAt(const RegionEdits& edits, int x, int y, std::optional<EditGroup> group = std::nullopt);
// The names a quest or a dialogue may point to: a place's name (or kind), a person's name (or kind), in file order, once each.
std::vector<std::string> placedNames(const RegionEdits& edits, EditGroup group);

// Each returns the change made (already applied to `edits`), or nothing with the reason in `problem`.
// Put an entry on the land; an empty id is filled in. Refused when placementProblem finds something wrong or the world file is full (kMaxWorldEntries).
std::optional<PlacedChange> addPlaced(Region& land, RegionEdits& edits, PlacedEdit entry, std::string& problem);
// Move an entry to another tile (the same checks).
std::optional<PlacedChange> movePlaced(Region& land, RegionEdits& edits, const std::string& id, int x, int y, std::string& problem);
// Take an entry off the list. Taking a tombstone off brings back the seed's own thing.
std::optional<PlacedChange> removePlaced(RegionEdits& edits, const std::string& id, std::string& problem);
// Hide the seed's own wood, berries, flint or herd at a tile: a tombstone entry, so regeneration cannot bring it back. Nothing when the seed has nothing there.
std::optional<PlacedChange> hideSeedThing(Region& land, RegionEdits& edits, int x, int y, std::string& problem);
// Set one property of an entry; an empty value clears it.
std::optional<PlacedChange> setPlacedProperty(RegionEdits& edits, const std::string& id, const std::string& key, const std::string& value, std::string& problem);
// The name of the seed's kind of resource as a tombstone writes it: "flint", "wood", "berries" or "herd".
const char* resourceWord(ResourceKind kind);

// Do (forward) or take back one change.
void applyPlacedChange(RegionEdits& edits, const PlacedChange& change, bool forward);

// What the owner may do with a conflict (design M12 section 7, rule 3). Keep changes nothing: the game shows the entry and skips placing it where it cannot stand.
enum class ConflictChoice { Keep, Move, Remove };
// Settles every conflict whose id is an entry of the list: Move puts the entry on the nearest tile where it can stand (searching 30 tiles out; an entry with no such
// tile stays and is left to the owner), Remove takes it off. Painted tiles ("tile x,y") are not entries and are left alone. Returns the changes made, in order.
std::vector<PlacedChange> resolveConflicts(Region& land, RegionEdits& edits, const std::vector<EditConflict>& conflicts, ConflictChoice choice);

} // namespace odysseus::sim
