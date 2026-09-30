// US-111 Quarrel, blame and revenge (M2b story engine).
#include "sim/chronicle.h"
#include "sim/report.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <string>

using odysseus::sim::EventKind;
using odysseus::sim::World;
namespace sim = odysseus::sim;

using namespace story_test;

TEST_CASE("US-111 Quarrel") {
    World world(42, realConfig());
    world.recordTheft(3, 4);
    world.recordTheft(4, 3);
    const int before = world.opinion(3, 4);
    const int loss = world.config().story.quarrel.opinionLoss;
    const int event = world.quarrel(3, 4);
    const sim::ChronicleEntry* entry = world.chronicle().find(event);
    REQUIRE(entry != nullptr);
    MESSAGE(sim::formatEntry(*entry));
    CHECK(entry->kind == EventKind::Quarrel);
    CHECK(entry->text.find("quarrelled over stolen meat") != std::string::npos); // it says why
    CHECK_FALSE(entry->causes.empty());
    // Both remember it and think less of each other.
    for (const int who : {3, 4}) {
        const sim::Person& person = world.people()[static_cast<std::size_t>(who)];
        const sim::Memory* memory = memoryOf(person, sim::MemoryKind::Quarrel);
        REQUIRE(memory != nullptr);
        CHECK(memory->event == event);
        CHECK(memory->subject == (who == 3 ? 4 : 3));
    }
    CHECK(world.opinion(3, 4) == before - loss);
    CHECK(world.opinion(4, 3) == before - loss);
}

TEST_CASE("US-111 Quarrel happens when short-tempered people meet") {
    // Hungry people who dislike each other quarrel; the land is barren so hunger comes on its own.
    sim::SimConfig config = barrenConfig();
    config.story.quarrel.calmPercent = 0;
    config.story.quarrel.irritablePercent = 100;
    config.story.quarrel.grudgeMeetPercent = 100;
    World world(42, config);
    makeEnemies(world, 3, 4);
    runDays(world, 4); // by now hunger is low enough to make them short-tempered
    bool found = false;
    for (const auto* quarrel : entriesOf(world, EventKind::Quarrel)) {
        if (quarrel->who == 3 && quarrel->other == 4) {
            found = true;
            CHECK(quarrel->text.find("while hungry") != std::string::npos);
        }
    }
    CHECK(found);
}

TEST_CASE("US-111 Blame") {
    // Two partners; the one who witnessed a theft knows of it. When the other starves, the
    // survivor blames the thief and remembers it for life.
    World world(42, barrenConfig());
    world.pair(5, 6);
    world.recordTheft(3, 6); // 6 saw person 3 steal
    runDays(world, 12);
    const sim::Person& griever = world.people()[6];
    const sim::ChronicleEntry* blame = nullptr;
    for (const auto* candidate : entriesOf(world, EventKind::Blame)) {
        if (candidate->who == 6 && candidate->aux == 5) {
            blame = candidate;
        }
    }
    REQUIRE(blame != nullptr);
    MESSAGE(sim::formatEntry(*blame));
    CHECK(blame->other == 3);
    CHECK(blame->text == griever.name + " blamed " + world.people()[3].name + " for " + world.people()[5].name + "'s death, because " +
                             world.people()[3].name + " had stolen from the store.");
    REQUIRE(blame->causes.size() == 1);
    CHECK(world.chronicle().find(blame->causes.front())->kind == EventKind::Death);
    const sim::Memory* memory = memoryOf(griever, sim::MemoryKind::Blame);
    REQUIRE(memory != nullptr);
    CHECK(memory->major); // remembered for life
    CHECK(memory->event == blame->id);
    CHECK(world.opinion(6, 3) <= -world.config().story.blame.opinionLoss);
}

TEST_CASE("US-111 Blame for a hunt someone led, once per death") {
    World world(42, realConfig());
    const int death = world.recordTheft(9, -1); // any earlier event will do as the "death"
    const int first = world.blame(0, 1, 2, death, "because Enko led the hunt");
    CHECK(first >= 0);
    CHECK(world.blame(0, 1, 2, death, "because Enko led the hunt") == -1); // no second blame for the same death
    CHECK(world.blame(0, 0, 2, death, "self") == -1);                        // nobody blames themselves
    const auto* entry = world.chronicle().find(first);
    CHECK(entry->text == world.people()[0].name + " blamed " + world.people()[1].name + " for " + world.people()[2].name +
                             "'s death, because Enko led the hunt.");
}

TEST_CASE("US-111 Revenge ends in a fight") {
    sim::SimConfig config = realConfig();
    config.story.revenge.exileClanOpinion = -100; // the clan never turns on the aggressor
    config.story.revenge.fightDeathPercent = 0;   // nobody dies here: injury
    World world(42, config);
    makeEnemies(world, 4, 5);
    runDays(world, 1);
    REQUIRE(world.feuds().size() == 1);
    const int event = world.takeRevenge(4, 5);
    const sim::ChronicleEntry* entry = world.chronicle().find(event);
    REQUIRE(entry != nullptr);
    MESSAGE(sim::formatEntry(*entry));
    CHECK(entry->kind == EventKind::Revenge);
    CHECK(entry->text.find("attacked") != std::string::npos);
    CHECK(entry->text.find("over stolen meat") != std::string::npos);                  // why
    CHECK(entry->text.find("lost the fight and was badly hurt") != std::string::npos); // which
    REQUIRE_FALSE(entry->causes.empty());
    CHECK(world.chronicle().find(entry->causes.front())->kind == EventKind::Feud);
    const bool someoneHurt = world.people()[4].health == sim::Health::Injured || world.people()[5].health == sim::Health::Injured;
    CHECK(someoneHurt);
    // The hurt mend in time.
    runDays(world, 14);
    CHECK(world.people()[4].health != sim::Health::Injured);
    CHECK(world.people()[5].health != sim::Health::Injured);
    CHECK_FALSE(entriesOf(world, EventKind::Recovery).empty());
}

TEST_CASE("US-111 Revenge can end in death, and the kin blame the killer") {
    sim::SimConfig config = realConfig();
    config.story.revenge.exileClanOpinion = -100;
    config.story.revenge.fightDeathPercent = 100;
    World world(42, config);
    world.pair(5, 7); // person 5 has a partner who will grieve
    makeEnemies(world, 4, 5);
    runDays(world, 1);
    const int event = world.takeRevenge(4, 5);
    const sim::ChronicleEntry* entry = world.chronicle().find(event);
    REQUIRE(entry != nullptr);
    CHECK(entry->text.find("was killed") != std::string::npos);
    const sim::Person& four = world.people()[4];
    const sim::Person& five = world.people()[5];
    REQUIRE((!four.alive || !five.alive));
    const sim::Person& dead = four.alive ? five : four;
    const sim::Person& killer = four.alive ? four : five;
    CHECK(dead.causeOfDeath == sim::CauseOfDeath::Fight);
    bool deathLine = false;
    for (const auto* death : entriesOf(world, EventKind::Death)) {
        if (death->who == dead.id && death->text == dead.name + " was killed by " + killer.name + " in a fight.") {
            deathLine = true;
            CHECK(death->causes.back() == event); // the fight is its cause
        }
    }
    CHECK(deathLine);
}

TEST_CASE("US-111 Revenge can end in exile") {
    sim::SimConfig config = realConfig();
    config.story.revenge.exileClanOpinion = 100; // the clan is against whoever attacks
    World world(42, config);
    makeEnemies(world, 4, 5);
    runDays(world, 1);
    const int event = world.takeRevenge(4, 5);
    const sim::ChronicleEntry* entry = world.chronicle().find(event);
    REQUIRE(entry != nullptr);
    MESSAGE(sim::formatEntry(*entry));
    CHECK(entry->kind == EventKind::Exile);
    CHECK(entry->text == "The clan drove " + world.people()[4].name + " out for attacking " + world.people()[5].name +
                             " over stolen meat.");
    CHECK_FALSE(world.people()[4].alive);
    CHECK(world.people()[4].exiled);
    CHECK(world.people()[4].causeOfDeath == sim::CauseOfDeath::None); // not dead: driven out
    CHECK(sim::makeReport(world).exiled == 1);
}

TEST_CASE("US-111 A feud that keeps worsening ends in revenge on its own") {
    sim::SimConfig config = realConfig();
    config.story.revenge.percentPerDay = 100;
    config.story.revenge.minFeudDays = 0;
    World world(42, config);
    makeEnemies(world, 4, 5);
    runDays(world, 3);
    const bool revenge = !entriesOf(world, EventKind::Revenge).empty() || !entriesOf(world, EventKind::Exile).empty();
    CHECK(revenge);
}
