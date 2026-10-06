// M4 in the game: US-043 streaming, US-080 autosave and backups, US-083 developer tools, and the region as a level.
#include "game/level.h"
#include "game/odyssey_game.h"
#include "game/region_level.h"
#include "luna/engine/chunk_streamer.h"
#include "luna/engine/renderer.h"
#include "sim/region.h"
#include "sim/save.h"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
namespace game = odysseus::game;
namespace sim = odysseus::sim;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Pointer;
using luna::engine::PointerButton;

namespace {

fs::path campLevel() { return fs::path(ODYSSEUS_DEMO_LEVEL).parent_path() / "camp.json"; }

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

Intents clickAt(int x, int y) {
    Intents intents;
    Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.held[static_cast<std::size_t>(PointerButton::Left)] = true;
    pointer.pressed[static_cast<std::size_t>(PointerButton::Left)] = true;
    intents.setPointer(pointer);
    return intents;
}

struct Play {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Play(const fs::path& level) : odyssey(ODYSSEUS_DATA_DIR, level) { odyssey.setViewScales(1, 1); odyssey.start(renderer); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
};

} // namespace

TEST_CASE("US-043 Streaming") {
    sim::Region region(1, sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"));
    const int chunkPixels = region.config().chunkSize * game::kTileSize; // 1024 pixels
    luna::engine::ChunkStreamer streamer(chunkPixels, region.chunksPerSide(), 2);
    int loads = 0;
    const auto load = [&](int cx, int cy) {
        region.chunk(cx, cy); // made on demand
        ++loads;
    };
    const auto unload = [](int, int) {};
    // The player walks from one side of the region to the other, east, at walking speed (4.8 pixels a tick), the view 960 x 540.
    double x = 40.0 * game::kTileSize;
    const double y = region.start().y * game::kTileSize;
    double worst = 0.0;
    bool popped = false;
    for (int tick = 0; tick < 2500; ++tick) {
        x += 4.8;
        const odysseus::core::Rect view{static_cast<int>(x) - 480, static_cast<int>(y) - 270, 960, 540};
        const auto began = std::chrono::steady_clock::now();
        streamer.update(view, load, unload, 2);
        worst = std::max(worst, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count());
        if (!streamer.covers(view)) popped = true;
    }
    CHECK_FALSE(popped);       // every chunk in view was loaded before it was seen
    CHECK(worst < 50.0);       // no tick stalled for 50 ms
    CHECK(loads > 4);          // and it did walk across chunks
    CHECK(streamer.loadedCount() < 40); // chunks far behind were let go
}

TEST_CASE("US-080 Autosave and backups") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-m4-saves";
    fs::remove_all(folder);
    Play play(campLevel());
    play.odyssey.setSaveDirectory(folder);
    play.odyssey.setClanSpeedFromTools(10);
    // The end of the first game day: one autosave, well under 200 ms.
    play.tick(260); // 2600 simulation ticks: a day is 2400
    REQUIRE(play.odyssey.autosaves() >= 1);
#pragma warning(suppress : 4996)
    const double widen = std::getenv("GITHUB_ACTIONS") != nullptr ? 5.0 : 1.0; // GitHub's shared runners are slower and noisy (X-M11: 202 ms in Debug there, 200 the budget)
    CHECK(play.odyssey.lastAutosaveMilliseconds() < 200.0 * widen);
    CHECK(fs::exists(folder / "clan.json"));
    // Five more days: the latest save and three backups.
    play.tick(1300);
    for (int number = 1; number <= 3; ++number) CHECK(fs::exists(sim::backupPath(folder / "clan.json", number)));
    CHECK_FALSE(fs::exists(sim::backupPath(folder / "clan.json", 4)));
    // The latest save is damaged: loading takes the newest backup and says what happened.
    std::ofstream(folder / "clan.json", std::ios::trunc) << "{ broken";
    Play fresh(campLevel());
    fresh.odyssey.setSaveDirectory(folder);
    CHECK(fresh.odyssey.loadAutosave());
    CHECK(fresh.odyssey.message().find("backup") != std::string::npos);
    CHECK(fresh.odyssey.clan()->population() > 0);
    CHECK(fresh.odyssey.clan()->date().day >= 1);
}

TEST_CASE("US-083 Developer tools") {
    Play play(campLevel());
    play.tick(40);
#ifndef NDEBUG
    CHECK_FALSE(play.odyssey.devToolsOpen());
    play.tick(1, pressing(Intent::DevTools));
    REQUIRE(play.odyssey.devToolsOpen());
    // Click a person: their needs, memories, relationships and AI scores are shown.
    const auto view = play.odyssey.cameraView();
    const game::Figure* target = nullptr;
    int index = -1;
    for (std::size_t i = 0; i < play.odyssey.clanView().figures().size(); ++i) {
        const game::Figure& figure = play.odyssey.clanView().figures()[i];
        const int x = static_cast<int>(figure.x) - view.x;
        const int y = static_cast<int>(figure.y) - view.y;
        if (figure.present && x > 500 && x < 920 && y > 160 && y < 500) {
            target = &figure;
            index = static_cast<int>(i);
        }
    }
    REQUIRE(target != nullptr);
    play.tick(1, clickAt(static_cast<int>(target->x) - view.x, static_cast<int>(target->y) - view.y - 20));
    CHECK(play.odyssey.selectedPerson() == index);
    play.renderer.clear();
    play.odyssey.render(play.renderer, 1.0);
    CHECK(play.renderer.draws().size() > 20);
    // Time: the 16x button sets the speed, the Day button skips to the next day.
    play.tick(1, clickAt(6 + 3 * 34 + 5, 58));
    CHECK(play.odyssey.clanSpeed() == 16);
    const auto day = play.odyssey.clan()->date().day;
    play.tick(1, clickAt(6 + 4 * 34 + 5, 58));
    CHECK(play.odyssey.clan()->date().day > day);
    play.tick(1, pressing(Intent::DevTools));
    CHECK_FALSE(play.odyssey.devToolsOpen());
#else
    // A Release build: the key opens nothing.
    play.tick(1, pressing(Intent::DevTools));
    CHECK_FALSE(play.odyssey.devToolsOpen());
#endif
}

TEST_CASE("US-040 The region as a level") {
    Play play(campLevel());
    play.odyssey.loadRegion(1);
    REQUIRE(play.odyssey.region() != nullptr);
    CHECK(play.odyssey.level().width == 256);
    CHECK(play.odyssey.level().height == 256);
    CHECK(play.odyssey.clanOn());
    REQUIRE(play.odyssey.rivals() != nullptr);
    CHECK(play.odyssey.rivals()->clans().size() == 2);
    CHECK_FALSE(play.odyssey.level().plants.empty()); // trees, berries
    // The hero stands where the region starts, and the region is not editable.
    const sim::Tile start = play.odyssey.region()->start();
    CHECK(play.odyssey.level().heroStart.x == start.x * game::kTileSize + game::kTileSize / 2);
    play.tick(2, pressing(Intent::ModeEditor));
    CHECK(play.odyssey.mode() == game::Mode::Game);
    play.tick(60);
    play.renderer.clear();
    play.odyssey.render(play.renderer, 1.0);
    CHECK(play.renderer.draws().size() > 10);
}
