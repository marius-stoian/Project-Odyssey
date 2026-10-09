// US-207 Play the edited region (simulation side): the fingerprint of a world file, what a run save records about its world, and the region changes of a run laid back
// onto the world's land.
#include "sim/data.h"
#include "sim/hero_data.h"
#include "sim/hero_life.h"
#include "sim/region.h"
#include "sim/region_save.h"
#include "sim/world.h"
#include "sim/world_file.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

sim::RegionConfig regionConfig() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }
sim::SimConfig simConfig() { return sim::loadSimConfig(ODYSSEUS_DATA_DIR); }

const sim::HeroData& heroData() {
    static const sim::HeroData loaded = sim::loadHeroData(ODYSSEUS_DATA_DIR);
    return loaded;
}

struct Folder {
    fs::path path;
    Folder() : path(fs::temp_directory_path() / ("odysseus-us207-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) { fs::create_directories(path); }
    ~Folder() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};

nlohmann::json readJson(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return nlohmann::json::parse(std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()));
}

void writeJson(const fs::path& file, const nlohmann::json& data) { std::ofstream(file, std::ios::binary | std::ios::trunc) << data.dump(1); }

} // namespace

TEST_CASE("US-207 Fingerprint: the same text gives the same digits, any change gives other ones, and a missing file gives none") {
    CHECK(sim::worldTextHash("a") == "af63dc4c8601ec8c"); // FNV-1a, 64 bit: the published value for "a"
    CHECK(sim::worldTextHash("abc") == sim::worldTextHash("abc"));
    CHECK(sim::worldTextHash("abc") != sim::worldTextHash("abd"));
    CHECK(sim::worldTextHash("").size() == 16);
    Folder folder;
    std::ofstream(folder.path / "w.json", std::ios::binary) << "{ \"version\": 1 }";
    CHECK(sim::worldFileHash(folder.path / "w.json") == sim::worldTextHash("{ \"version\": 1 }"));
    CHECK(sim::worldFileHash(folder.path / "missing.json").empty());
}

TEST_CASE("US-207 Run save: the world file's name and fingerprint are written, and a save from before region editing still loads") {
    Folder folder;
    const fs::path file = folder.path / "hero.json";
    sim::World world(7, simConfig());
    sim::NewGame game;
    game.seed = 7;
    game.preset = 2;
    game.world = "my-land";
    game.worldHash = "0123456789abcdef";
    sim::HeroLife life(heroData(), world, game);
    life.save(file);

    const sim::HeroLife::SavedWorld saved = sim::HeroLife::savedWorld(file);
    CHECK(saved.name == "my-land");
    CHECK(saved.hash == "0123456789abcdef");
    CHECK(readJson(file).at("version").get<int>() == 3);
    sim::World again(7, simConfig());
    const sim::HeroLife loaded = sim::HeroLife::load(heroData(), again, file);
    CHECK(loaded.game().world == "my-land");
    CHECK(loaded.game().worldHash == "0123456789abcdef");

    // Version 2 (US-195): the rules but no world. Version 1 (US-080): neither. Both load as a run on generated land.
    nlohmann::json older = readJson(file);
    older["version"] = 2;
    older.erase("world");
    older.erase("worldHash");
    writeJson(file, older);
    CHECK(sim::HeroLife::savedWorld(file).name.empty());
    sim::World second(7, simConfig());
    const sim::HeroLife v2 = sim::HeroLife::load(heroData(), second, file);
    CHECK(v2.game().world.empty());
    CHECK(v2.game().seed == 7);

    older["version"] = 1;
    older.erase("rules");
    writeJson(file, older);
    sim::World third(7, simConfig());
    const sim::HeroLife v1 = sim::HeroLife::load(heroData(), third, file);
    CHECK(v1.game().world.empty());
    CHECK(v1.game().rules.empty());

    // A file that cannot be read names no world (the load itself reports the damage).
    CHECK(sim::HeroLife::savedWorld(folder.path / "nothing.json").name.empty());
}

TEST_CASE("US-207 Region save: the changes of a run are laid back onto the world's land, with the painted tiles in it") {
    const sim::RegionConfig config = regionConfig();
    sim::Region plain(1, config);
    sim::Tile painted{-1, -1};
    for (int y = 140; y < 200 && painted.x < 0; ++y) {
        for (int x = 140; x < 200 && painted.x < 0; ++x) {
            if (plain.biomeAt(x, y) == sim::Biome::Steppe) painted = {x, y};
        }
    }
    REQUIRE(painted.x >= 0);
    sim::WorldFile world;
    world.seed = 1;
    world.edits.tiles.push_back({painted.x, painted.y, sim::Biome::Water});

    sim::Region played = sim::makeWorldRegion(world, config);
    REQUIRE(played.biomeAt(painted.x, painted.y) == sim::Biome::Water);
    sim::Tile wood{-1, -1};
    for (int cy = 1; cy < 4 && wood.x < 0; ++cy) {
        for (int cx = 1; cx < 4 && wood.x < 0; ++cx) {
            for (const sim::Resource& resource : played.chunk(cx, cy).resources) {
                if (resource.kind == sim::ResourceKind::Wood && wood.x < 0) wood = {resource.x, resource.y};
            }
        }
    }
    REQUIRE(wood.x >= 0);
    REQUIRE(played.harvest(wood.x, wood.y, sim::Date{}));

    Folder folder;
    const fs::path file = folder.path / "region.json";
    sim::saveRegion(played, file);

    // The plain loader makes the land of the seed: the painted tile is not there. With the world as the base it is, and so is the harvest.
    const sim::LoadedRegion bare = sim::loadRegion(file, config);
    CHECK(bare.region.biomeAt(painted.x, painted.y) == sim::Biome::Steppe);
    const sim::LoadedRegion onWorld = sim::loadRegion(file, config, [&] { return sim::makeWorldRegion(world, config); });
    CHECK(onWorld.region.biomeAt(painted.x, painted.y) == sim::Biome::Water);
    CHECK(onWorld.region.changedChunks().size() == played.changedChunks().size());
    CHECK(onWorld.region.changedChunks().size() >= 1);

    // A base for another seed is refused: the changes belong to the land they were made on.
    CHECK_THROWS(sim::loadRegion(file, config, [&] {
        sim::WorldFile other;
        other.seed = 2;
        return sim::makeWorldRegion(other, config);
    }));
}
