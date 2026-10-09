// US-202 Hand edits on top of the seed: brush, rectangle and fill on the region, one step of Undo each, saved as differences in the world file.
#include "core/text.h"
#include "game/region_view.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"
#include "sim/region.h"
#include "sim/world_file.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
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
    eng::RecordingRenderer renderer;
    eng::Texture sheet;
    game::RegionView view;

    explicit Rig(fs::path folder = {}, std::uint64_t seed = 1)
        : base(folder.empty() ? fs::temp_directory_path() / ("odysseus-us202-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())) : folder),
          view(960, 540, [this](const std::string& message) { said.push_back(message); }) {
        sheet = renderer.createTexture(eng::makeUiSheet());
        view.setWorldsFolder(base / "worlds");
        view.open(seed, config());
    }
    ~Rig() {
        std::error_code error;
        fs::remove_all(base, error);
    }
    void frame() {
        renderer.clear();
        eng::UiPainter painter(renderer, sheet);
        painter.setScreen({0, 0, 960, 540});
        view.render(renderer, painter);
    }
    void settle() {
        for (int frames = 0; frames < 400; ++frames) {
            frame();
            if (view.chunkBuildsThisFrame() == 0) return;
        }
    }
    sim::Tile tileOf(sim::Biome biome, int from = 30) {
        for (int y = from; y < 226; ++y) {
            for (int x = from; x < 226; ++x) {
                if (view.region()->seedBiomeAt(x, y) == biome) return {x, y};
            }
        }
        return {-1, -1};
    }
};

eng::Intents pointerAt(int x, int y, bool held, bool pressed, eng::PointerButton button = eng::PointerButton::Left) {
    eng::Intents intents;
    eng::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.held[static_cast<std::size_t>(button)] = held;
    pointer.pressed[static_cast<std::size_t>(button)] = pressed;
    intents.setPointer(pointer);
    return intents;
}

eng::Intents key(eng::Intent intent) {
    eng::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

} // namespace

TEST_CASE("US-202 Paint: a meadow painted into a forest shows and is saved as an override of those chunks") {
    Rig rig;
    const sim::Tile forest = rig.tileOf(sim::Biome::Forest);
    REQUIRE(forest.x >= 0);
    rig.view.setZoom(4);
    rig.view.centreOn(forest.x + 0.5, forest.y + 0.5);
    rig.settle();
    const int picturesBefore = rig.view.chunkPicturesMade();
    REQUIRE(picturesBefore > 0);

    rig.view.setTool(game::RegionTool::Brush);
    rig.view.setPaintBiome(sim::Biome::Steppe);
    rig.view.setBrushSize(3);
    const int changed = rig.view.paintBrush(forest.x, forest.y);
    CHECK(changed >= 1);
    CHECK(rig.view.region()->biomeAt(forest.x, forest.y) == sim::Biome::Steppe);
    CHECK(rig.view.cellAt(forest.x, forest.y).biome == sim::Biome::Steppe);
    CHECK(rig.view.worldDirty());

    // The picture of the chunk is made again and the old one is given back to the renderer.
    rig.settle();
    rig.frame();
    CHECK_FALSE(rig.renderer.destroyed().empty());
    const eng::Image terrain = rig.view.chunkImage(forest.x / 32, forest.y / 32, game::RegionLayer::Terrain);
    const eng::Color meadow = terrain.get(forest.x % 32, forest.y % 32);
    CHECK(meadow == eng::Color{166, 176, 96, 255}); // steppe, no longer forest green

    // Saved: the file holds the seed and an override in that chunk, and reading it makes the same land.
    REQUIRE(rig.view.saveWorld());
    const sim::WorldFile saved = sim::loadWorld(rig.view.worldFile(), config());
    CHECK(saved.seed == 1);
    bool found = false;
    for (const sim::TileEdit& edit : saved.edits.tiles) found = found || (edit.x == forest.x && edit.y == forest.y && edit.biome == sim::Biome::Steppe);
    CHECK(found);
    CHECK(saved.edits.tiles.size() == rig.view.region()->tileEditCount());
    sim::Region again = sim::makeWorldRegion(saved, config());
    CHECK(again.fingerprint() == rig.view.region()->fingerprint());
    CHECK_FALSE(rig.view.worldDirty());
}

TEST_CASE("US-202 Paint with the mouse: a drag is one stroke, one step of Undo") {
    Rig rig;
    const sim::Tile steppe = rig.tileOf(sim::Biome::Steppe, 60);
    rig.view.setZoom(3); // 16 pixels a tile
    rig.view.centreOn(steppe.x + 0.5, steppe.y + 0.5);
    rig.view.setTool(game::RegionTool::Brush);
    rig.view.setPaintBiome(sim::Biome::Mountain);
    rig.view.setBrushSize(1);
    rig.view.update(eng::Intents{});

    rig.view.update(pointerAt(480, 300, true, true));
    for (int x = 500; x <= 600; x += 25) rig.view.update(pointerAt(x, 300, true, false)); // dragged fast: the dabs are joined up
    rig.view.update(pointerAt(600, 300, false, false));                                  // the button goes up
    CHECK(rig.view.history().size() == 1);
    // Every tile under the line is painted, none skipped.
    const auto first = rig.view.tileAtScreen(480, 300);
    const auto last = rig.view.tileAtScreen(600, 300);
    REQUIRE(first.has_value());
    REQUIRE(last.has_value());
    CHECK(last->x - first->x >= 6);
    for (int x = first->x; x <= last->x; ++x) CHECK(rig.view.region()->biomeAt(x, first->y) == sim::Biome::Mountain);

    // Ctrl+Z takes the whole stroke back, Ctrl+Y does it again.
    const std::size_t painted = rig.view.region()->tileEditCount();
    REQUIRE(painted > 0);
    rig.view.update(key(eng::Intent::Undo));
    CHECK(rig.view.region()->tileEditCount() == 0);
    for (int x = first->x; x <= last->x; ++x) CHECK(rig.view.region()->biomeAt(x, first->y) == rig.view.region()->seedBiomeAt(x, first->y));
    rig.view.update(key(eng::Intent::Redo));
    CHECK(rig.view.region()->tileEditCount() == painted);
}

TEST_CASE("US-202 Undo: rectangle and fill are one step each, in order, back to the seed") {
    Rig rig;
    const sim::Tile steppe = rig.tileOf(sim::Biome::Steppe, 60);
    const std::uint64_t seedPrint = rig.view.region()->fingerprint();
    rig.view.setPaintBiome(sim::Biome::Water);
    rig.view.setTool(game::RegionTool::Rectangle);
    const int rect = rig.view.paintRectangle(steppe.x, steppe.y, steppe.x + 4, steppe.y + 3);
    CHECK(rect > 0);
    CHECK(rig.view.history().size() == 1);
    const std::uint64_t afterRect = rig.view.region()->fingerprint();

    // Fill the little lake just made with mountain: one more step, and it covers the connected water only.
    rig.view.setPaintBiome(sim::Biome::Mountain);
    rig.view.setTool(game::RegionTool::Fill);
    const int filled = rig.view.paintFill(steppe.x, steppe.y);
    CHECK(filled >= rect); // at least the rectangle; the lake may touch more water
    CHECK(rig.view.history().size() == 2);
    CHECK(rig.view.region()->biomeAt(steppe.x, steppe.y) == sim::Biome::Mountain);

    CHECK(rig.view.undo());
    CHECK(rig.view.region()->fingerprint() == afterRect);
    CHECK(rig.view.undo());
    CHECK(rig.view.region()->fingerprint() == seedPrint);
    CHECK(rig.view.region()->tileEditCount() == 0);
    CHECK_FALSE(rig.view.undo()); // nothing left
    CHECK(rig.view.redo());
    CHECK(rig.view.redo());
    CHECK(rig.view.region()->biomeAt(steppe.x, steppe.y) == sim::Biome::Mountain);
    // A new edit drops what could have been redone.
    CHECK(rig.view.undo());
    rig.view.setTool(game::RegionTool::Brush);
    const sim::Tile elsewhere{steppe.x + 20, steppe.y + 20};
    rig.view.setPaintBiome(rig.view.region()->seedBiomeAt(elsewhere.x, elsewhere.y) == sim::Biome::Water ? sim::Biome::Steppe : sim::Biome::Water);
    CHECK(rig.view.paintBrush(elsewhere.x, elsewhere.y) > 0);
    CHECK_FALSE(rig.view.redo());
}

TEST_CASE("US-202 Reset gives tiles back to the seed, and painting the seed's own biome keeps nothing") {
    Rig rig;
    const sim::Tile forest = rig.tileOf(sim::Biome::Forest);
    rig.view.setTool(game::RegionTool::Brush);
    rig.view.setBrushSize(1);
    rig.view.setPaintBiome(sim::Biome::Water);
    rig.view.paintBrush(forest.x, forest.y);
    CHECK(rig.view.region()->tileEditCount() == 1);
    rig.view.setTool(game::RegionTool::Erase);
    rig.view.paintBrush(forest.x, forest.y);
    CHECK(rig.view.region()->tileEditCount() == 0);
    CHECK(rig.view.region()->biomeAt(forest.x, forest.y) == sim::Biome::Forest);
    rig.view.setTool(game::RegionTool::Brush);
    rig.view.setPaintBiome(sim::Biome::Forest);
    CHECK(rig.view.paintBrush(forest.x, forest.y) == 0); // already forest: nothing changed, no step recorded
    CHECK(rig.view.history().size() == 2);
}

TEST_CASE("US-202 The edge wall is not painted, and a fill that would cover the land is refused") {
    Rig rig;
    rig.view.setTool(game::RegionTool::Brush);
    rig.view.setPaintBiome(sim::Biome::Water);
    rig.view.setBrushSize(5);
    CHECK(rig.view.paintBrush(1, 1) == 0); // inside the rim of mountains
    CHECK(rig.view.region()->tileEditCount() == 0);

    // A land that is all meadow: filling it would change tens of thousands of tiles.
    sim::RegionConfig flat = config();
    flat.lakeLevel = 0;
    flat.mountainLevel = 1023;
    flat.forestMoisture = 1023;
    flat.riverBand = 0;
    rig.view.open(1, flat);
    rig.said.clear();
    rig.view.setTool(game::RegionTool::Fill);
    CHECK(rig.view.paintFill(128, 128) == 0);
    CHECK(rig.view.region()->tileEditCount() == 0);
    CHECK(rig.view.history().size() == 0);
    REQUIRE_FALSE(rig.said.empty());
    CHECK(rig.said.back().find("Fill refused") != std::string::npos);
}

TEST_CASE("US-202 Opening the region again reads the world file when it is for the same seed") {
    Rig first;
    const sim::Tile steppe = first.tileOf(sim::Biome::Steppe, 60);
    first.view.setTool(game::RegionTool::Rectangle);
    first.view.setPaintBiome(sim::Biome::Water);
    first.view.paintRectangle(steppe.x, steppe.y, steppe.x + 5, steppe.y + 5);
    REQUIRE(first.view.saveWorld());
    const std::uint64_t painted = first.view.region()->fingerprint();

    // The same seed in the same folder: the painted land comes back.
    Rig second(first.base);
    CHECK(second.view.region()->fingerprint() == painted);
    CHECK(second.view.region()->tileEditCount() == first.view.region()->tileEditCount());
    CHECK_FALSE(second.view.worldDirty());
    // Its overview and chunk pictures show the painted land too ("same world" as the game that reads the file).
    CHECK(second.view.edits().tiles.size() == second.view.region()->tileEditCount());

    // Another seed: the file is not applied, and saving does not overwrite it.
    Rig other(first.base, 2);
    CHECK(other.view.region()->tileEditCount() == 0);
    other.view.setTool(game::RegionTool::Brush);
    other.view.paintBrush(100, 100);
    const std::string before = *odysseus::core::readTextFile(first.view.worldFile());
    CHECK_FALSE(other.view.saveWorld());
    CHECK(*odysseus::core::readTextFile(first.view.worldFile()) == before);
    first.base.clear(); // the folder is removed by `first` only once
}

TEST_CASE("US-202 Changing the generator settings keeps the painted tiles") {
    Rig rig;
    const sim::Tile steppe = rig.tileOf(sim::Biome::Steppe, 60);
    rig.view.setTool(game::RegionTool::Rectangle);
    rig.view.setPaintBiome(sim::Biome::Mountain);
    rig.view.paintRectangle(steppe.x, steppe.y, steppe.x + 2, steppe.y + 2);
    const auto painted = rig.view.region()->tileEditList();
    REQUIRE_FALSE(painted.empty());

    // Apply writes region.json and reopens; here the file is a copy so the real one is untouched.
    fs::create_directories(rig.base);
    fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json", rig.base / "region.json", fs::copy_options::overwrite_existing);
    rig.view.setSource(rig.base / "region.json", [] { return std::uint64_t{1}; });
    rig.view.settings().show(true);
    rig.view.settings().setDraft("forestMoisture", 900);
    REQUIRE(rig.view.settings().apply());
    CHECK(rig.view.region()->config().forestMoisture == 900);
    CHECK(rig.view.region()->tileEditList() == painted); // the same tiles, the same biomes
    CHECK(rig.view.worldDirty());
}

TEST_CASE("US-202 Middle button pans while a painting tool is chosen") {
    Rig rig;
    rig.view.setZoom(3);
    rig.view.centreOn(100.0, 100.0);
    rig.view.setTool(game::RegionTool::Brush);
    rig.view.update(pointerAt(480, 300, true, true, eng::PointerButton::Middle));
    rig.view.update(pointerAt(440, 300, true, false, eng::PointerButton::Middle));
    CHECK(rig.view.centreX() > 100.0);
    CHECK(rig.view.region()->tileEditCount() == 0);
}
