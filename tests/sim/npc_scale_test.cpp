// US-263 The NPC store at scale: the spatial grid, detail by distance (near persons are simulated hour by hour, far ones at the end of the day) and a
// load of 100,000 generated persons within the budget of ADR-022.
#include "sim/npc_population.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <tuple>

namespace sim = odysseus::sim;
namespace fs = std::filesystem;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

// A small deterministic generator (the tests must not depend on a library's random numbers).
struct Lcg {
    std::uint64_t state = 12345;
    std::uint32_t next(std::uint32_t below) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<std::uint32_t>((state >> 33) % below);
    }
};

// `count` persons spread over a square region `side` pixels wide.
sim::NpcPopulation crowd(int count, int side, std::uint64_t seed = 12345) {
    sim::NpcPopulation population(calendar(), needs());
    Lcg random;
    random.state = seed;
    for (int i = 0; i < count; ++i) {
        population.add(i + 1, i % 7 == 0 ? "elder" : "wanderer", 20 * 28 + static_cast<int>(random.next(1000)), static_cast<int>(random.next(50)),
                       static_cast<int>(random.next(static_cast<std::uint32_t>(side))), static_cast<int>(random.next(static_cast<std::uint32_t>(side))));
    }
    return population;
}

#ifdef NDEBUG
constexpr bool kStrictTiming = true; // Release: the budgets of ADR-022 are checked
#else
constexpr bool kStrictTiming = false; // Debug with AddressSanitizer is many times slower: only that the run completes
#endif

// ADR-022 budgets (D-06 PC, Release): one in-game day of 100,000 persons costs at most this much CPU time in all, and no single tick more than the
// second figure (the day-end tick, which brings every person to the end of the day).
constexpr double kDayBudgetMs = 50.0;
constexpr double kWorstTickBudgetMs = 8.0;
constexpr double kSaveBudgetMs = 100.0; // the autosave of the whole game must stay under 200 ms (US-080)
constexpr double kLoadBudgetMs = 2000.0;

double millisecondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

} // namespace

TEST_CASE("US-263 Grid: near() finds exactly the persons within the radius, also after they move") {
    sim::NpcPopulation population = crowd(3000, 4000);
    const auto brute = [&](int x, int y, int radius) {
        std::vector<int> expected;
        for (std::size_t i = 0; i < population.size(); ++i) {
            const std::int64_t dx = population.x(static_cast<int>(i)) - x;
            const std::int64_t dy = population.y(static_cast<int>(i)) - y;
            if (dx * dx + dy * dy <= static_cast<std::int64_t>(radius) * radius) expected.push_back(static_cast<int>(i));
        }
        return expected;
    };
    Lcg random;
    random.state = 99;
    for (int query = 0; query < 25; ++query) {
        const int x = static_cast<int>(random.next(5000)) - 500; // also queries that reach outside the region
        const int y = static_cast<int>(random.next(5000)) - 500;
        const int radius = 20 + static_cast<int>(random.next(900));
        CHECK(population.near(x, y, radius) == brute(x, y, radius));
    }
    // Moving a person moves them in the grid.
    population.move(5, 3900, 3900);
    population.move(6, -300, -300);
    for (const auto& [x, y, radius] : {std::tuple{3900, 3900, 10}, std::tuple{-300, -300, 10}, std::tuple{500, 500, 600}}) {
        CHECK(population.near(x, y, radius) == brute(x, y, radius));
    }
    CHECK(population.near(3900, 3900, 10).size() >= 1);
    const auto at = population.near(3900, 3900, 1);
    CHECK(std::find(at.begin(), at.end(), 5) != at.end());
}

TEST_CASE("US-263 Near and far: a person who walks out of the near radius and back loses and jumps nothing") {
    const int day = calendar().ticksPerDay;
    const int hour = day / 24;
    // Three copies of the same person: always near, always far, and one that walks away at noon of day 1 and returns at noon of day 2.
    const auto make = [&](int x) {
        sim::NpcPopulation population(calendar(), needs());
        population.add(1, "wanderer", 20 * 28, 0, x, 500);
        population.setFocus(500, 500);
        return population;
    };
    sim::NpcPopulation always = make(500);
    sim::NpcPopulation never = make(5000);
    sim::NpcPopulation wanderer = make(500);
    REQUIRE(always.isNear(0));
    REQUIRE_FALSE(never.isNear(0));

    // Inside a day the near person is simulated hour by hour; the far one is not.
    always.runTicks(static_cast<std::uint64_t>(hour) * 6);
    never.runTicks(static_cast<std::uint64_t>(hour) * 6);
    CHECK(always.hoursApplied(0) == 6);
    CHECK(never.hoursApplied(0) == 0);
    CHECK(always.need(0, sim::Need::Hunger) < 100);
    CHECK(never.need(0, sim::Need::Hunger) == 100);
    CHECK(always.nearCount() == 1);
    CHECK(never.nearCount() == 0);

    wanderer.runTicks(static_cast<std::uint64_t>(day) + static_cast<std::uint64_t>(hour) * 12); // noon of day 1
    wanderer.move(0, 5000, 500);                                                                    // walks out of the radius
    wanderer.runTicks(static_cast<std::uint64_t>(day));                                             // noon of day 2
    wanderer.move(0, 500, 500);                                                                     // and back
    wanderer.runTicks(static_cast<std::uint64_t>(day) * 2 - static_cast<std::uint64_t>(hour) * 12); // the end of day 4
    always.runTicks(static_cast<std::uint64_t>(day) * 4 - static_cast<std::uint64_t>(hour) * 6);
    never.runTicks(static_cast<std::uint64_t>(day) * 4 - static_cast<std::uint64_t>(hour) * 6);
    REQUIRE(always.day() == 4);
    REQUIRE(never.day() == 4);
    REQUIRE(wanderer.day() == 4);

    // At the end of a day they are the same, whichever way the days were simulated: needs, age and the memories of the days.
    for (const sim::NpcPopulation* other : {&never, &wanderer}) {
        CHECK(other->ageDays(0) == always.ageDays(0));
        for (int n = 0; n < static_cast<int>(sim::kNeedCount); ++n) CHECK(other->need(0, static_cast<sim::Need>(n)) == always.need(0, static_cast<sim::Need>(n)));
        REQUIRE(other->noteCount(0) == always.noteCount(0));
        for (int k = 0; k < always.noteCount(0); ++k) {
            CHECK(other->note(0, k).text == always.note(0, k).text);
            CHECK(other->note(0, k).day == always.note(0, k).day);
            CHECK(other->note(0, k).feeling == always.note(0, k).feeling);
        }
        CHECK(other->hoursApplied(0) == 0);
    }
    CHECK(always.ageDays(0) == 20 * 28 + 4);
    // Mid-day too, a person who arrives near catches the missed hours up at once, with no loss.
    sim::NpcPopulation late = make(5000);
    late.runTicks(static_cast<std::uint64_t>(hour) * 5);
    late.move(0, 500, 500);
    late.runTicks(static_cast<std::uint64_t>(hour)); // the sixth hour mark: near now, brought up to hour 6
    sim::NpcPopulation steady = make(500);
    steady.runTicks(static_cast<std::uint64_t>(hour) * 6);
    CHECK(late.hoursApplied(0) == 6);
    CHECK(late.need(0, sim::Need::Hunger) == steady.need(0, sim::Need::Hunger));
    CHECK(late.need(0, sim::Need::Energy) == steady.need(0, sim::Need::Energy));
}

TEST_CASE("US-263 Save: a crowd saves compactly and loads to the same state") {
    sim::NpcPopulation population = crowd(500, 3000);
    population.setFocus(1500, 1500);
    population.runTicks(static_cast<std::uint64_t>(calendar().ticksPerDay) + 1234);
    const std::string text = population.toText();
    sim::NpcPopulation loaded = sim::NpcPopulation::fromText(text, calendar(), needs());
    loaded.setFocus(1500, 1500);
    CHECK(loaded.size() == 500);
    CHECK(loaded.hash() == population.hash());
    CHECK(loaded.toText() == text);
    // The grid is rebuilt by loading, and both go on the same way.
    population.runTicks(5000);
    loaded.runTicks(5000);
    CHECK(loaded.hash() == population.hash());
    CHECK(loaded.near(1500, 1500, 400) == population.near(1500, 1500, 400));
    CHECK(text.size() < 500 * 130); // about 100 characters a person
}

TEST_CASE("US-263 Load: 100,000 persons, one in-game day, within the budget of ADR-022, the same hash in two runs") {
    const int persons = 100000;
    const int side = 16384; // 512 tiles: a region of 262,144 tiles with one person to 2.6 tiles
    const std::uint64_t day = static_cast<std::uint64_t>(calendar().ticksPerDay);

    const auto runDay = [&](double& totalMs, double& worstMs) {
        sim::NpcPopulation population = crowd(persons, side);
        population.setFocus(side / 2, side / 2); // the hero stands in the middle of the crowd: a few hundred are near, the rest far
        totalMs = 0.0;
        worstMs = 0.0;
        for (std::uint64_t t = 0; t < day; ++t) {
            const auto start = std::chrono::steady_clock::now();
            population.tick();
            const double ms = millisecondsSince(start);
            totalMs += ms;
            worstMs = std::max(worstMs, ms);
        }
        REQUIRE(population.day() == 1);
        return population.hash();
    };
    double totalA = 0.0, worstA = 0.0, totalB = 0.0, worstB = 0.0;
    const std::uint64_t first = runDay(totalA, worstA);
    const std::uint64_t second = runDay(totalB, worstB);
    CHECK(first == second); // the hash is stable over two runs
    MESSAGE("100,000 persons, one day: total ", totalA, " ms and ", totalB, " ms; worst tick ", worstA, " ms and ", worstB, " ms");
    if (kStrictTiming) {
        CHECK(totalA < kDayBudgetMs);
        CHECK(totalB < kDayBudgetMs);
        CHECK(worstA < kWorstTickBudgetMs);
        CHECK(worstB < kWorstTickBudgetMs);
    }

    // The save of a crowd that size: measured and kept in ADR-022 (the autosave of the clan must stay under 200 ms).
    sim::NpcPopulation population = crowd(persons, side);
    const auto saveStart = std::chrono::steady_clock::now();
    const std::string text = population.toText();
    const double saveMs = millisecondsSince(saveStart);
    MESSAGE("save of 100,000 persons: ", saveMs, " ms, ", text.size() / 1024, " KiB");
    CHECK(text.size() < static_cast<std::size_t>(persons) * 100);
    const auto loadStart = std::chrono::steady_clock::now();
    const sim::NpcPopulation loaded = sim::NpcPopulation::fromText(text, calendar(), needs());
    const double loadMs = millisecondsSince(loadStart);
    MESSAGE("load of 100,000 persons: ", loadMs, " ms");
    CHECK(loaded.hash() == population.hash());
    if (kStrictTiming) {
        CHECK(saveMs < kSaveBudgetMs);
        CHECK(loadMs < kLoadBudgetMs);
    }
}
