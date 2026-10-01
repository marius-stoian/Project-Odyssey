// M6: US-090 the elder's first day, US-091 the crash report, US-092 opt-in local statistics.
#include "game/session_stats.h"
#include "game/tutorial.h"
#include "sim/data.h"
#include "luna/platform/crash.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
namespace game = odysseus::game;

namespace {

game::TutorialScript script() { return game::loadTutorial(fs::path(ODYSSEUS_DATA_DIR) / "hero" / "tutorial.json"); }

fs::path fresh(const char* name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-m6" / name;
    fs::remove_all(folder);
    return folder;
}

} // namespace

TEST_CASE("US-090 Guide") {
    game::Tutorial tutorial;
    tutorial.start(script());
    REQUIRE(tutorial.active());
    CHECK(tutorial.text().find("Gather") != std::string::npos); // the first step: gather
    CHECK_FALSE(tutorial.notify("eat"));                       // out of order does nothing
    CHECK(tutorial.step() == 0);
    CHECK(tutorial.notify("gather"));
    CHECK(tutorial.text().find("eat") != std::string::npos);
    CHECK(tutorial.notify("eat"));
    CHECK(tutorial.text().find("Tend") != std::string::npos);
    CHECK(tutorial.notify("tend"));
    CHECK(tutorial.finished());
    CHECK_FALSE(tutorial.text().empty()); // the closing words
    for (int i = 0; i < 10 * game::Tutorial::kTicksPerSecond; ++i) tutorial.tick();
    CHECK_FALSE(tutorial.active());
}

TEST_CASE("US-090 Skip") {
    game::Tutorial tutorial; // never started: tutorial off in New Game
    CHECK_FALSE(tutorial.active());
    CHECK(tutorial.text().empty());
    CHECK_FALSE(tutorial.notify("gather"));
    tutorial.tick();
    CHECK(tutorial.text().empty());
}

TEST_CASE("US-090 Stuck") {
    game::Tutorial tutorial;
    tutorial.start(script());
    const std::string say = tutorial.text();
    for (int i = 0; i < 119 * game::Tutorial::kTicksPerSecond; ++i) tutorial.tick();
    CHECK(tutorial.text() == say); // not yet
    for (int i = 0; i < 2 * game::Tutorial::kTicksPerSecond; ++i) tutorial.tick();
    CHECK(tutorial.hinting()); // two minutes without progress: a hint
    CHECK(tutorial.text() != say);
    tutorial.notify("gather");
    CHECK_FALSE(tutorial.hinting()); // progress clears it
}

TEST_CASE("US-090 The script names its mistakes") {
    const fs::path folder = fresh("script");
    fs::create_directories(folder);
    std::ofstream(folder / "tutorial.json") << R"({"hintAfterSeconds": 120, "elder": "Elder", "done": "x", "steps": [{"goal": "gather", "say": "a"}]})";
    try {
        game::loadTutorial(folder / "tutorial.json");
        FAIL("a step without a hint was accepted");
    } catch (const odysseus::sim::DataError& error) {
        const std::string message = error.what();
        CHECK(message.find("tutorial.json") != std::string::npos);
        CHECK(message.find("steps[0].hint") != std::string::npos);
    }
}

TEST_CASE("US-091 Crash") {
    const fs::path folder = fresh("crash");
    const fs::path saves = folder / "saves";
    fs::create_directories(saves);
    std::ofstream(saves / "clan.json") << "{}";
    const fs::path log = luna::platform::writeCrashReport(folder / "crash", saves, "test failure");
    CHECK(fs::exists(log));
    std::ostringstream text;
    text << std::ifstream(log).rdbuf();
    CHECK(text.str().find("test failure") != std::string::npos);
    CHECK(fs::exists(folder / "crash" / "last-save" / "clan.json")); // the last save travels with the log
}

TEST_CASE("US-092 Opt-in") {
    const fs::path folder = fresh("stats-off");
    game::SessionStats stats; // nobody agreed
    stats.record("run-started", 100);
    CHECK(stats.events() == 0);
    CHECK(stats.finish(folder, 2000, "x").empty());
    CHECK_FALSE(fs::exists(folder));
}

TEST_CASE("US-092 Local only") {
    const fs::path folder = fresh("stats-on");
    game::SessionStats stats;
    stats.enable(true);
    stats.record("run-started", 0);
    stats.record("year-lived", 400);
    const fs::path file = stats.finish(folder, 2400, "20261001-120000");
    REQUIRE(fs::exists(file));
    std::ostringstream text;
    text << std::ifstream(file).rdbuf();
    CHECK(text.str().find("\"playSeconds\": 120") != std::string::npos);
    CHECK(text.str().find("year-lived") != std::string::npos);
    CHECK(stats.finish(folder, 2500, "again").empty()); // one file a session
}
