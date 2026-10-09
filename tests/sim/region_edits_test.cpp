// US-201 Generator settings: the rules for a draft, and what changing them does to hand edits (they stay, the ones the new land does not suit are listed).
#include "sim/region.h"
#include "sim/region_edits.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

fs::path shippedFile() { return fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"; }
sim::RegionConfig config() { return sim::loadRegionConfig(shippedFile()); }

struct Key {
    const char* name;
    int sim::RegionConfig::*member;
    int minimum;
    int maximum;
};

const Key kKeys[] = {
    {"size", &sim::RegionConfig::size, 64, 1024},
    {"chunkSize", &sim::RegionConfig::chunkSize, 8, 128},
    {"lakeLevel", &sim::RegionConfig::lakeLevel, 0, 1023},
    {"riverBand", &sim::RegionConfig::riverBand, 0, 100},
    {"forestMoisture", &sim::RegionConfig::forestMoisture, 0, 1023},
    {"caveNoise", &sim::RegionConfig::caveNoise, 0, 1023},
    {"edgeWall", &sim::RegionConfig::edgeWall, 0, 32},
    {"flintPerMille", &sim::RegionConfig::flintPerMille, 0, 1000},
    {"woodPerMille", &sim::RegionConfig::woodPerMille, 0, 1000},
    {"berriesPerMille", &sim::RegionConfig::berriesPerMille, 0, 1000},
    {"herdPerMille", &sim::RegionConfig::herdPerMille, 0, 1000},
    {"herdMinimum", &sim::RegionConfig::herdMinimum, 1, 100},
    {"berryRegrowDays", &sim::RegionConfig::berryRegrowDays, 1, 365},
    {"startNeedWithin", &sim::RegionConfig::startNeedWithin, 5, 60},
};

// Does loadRegionConfig accept region.json with this one setting changed?
bool loaderAccepts(const std::string& key, int value) {
    std::ifstream in(shippedFile());
    nlohmann::json json = nlohmann::json::parse(in, nullptr, true, true);
    json[key] = value;
    const fs::path file = fs::temp_directory_path() / ("odysseus-us201-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".json");
    std::ofstream(file) << json.dump(2);
    bool accepted = true;
    try {
        (void)sim::loadRegionConfig(file);
    } catch (const std::exception&) {
        accepted = false;
    }
    std::error_code error;
    fs::remove(file, error);
    return accepted;
}

bool hasResource(sim::Region& land, int x, int y) {
    const sim::Tile chunk = land.chunkOf(x, y);
    for (const sim::Resource& resource : land.chunk(chunk.x, chunk.y).resources) {
        if (resource.x == x && resource.y == y) return true;
    }
    return false;
}

// A tile that is walkable land in `before` and water in `after`.
sim::Tile tileThatFloods(sim::Region& before, sim::Region& after) {
    for (int y = 20; y < 236; ++y) {
        for (int x = 20; x < 236; ++x) {
            if (sim::walkable(before.biomeAt(x, y)) && after.biomeAt(x, y) == sim::Biome::Water) return {x, y};
        }
    }
    return {-1, -1};
}

} // namespace

TEST_CASE("US-201 A draft is checked by the same rules as the file") {
    CHECK(sim::regionConfigProblems(config()).empty());
    for (const Key& key : kKeys) {
        CAPTURE(key.name);
        sim::RegionConfig low = config();
        low.*key.member = key.minimum - 1;
        sim::RegionConfig high = config();
        high.*key.member = key.maximum + 1;
        CHECK_FALSE(sim::regionConfigProblems(low).empty());
        CHECK_FALSE(sim::regionConfigProblems(high).empty());
        CHECK_FALSE(loaderAccepts(key.name, key.minimum - 1));
        CHECK_FALSE(loaderAccepts(key.name, key.maximum + 1));
        sim::RegionConfig edge = config();
        edge.*key.member = key.minimum;
        if (std::string(key.name) != "size" && std::string(key.name) != "chunkSize") {
            CHECK(sim::regionConfigProblems(edge).empty());
            CHECK(loaderAccepts(key.name, key.minimum));
        }
    }
    // The rules between two settings.
    sim::RegionConfig mountains = config();
    mountains.mountainLevel = mountains.lakeLevel + 99;
    CHECK_FALSE(sim::regionConfigProblems(mountains).empty());
    mountains.mountainLevel = mountains.lakeLevel + 100;
    CHECK(sim::regionConfigProblems(mountains).empty());
    sim::RegionConfig herds = config();
    herds.herdMaximum = herds.herdMinimum - 1;
    CHECK_FALSE(sim::regionConfigProblems(herds).empty());
    sim::RegionConfig size = config();
    size.size = 250;
    CHECK_FALSE(sim::regionConfigProblems(size).empty()); // not a multiple of the chunk size
    CHECK_FALSE(loaderAccepts("size", 250));
}

TEST_CASE("US-201 Keep edits: new land never moves an edit, and the ones it does not suit are listed") {
    const sim::RegionConfig shipped = config();
    sim::RegionConfig wetter = shipped;
    wetter.lakeLevel = 300;
    wetter.mountainLevel = 900;
    sim::Region before(1, shipped);
    sim::Region after(1, wetter);
    const sim::Tile flooded = tileThatFloods(before, after);
    REQUIRE(flooded.x >= 0);

    // A tile that is walkable in both lands and has no resource of the seed in either.
    sim::Tile dry{-1, -1};
    for (int y = 30; y < 226 && dry.x < 0; ++y) {
        for (int x = 30; x < 226; ++x) {
            if (sim::walkable(before.biomeAt(x, y)) && sim::walkable(after.biomeAt(x, y)) && !hasResource(before, x, y) && !hasResource(after, x, y)) {
                dry = {x, y};
                break;
            }
        }
    }
    REQUIRE(dry.x >= 0);

    sim::RegionEdits edits;
    edits.placed = {
        {"t-0001", sim::EditGroup::Thing, "boulder", flooded.x, flooded.y, false, false},       // now in the water: conflict
        {"t-0002", sim::EditGroup::Thing, "boulder", dry.x, dry.y, false, false},               // fine
        {"c-0001", sim::EditGroup::Camp, "elder-camp", flooded.x, flooded.y, false, false},     // a camp in the water: conflict
        {"c-0002", sim::EditGroup::Camp, "island-camp", flooded.x, flooded.y, false, true},     // forced on purpose, but a camp cannot stand in water either
        {"p-0001", sim::EditGroup::Person, "wanderer", 999, 3, false, false},                   // outside the region: conflict
        {"r-0001", sim::EditGroup::Resource, "flint", dry.x, dry.y, true, false},               // a tombstone with nothing left to take away: conflict
    };
    edits.tiles = {{flooded.x, flooded.y, sim::Biome::Steppe}}; // the owner painted this tile dry by hand
    const sim::RegionEdits kept = edits;

    // With the tile painted dry, the things on it are on land again: no conflict. Without the paint they are in the water.
    {
        const auto conflicts = sim::findConflicts(after, edits);
        std::vector<std::string> ids;
        for (const auto& conflict : conflicts) ids.push_back(conflict.id);
        CHECK(std::find(ids.begin(), ids.end(), "t-0001") == ids.end());
        CHECK(std::find(ids.begin(), ids.end(), "t-0002") == ids.end());
        CHECK(std::find(ids.begin(), ids.end(), "p-0001") != ids.end());
        CHECK(std::find(ids.begin(), ids.end(), "r-0001") != ids.end());
    }
    edits.tiles.clear();
    const auto conflicts = sim::findConflicts(after, edits);
    std::vector<std::string> ids;
    for (const auto& conflict : conflicts) ids.push_back(conflict.id);
    CHECK(ids == std::vector<std::string>{"t-0001", "c-0001", "c-0002", "p-0001", "r-0001"});
    for (const auto& conflict : conflicts) CHECK_FALSE(conflict.reason.empty());
    // In the land they were made on, only the person outside the region and the empty tombstone are wrong.
    std::vector<std::string> then;
    for (const auto& conflict : sim::findConflicts(before, edits)) then.push_back(conflict.id);
    CHECK(std::find(then.begin(), then.end(), "t-0001") == then.end());
    CHECK(std::find(then.begin(), then.end(), "t-0002") == then.end());
    CHECK(std::find(then.begin(), then.end(), "p-0001") != then.end());
    CHECK(std::find(then.begin(), then.end(), "r-0001") != then.end());

    CHECK(sim::effectiveBiome(after, kept, flooded.x, flooded.y) == sim::Biome::Steppe);
    CHECK(after.biomeAt(flooded.x, flooded.y) == sim::Biome::Water);
    CHECK(edits.placed == kept.placed); // finding conflicts changed nothing
}

TEST_CASE("US-201 A tombstone is a conflict only when the seed no longer has anything there") {
    sim::Region land(1, config());
    sim::Tile resource{-1, -1};
    for (int cy = 0; cy < land.chunksPerSide() && resource.x < 0; ++cy) {
        for (int cx = 0; cx < land.chunksPerSide() && resource.x < 0; ++cx) {
            const auto& resources = land.chunk(cx, cy).resources;
            if (!resources.empty()) resource = {resources.front().x, resources.front().y};
        }
    }
    REQUIRE(resource.x >= 0);
    sim::RegionEdits edits;
    edits.placed = {{"r-0001", sim::EditGroup::Resource, "wood", resource.x, resource.y, true, false}};
    CHECK(sim::findConflicts(land, edits).empty());
    sim::RegionConfig barren = config();
    barren.woodPerMille = 0;
    barren.berriesPerMille = 0;
    barren.flintPerMille = 0;
    barren.herdPerMille = 0;
    sim::Region empty(1, barren);
    CHECK(sim::findConflicts(empty, edits).size() == 1);
}
