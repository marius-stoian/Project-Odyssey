// US-294 The soak: 100,000 persons live 30 in-game days headless (population, schedules, dealings, a market of traders), twice with the same seed. The save hash of the two runs
// must be identical (determinism, ADR-011) and, in Release, the budget of ADR-022 must hold. This file builds its own executable (odysseus_sim_soak) so the slow run does not
// hold up the quick unit tests.
#include "sim/data.h"
#include "sim/npc_director.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

const rules::InteractionRegistry& shippedInteractions() {
    static const rules::InteractionRegistry registry = [] {
        rules::LoadReport report;
        rules::InteractionRegistry loaded = rules::InteractionRegistry::load(fs::path(ODYSSEUS_DATA_DIR) / "interactions", report);
        for (const auto& error : report.errors) FAIL(error.text());
        return loaded;
    }();
    return registry;
}

#ifdef NDEBUG
constexpr bool kStrictTiming = true; // Release: the budget of ADR-022 is checked
#else
constexpr bool kStrictTiming = false; // Debug with AddressSanitizer is many times slower: only that the run completes and is repeatable
#endif

// ADR-022 (addendum for the director): one in-game day of 100,000 persons costs at most this much CPU time in all (population and director together), and no single tick more
// than the second figure.
constexpr double kDayBudgetMs = 100.0;
constexpr double kWorstTickBudgetMs = 8.0;

struct Lcg {
    std::uint64_t state = 12345;
    std::uint32_t next(std::uint32_t below) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<std::uint32_t>((state >> 33) % below);
    }
};

std::uint64_t fnv(const std::string& text, std::uint64_t seed) {
    std::uint64_t hash = seed ^ 14695981039346656037ULL;
    for (const unsigned char c : text) hash = (hash ^ c) * 1099511628211ULL;
    return hash;
}

struct Soak {
    std::uint64_t saveHash = 0;
    double worstDayMs = 0.0;
    double worstTickMs = 0.0;
    int days = 0;
    int traders = 0;
};

// Builds the crowd, runs `days` days and returns what the save would hold, hashed (the text of the population, the director and the market).
Soak run(int persons, int days, std::uint64_t seed) {
    const int side = 16384; // 512 tiles
    sim::NpcPopulation population(calendar(), needs());
    sim::NpcDirector director;
    sim::TradeMarket market;
    market.setSeed(seed);
    director.setInteractions(&shippedInteractions());
    director.setMarket(&market);
    director.setSeed(seed);
    population.setOpinionConfig(sim::OpinionConfig{});
    director.setPlaces({{"market", side / 2, side / 2, {}}, {"grove", side / 2 + 600, side / 2, {"forage"}}, {"shrine", side / 2, side / 2 + 600, {"shrine"}}});

    rules::Schedule worker;
    worker.day = {{6 * 60, "work", "market"}, {12 * 60, "eat", "home"}, {13 * 60, "work", "market"}, {21 * 60, "sleep", "home"}};
    rules::Schedule farmer;
    farmer.day = {{5 * 60, "work", "grove"}, {19 * 60, "rest", "home"}, {22 * 60, "sleep", "home"}};
    rules::Schedule watcher;
    watcher.day = {{6 * 60, "idle", "home"}, {18 * 60, "go", "shrine"}, {21 * 60, "sleep", "home"}};
    watcher.night = {{21 * 60, "sleep", "home"}};
    const rules::Schedule* schedules[] = {&worker, &farmer, &watcher};

    sim::NpcProfile talker;
    talker.tags = {"npc"};
    talker.classes = {"talker"};
    sim::NpcProfile trader = talker;
    trader.tags = {"npc", "trader"};
    trader.classes = {"trader"};

    rules::TradeProfile goods;
    goods.stock = {{"flint", 4}, {"fur", 4}};
    goods.wants = {"berries"};

    Lcg random;
    random.state = seed;
    Soak result;
    for (int i = 0; i < persons; ++i) {
        const int x = static_cast<int>(random.next(static_cast<std::uint32_t>(side)));
        const int y = static_cast<int>(random.next(static_cast<std::uint32_t>(side)));
        const int index = population.add(i + 1, i % 7 == 0 ? "elder" : "wanderer", 20 * 28 + static_cast<int>(random.next(1000)), static_cast<int>(random.next(50)), x, y);
        director.setHome(index, x, y);
        director.setSchedule(index, *schedules[i % 3]);
        const bool sells = i % 100 == 0;
        director.setProfile(index, sells ? trader : talker);
        if (sells && market.addTrader(i + 1, goods, 0)) ++result.traders;
    }
    population.setFocus(side / 2, side / 2); // the hero stands in the middle of the crowd: a few hundred persons are near, the rest far

    const sim::RegionEconomy economy;
    const std::uint64_t perDay = static_cast<std::uint64_t>(calendar().ticksPerDay);
    for (int day = 0; day < days; ++day) {
        double dayMs = 0.0;
        for (std::uint64_t t = 0; t < perDay; ++t) {
            const auto start = std::chrono::steady_clock::now();
            population.tick();
            director.tick(population);
            if (t == 0) market.dailyUpdate(day, economy);
            const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
            dayMs += ms;
            result.worstTickMs = std::max(result.worstTickMs, ms);
            if (t % 200 == 0) (void)director.takeEvents(); // the game drains them; a soak must not let them pile up
        }
        result.worstDayMs = std::max(result.worstDayMs, dayMs);
        ++result.days;
    }
    result.saveHash = fnv(population.toText(), 1) ^ fnv(director.toText(), 2) ^ fnv(market.toText(), 3) ^ population.hash() ^ director.hash() ^ market.hash();
    return result;
}

} // namespace

TEST_CASE("US-294 Soak: 100,000 persons live 30 days; two runs with the same seed give the same save hash, and the budget of ADR-022 holds") {
    const Soak first = run(100000, 30, 7);
    const Soak second = run(100000, 30, 7);
    REQUIRE(first.days == 30);
    CHECK(first.traders == 1000);
    CHECK(first.saveHash == second.saveHash);
    MESSAGE("30 days: worst day ", first.worstDayMs, " ms and ", second.worstDayMs, " ms; worst tick ", first.worstTickMs, " ms and ", second.worstTickMs, " ms; save hash ", first.saveHash);
    if (kStrictTiming) {
        CHECK(first.worstDayMs < kDayBudgetMs);
        CHECK(second.worstDayMs < kDayBudgetMs);
        CHECK(first.worstTickMs < kWorstTickBudgetMs);
        CHECK(second.worstTickMs < kWorstTickBudgetMs);
    }
}

TEST_CASE("US-294 Soak: another seed gives another save hash (the hash really covers the run)") {
    const Soak a = run(2000, 5, 7);
    const Soak b = run(2000, 5, 8);
    const Soak again = run(2000, 5, 7);
    CHECK(a.saveHash == again.saveHash);
    CHECK(a.saveHash != b.saveHash);
}
