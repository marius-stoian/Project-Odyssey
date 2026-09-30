#include "sim/data.h"
#include "sim/game_clock.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace fs = std::filesystem;
using odysseus::sim::GameClock;
using odysseus::sim::Season;
using odysseus::sim::Speed;
using odysseus::sim::World;

namespace {

odysseus::sim::SimConfig realConfig() {
    return odysseus::sim::loadSimConfig(ODYSSEUS_DATA_DIR);
}

} // namespace

TEST_CASE("US-010 Calendar") {
    World world(42, realConfig());
    const auto& calendar = world.calendar();
    CHECK(world.date().year == 1);
    CHECK(world.date().season == Season::Spring);
    CHECK(world.date().dayOfSeason == 1);

    std::set<Season> seasonsSeen;
    for (int day = 0; day < calendar.daysPerYear(); ++day) {
        seasonsSeen.insert(world.date().season);
        world.runTicks(static_cast<std::uint64_t>(calendar.ticksPerDay()));
    }
    CHECK(seasonsSeen.size() == 4);                          // all four seasons passed
    CHECK(world.date().day == calendar.daysPerYear());       // the day counter matches the year length
    CHECK(world.date().year == 2);                           // and a new year began
    CHECK(world.date().season == Season::Spring);
    CHECK(world.date().dayOfSeason == 1);
    CHECK(world.ticks() == calendar.ticksPerYear());
}

TEST_CASE("US-010 Determinism") {
    World first(42, realConfig());
    World second(42, realConfig());
    first.runTicks(10'000);
    second.runTicks(10'000);
    CHECK(first.hash() == second.hash());   // same seed, same inputs: the same world
    CHECK(first.temperature() == second.temperature());

    World other(43, realConfig());
    other.runTicks(10'000);
    CHECK(other.hash() != first.hash());    // a different seed is a different world
    MESSAGE("world hash after 10,000 ticks with seed 42: ", first.hash());
}

TEST_CASE("US-010 Speed control") {
    constexpr std::uint64_t kOneRealSecond = 1'000'000'000ULL;
    GameClock clock;
    clock.setSpeed(Speed::Fastest);                        // 4x
    CHECK(clock.advance(kOneRealSecond) == 4 * GameClock::kTicksPerGameSecond); // 4 game seconds

    clock.setSpeed(Speed::Normal);
    CHECK(clock.advance(kOneRealSecond) == GameClock::kTicksPerGameSecond);
    clock.setSpeed(Speed::Paused);
    CHECK(clock.advance(kOneRealSecond) == 0);

    SUBCASE("no drift with odd frame times at 4x") {
        clock.setSpeed(Speed::Fastest);
        std::uint64_t ticks = 0;
        for (int frame = 0; frame < 144; ++frame) {        // one second of 144 Hz frames
            ticks += clock.advance(kOneRealSecond / 144);
        }
        CHECK(ticks >= 79);
        CHECK(ticks <= 80);
    }
}

TEST_CASE("Content errors name the file and the field") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-tests-bad-data" / "sim";
    fs::create_directories(folder);
    std::ofstream(folder / "calendar.json") << R"({ "ticksPerDay": 2400, "daysPerSeason": "seven" })";
    try {
        odysseus::sim::loadSimConfig(folder.parent_path());
        FAIL("a bad content file must be rejected");
    } catch (const odysseus::sim::DataError& error) {
        const std::string message = error.what();
        CHECK(message.find("calendar.json") != std::string::npos);
        CHECK(message.find("daysPerSeason") != std::string::npos);
    }
}
