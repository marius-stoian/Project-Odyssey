#include "world_places.h"

#include "world_file.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <format>

namespace odysseus::sim {

namespace {

char prefixOf(EditGroup group) {
    switch (group) {
    case EditGroup::Thing: return 't';
    case EditGroup::Person: return 'p';
    case EditGroup::Place: return 'l';
    case EditGroup::Camp: return 'c';
    case EditGroup::Resource: return 'r';
    }
    return 'x';
}

const char* groupWord(EditGroup group) {
    switch (group) {
    case EditGroup::Thing: return "thing";
    case EditGroup::Person: return "person";
    case EditGroup::Place: return "place";
    case EditGroup::Camp: return "camp";
    case EditGroup::Resource: return "resource";
    }
    return "entry";
}

bool inside(const Region& land, int x, int y) { return x >= 0 && y >= 0 && x < land.size() && y < land.size(); }

// A whole number in [low, high], the way a file writes it.
bool wholeNumber(const std::string& text, int low, int high) {
    int value = 0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return !text.empty() && result.ec == std::errc{} && result.ptr == text.data() + text.size() && value >= low && value <= high;
}

bool oneWord(const std::string& text) {
    return !text.empty() && std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isalnum(c) != 0 || c == '-' || c == '_'; });
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::string displayName(const PlacedEdit& entry) { return entry.name.empty() ? entry.kind : entry.name; }

} // namespace

const char* resourceWord(ResourceKind kind) {
    switch (kind) {
    case ResourceKind::Flint: return "flint";
    case ResourceKind::Wood: return "wood";
    case ResourceKind::Berries: return "berries";
    case ResourceKind::Herd: return "herd";
    }
    return "?";
}

std::string nextPlacedId(const RegionEdits& edits, EditGroup group) {
    int largest = 0;
    for (const PlacedEdit& entry : edits.placed) {
        if (entry.group != group || entry.id.size() < 3 || entry.id[1] != '-') continue;
        int number = 0;
        const auto result = std::from_chars(entry.id.data() + 2, entry.id.data() + entry.id.size(), number);
        if (result.ec == std::errc{} && number > largest) largest = number;
    }
    return std::format("{}-{:04}", prefixOf(group), largest + 1);
}

std::string placedPropertyProblem(EditGroup group, const std::string& key, const std::string& value) {
    if (key.empty()) return "a property needs a name";
    if (group == EditGroup::Person) {
        if (key == "hp") return wholeNumber(value, 1, 10000) ? std::string() : "hp is a whole number from 1 to 10000";
        if (key == "swordDamage") return wholeNumber(value, 0, 1000) ? std::string() : "swordDamage is a whole number from 0 to 1000";
        if (key == "family") return wholeNumber(value, 0, 9999) ? std::string() : "family is a whole number from 0 to 9999";
        if (key == "attitude") return oneWord(value) ? std::string() : "attitude is one word";
        if (key == "tags") return value.empty() || value.find_first_of(":;") == std::string::npos ? std::string() : "tags are words separated by commas";
        return "a person can set hp, swordDamage, family, attitude or tags";
    }
    if (group == EditGroup::Thing) {
        if (key == "hp") return wholeNumber(value, 1, 10000) ? std::string() : "hp is a whole number from 1 to 10000";
        const std::size_t dot = key.rfind('.');
        const std::string field = dot == std::string::npos ? std::string() : key.substr(dot + 1);
        if (dot != std::string::npos && dot > 0 && (field == "duration" || field == "delay")) return wholeNumber(value, 0, 86400) ? std::string() : key + " is whole seconds from 0 to 86400";
        return "a thing can set hp, or <interaction>.duration or <interaction>.delay";
    }
    if (group == EditGroup::Place) {
        if (key == "tags") return value.empty() || value.find_first_of(":;") == std::string::npos ? std::string() : "tags are words separated by commas";
        return "a place can set tags";
    }
    if (group == EditGroup::Camp) {
        if (key == "people") return wholeNumber(value, 1, 100) ? std::string() : "people is a whole number from 1 to 100";
        return "a camp can set people (a rival's starting size)";
    }
    if (key == "amount") return wholeNumber(value, 1, 1000) ? std::string() : "amount is a whole number from 1 to 1000";
    return "a resource can set amount (how many can be taken there)";
}

const PlacedEdit* findPlaced(const RegionEdits& edits, const std::string& id) {
    const auto found = std::find_if(edits.placed.begin(), edits.placed.end(), [&id](const PlacedEdit& entry) { return entry.id == id; });
    return found == edits.placed.end() ? nullptr : &*found;
}

const PlacedEdit* placedAt(const RegionEdits& edits, int x, int y, std::optional<EditGroup> group) {
    for (const PlacedEdit& entry : edits.placed) {
        if (entry.removal || entry.x != x || entry.y != y) continue;
        if (group && entry.group != *group) continue;
        return &entry;
    }
    return nullptr;
}

std::vector<std::string> placedNames(const RegionEdits& edits, EditGroup group) {
    std::vector<std::string> names;
    for (const PlacedEdit& entry : edits.placed) {
        if (entry.group != group || entry.removal) continue;
        const std::string name = displayName(entry);
        if (std::find(names.begin(), names.end(), name) == names.end()) names.push_back(name);
    }
    return names;
}

std::string placementProblem(Region& land, const RegionEdits& edits, const PlacedEdit& entry) {
    if (entry.kind.empty()) return std::format("choose the kind of {} first", groupWord(entry.group));
    if (!inside(land, entry.x, entry.y)) return std::format("({}, {}) is outside the region", entry.x, entry.y);
    const Biome biome = effectiveBiome(land, edits, entry.x, entry.y);
    if (!walkable(biome)) return std::format("({}, {}) is {}: people cannot stand there", entry.x, entry.y, biome == Biome::Water ? "water" : "mountain");
    if (entry.group == EditGroup::Place) {
        if (entry.name.empty()) return "a place needs a name (a quest points to it by name)";
        if (entry.name.size() > 40) return "a place name is at most 40 letters";
        if (entry.name.find_first_of(":,;") != std::string::npos) return "a place name cannot hold : , or ;";
        for (const PlacedEdit& other : edits.placed) {
            if (other.id != entry.id && other.group == EditGroup::Place && !other.removal && lower(other.name) == lower(entry.name)) return "another place already has that name";
        }
    }
    if (entry.group == EditGroup::Person && entry.name.size() > 18) return "a person's name is at most 18 letters";
    if (entry.group == EditGroup::Camp) {
        if (entry.kind != "player" && entry.kind != "rival") return "a camp is a 'player' camp or a 'rival' camp";
        if (entry.name.size() > 30) return "a clan name is at most 30 letters";
        if (biome == Biome::Cave) return std::format("({}, {}) is a cave mouth: a camp needs open ground", entry.x, entry.y);
        for (const PlacedEdit& other : edits.placed) {
            if (other.id == entry.id || other.group != EditGroup::Camp || other.removal) continue;
            if (entry.kind == "player" && other.kind == "player") return "the player camp is placed already: move it instead";
            if (std::max(std::abs(other.x - entry.x), std::abs(other.y - entry.y)) < 8) return std::format("too close to the camp at ({}, {}): camps are at least 8 tiles apart", other.x, other.y);
        }
        if (!entry.forced && !land.goodSite({entry.x, entry.y})) return "the site has no water and food within reach (and a walk to both): choose another, or place it anyway";
    }
    if (entry.group == EditGroup::Resource && entry.kind != "flint" && entry.kind != "wood" && entry.kind != "berries" && entry.kind != "herd") {
        return "a resource is flint, wood, berries or herd";
    }
    for (const PlacedEdit& other : edits.placed) {
        if (other.id != entry.id && other.group == entry.group && !other.removal && other.x == entry.x && other.y == entry.y) {
            return std::format("({}, {}) already holds {} '{}'", entry.x, entry.y, groupWord(entry.group), displayName(other));
        }
    }
    for (const auto& [key, value] : entry.properties) {
        if (const std::string problem = placedPropertyProblem(entry.group, key, value); !problem.empty()) return problem;
    }
    return {};
}

void applyPlacedChange(RegionEdits& edits, const PlacedChange& change, bool forward) {
    const std::optional<PlacedEdit>& wanted = forward ? change.after : change.before;
    const auto at = std::find_if(edits.placed.begin(), edits.placed.end(), [&change](const PlacedEdit& entry) { return entry.id == change.id; });
    if (wanted) {
        if (at != edits.placed.end()) *at = *wanted;
        else edits.placed.push_back(*wanted);
    } else if (at != edits.placed.end()) {
        edits.placed.erase(at);
    }
}

namespace {

std::optional<PlacedChange> finish(RegionEdits& edits, PlacedChange change) {
    applyPlacedChange(edits, change, true);
    return change;
}

} // namespace

std::optional<PlacedChange> addPlaced(Region& land, RegionEdits& edits, PlacedEdit entry, std::string& problem) {
    if (edits.tiles.size() + edits.placed.size() >= kMaxWorldEntries) {
        problem = std::format("a world keeps at most {} edits", kMaxWorldEntries);
        return std::nullopt;
    }
    entry.removal = false;
    if (entry.id.empty()) entry.id = nextPlacedId(edits, entry.group);
    else if (findPlaced(edits, entry.id) != nullptr) {
        problem = "that id is already in use";
        return std::nullopt;
    }
    problem = placementProblem(land, edits, entry);
    if (!problem.empty()) return std::nullopt;
    return finish(edits, {entry.id, std::nullopt, entry});
}

std::optional<PlacedChange> movePlaced(Region& land, RegionEdits& edits, const std::string& id, int x, int y, std::string& problem) {
    const PlacedEdit* found = findPlaced(edits, id);
    if (found == nullptr) {
        problem = "that entry is not in the world";
        return std::nullopt;
    }
    if (found->removal) {
        problem = "a hidden thing of the seed cannot be moved";
        return std::nullopt;
    }
    PlacedEdit moved = *found;
    moved.x = x;
    moved.y = y;
    problem = placementProblem(land, edits, moved);
    if (!problem.empty()) return std::nullopt;
    return finish(edits, {id, *found, moved});
}

std::optional<PlacedChange> removePlaced(RegionEdits& edits, const std::string& id, std::string& problem) {
    const PlacedEdit* found = findPlaced(edits, id);
    if (found == nullptr) {
        problem = "that entry is not in the world";
        return std::nullopt;
    }
    return finish(edits, {id, *found, std::nullopt});
}

std::optional<PlacedChange> hideSeedThing(Region& land, RegionEdits& edits, int x, int y, std::string& problem) {
    if (!inside(land, x, y)) {
        problem = std::format("({}, {}) is outside the region", x, y);
        return std::nullopt;
    }
    const std::optional<Resource> seedThing = land.seedResource(x, y);
    const Resource* seed = seedThing ? &*seedThing : nullptr;
    if (seed == nullptr) {
        problem = std::format("the seed has nothing at ({}, {}) to take away", x, y);
        return std::nullopt;
    }
    for (const PlacedEdit& entry : edits.placed) {
        if (entry.removal && entry.x == x && entry.y == y) {
            problem = "it is already taken away";
            return std::nullopt;
        }
    }
    if (edits.tiles.size() + edits.placed.size() >= kMaxWorldEntries) {
        problem = std::format("a world keeps at most {} edits", kMaxWorldEntries);
        return std::nullopt;
    }
    PlacedEdit tombstone;
    tombstone.id = nextPlacedId(edits, EditGroup::Thing);
    tombstone.group = EditGroup::Thing;
    tombstone.kind = resourceWord(seed->kind);
    tombstone.x = x;
    tombstone.y = y;
    tombstone.removal = true;
    return finish(edits, {tombstone.id, std::nullopt, tombstone});
}

std::optional<PlacedChange> setPlacedProperty(RegionEdits& edits, const std::string& id, const std::string& key, const std::string& value, std::string& problem) {
    const PlacedEdit* found = findPlaced(edits, id);
    if (found == nullptr || found->removal) {
        problem = "that entry is not in the world";
        return std::nullopt;
    }
    PlacedEdit changed = *found;
    const auto at = std::find_if(changed.properties.begin(), changed.properties.end(), [&key](const auto& property) { return property.first == key; });
    if (value.empty()) {
        if (at == changed.properties.end()) return std::nullopt; // nothing to clear
        changed.properties.erase(at);
    } else {
        problem = placedPropertyProblem(changed.group, key, value);
        if (!problem.empty()) return std::nullopt;
        if (at != changed.properties.end()) at->second = value;
        else changed.properties.push_back({key, value});
    }
    if (changed == *found) return std::nullopt;
    return finish(edits, {id, *found, changed});
}

std::vector<PlacedChange> resolveConflicts(Region& land, RegionEdits& edits, const std::vector<EditConflict>& conflicts, ConflictChoice choice) {
    std::vector<PlacedChange> made;
    if (choice == ConflictChoice::Keep) return made;
    for (const EditConflict& conflict : conflicts) {
        const PlacedEdit* found = findPlaced(edits, conflict.id);
        if (found == nullptr) continue; // a painted tile, or an entry settled already
        std::string problem;
        if (choice == ConflictChoice::Remove || found->removal) { // a tombstone has nothing to move
            if (auto change = removePlaced(edits, conflict.id, problem)) made.push_back(*change);
            continue;
        }
        PlacedEdit candidate = *found;
        bool moved = false;
        for (int ring = 1; ring <= 30 && !moved; ++ring) {
            for (int dy = -ring; dy <= ring && !moved; ++dy) {
                for (int dx = -ring; dx <= ring && !moved; ++dx) {
                    if (std::max(std::abs(dx), std::abs(dy)) != ring) continue;
                    candidate.x = found->x + dx;
                    candidate.y = found->y + dy;
                    if (!placementProblem(land, edits, candidate).empty()) continue;
                    if (candidate.group == EditGroup::Camp && !candidate.forced && !land.goodSite({candidate.x, candidate.y})) continue;
                    moved = true;
                }
            }
        }
        if (moved) {
            if (auto change = movePlaced(land, edits, conflict.id, candidate.x, candidate.y, problem)) made.push_back(*change);
        }
    }
    return made;
}


std::vector<CampSite> campsOf(const RegionEdits& edits) {
    std::vector<CampSite> camps;
    for (const PlacedEdit& entry : edits.placed) {
        if (entry.group != EditGroup::Camp || entry.removal) continue;
        CampSite site;
        site.player = entry.kind == "player";
        site.clan = entry.name;
        site.at = {entry.x, entry.y};
        for (const auto& [key, value] : entry.properties) {
            if (key == "people") site.people = std::atoi(value.c_str());
        }
        if (site.player) camps.insert(camps.begin(), site);
        else camps.push_back(site);
    }
    return camps;
}

void applyPlacedToRegion(Region& land, const RegionEdits& edits) {
    land.clearPlacedEdits();
    for (const PlacedEdit& entry : edits.placed) {
        if (entry.x < 0 || entry.y < 0 || entry.x >= land.size() || entry.y >= land.size()) continue;
        if (entry.removal) {
            if (entry.group == EditGroup::Thing || entry.group == EditGroup::Resource) land.hideResource(entry.x, entry.y);
        } else if (entry.group == EditGroup::Resource) {
            int amount = 0;
            for (const auto& [key, value] : entry.properties) {
                if (key == "amount") amount = std::atoi(value.c_str());
            }
            if (land.seedResource(entry.x, entry.y)) {
                if (amount > 0) land.setResourceAmount(entry.x, entry.y, amount); // a spot of the seed: how many can be taken there
            } else {
                const ResourceKind kind = entry.kind == "wood" ? ResourceKind::Wood : entry.kind == "berries" ? ResourceKind::Berries : entry.kind == "herd" ? ResourceKind::Herd : ResourceKind::Flint;
                land.addResource(kind, entry.x, entry.y, amount > 0 ? amount : (kind == ResourceKind::Herd ? land.config().herdMinimum : 1));
            }
        } else if (entry.group == EditGroup::Camp && entry.kind == "player" && walkable(land.biomeAt(entry.x, entry.y))) {
            land.setStart({entry.x, entry.y});
        }
    }
}

} // namespace odysseus::sim
