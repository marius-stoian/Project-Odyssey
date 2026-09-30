#include "sim/chronicle.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <regex>
#include <set>
#include <string>

using odysseus::sim::World;
namespace sim = odysseus::sim;

namespace {

sim::SimConfig realConfig() {
    return sim::loadSimConfig(ODYSSEUS_DATA_DIR);
}

std::size_t count(const World& world) {
    return world.chronicle().entries().size();
}

const sim::ChronicleEntry& last(const World& world) {
    return world.chronicle().entries().back();
}

void runDays(World& world, int days) {
    world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()) * static_cast<std::uint64_t>(days));
}

// Two living adults of each sex, for the tests that need a couple.
std::pair<int, int> aCouple(const World& world) {
    int woman = -1;
    int man = -1;
    for (const auto& p : world.people()) {
        const int age = p.ageYears(world.calendar().daysPerYear());
        if (p.alive && age >= 16 && age <= 35 && p.partner < 0) {
            if (p.sex == sim::Sex::Female && woman < 0) {
                woman = p.id;
            } else if (p.sex == sim::Sex::Male && man < 0) {
                man = p.id;
            }
        }
    }
    REQUIRE(woman >= 0);
    REQUIRE(man >= 0);
    return {woman, man};
}

} // namespace

TEST_CASE("US-014 Record") {
    // A birth: a couple who conceive at once, with a one-day pregnancy, so the test is exact.
    sim::SimConfig config = realConfig();
    config.life.conceptionPerMille = 1000;
    config.life.pregnancyDays = 1;
    config.life.childbirthDeathPercent = 0;
    World world(42, config);
    const auto [mother, father] = aCouple(world);
    world.pair(mother, father);
    CHECK(last(world).text == world.people()[static_cast<std::size_t>(mother)].name + " and " +
                                  world.people()[static_cast<std::size_t>(father)].name + " became partners.");
    const std::size_t before = world.people().size();
    std::size_t births = 0;
    for (int day = 0; day < 3 && world.people().size() == before; ++day) {
        runDays(world, 1);
    }
    REQUIRE(world.people().size() == before + 1);
    const sim::Person& child = world.people().back();
    for (const auto& entry : world.chronicle().entries()) {
        if (entry.importance == sim::kImportanceBirth) {
            ++births;
            const std::string expected = child.name + " was born to " + world.people()[static_cast<std::size_t>(father)].name +
                                         " and " + world.people()[static_cast<std::size_t>(mother)].name + ".";
            CHECK(entry.text == expected); // the story's own example: "Ura was born to Tok and Maa."
            const std::string line = sim::formatEntry(entry);
            MESSAGE(line);
            CHECK(std::regex_match(line, std::regex("(Spring|Summer|Autumn|Winter), year [0-9]+: .+ was born to .+ and .+\\.")));
        }
    }
    CHECK(births == 1);
    CHECK(child.mother == mother);
    CHECK(child.father == father);

    // The clan's first mammoth, and then an ordinary one.
    world.bringDownMammoth(father);
    CHECK(last(world).importance == sim::kImportanceFirstMammoth);
    CHECK(last(world).text == world.people()[static_cast<std::size_t>(father)].name + " brought down the clan's first mammoth.");
    CHECK(last(world).date.day == world.date().day);
    world.bringDownMammoth(father);
    CHECK(last(world).importance < sim::kImportanceFirstMammoth);

    // A feud: two people who each saw the other steal, three times over.
    World feuding(42, realConfig());
    for (int i = 0; i < 3; ++i) {
        feuding.recordTheft(4, 5);
        feuding.recordTheft(5, 4);
    }
    const std::size_t entries = count(feuding);
    runDays(feuding, 1);
    bool feudRecorded = false;
    for (std::size_t i = entries; i < count(feuding); ++i) {
        const auto& entry = feuding.chronicle().entries()[i];
        if (entry.importance == sim::kImportanceFeud) {
            feudRecorded = entry.text == "A feud broke out between " + feuding.people()[4].name + " and " + feuding.people()[5].name + ".";
            MESSAGE(sim::formatEntry(entry));
        }
    }
    CHECK(feudRecorded);
    CHECK(feuding.feuds().size() == 1);

    // A death: with daily life off nobody eats, and starvation is recorded with its date.
    World starving(42, realConfig());
    starving.setDailyLife(false);
    runDays(starving, 8);
    REQUIRE(count(starving) > 0);
    CHECK(starving.chronicle().entries().front().importance == sim::kImportanceDeath);
    CHECK(starving.chronicle().entries().front().text.find("died of starvation.") != std::string::npos);
}

TEST_CASE("US-014 Filter") {
    // Hundreds of minor events and a few important ones in one year.
    sim::Chronicle chronicle;
    sim::Date spring;
    spring.year = 2;
    for (int i = 0; i < 300; ++i) {
        chronicle.add(spring, sim::kImportanceGift, "Someone gave someone a gift.");
    }
    chronicle.add(spring, sim::kImportanceBirth, "Ura was born to Tok and Maa.");
    chronicle.add(spring, sim::kImportanceFeud, "A feud broke out between Tok and Brak.");
    chronicle.add(spring, sim::kImportanceDeath, "Oren died of old age, 61 years old.");
    sim::Date nextYear = spring;
    nextYear.year = 3;
    chronicle.add(nextYear, sim::kImportanceDeath, "Maa died of the cold.");
    const auto year2 = chronicle.select(2, sim::kDefaultChronicleThreshold);
    REQUIRE(year2.size() == 3); // only the events above the threshold, only that year
    CHECK(year2[0].text == "Ura was born to Tok and Maa.");
    CHECK(chronicle.select(2, 0).size() == 303);
    CHECK(chronicle.select(0, sim::kDefaultChronicleThreshold).size() == 4); // every year

    // The living clan's first year: many small events, a short story.
    World world(42, realConfig());
    world.runTicks(world.calendar().ticksPerYear());
    const auto all = world.chronicle().select(1, 0);
    const auto story = world.chronicle().select(1, sim::kDefaultChronicleThreshold);
    MESSAGE("year 1: ", all.size(), " events, ", story.size(), " worth telling");
    CHECK(all.size() > 50);
    CHECK(story.size() < all.size());
    for (const auto& entry : story) {
        CHECK(entry.importance >= sim::kDefaultChronicleThreshold);
    }
}

TEST_CASE("US-014 A clan lives for generations") {
    World world(42, realConfig());
    world.runTicks(world.calendar().ticksPerYear() * 30);
    int born = 0;
    int oldAge = 0;
    for (const auto& person : world.people()) {
        born += person.mother >= 0 ? 1 : 0;
        oldAge += person.causeOfDeath == sim::CauseOfDeath::OldAge ? 1 : 0;
    }
    MESSAGE("after 30 years: ", world.people().size(), " people ever, ", world.population(), " alive, ", born, " born, ",
            oldAge, " died of old age, ", world.feuds().size(), " feuds now, ", world.mammothsKilled(), " mammoths");
    CHECK(born > 0);
    CHECK(world.population() > 0);
}

TEST_CASE("US-014 No two living people share a name") {
    // Readers must never meet two Kals at once (found while preparing Kill Gate 1).
    for (const std::uint64_t seed : {7ULL, 42ULL}) {
        World world(seed, realConfig());
        for (int decade = 0; decade < 6; ++decade) {
            world.runTicks(world.calendar().ticksPerYear() * 10);
            std::set<std::string> names;
            for (const auto& person : world.people()) {
                if (person.alive) {
                    CHECK(names.insert(person.name).second);
                }
            }
        }
    }
}
