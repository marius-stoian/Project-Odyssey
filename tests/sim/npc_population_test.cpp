// US-262 Placed people are persons: needs, ageing, memories, families, saving and the hash, in the compact store of the NPC population.
#include "sim/npc_population.h"

#include "sim/data.h"

#include <doctest/doctest.h>

#include <filesystem>

namespace sim = odysseus::sim;
namespace fs = std::filesystem;

namespace {

sim::CalendarConfig calendar() { return sim::loadCalendarConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "calendar.json"); }
sim::NeedsConfig needs() { return sim::loadNeedsConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "needs.json"); }

sim::NpcPopulation make() {
    sim::NpcPopulation population(calendar(), needs());
    population.add(7, "wanderer", 20 * 28, 0, 100, 120);
    population.add(9, "wanderer", 30 * 28, 3, 240, 120);
    return population;
}

} // namespace

TEST_CASE("US-262 Person: a day passes, the needs change and a memory of the day exists") {
    sim::NpcPopulation population = make();
    REQUIRE(population.size() == 2);
    const int one = population.indexOf(7);
    REQUIRE(one >= 0);
    CHECK(population.indexOf(8) == -1);
    CHECK(population.id(one) == 7);
    CHECK(population.kind(one) == "wanderer");
    CHECK(population.family(population.indexOf(9)) == 3);
    CHECK(population.noteCount(one) == 0);
    for (int n = 0; n < static_cast<int>(sim::kNeedCount); ++n) CHECK(population.need(one, static_cast<sim::Need>(n)) == 100);

    const int ticksPerDay = calendar().ticksPerDay;
    population.runTicks(static_cast<std::uint64_t>(ticksPerDay) - 1);
    CHECK(population.ageDays(one) == 20 * 28); // not yet
    population.tick();                          // the day ends
    CHECK(population.day() == 1);
    CHECK(population.ageDays(one) == 20 * 28 + 1);
    CHECK(population.need(one, sim::Need::Hunger) == 100 - needs().dailyDecay[0]);
    CHECK(population.need(one, sim::Need::Energy) == 100 - needs().dailyDecay[1]);
    REQUIRE(population.noteCount(one) == 1);
    const auto note = population.note(one, 0);
    CHECK(note.day == 1);
    CHECK_FALSE(note.text.empty());

    // The needs keep moving and never leave 0..100, and nobody dies of it (the land provides).
    population.runTicks(static_cast<std::uint64_t>(ticksPerDay) * 60);
    for (int n = 0; n < static_cast<int>(sim::kNeedCount); ++n) {
        const int value = population.need(one, static_cast<sim::Need>(n));
        CHECK(value >= 0);
        CHECK(value <= 100);
    }
    CHECK(population.ageDays(one) == 20 * 28 + 61);
    CHECK(population.noteCount(one) == sim::NpcPopulation::kNotesPerPerson); // only the last six days are kept
    CHECK(population.note(one, sim::NpcPopulation::kNotesPerPerson - 1).day == 61);
    CHECK(population.note(one, 0).day == 56);
}

TEST_CASE("US-262 Saved: age, needs and memories are the same after saving and loading") {
    sim::NpcPopulation population = make();
    const int ticksPerDay = calendar().ticksPerDay;
    population.runTicks(static_cast<std::uint64_t>(ticksPerDay) * 3);
    const int one = population.indexOf(7);
    population.meetHero(one); // met the hero on day 3
    population.setFamily(one, 5);

    const std::string text = population.toText();
    const sim::NpcPopulation loaded = sim::NpcPopulation::fromText(text, calendar(), needs());
    REQUIRE(loaded.size() == 2);
    const int again = loaded.indexOf(7);
    CHECK(loaded.ageDays(again) == population.ageDays(one));
    CHECK(loaded.family(again) == 5);
    CHECK(loaded.x(again) == 100);
    for (int n = 0; n < static_cast<int>(sim::kNeedCount); ++n) CHECK(loaded.need(again, static_cast<sim::Need>(n)) == population.need(one, static_cast<sim::Need>(n)));
    REQUIRE(loaded.noteCount(again) == population.noteCount(one));
    for (int k = 0; k < loaded.noteCount(again); ++k) {
        CHECK(loaded.note(again, k).text == population.note(one, k).text);
        CHECK(loaded.note(again, k).day == population.note(one, k).day);
        CHECK(loaded.note(again, k).feeling == population.note(one, k).feeling);
    }
    CHECK(loaded.note(again, loaded.noteCount(again) - 1).text == "met the hero");
    CHECK(loaded.hash() == population.hash());
    CHECK(loaded.toText() == text); // the same text again
    // The loaded people carry on exactly as the originals would.
    sim::NpcPopulation a = population;
    sim::NpcPopulation b = sim::NpcPopulation::fromText(text, calendar(), needs());
    a.runTicks(static_cast<std::uint64_t>(ticksPerDay) * 5);
    b.runTicks(static_cast<std::uint64_t>(ticksPerDay) * 5);
    CHECK(a.hash() == b.hash());

    // A damaged or newer save is refused with a message, not half read.
    CHECK_THROWS_AS(sim::NpcPopulation::fromText("{ not json", calendar(), needs()), odysseus::sim::DataError);
    CHECK_THROWS_AS(sim::NpcPopulation::fromText(R"({"version":99,"ticks":0,"people":[]})", calendar(), needs()), odysseus::sim::DataError);
    CHECK_THROWS_AS(sim::NpcPopulation::fromText(R"({"version":1,"people":[]})", calendar(), needs()), odysseus::sim::DataError);
}

TEST_CASE("US-262 Determinism: the same persons and ticks give the same hash; a change changes it") {
    sim::NpcPopulation first = make();
    sim::NpcPopulation second = make();
    first.runTicks(5000);
    second.runTicks(5000);
    CHECK(first.hash() == second.hash());
    second.meetHero(0);
    CHECK(first.hash() != second.hash());
    sim::NpcPopulation third = make();
    third.runTicks(5001);
    sim::NpcPopulation fourth = make();
    fourth.runTicks(5000);
    CHECK(third.hash() != fourth.hash());
    // Adding the same id twice is the same person.
    CHECK(first.add(7, "elder", 1, 1, 0, 0) == first.indexOf(7));
    CHECK(first.size() == 2);
}
