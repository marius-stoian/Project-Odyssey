#include "game/level.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <system_error>

namespace odysseus::game {

using nlohmann::json;
using sim::DataError;

namespace {

constexpr const char* kFacingCodes[] = {"S", "SW", "W", "NW", "N", "NE", "E", "SE"};

std::string text(const json& object, const std::filesystem::path& file, const std::string& field) {
    if (!object.contains(field) || !object.at(field).is_string() || object.at(field).get<std::string>().empty()) {
        throw DataError(file, field, "must be a text in quotes");
    }
    return object.at(field).get<std::string>();
}

int whole(const json& object, const std::filesystem::path& file, const std::string& field, int minimum, int maximum) {
    if (!object.contains(field) || !object.at(field).is_number_integer()) {
        throw DataError(file, field, "must be a whole number");
    }
    const auto value = object.at(field).get<long long>();
    if (value < minimum || value > maximum) {
        throw DataError(file, field, std::format("must be between {} and {} (is {})", minimum, maximum, value));
    }
    return static_cast<int>(value);
}

PixelPoint point(const json& value, const std::filesystem::path& file, const std::string& field, const Level& level) {
    if (!value.is_array() || value.size() != 2 || !value.at(0).is_number_integer() || !value.at(1).is_number_integer()) {
        throw DataError(file, field, "must be [x, y] in whole pixels");
    }
    const PixelPoint p{value.at(0).get<int>(), value.at(1).get<int>()};
    if (p.x < 0 || p.y < 0 || p.x >= level.width * kTileSize || p.y >= level.height * kTileSize) {
        throw DataError(file, field, std::format("({}, {}) is outside the level", p.x, p.y));
    }
    return p;
}

} // namespace

const char* facingCode(Facing facing) {
    return kFacingCodes[static_cast<std::size_t>(facing)];
}

int Definitions::tileNumber(const std::string& name) const {
    for (std::size_t i = 0; i < tiles.size(); ++i) {
        if (tiles[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

bool Definitions::hasWeapon(const std::string& name) const {
    return std::find(weapons.begin(), weapons.end(), name) != weapons.end();
}

bool Definitions::hasLoopingEffect(const std::string& name) const {
    return std::find(loopingEffects.begin(), loopingEffects.end(), name) != loopingEffects.end();
}

bool Definitions::hasPlant(const std::string& name) const {
    return std::find(plants.begin(), plants.end(), name) != plants.end();
}

const CharacterKindDef* Definitions::character(const std::string& name) const {
    for (const CharacterKindDef& kind : characters) {
        if (kind.name == name) return &kind;
    }
    return nullptr;
}

Definitions loadDefinitions(const std::filesystem::path& dataDirectory) {
    Definitions definitions;
    const std::filesystem::path tilesFile = dataDirectory / "tiles.json";
    const json tiles = sim::readJsonFile(tilesFile);
    if (!tiles.contains("tiles") || !tiles.at("tiles").is_array() || tiles.at("tiles").empty()) {
        throw DataError(tilesFile, "tiles", "must be a list of tile kinds");
    }
    for (std::size_t i = 0; i < tiles.at("tiles").size(); ++i) {
        const json& entry = tiles.at("tiles").at(i);
        const std::string where = std::format("tiles[{}]", i);
        TileKindDef kind{text(entry, tilesFile, "name"), text(entry, tilesFile, "atlas"), entry.value("solid", false)};
        if (definitions.tileNumber(kind.name) >= 0) {
            throw DataError(tilesFile, where + ".name", "\"" + kind.name + "\" is listed twice");
        }
        definitions.tiles.push_back(kind);
    }
    const std::filesystem::path charactersFile = dataDirectory / "characters.json";
    const json characters = sim::readJsonFile(charactersFile);
    if (!characters.contains("characters") || !characters.at("characters").is_array() || characters.at("characters").empty()) {
        throw DataError(charactersFile, "characters", "must be a list of character kinds");
    }
    for (std::size_t i = 0; i < characters.at("characters").size(); ++i) {
        const json& entry = characters.at("characters").at(i);
        const std::string where = std::format("characters[{}]", i);
        CharacterKindDef kind;
        kind.name = text(entry, charactersFile, "name");
        kind.frames = text(entry, charactersFile, "frames");
        kind.directions = whole(entry, charactersFile, "directions", 1, 8);
        if (kind.directions != 1 && kind.directions != 8) {
            throw DataError(charactersFile, where + ".directions", "must be 1 (front view) or 8");
        }
        kind.hp = whole(entry, charactersFile, "hp", 1, 9999);
        kind.swordDamage = whole(entry, charactersFile, "swordDamage", 0, 999);
        kind.enemy = entry.value("enemy", true);
        if (entry.contains("reach")) {
            // How far its strike back reaches, in metres (US-131); 1.5 when not given.
            if (!entry.at("reach").is_number() || entry.at("reach").get<double>() < 0.5 || entry.at("reach").get<double>() > 10.0) {
                throw DataError(charactersFile, where + ".reach", "must be a number of metres from 0.5 to 10");
            }
            kind.reach = entry.at("reach").get<double>();
        }
        if (definitions.character(kind.name) != nullptr) {
            throw DataError(charactersFile, where + ".name", "\"" + kind.name + "\" is listed twice");
        }
        definitions.characters.push_back(kind);
    }
    // The animals (US-137) are character kinds too: placed, named, hit and struck back by like the others.
    const std::filesystem::path animalsFile = dataDirectory / "animals.json";
    if (std::filesystem::exists(animalsFile)) {
        const json animals = sim::readJsonFile(animalsFile);
        if (!animals.contains("animals") || !animals.at("animals").is_array()) {
            throw DataError(animalsFile, "animals", "must be a list of animals");
        }
        for (std::size_t i = 0; i < animals.at("animals").size(); ++i) {
            const json& entry = animals.at("animals").at(i);
            const std::string where = std::format("animals[{}]", i);
            CharacterKindDef kind;
            kind.name = text(entry, animalsFile, "name");
            kind.frames = text(entry, animalsFile, "frame");
            kind.directions = 1;
            kind.hp = whole(entry, animalsFile, "hp", 1, 9999);
            kind.swordDamage = whole(entry, animalsFile, "strikeDamage", 0, 999);
            kind.enemy = entry.value("enemy", false);
            kind.reach = entry.value("reach", 1.5);
            kind.animal = true;
            if (definitions.character(kind.name) != nullptr) {
                throw DataError(animalsFile, where + ".name", "\"" + kind.name + "\" is already a character kind");
            }
            definitions.characters.push_back(kind);
        }
    }
    // Weapon names, so a level can be checked when it is read (catalogs.cpp checks the rest).
    const std::filesystem::path weaponsFile = dataDirectory / "weapons.json";
    if (std::filesystem::exists(weaponsFile)) {
        const json weapons = sim::readJsonFile(weaponsFile);
        if (!weapons.contains("weapons") || !weapons.at("weapons").is_array()) {
            throw DataError(weaponsFile, "weapons", "must be a list of weapons");
        }
        for (std::size_t i = 0; i < weapons.at("weapons").size(); ++i) {
            definitions.weapons.push_back(text(weapons.at("weapons").at(i), weaponsFile, "name"));
        }
    }
    const std::filesystem::path plantsFile = dataDirectory / "plants.json";
    if (std::filesystem::exists(plantsFile)) {
        const json plants = sim::readJsonFile(plantsFile);
        if (!plants.contains("plants") || !plants.at("plants").is_array()) {
            throw DataError(plantsFile, "plants", "must be a list of plants");
        }
        for (std::size_t i = 0; i < plants.at("plants").size(); ++i) {
            definitions.plants.push_back(text(plants.at("plants").at(i), plantsFile, "name"));
        }
    }
    const std::filesystem::path effectsFile = dataDirectory / "effects.json";
    if (std::filesystem::exists(effectsFile)) {
        const json effects = sim::readJsonFile(effectsFile);
        if (!effects.contains("effects") || !effects.at("effects").is_array()) {
            throw DataError(effectsFile, "effects", "must be a list of effects");
        }
        for (std::size_t i = 0; i < effects.at("effects").size(); ++i) {
            const json& entry = effects.at("effects").at(i);
            if (entry.value("loop", false)) definitions.loopingEffects.push_back(text(entry, effectsFile, "name"));
        }
    }
    definitions.weapons.push_back(kSpearThrowName);
    definitions.weapons.push_back(kSwordSlashName);
    return definitions;
}

Level makeLevel(std::string name, int width, int height, int ground) {
    Level level;
    level.name = std::move(name);
    level.width = width;
    level.height = height;
    level.defaultGround = ground;
    level.ground.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), ground);
    level.heroStart = {width * kTileSize / 2 + kTileSize / 2, height * kTileSize / 2 + kTileSize * 3 / 4};
    return level;
}

Level resized(const Level& level, int width, int height) {
    Level out = level;
    out.width = std::clamp(width, kLevelMinSize, kLevelMaxSize);
    out.height = std::clamp(height, kLevelMinSize, kLevelMaxSize);
    out.ground.assign(static_cast<std::size_t>(out.width) * static_cast<std::size_t>(out.height), level.defaultGround);
    for (int y = 0; y < std::min(level.height, out.height); ++y) {
        for (int x = 0; x < std::min(level.width, out.width); ++x) {
            out.set(x, y, level.at(x, y));
        }
    }
    const int pixelsWide = out.width * kTileSize;
    const int pixelsHigh = out.height * kTileSize;
    auto inside = [&](const PixelPoint& p) { return p.x < pixelsWide && p.y < pixelsHigh; };
    std::erase_if(out.characters, [&](const PlacedCharacter& c) { return !inside(c.feet); });
    std::erase_if(out.targets, [&](const PixelPoint& p) { return !inside(p); });
    std::erase_if(out.pickups, [&](const PlacedPickup& p) { return !inside(p.at); });
    std::erase_if(out.plants, [&](const PlacedPlant& p) { return !inside(p.feet); });
    std::erase_if(out.effects, [&](const PlacedEffect& p) { return !inside(p.at); });
    out.heroStart = {std::min(out.heroStart.x, pixelsWide - 1), std::min(out.heroStart.y, pixelsHigh - 1)};
    return out;
}

luna::engine::TileMap buildTileMap(const Level& level, const Definitions& definitions) {
    luna::engine::TileMap map(level.width, level.height, kTileSize, level.defaultGround);
    for (std::size_t i = 0; i < definitions.tiles.size(); ++i) {
        map.setSolid(static_cast<int>(i), definitions.tiles[i].solid);
    }
    for (int y = 0; y < level.height; ++y) {
        for (int x = 0; x < level.width; ++x) {
            map.set(x, y, level.at(x, y));
        }
    }
    return map;
}

Level readLevelFile(const std::filesystem::path& file, const Definitions& definitions) {
    const json data = sim::readJsonFile(file);
    if (!data.is_object()) {
        throw DataError(file, "(file)", "is not a level");
    }
    const int version = whole(data, file, "levelVersion", 1, 1'000'000);
    if (version > kLevelVersion) {
        throw DataError(file, "levelVersion", std::format("is {}: the level was made by a newer version of the game (this one reads up to {})",
                                                          version, kLevelVersion));
    }
    Level level;
    level.name = text(data, file, "name");
    level.width = whole(data, file, "width", kLevelMinSize, kLevelMaxSize);
    level.height = whole(data, file, "height", kLevelMinSize, kLevelMaxSize);
    auto tile = [&](const std::string& name, const std::string& field) {
        const int number = definitions.tileNumber(name);
        if (number < 0) throw DataError(file, field, "\"" + name + "\" is not a tile kind in tiles.json");
        return number;
    };
    level.defaultGround = tile(text(data, file, "defaultGround"), "defaultGround");
    // Ground: one list per row of [kind, how many] runs, so a big grass field is one entry.
    if (!data.contains("ground") || !data.at("ground").is_array() || static_cast<int>(data.at("ground").size()) != level.height) {
        throw DataError(file, "ground", std::format("must be a list of {} rows", level.height));
    }
    for (int y = 0; y < level.height; ++y) {
        const json& row = data.at("ground").at(static_cast<std::size_t>(y));
        const std::string rowField = std::format("ground[{}]", y);
        int cells = 0;
        if (!row.is_array()) throw DataError(file, rowField, "must be a list of [kind, count] runs");
        for (std::size_t r = 0; r < row.size(); ++r) {
            const json& run = row.at(r);
            const std::string runField = std::format("{}[{}]", rowField, r);
            if (!run.is_array() || run.size() != 2 || !run.at(0).is_string() || !run.at(1).is_number_integer() || run.at(1).get<int>() < 1) {
                throw DataError(file, runField, "must be [\"kind\", count] with a count of at least 1");
            }
            const int number = tile(run.at(0).get<std::string>(), runField);
            const int count = run.at(1).get<int>();
            if (cells + count > level.width) throw DataError(file, rowField, std::format("is longer than the level's width {}", level.width));
            level.ground.insert(level.ground.end(), static_cast<std::size_t>(count), number);
            cells += count;
        }
        if (cells != level.width) throw DataError(file, rowField, std::format("has {} cells, the level is {} wide", cells, level.width));
    }
    level.heroStart = point(data.value("heroStart", json()), file, "heroStart", level);
    level.nextId = whole(data, file, "nextId", 1, 1'000'000'000);
    if (!data.contains("characters") || !data.at("characters").is_array()) {
        throw DataError(file, "characters", "must be a list (it may be empty)");
    }
    for (std::size_t i = 0; i < data.at("characters").size(); ++i) {
        const json& entry = data.at("characters").at(i);
        const std::string where = std::format("characters[{}]", i);
        PlacedCharacter placed;
        placed.id = whole(entry, file, "id", 1, level.nextId - 1);
        for (const PlacedCharacter& earlier : level.characters) {
            if (earlier.id == placed.id) throw DataError(file, where + ".id", std::format("{} is used twice", placed.id));
        }
        placed.kind = text(entry, file, "kind");
        if (definitions.character(placed.kind) == nullptr) {
            throw DataError(file, where + ".kind", "\"" + placed.kind + "\" is not a character kind in characters.json");
        }
        placed.feet = point(json::array({entry.value("x", -1), entry.value("y", -1)}), file, where + ".x/y", level);
        const std::string facing = entry.value("facing", std::string());
        const auto* code = std::find_if(std::begin(kFacingCodes), std::end(kFacingCodes), [&](const char* c) { return facing == c; });
        if (code == std::end(kFacingCodes)) throw DataError(file, where + ".facing", "must be S, SW, W, NW, N, NE, E or SE");
        placed.facing = static_cast<Facing>(code - std::begin(kFacingCodes));
        placed.name = text(entry, file, "name");
        placed.hp = whole(entry, file, "hp", 1, 9999);
        placed.swordDamage = whole(entry, file, "swordDamage", 0, 999);
        level.characters.push_back(placed);
    }
    if (data.contains("pickups")) { // level version 2 (US-134); a version 1 file has none
        if (!data.at("pickups").is_array()) throw DataError(file, "pickups", "must be a list");
        for (std::size_t i = 0; i < data.at("pickups").size(); ++i) {
            const json& entry = data.at("pickups").at(i);
            const std::string where = std::format("pickups[{}]", i);
            PlacedPickup pickup;
            pickup.id = whole(entry, file, "id", 1, level.nextId - 1);
            for (const PlacedCharacter& character : level.characters) {
                if (character.id == pickup.id) throw DataError(file, where + ".id", std::format("{} is used twice", pickup.id));
            }
            for (const PlacedPickup& earlier : level.pickups) {
                if (earlier.id == pickup.id) throw DataError(file, where + ".id", std::format("{} is used twice", pickup.id));
            }
            pickup.weapon = text(entry, file, "weapon");
            if (!definitions.hasWeapon(pickup.weapon)) {
                throw DataError(file, where + ".weapon", "\"" + pickup.weapon + "\" is not a weapon in weapons.json");
            }
            pickup.at = point(json::array({entry.value("x", -1), entry.value("y", -1)}), file, where + ".x/y", level);
            level.pickups.push_back(pickup);
        }
    }
    if (data.contains("plants")) { // level version 2 (US-136); a version 1 file has none
        if (!data.at("plants").is_array()) throw DataError(file, "plants", "must be a list");
        for (std::size_t i = 0; i < data.at("plants").size(); ++i) {
            const json& entry = data.at("plants").at(i);
            const std::string where = std::format("plants[{}]", i);
            PlacedPlant plant;
            plant.id = whole(entry, file, "id", 1, level.nextId - 1);
            const auto used = [&] { return DataError(file, where + ".id", std::format("{} is used twice", plant.id)); };
            for (const PlacedCharacter& other : level.characters) if (other.id == plant.id) throw used();
            for (const PlacedPickup& other : level.pickups) if (other.id == plant.id) throw used();
            for (const PlacedPlant& other : level.plants) if (other.id == plant.id) throw used();
            plant.kind = text(entry, file, "kind");
            if (!definitions.hasPlant(plant.kind)) {
                throw DataError(file, where + ".kind", "\"" + plant.kind + "\" is not a plant in plants.json");
            }
            plant.feet = point(json::array({entry.value("x", -1), entry.value("y", -1)}), file, where + ".x/y", level);
            level.plants.push_back(plant);
        }
    }
    if (data.contains("effects")) { // level version 2 (US-138); a version 1 file has none
        if (!data.at("effects").is_array()) throw DataError(file, "effects", "must be a list");
        for (std::size_t i = 0; i < data.at("effects").size(); ++i) {
            const json& entry = data.at("effects").at(i);
            const std::string where = std::format("effects[{}]", i);
            PlacedEffect effect;
            effect.id = whole(entry, file, "id", 1, level.nextId - 1);
            const auto used = [&] { return DataError(file, where + ".id", std::format("{} is used twice", effect.id)); };
            for (const PlacedCharacter& other : level.characters) if (other.id == effect.id) throw used();
            for (const PlacedPickup& other : level.pickups) if (other.id == effect.id) throw used();
            for (const PlacedPlant& other : level.plants) if (other.id == effect.id) throw used();
            for (const PlacedEffect& other : level.effects) if (other.id == effect.id) throw used();
            effect.name = text(entry, file, "name");
            if (!definitions.hasLoopingEffect(effect.name)) {
                throw DataError(file, where + ".name", "\"" + effect.name + "\" is not a looping effect in effects.json");
            }
            effect.at = point(json::array({entry.value("x", -1), entry.value("y", -1)}), file, where + ".x/y", level);
            level.effects.push_back(effect);
        }
    }
    if (data.contains("targets")) {
        if (!data.at("targets").is_array()) throw DataError(file, "targets", "must be a list of [x, y]");
        for (std::size_t i = 0; i < data.at("targets").size(); ++i) {
            level.targets.push_back(point(data.at("targets").at(i), file, std::format("targets[{}]", i), level));
        }
    }
    return level;
}

LoadedLevel loadLevel(const std::filesystem::path& file, const Definitions& definitions) {
    LoadedLevel loaded;
    std::string problems;
    for (int number = 0; number <= kLevelBackups; ++number) {
        const std::filesystem::path candidate = number == 0 ? file : std::filesystem::path(file.string() + ".bak" + std::to_string(number));
        if (!std::filesystem::exists(candidate)) continue;
        try {
            loaded.level = readLevelFile(candidate, definitions);
            loaded.loadedFrom = candidate;
            return loaded;
        } catch (const DataError& error) {
            if (std::string(error.what()).find("newer version") != std::string::npos) throw; // never silently lose newer work
            loaded.notes.push_back(std::format("skipped {}: {}", candidate.filename().string(), error.what()));
            problems += std::string("\n  ") + error.what();
        }
    }
    throw DataError(file, "(file)", problems.empty() ? "no level found" : "no level could be loaded:" + problems);
}

void saveLevel(const Level& level, const Definitions& definitions, const std::filesystem::path& file) {
    namespace fs = std::filesystem;
    json ground = json::array();
    for (int y = 0; y < level.height; ++y) {
        json row = json::array();
        for (int x = 0; x < level.width;) {
            const int kind = level.at(x, y);
            int count = 0;
            while (x < level.width && level.at(x, y) == kind) {
                ++count;
                ++x;
            }
            row.push_back(json::array({definitions.tiles.at(static_cast<std::size_t>(kind)).name, count}));
        }
        ground.push_back(row);
    }
    json characters = json::array();
    for (const PlacedCharacter& c : level.characters) {
        characters.push_back({{"id", c.id}, {"kind", c.kind}, {"x", c.feet.x}, {"y", c.feet.y}, {"facing", facingCode(c.facing)},
                              {"name", c.name}, {"hp", c.hp}, {"swordDamage", c.swordDamage}});
    }
    json pickups = json::array();
    for (const PlacedPickup& p : level.pickups) pickups.push_back({{"id", p.id}, {"weapon", p.weapon}, {"x", p.at.x}, {"y", p.at.y}});
    json plants = json::array();
    for (const PlacedPlant& p : level.plants) plants.push_back({{"id", p.id}, {"kind", p.kind}, {"x", p.feet.x}, {"y", p.feet.y}});
    json effects = json::array();
    for (const PlacedEffect& e : level.effects) effects.push_back({{"id", e.id}, {"name", e.name}, {"x", e.at.x}, {"y", e.at.y}});
    json targets = json::array();
    for (const PixelPoint& t : level.targets) targets.push_back({t.x, t.y});
    const json data{{"levelVersion", kLevelVersion},
                    {"name", level.name},
                    {"width", level.width},
                    {"height", level.height},
                    {"defaultGround", definitions.tiles.at(static_cast<std::size_t>(level.defaultGround)).name},
                    {"heroStart", {level.heroStart.x, level.heroStart.y}},
                    {"nextId", level.nextId},
                    {"characters", characters},
                    {"pickups", pickups},
                    {"plants", plants},
                    {"effects", effects},
                    {"targets", targets},
                    {"ground", ground}};
    fs::create_directories(file.parent_path());
    const fs::path temporary = fs::path(file.string() + ".tmp");
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) throw DataError(temporary, "(file)", "cannot be written");
        out << data.dump(1) << '\n';
        out.flush();
        if (!out) throw DataError(temporary, "(file)", "could not be written completely (is the disk full?)");
    }
    auto backup = [&](int number) { return fs::path(file.string() + ".bak" + std::to_string(number)); };
    std::error_code ignored;
    fs::remove(backup(kLevelBackups), ignored);
    for (int number = kLevelBackups - 1; number >= 1; --number) {
        if (fs::exists(backup(number))) fs::rename(backup(number), backup(number + 1));
    }
    if (fs::exists(file)) fs::rename(file, backup(1));
    fs::rename(temporary, file);
}

} // namespace odysseus::game
