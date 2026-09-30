#include "sim/needs.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <string>

using odysseus::sim::CauseOfDeath;
using odysseus::sim::Need;
using odysseus::sim::Needs;
using odysseus::sim::World;
namespace sim = odysseus::sim;

namespace {

sim::SimConfig realConfig() {
    return sim::loadSimConfig(ODYSSEUS_DATA_DIR);
}

} // namespace

TEST_CASE("US-011 Decay") {
    const sim::SimConfig config = realConfig();
    // The hourly drops add up to exactly the daily rate, whatever the rate.
    for (int rate = 0; rate <= 100; ++rate) {
        int total = 0;
        for (int hour = 1; hour <= sim::kHoursPerDay; ++hour) {
            total += sim::hourlyDrop(rate, hour);
        }
        REQUIRE(total == rate);
    }

    // In the world: everyone starts with full needs; after one day (spring) without eating,
    // sleeping, warmth or company, each need has dropped by its configured daily rate.
    World world(42, config);
    REQUIRE(world.population() == config.clan.startingPeople);
    for (const sim::Person& person : world.people()) {
        for (const int value : person.needs.values) {
            REQUIRE(value == config.needs.maximum);
        }
    }
    world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()));
    for (const sim::Person& person : world.people()) {
        CHECK(person.needs[Need::Hunger] == config.needs.maximum - config.needs.dailyDecay[0]);
        CHECK(person.needs[Need::Energy] == config.needs.maximum - config.needs.dailyDecay[1]);
        CHECK(person.needs[Need::Warmth] == config.needs.maximum - config.needs.dailyDecay[2]);
        CHECK(person.needs[Need::Social] == config.needs.maximum - config.needs.dailyDecay[3]);
    }

    // Winter cold: Warmth drops by the winter rate instead.
    Needs winter;
    for (int hour = 1; hour <= sim::kHoursPerDay; ++hour) {
        sim::decayForHour(winter, config.needs, true, hour);
    }
    CHECK(winter[Need::Warmth] == config.needs.maximum - config.needs.winterWarmthDecay);
}

TEST_CASE("US-011 Satisfaction") {
    const sim::NeedsConfig config = realConfig().needs;
    Needs hungry;
    hungry[Need::Hunger] = 30;
    sim::satisfy(hungry, Need::Hunger, config.mealValue, config.maximum);
    CHECK(hungry[Need::Hunger] == 30 + config.mealValue); // rises by the meal's value

    Needs almostFull;
    almostFull[Need::Hunger] = config.maximum - 5;
    sim::satisfy(almostFull, Need::Hunger, config.mealValue, config.maximum);
    CHECK(almostFull[Need::Hunger] == config.maximum); // capped at the maximum
}

TEST_CASE("US-011 Consequence") {
    // Nobody eats yet (choosing actions arrives in US-012), so Hunger reaches 0 on the
    // morning after ceil(maximum / rate) days; after the configured number of days at zero,
    // the next morning they die, and the chronicle records the cause.
    const sim::SimConfig config = realConfig();
    const int rate = config.needs.dailyDecay[0];
    const int firstEmptyMorning = (config.needs.maximum + rate - 1) / rate;
    const int deathMorning = firstEmptyMorning + config.needs.hungerDaysBeforeDeath;
    const auto ticksPerDay = static_cast<std::uint64_t>(config.calendar.ticksPerDay);

    World world(42, config);
    world.runTicks(ticksPerDay * static_cast<std::uint64_t>(deathMorning) - 1);
    CHECK(world.population() == config.clan.startingPeople); // the last evening: still alive
    CHECK(world.people().front().daysAtZeroHunger == config.needs.hungerDaysBeforeDeath);

    world.tick(); // the next morning
    CHECK(world.population() == 0);
    for (const sim::Person& person : world.people()) {
        CHECK_FALSE(person.alive);
        CHECK(person.causeOfDeath == CauseOfDeath::Starvation);
    }
    REQUIRE(world.chronicle().entries().size() == world.people().size());
    const sim::ChronicleEntry& first = world.chronicle().entries().front();
    MESSAGE(sim::describe(first.date), ": ", first.text);
    CHECK(first.text == world.people().front().name + " died of starvation.");
    CHECK(first.date.day == deathMorning);
    CHECK(first.importance >= 50);
}

TEST_CASE("US-011 The starting clan comes from data") {
    const sim::SimConfig config = realConfig();
    World world(7, config);
    const int daysPerYear = world.calendar().daysPerYear();
    for (const sim::Person& person : world.people()) {
        CHECK_FALSE(person.name.empty());
        CHECK(person.ageYears(daysPerYear) >= config.clan.minimumStartingAgeYears);
        CHECK(person.ageYears(daysPerYear) <= config.clan.maximumStartingAgeYears);
    }
    CHECK(world.food() == config.clan.startingFood);
    // The same seed always founds the same clan.
    World again(7, config);
    CHECK(again.people().front().name == world.people().front().name);
    CHECK(again.hash() == world.hash());
}
