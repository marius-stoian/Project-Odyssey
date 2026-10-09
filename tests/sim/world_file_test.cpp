// US-202 Hand edits on top of the seed: tile edits in the region, and the world file that keeps only the differences.
#include "core/text.h"
#include "sim/data.h"
#include "sim/region.h"
#include "sim/world_file.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

sim::RegionConfig config() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }

struct Folder {
    fs::path path;
    Folder() : path(fs::temp_directory_path() / ("odysseus-us202-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) { fs::create_directories(path); }
    ~Folder() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};

// A tile with the given seed biome well inside the region, and the first one found.
sim::Tile findTile(sim::Region& land, sim::Biome biome, int from = 20) {
    for (int y = from; y < 236; ++y) {
        for (int x = from; x < 236; ++x) {
            if (land.seedBiomeAt(x, y) == biome) return {x, y};
        }
    }
    return {-1, -1};
}

void writeText(const fs::path& file, const std::string& text) { std::ofstream(file) << text; }

} // namespace

TEST_CASE("US-202 Tile edits: a painted tile changes the land, and painting the seed's own biome is no edit") {
    sim::Region land(1, config());
    const sim::Tile forest = findTile(land, sim::Biome::Forest);
    REQUIRE(forest.x >= 0);
    const std::uint64_t before = land.fingerprint();

    CHECK(land.setTileEdit(forest.x, forest.y, sim::Biome::Steppe));
    CHECK(land.biomeAt(forest.x, forest.y) == sim::Biome::Steppe);
    CHECK(land.seedBiomeAt(forest.x, forest.y) == sim::Biome::Forest);
    CHECK(land.tileEdit(forest.x, forest.y) == sim::Biome::Steppe);
    CHECK(land.tileEditCount() == 1);
    CHECK(land.fingerprint() != before);
    // What grows follows the land: no tree on a tile that is meadow now.
    for (const sim::Resource& resource : land.chunk(forest.x / 32, forest.y / 32).resources) {
        if (resource.x == forest.x && resource.y == forest.y) CHECK(resource.kind != sim::ResourceKind::Wood);
    }

    CHECK(land.setTileEdit(forest.x, forest.y, sim::Biome::Forest)); // the seed's own biome again: the edit goes away
    CHECK(land.tileEditCount() == 0);
    CHECK(land.fingerprint() == before);

    CHECK_FALSE(land.clearTileEdit(forest.x, forest.y)); // nothing to clear
    CHECK_FALSE(land.setTileEdit(-1, 5, sim::Biome::Water));
    CHECK_FALSE(land.setTileEdit(0, 0, sim::Biome::Water)); // the edge wall keeps the world closed
    CHECK(land.onEdgeWall(3, 100));
    CHECK_FALSE(land.onEdgeWall(100, 100));
    CHECK(land.tileEditCount() == 0);
}

TEST_CASE("US-202 Tile edits come back in tile order") {
    sim::Region land(1, config());
    land.setTileEdit(50, 40, sim::Biome::Water);
    land.setTileEdit(10, 40, sim::Biome::Water);
    land.setTileEdit(99, 12, sim::Biome::Mountain);
    const auto list = land.tileEditList();
    std::vector<std::pair<int, int>> order;
    for (const auto& [tile, biome] : list) order.push_back({tile.y, tile.x});
    CHECK(order == std::vector<std::pair<int, int>>{{12, 99}, {40, 10}, {40, 50}});
}

TEST_CASE("US-202 World file: write, read, write again gives the same file") {
    Folder folder;
    const sim::RegionConfig base = config();
    sim::WorldFile world;
    world.seed = 7;
    world.generator = {{"lakeLevel", 200}, {"woodPerMille", 90}};
    world.edits.tiles = {{40, 12, sim::Biome::Water}, {41, 12, sim::Biome::Water}, {100, 70, sim::Biome::Forest}, {255 - 9, 120, sim::Biome::Cave}};
    world.edits.placed = {
        {"t-0001", sim::EditGroup::Thing, "boulder", 60, 61, false, false},
        {"c-0001", sim::EditGroup::Camp, "elder-camp", 70, 71, false, true},
        {"r-0001", sim::EditGroup::Resource, "wood", 80, 81, true, false},
        {"p-0001", sim::EditGroup::Person, "wanderer", 90, 91, false, false},
        {"l-0001", sim::EditGroup::Place, "shrine", 95, 96, false, false},
    };
    const fs::path file = folder.path / "test.json";
    sim::saveWorld(world, file, base);
    const sim::WorldFile read = sim::loadWorld(file, base);
    CHECK(read.version == sim::kWorldVersion);
    CHECK(read.seed == 7);
    CHECK(read.generator == world.generator);
    CHECK(read.edits.tiles.size() == world.edits.tiles.size());
    for (const sim::TileEdit& edit : world.edits.tiles) CHECK(std::find(read.edits.tiles.begin(), read.edits.tiles.end(), edit) != read.edits.tiles.end());
    // The entries come back grouped (things, people, places, camps, resources), each group in its own order.
    CHECK(read.edits.placed.size() == 5);
    for (const sim::PlacedEdit& entry : world.edits.placed) CHECK(std::find(read.edits.placed.begin(), read.edits.placed.end(), entry) != read.edits.placed.end());

    sim::saveWorld(read, folder.path / "again.json", base);
    CHECK(*odysseus::core::readTextFile(folder.path / "again.json") == *odysseus::core::readTextFile(file));

    // The settings of the world are region.json with its own differences laid over.
    const sim::RegionConfig mine = sim::worldConfig(read, base);
    CHECK(mine.lakeLevel == 200);
    CHECK(mine.woodPerMille == 90);
    CHECK(mine.mountainLevel == base.mountainLevel);
    CHECK(sim::generatorDifferences(mine, base) == world.generator);
}

TEST_CASE("US-202 World file: chunks are listed by their place, tile coordinates are inside the chunk") {
    Folder folder;
    const sim::RegionConfig base = config();
    sim::WorldFile world;
    world.edits.tiles = {{70, 40, sim::Biome::Water}}; // chunk (2, 1), tile (6, 8)
    const std::string text = sim::worldText(world, base);
    const nlohmann::json json = nlohmann::json::parse(text);
    REQUIRE(json.at("overrides").at("chunks").contains("2,1"));
    const auto& tile = json.at("overrides").at("chunks").at("2,1").at(0);
    CHECK(tile.at("x") == 6);
    CHECK(tile.at("y") == 8);
    CHECK(tile.at("biome") == "Water");
    CHECK(json.at("worldVersion") == 1);
}

TEST_CASE("US-202 Small saves: 100 painted tiles are 100 entries, not the map") {
    Folder folder;
    const sim::RegionConfig base = config();
    sim::Region land(1, base);
    sim::WorldFile world;
    world.seed = 1;
    int painted = 0;
    for (int y = 100; painted < 100; ++y) {
        for (int x = 100; x < 110 && painted < 100; ++x) {
            // Mountains in the middle of the land: always an edit, whatever the seed has there (unless it is mountain).
            if (land.setTileEdit(x, y, sim::Biome::Mountain)) {
                if (land.tileEdit(x, y)) ++painted;
            }
        }
    }
    REQUIRE(painted == 100);
    for (const auto& [tile, biome] : land.tileEditList()) world.edits.tiles.push_back({tile.x, tile.y, biome});
    const fs::path file = folder.path / "small.json";
    sim::saveWorld(world, file, base);
    const std::string text = *odysseus::core::readTextFile(file);
    const nlohmann::json json = nlohmann::json::parse(text);
    int entries = 0;
    for (const auto& [name, list] : json.at("overrides").at("chunks").items()) entries += static_cast<int>(list.size());
    CHECK(entries == 100);
    CHECK(json.at("seed") == 1);
    CHECK(text.size() < 12000); // the 65,536 tiles of the map would be hundreds of kilobytes
    // And it makes the same land again.
    sim::Region again = sim::makeWorldRegion(sim::loadWorld(file, base), base);
    CHECK(again.fingerprint() == land.fingerprint());
}

TEST_CASE("US-202 World file: a file that is wrong is refused with the file and the field") {
    Folder folder;
    const sim::RegionConfig base = config();
    const auto refused = [&](const std::string& body) {
        const fs::path file = folder.path / "bad.json";
        writeText(file, body);
        try {
            (void)sim::loadWorld(file, base);
        } catch (const sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    const std::string good = R"({"worldVersion":1,"seed":1,"generator":{},"overrides":{"chunks":{}}})";
    CHECK(refused(good).empty());
    CHECK(refused(R"({"worldVersion":9,"seed":1,"generator":{},"overrides":{"chunks":{}}})").find("worldVersion") != std::string::npos);
    CHECK(refused(R"({"worldVersion":1,"generator":{},"overrides":{"chunks":{}}})").find("bad.json") != std::string::npos); // no seed
    CHECK(refused(R"({"worldVersion":1,"seed":1,"generator":{"size":512},"overrides":{"chunks":{}}})").find("generator.size") != std::string::npos);
    CHECK(refused(R"({"worldVersion":1,"seed":1,"generator":{},"overrides":{"chunks":{"9,9":[]}}})").find("overrides.chunks.9,9") != std::string::npos);
    CHECK(refused(R"({"worldVersion":1,"seed":1,"generator":{},"overrides":{"chunks":{"0,0":[{"x":40,"y":1,"biome":"Water"}]}}})").find("outside its chunk") != std::string::npos);
    CHECK(refused(R"({"worldVersion":1,"seed":1,"generator":{},"overrides":{"chunks":{"0,0":[{"x":1,"y":1,"biome":"Lava"}]}}})").find("biome") != std::string::npos);
    CHECK(refused(R"({"worldVersion":1,"seed":1,"generator":{},"overrides":{"chunks":{},"things":[{"id":"","kind":"x","at":[1,1]}]}})").find("id and a kind") != std::string::npos);
}

TEST_CASE("US-202 World file: too many edits are refused when saving") {
    Folder folder;
    const sim::RegionConfig base = config();
    sim::WorldFile world;
    for (std::size_t i = 0; i <= sim::kMaxWorldEntries; ++i) world.edits.placed.push_back({"t-" + std::to_string(i), sim::EditGroup::Thing, "boulder", 50, 50, false, false});
    CHECK_THROWS_AS(sim::saveWorld(world, folder.path / "huge.json", base), sim::DataError);
    CHECK_FALSE(fs::exists(folder.path / "huge.json"));
}
