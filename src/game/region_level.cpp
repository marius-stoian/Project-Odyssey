#include "game/region_level.h"

#include "game/game_rules.h"
#include "sim/npc_extras.h"

#include <algorithm>
#include <cstdlib>
#include <set>
#include <format>

namespace odysseus::game {

namespace {

std::vector<std::string> plantNames(const Catalogs& catalogs, bool blocks, bool edible, const std::string& size) {
    std::vector<std::string> names;
    for (const PlantDef& plant : catalogs.plants) {
        if (plant.blocks == blocks && plant.edible == edible && plant.size == size) names.push_back(plant.name);
    }
    return names;
}

} // namespace

Level levelFromRegion(sim::Region& region, const Definitions& definitions, const Catalogs& catalogs) {
    return levelFromRegion(region, definitions, catalogs, sim::RegionEdits{});
}

Level levelFromRegion(sim::Region& region, const Definitions& definitions, const Catalogs& catalogs, const sim::RegionEdits& edits, std::vector<std::string>* problems) {
    const int grass = std::max(0, definitions.tileNumber("grass"));
    const int water = std::max(0, definitions.tileNumber("water"));
    const int stone = std::max(0, definitions.tileNumber("stone"));
    const int path = std::max(0, definitions.tileNumber("path"));
    Level level = makeLevel(std::format("Region {}", region.seed()), region.size(), region.size(), grass);
    for (int y = 0; y < region.size(); ++y) {
        for (int x = 0; x < region.size(); ++x) {
            switch (region.biomeAt(x, y)) {
            case sim::Biome::Water: level.set(x, y, water); break;
            case sim::Biome::Mountain: level.set(x, y, stone); break;
            case sim::Biome::Cave: level.set(x, y, path); break;
            default: break;
            }
        }
    }
    const std::vector<std::string> trees = plantNames(catalogs, true, false, "tree");
    std::vector<std::string> berries = plantNames(catalogs, false, true, "small");
    if (berries.size() > 6) berries.resize(6);
    std::string flintPlant = "moss";
    if (catalogs.plant(flintPlant) == nullptr && !catalogs.plants.empty()) flintPlant = catalogs.plants.front().name;
    std::string herdAnimal = "deer";
    if (definitions.character(herdAnimal) == nullptr) herdAnimal.clear();

    const auto footOf = [](int tile) { return tile * kTileSize + kTileSize / 2; };
    const auto bottomOf = [](int tile) { return tile * kTileSize + kTileSize - 4; };
    std::set<std::pair<int, int>> hidden; // seed things the owner took away (tombstones)
    for (const sim::PlacedEdit& entry : edits.placed) {
        if (entry.removal) hidden.insert({entry.x, entry.y});
    }
    for (int cy = 0; cy < region.chunksPerSide(); ++cy) {
        for (int cx = 0; cx < region.chunksPerSide(); ++cx) {
            for (const sim::Resource& resource : region.chunk(cx, cy).resources) {
                if (hidden.contains({resource.x, resource.y})) continue;
                const int id = level.nextId;
                const std::uint32_t pick = static_cast<std::uint32_t>(resource.x * 73856093 ^ resource.y * 19349663);
                if (resource.kind == sim::ResourceKind::Wood && !trees.empty()) {
                    level.plants.push_back({id, trees[pick % trees.size()], {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}});
                    ++level.nextId;
                } else if (resource.kind == sim::ResourceKind::Berries && !berries.empty()) {
                    level.plants.push_back({id, berries[pick % berries.size()], {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}});
                    ++level.nextId;
                } else if (resource.kind == sim::ResourceKind::Flint && !flintPlant.empty() && catalogs.plant(flintPlant) != nullptr) {
                    level.plants.push_back({id, flintPlant, {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}});
                    ++level.nextId;
                } else if (resource.kind == sim::ResourceKind::Herd && !herdAnimal.empty()) {
                    const CharacterKindDef* kind = definitions.character(herdAnimal);
                    level.characters.push_back({id, herdAnimal, {footOf(resource.x), resource.y * kTileSize + kTileSize - 4}, (pick & 1U) != 0U ? Facing::East : Facing::West,
                                                herdAnimal, kind->hp, kind->swordDamage});
                    ++level.nextId;
                }
            }
        }
    }
    // What the owner put on the land (US-204), in the order of the groups and then of the file, so the same world always gives the same ids. An entry that cannot be
    // made (an unknown kind, or land that is water or mountain now) is skipped and named in `problems`; the world file still loads.
    const auto skip = [problems](const sim::PlacedEdit& entry, const std::string& why) {
        if (problems != nullptr) problems->push_back(std::format("{} '{}' at ({}, {}): {}", entry.id, entry.kind, entry.x, entry.y, why));
    };
    for (const sim::EditGroup group : {sim::EditGroup::Thing, sim::EditGroup::Person, sim::EditGroup::Place}) {
        for (const sim::PlacedEdit& entry : edits.placed) {
            if (entry.group != group || entry.removal) continue;
            if (entry.x < 0 || entry.y < 0 || entry.x >= region.size() || entry.y >= region.size() || !sim::walkable(sim::effectiveBiome(region, edits, entry.x, entry.y))) {
                skip(entry, "the land there cannot hold it");
                continue;
            }
            const auto property = [&entry](const std::string& key) -> const std::string* {
                for (const auto& [name, value] : entry.properties) {
                    if (name == key) return &value;
                }
                return nullptr;
            };
            const auto number = [&property](const std::string& key, int fallback) {
                const std::string* value = property(key);
                return value != nullptr ? std::atoi(value->c_str()) : fallback;
            };
            const auto words = [](const std::string& text) {
                std::vector<std::string> found;
                std::size_t from = 0;
                while (from <= text.size()) {
                    std::size_t to = text.find(',', from);
                    if (to == std::string::npos) to = text.size();
                    std::string word = text.substr(from, to - from);
                    word.erase(0, word.find_first_not_of(' '));
                    word.erase(word.find_last_not_of(' ') + 1);
                    if (!word.empty()) found.push_back(word);
                    from = to + 1;
                }
                return found;
            };
            const PixelPoint feet{footOf(entry.x), bottomOf(entry.y)};
            if (group == sim::EditGroup::Place) {
                PlacedPlace place;
                place.name = questWord(entry.name.empty() ? entry.kind : entry.name); // a level names its places with words (lower case and hyphens), as a quest says them
                place.at = {footOf(entry.x), footOf(entry.y)};
                place.tags.push_back(entry.kind);
                if (const std::string* tags = property("tags")) {
                    for (const std::string& tag : words(*tags)) place.tags.push_back(tag);
                }
                level.places.push_back(std::move(place));
            } else if (group == sim::EditGroup::Thing && catalogs.plant(entry.kind) != nullptr) {
                PlacedPlant plant{level.nextId++, entry.kind, feet, {}};
                for (const auto& [key, value] : entry.properties) { // "<interaction>.duration" or ".delay", in whole seconds
                    const std::size_t dot = key.rfind('.');
                    if (dot != std::string::npos) plant.overrides.push_back({key.substr(0, dot), key.substr(dot + 1), std::atoi(value.c_str()) * 1000});
                }
                level.plants.push_back(std::move(plant));
            } else if (const CharacterKindDef* kind = definitions.character(entry.kind)) {
                PlacedCharacter character;
                character.id = level.nextId++;
                character.kind = entry.kind;
                character.feet = feet;
                character.name = entry.name.empty() ? entry.kind : entry.name;
                character.hp = number("hp", kind->hp);
                character.swordDamage = number("swordDamage", kind->swordDamage);
                if (!entry.npcClass.empty()) character.classes.push_back(entry.npcClass);
                if (const std::string* attitude = property("attitude")) character.attitude = *attitude;
                if (const std::string* tags = property("tags")) character.tags = words(*tags);
                character.family = number("family", 0);
                // The rest of this person's setup (US-206): what it may do, its routine, what it owns, what it does on its own. A value the rules refuse is named in `problems`.
                const auto ids = [](const std::string& text) {
                    std::vector<std::string> found;
                    std::string id;
                    for (const char c : text + " ") {
                        if (c == ' ' || c == ',') {
                            if (!id.empty()) found.push_back(id);
                            id.clear();
                        } else {
                            id += c;
                        }
                    }
                    return found;
                };
                for (const auto& [key, value] : entry.properties) {
                    std::string problem;
                    if (key == "allow") character.allow = ids(value);
                    else if (key == "deny") character.deny = ids(value);
                    else if (key == "day" || key == "night") sim::rules::setScheduleField(character.extras.schedule, key, value, problem);
                    else if (key == "stock") sim::rules::setTradeField(character.extras.trade, "stock", value, problem);
                    else if (key == "does") sim::rules::setDoes(character.extras.does, value, problem);
                    if (!problem.empty()) skip(entry, key + ": " + problem);
                }
                level.characters.push_back(std::move(character));
            } else {
                skip(entry, "no such kind in the catalogs");
            }
        }
    }
    const sim::Tile start = region.start();
    level.heroStart = {footOf(start.x), footOf(start.y)};
    // The fire of the player's camp burns where the hero starts.
    if (definitions.hasLoopingEffect("flame")) level.effects.push_back({level.nextId++, "flame", level.heroStart});
    level.clan = true;
    return level;
}

} // namespace odysseus::game
