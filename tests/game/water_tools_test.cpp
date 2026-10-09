// US-203 Water and mountains: rivers with fords, lakes, ridges, cave mouths and removing water, all as tile edits over the seed.
#include "game/catalogs.h"
#include "game/level.h"
#include "game/region_level.h"
#include "game/region_view.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"
#include "sim/region.h"
#include "sim/world_file.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <queue>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace game = odysseus::game;
namespace sim = odysseus::sim;
namespace eng = luna::engine;

namespace {

sim::RegionConfig config() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }

struct Rig {
    fs::path base;
    std::vector<std::string> said;
    game::RegionView view;

    explicit Rig(std::uint64_t seed = 1)
        : base(fs::temp_directory_path() / ("odysseus-us203-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))),
          view(960, 540, [this](const std::string& message) { said.push_back(message); }) {
        view.setWorldsFolder(base / "worlds");
        view.open(seed, config());
    }
    ~Rig() {
        std::error_code error;
        fs::remove_all(base, error);
    }
};

// Can people on foot (four directions) get from a to b?
bool reachable(const sim::Region& land, sim::Tile a, sim::Tile b) {
    std::set<std::pair<int, int>> seen{{a.x, a.y}};
    std::queue<sim::Tile> open;
    open.push(a);
    while (!open.empty()) {
        const sim::Tile at = open.front();
        open.pop();
        if (at == b) return true;
        for (const sim::Tile next : {sim::Tile{at.x + 1, at.y}, sim::Tile{at.x - 1, at.y}, sim::Tile{at.x, at.y + 1}, sim::Tile{at.x, at.y - 1}}) {
            if (!sim::walkable(land.biomeAt(next.x, next.y)) || !seen.insert({next.x, next.y}).second) continue;
            open.push(next);
        }
    }
    return false;
}

// The water tiles connected (four directions) to a tile of the seed's land, and how many.
std::vector<sim::Tile> waterBody(const sim::Region& land, sim::Tile from) {
    std::vector<sim::Tile> body;
    std::set<std::pair<int, int>> seen{{from.x, from.y}};
    std::queue<sim::Tile> open;
    open.push(from);
    while (!open.empty()) {
        const sim::Tile at = open.front();
        open.pop();
        body.push_back(at);
        for (const sim::Tile next : {sim::Tile{at.x + 1, at.y}, sim::Tile{at.x - 1, at.y}, sim::Tile{at.x, at.y + 1}, sim::Tile{at.x, at.y - 1}}) {
            if (land.biomeAt(next.x, next.y) != sim::Biome::Water || !seen.insert({next.x, next.y}).second) continue;
            open.push(next);
        }
    }
    return body;
}

eng::Intents pointerAt(int x, int y, bool held, bool pressed) {
    eng::Intents intents;
    eng::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.held[static_cast<std::size_t>(eng::PointerButton::Left)] = held;
    pointer.pressed[static_cast<std::size_t>(eng::PointerButton::Left)] = pressed;
    intents.setPointer(pointer);
    return intents;
}

const game::Definitions& definitions() {
    static const game::Definitions loaded = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    return loaded;
}
const game::Catalogs& catalogs() {
    static const game::Catalogs loaded = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    return loaded;
}

bool solidAt(const game::Level& level, int x, int y) { return definitions().tiles[static_cast<std::size_t>(level.at(x, y))].solid; }

} // namespace

TEST_CASE("US-203 River: it runs from the start to the end, blocks walking, and has a way over only at its fords") {
    Rig rig;
    // A land with nothing in the way: all meadow, so only the river decides who can walk where.
    sim::RegionConfig flat = config();
    flat.lakeLevel = 0;
    flat.mountainLevel = 1023;
    flat.forestMoisture = 1023;
    flat.riverBand = 0;
    rig.view.open(1, flat);
    sim::Region& land = *rig.view.region();

    // One river from its source to its mouth: every tile on the way is water, three wide.
    REQUIRE(rig.view.paintRiver({60, 128}, {200, 128}, 3, 0) > 0);
    CHECK(rig.view.history().size() == 1);
    for (int x = 60; x <= 200; x += 10) {
        CHECK(land.biomeAt(x, 127) == sim::Biome::Water);
        CHECK(land.biomeAt(x, 128) == sim::Biome::Water);
        CHECK(land.biomeAt(x, 129) == sim::Biome::Water);
    }
    CHECK(land.biomeAt(30, 128) == sim::Biome::Steppe); // before the source
    CHECK(land.biomeAt(230, 128) == sim::Biome::Steppe); // after the mouth
    CHECK(rig.view.undo());

    // A river round a patch of land: nobody gets in or out.
    const sim::Tile inside{128, 128};
    const sim::Tile outside{100, 128};
    CHECK(reachable(land, inside, outside));
    const auto ring = [&](int fordEvery) {
        rig.view.paintRiver({108, 108}, {148, 108}, 3, fordEvery); // the top side is the only one with a ford
        rig.view.paintRiver({148, 108}, {148, 148}, 3, 0);
        rig.view.paintRiver({148, 148}, {108, 148}, 3, 0);
        rig.view.paintRiver({108, 148}, {108, 108}, 3, 0);
    };
    ring(0);
    CHECK_FALSE(reachable(land, inside, outside)); // no way over
    for (int i = 0; i < 4; ++i) CHECK(rig.view.undo());
    CHECK(reachable(land, inside, outside));

    // The same river with a ford in its top side (index 20 of the line): people can cross, and only there.
    ring(40);
    CHECK(land.biomeAt(128, 108) == sim::Biome::Steppe); // the ford
    CHECK(reachable(land, inside, outside));
    CHECK(land.biomeAt(118, 108) == sim::Biome::Water);  // the rest of the top side is still water
    CHECK(land.biomeAt(148, 128) == sim::Biome::Water);
    CHECK(land.biomeAt(128, 148) == sim::Biome::Water);
    CHECK(land.biomeAt(108, 128) == sim::Biome::Water);
}

TEST_CASE("US-203 River in the game: water blocks, a ford is meadow") {
    Rig rig;
    sim::Region& land = *rig.view.region();
    rig.view.paintRiver({128, 8}, {128, 247}, 3, 120); // a ford at index 60: tile (128, 68)
    game::Level level = game::levelFromRegion(land, definitions(), catalogs());
    CHECK(solidAt(level, 128, 30));
    CHECK(solidAt(level, 127, 30));
    CHECK(solidAt(level, 129, 30));
    CHECK_FALSE(solidAt(level, 128, 68)); // the ford
    CHECK(level.at(128, 30) == definitions().tileNumber("water"));
    CHECK(level.at(128, 68) == definitions().tileNumber("grass"));
}

TEST_CASE("US-203 Lake and ridge") {
    Rig rig;
    sim::Region& land = *rig.view.region();
    sim::Tile spot{-1, -1};
    for (int y = 60; y < 200 && spot.x < 0; ++y) {
        for (int x = 60; x < 200; ++x) {
            if (land.seedBiomeAt(x, y) == sim::Biome::Steppe && land.seedBiomeAt(x + 12, y) == sim::Biome::Steppe && land.seedBiomeAt(x, y + 12) == sim::Biome::Steppe) {
                spot = {x, y};
                break;
            }
        }
    }
    REQUIRE(spot.x >= 0);
    const int changed = rig.view.paintLake(spot, 4);
    CHECK(changed > 0);
    CHECK(changed <= 49); // a disc of radius 4 is 49 tiles
    CHECK(land.biomeAt(spot.x, spot.y) == sim::Biome::Water);
    CHECK(rig.view.history().size() == 1);

    CHECK(rig.view.paintRidge({spot.x, spot.y + 10}, {spot.x + 10, spot.y + 10}, 1) > 0);
    for (int x = spot.x; x <= spot.x + 10; ++x) CHECK(land.biomeAt(x, spot.y + 10) == sim::Biome::Mountain);
    CHECK(rig.view.history().size() == 2);
    CHECK(rig.view.undo());
    CHECK(rig.view.undo());
    CHECK(land.tileEditCount() == 0);
}

TEST_CASE("US-203 Cave: a mouth is put into a cliff and can be entered in the game") {
    Rig rig(12);
    sim::Region& land = *rig.view.region();
    sim::Tile cliff{-1, -1};
    for (int y = 20; y < 236 && cliff.x < 0; ++y) {
        for (int x = 20; x < 236; ++x) {
            if (land.seedBiomeAt(x, y) != sim::Biome::Mountain) continue;
            if (sim::walkable(land.biomeAt(x + 1, y)) && land.biomeAt(x + 1, y) != sim::Biome::Cave) {
                cliff = {x, y};
                break;
            }
        }
    }
    REQUIRE(cliff.x >= 0);

    // Not on open land.
    sim::Tile meadow{-1, -1};
    for (int y = 60; y < 200 && meadow.x < 0; ++y) {
        for (int x = 60; x < 200; ++x) {
            if (land.seedBiomeAt(x, y) == sim::Biome::Steppe) {
                meadow = {x, y};
                break;
            }
        }
    }
    CHECK_FALSE(rig.view.placeCave(meadow.x, meadow.y));
    CHECK(land.tileEditCount() == 0);
    REQUIRE_FALSE(rig.said.empty());

    game::Level before = game::levelFromRegion(land, definitions(), catalogs());
    CHECK(solidAt(before, cliff.x, cliff.y)); // the cliff stops people
    REQUIRE(rig.view.placeCave(cliff.x, cliff.y));
    CHECK(land.biomeAt(cliff.x, cliff.y) == sim::Biome::Cave);
    CHECK(rig.view.warnings().empty()); // reachable from the meadow next to it
    game::Level after = game::levelFromRegion(land, definitions(), catalogs());
    CHECK_FALSE(solidAt(after, cliff.x, cliff.y)); // and the mouth does not
    CHECK(after.at(cliff.x, cliff.y) == definitions().tileNumber("path"));
    CHECK(rig.view.history().size() == 1);
}

TEST_CASE("US-203 A cave mouth in the middle of a cliff is a warning, not a refusal") {
    Rig rig(12);
    sim::Region& land = *rig.view.region();
    sim::Tile deep{-1, -1};
    for (int y = 20; y < 236 && deep.x < 0; ++y) {
        for (int x = 20; x < 236; ++x) {
            if (land.seedBiomeAt(x, y) != sim::Biome::Mountain) continue;
            int walk = 0;
            for (const sim::Tile n : {sim::Tile{x + 1, y}, sim::Tile{x - 1, y}, sim::Tile{x, y + 1}, sim::Tile{x, y - 1}}) walk += sim::walkable(land.biomeAt(n.x, n.y)) ? 1 : 0;
            if (walk == 0) {
                deep = {x, y};
                break;
            }
        }
    }
    REQUIRE(deep.x >= 0);
    CHECK(rig.view.placeCave(deep.x, deep.y));
    REQUIRE(rig.view.warnings().size() == 1);
    CHECK(rig.view.warnings().front().find("cave mouth") != std::string::npos);
    CHECK(rig.view.undo());
    CHECK(rig.view.warnings().empty());
}

TEST_CASE("US-203 Remove: a generated lake is replaced by land, as an override that can be undone and saved") {
    Rig rig;
    sim::Tile lake{-1, -1};
    std::vector<sim::Tile> body;
    for (std::uint64_t seed = 1; seed <= 12 && lake.x < 0; ++seed) { // the first seed that has a lake of a good size, not touching the rim
    rig.view.open(seed, config());
    const sim::Region& seedLand = *rig.view.region();
    std::set<std::pair<int, int>> rejected; // water bodies already looked at, so none is searched twice
    for (int y = 30; y < 226 && lake.x < 0; ++y) {
        for (int x = 30; x < 226; ++x) {
            if (seedLand.seedBiomeAt(x, y) != sim::Biome::Water || rejected.contains({x, y})) continue;
            body = waterBody(seedLand, {x, y});
            for (const sim::Tile& at : body) rejected.insert({at.x, at.y});
            const bool touchesRim = std::any_of(body.begin(), body.end(), [&seedLand](const sim::Tile& at) { return seedLand.onEdgeWall(at.x, at.y); });
            if (!touchesRim && body.size() >= 8 && body.size() <= 3000) {
                lake = {x, y};
                break;
            }
        }
    }
    }
    REQUIRE(lake.x >= 0);
    sim::Region& land = *rig.view.region();
    const std::size_t size = body.size();

    CHECK(rig.view.dryWater(lake.x, lake.y) == static_cast<int>(size));
    for (const sim::Tile& at : body) {
        CHECK(land.biomeAt(at.x, at.y) == sim::Biome::Steppe);          // land replaces the lake
        CHECK(land.seedBiomeAt(at.x, at.y) == sim::Biome::Water);       // the seed is untouched
        CHECK(land.tileEdit(at.x, at.y) == sim::Biome::Steppe);         // the change is an override
    }
    CHECK(land.tileEditCount() == size);
    CHECK(rig.view.history().size() == 1);

    REQUIRE(rig.view.saveWorld());
    CHECK(sim::loadWorld(rig.view.worldFile(), config()).edits.tiles.size() == size);

    CHECK(rig.view.undo());
    CHECK(land.tileEditCount() == 0);
    CHECK(land.biomeAt(lake.x, lake.y) == sim::Biome::Water);
}

TEST_CASE("US-203 Ford: a crossing over water under the brush") {
    Rig rig;
    sim::Region& land = *rig.view.region();
    rig.view.paintRiver({128, 8}, {128, 247}, 3, 0);
    rig.view.setBrushSize(3);
    CHECK(rig.view.paintFord(128, 100) > 0);
    CHECK(land.biomeAt(128, 100) == sim::Biome::Steppe);
    CHECK(land.biomeAt(128, 130) == sim::Biome::Water);
    sim::Tile dry{-1, -1};
    for (int x = 40; x < 120 && dry.x < 0; ++x) {
        if (land.biomeAt(x, 100) != sim::Biome::Water) dry = {x, 100};
    }
    REQUIRE(dry.x >= 0);
    rig.said.clear();
    CHECK((rig.view.paintFord(dry.x, dry.y) == 0 || land.biomeAt(dry.x, dry.y) != sim::Biome::Water)); // on dry land: nothing to wade through
}

TEST_CASE("US-203 With the mouse: a river is dragged from its source to its mouth, in one step") {
    Rig rig;
    sim::Tile spot{-1, -1};
    for (int y = 60; y < 200 && spot.x < 0; ++y) {
        for (int x = 60; x < 200; ++x) {
            if (rig.view.region()->seedBiomeAt(x, y) == sim::Biome::Steppe) {
                spot = {x, y};
                break;
            }
        }
    }
    rig.view.setZoom(3);
    rig.view.centreOn(spot.x + 0.5, spot.y + 0.5);
    rig.view.setTool(game::RegionTool::River);
    rig.view.setBrushSize(1);
    rig.view.update(eng::Intents{});
    rig.view.update(pointerAt(480, 300, true, true));
    rig.view.update(pointerAt(560, 300, true, false));
    rig.view.update(pointerAt(640, 300, true, false));
    CHECK(rig.view.history().size() == 0); // not until the button goes up
    rig.view.update(pointerAt(640, 300, false, false));
    CHECK(rig.view.history().size() == 1);
    const auto a = rig.view.tileAtScreen(480, 300);
    const auto b = rig.view.tileAtScreen(640, 300);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    for (int x = a->x; x <= b->x; ++x) CHECK(rig.view.region()->biomeAt(x, a->y) == sim::Biome::Water);
}

TEST_CASE("US-203 Water over the start is a warning that says so") {
    Rig rig;
    const sim::Tile start = rig.view.region()->start();
    rig.view.paintLake(start, 3);
    REQUIRE_FALSE(rig.view.warnings().empty());
    CHECK(rig.view.warnings().front().find("start") != std::string::npos);
    rig.view.undo();
    CHECK(rig.view.warnings().empty());
}
