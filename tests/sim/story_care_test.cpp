// US-112 Share food and nurse the sick (M2b story engine).
#include "sim/chronicle.h"
#include "sim/data.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>

using odysseus::sim::EventKind;
using odysseus::sim::World;
namespace sim = odysseus::sim;

using namespace story_test;

TEST_CASE("US-112 Sickness") {
    // Hunger weakens everyone (the land is barren and the threshold is the whole scale): all fall
    // sick on the second morning, for exactly four days.
    sim::SimConfig config = barrenConfig();
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerBelow = 100;
    config.story.sickness.hungerPerMille = 1000;
    config.story.sickness.coldPerMille = 0;
    config.story.sickness.daysMin = 4;
    config.story.sickness.daysMax = 4;
    config.story.sickness.deathPerMille = 0;
    World world(42, config);
    runDays(world, 1);
    const auto sicknesses = entriesOf(world, EventKind::Sickness);
    REQUIRE_FALSE(sicknesses.empty());
    const sim::Person& sick = world.people()[static_cast<std::size_t>(sicknesses.front()->who)];
    MESSAGE(sim::formatEntry(*sicknesses.front()));
    CHECK(sicknesses.front()->text == sick.name + " fell sick, weakened by hunger.");
    CHECK(sick.health == sim::Health::Sick);
    CHECK(sick.healthDays == 4);
    // The sick do not work: nobody who is unwell gathers, hunts or steals.
    runDays(world, 1);
    for (const auto& person : world.people()) {
        if (person.alive && person.health != sim::Health::Well) {
            CHECK(person.action != sim::Action::Gather);
            CHECK(person.action != sim::Action::Hunt);
            CHECK(person.action != sim::Action::Steal);
        }
    }
}

TEST_CASE("US-112 The cold weakens too") {
    sim::SimConfig config = realConfig();
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerPerMille = 0;
    config.story.sickness.coldBelow = 100;
    config.story.sickness.coldPerMille = 1000;
    World world(42, config);
    runDays(world, 1);
    const auto sicknesses = entriesOf(world, EventKind::Sickness);
    REQUIRE_FALSE(sicknesses.empty());
    CHECK(sicknesses.front()->text.find("weakened by the cold") != std::string::npos);
}

TEST_CASE("US-112 A hunting wound") {
    sim::SimConfig config = barrenConfig(); // a hungry clan sends hunters out
    config.story.sickness.huntWoundPerMille = 1000;
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerPerMille = 0;
    config.story.sickness.coldPerMille = 0;
    World world(42, config);
    runDays(world, 6);
    const auto injuries = entriesOf(world, EventKind::Injury);
    REQUIRE_FALSE(injuries.empty());
    const sim::Person& hunter = world.people()[static_cast<std::size_t>(injuries.front()->who)];
    MESSAGE(sim::formatEntry(*injuries.front()));
    CHECK(injuries.front()->text == hunter.name + " was hurt hunting.");
}

TEST_CASE("US-112 Sickness can kill") {
    sim::SimConfig config = barrenConfig();
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerBelow = 100;
    config.story.sickness.hungerPerMille = 1000;
    config.story.sickness.deathPerMille = 1000;
    World world(42, config);
    runDays(world, 3);
    bool died = false;
    for (const auto* death : entriesOf(world, EventKind::Death)) {
        const sim::Person& person = world.people()[static_cast<std::size_t>(death->who)];
        if (person.causeOfDeath == sim::CauseOfDeath::Illness) {
            died = true;
            MESSAGE(sim::formatEntry(*death));
            CHECK(death->text == person.name + " died of the sickness, weakened by hunger.");
            REQUIRE_FALSE(death->causes.empty());
            CHECK(world.chronicle().find(death->causes.back())->kind == EventKind::Sickness);
        }
    }
    CHECK(died);
}

TEST_CASE("US-112 Nursing") {
    sim::SimConfig config = realConfig();
    config.story.nursing.percent = 100;
    config.story.sickness.deathPerMille = 0;
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerPerMille = 0;
    config.story.sickness.coldPerMille = 0;
    World nursed(42, config);
    nursed.pair(5, 6);
    nursed.sicken(5, 6, -1);
    runDays(nursed, 1);
    const auto nursings = entriesOf(nursed, EventKind::Nursing);
    REQUIRE(nursings.size() == 1);
    MESSAGE(sim::formatEntry(*nursings.front()));
    CHECK(nursings.front()->who == 6); // the partner comes
    CHECK(nursings.front()->other == 5);
    CHECK(nursed.people()[5].carer == 6);
    CHECK(nursed.people()[6].nursing == 5);
    // The patient remembers with gratitude, for life, and thinks better of the carer.
    const sim::Memory* memory = memoryOf(nursed.people()[5], sim::MemoryKind::Nursing);
    REQUIRE(memory != nullptr);
    CHECK(memory->major);
    CHECK(memory->subject == 6);
    CHECK(memory->feeling > 0);
    CHECK(nursed.opinion(5, 6) >= config.story.nursing.opinionGain);

    // Recovery is quicker with a carer: 6 days alone, 3 days nursed (two days' mending each day).
    sim::SimConfig alone = config;
    alone.story.nursing.percent = 0;
    World unnursed(42, alone);
    unnursed.pair(5, 6);
    unnursed.sicken(5, 6, -1);
    runDays(unnursed, 4);
    runDays(nursed, 3); // four days in all
    CHECK(nursed.people()[5].health == sim::Health::Well);
    CHECK(unnursed.people()[5].health == sim::Health::Sick);
    const auto recoveries = entriesOf(nursed, EventKind::Recovery);
    REQUIRE_FALSE(recoveries.empty());
    CHECK(recoveries.front()->text.find(", nursed by " + nursed.people()[6].name) != std::string::npos);
}

TEST_CASE("US-112 Care saves lives") {
    // The same patient in two worlds: with a carer a sickness that would certainly kill is survived;
    // without one it kills.
    sim::SimConfig config = realConfig();
    config.story.nursing.percent = 100;
    config.story.nursing.deathPercentWhenNursed = 0;
    config.story.sickness.deathPerMille = 1000;
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerPerMille = 0;
    config.story.sickness.coldPerMille = 0;
    World cared(42, config);
    cared.pair(5, 6);
    cared.sicken(5, 6, -1);
    runDays(cared, 1); // the carer arrives on the first morning; the patient is safe from then on
    CHECK(cared.people()[5].carer == 6);
    runDays(cared, 1);
    CHECK(cared.people()[5].alive);

    sim::SimConfig alone = config;
    alone.story.nursing.percent = 0;
    World uncared(42, alone);
    uncared.pair(5, 6);
    uncared.sicken(5, 6, -1);
    runDays(uncared, 2);
    CHECK_FALSE(uncared.people()[5].alive);
    CHECK(uncared.people()[5].causeOfDeath == sim::CauseOfDeath::Illness);
}

TEST_CASE("US-112 Sharing food") {
    World world(42, realConfig());
    const int giverBefore = world.people()[3].needs[sim::Need::Hunger];
    const int receiverBefore = world.people()[4].needs[sim::Need::Hunger];
    const int event = world.share(3, 4);
    const sim::ChronicleEntry* entry = world.chronicle().find(event);
    REQUIRE(entry != nullptr);
    MESSAGE(sim::formatEntry(*entry));
    CHECK(entry->kind == EventKind::Sharing);
    CHECK(entry->text == world.people()[3].name + " shared food with hungry " + world.people()[4].name + ".");
    const int amount = world.config().story.sharing.amount;
    CHECK(world.people()[3].needs[sim::Need::Hunger] == giverBefore - amount);
    CHECK(world.people()[4].needs[sim::Need::Hunger] == std::min(100, receiverBefore + amount));
    // The one helped remembers it and thinks better of the giver.
    const sim::Memory* memory = memoryOf(world.people()[4], sim::MemoryKind::Sharing);
    REQUIRE(memory != nullptr);
    CHECK(memory->subject == 3);
    CHECK(world.opinion(4, 3) == world.config().story.sharing.opinionGain);
    // Feeding the same person again soon is not news.
    CHECK(world.share(3, 4) == -1);
}

TEST_CASE("US-112 Hungry days bring sharing") {
    // A scarce land: the store runs low, and the better fed feed the hungriest.
    sim::SimConfig config = barrenConfig();
    config.clan.startingFood = 60;
    config.actions.forageDaily = {6, 6, 6, 0};
    World world(7, config);
    runDays(world, 25);
    const auto sharings = entriesOf(world, EventKind::Sharing);
    REQUIRE_FALSE(sharings.empty());
    for (const auto* sharing : sharings) {
        CHECK(sharing->text.find("shared food with hungry") != std::string::npos);
    }
}

TEST_CASE("US-112 Adoption") {
    sim::SimConfig config = realConfig();
    config.life.conceptionPerMille = 1000; // a child at once
    config.life.pregnancyDays = 1;
    config.life.childbirthDeathPercent = 0;
    config.story.adoption.minScore = 0;    // anyone kind enough: the highest score wins
    World world(42, config);
    const auto [mother, father] = aCouple(world);
    REQUIRE(mother >= 0);
    REQUIRE(father >= 0);
    world.pair(mother, father);
    const std::size_t before = world.people().size();
    for (int day = 0; day < 4 && world.people().size() == before; ++day) {
        runDays(world, 1);
    }
    REQUIRE(world.people().size() == before + 1);
    const int child = world.people().back().id;
    CHECK_FALSE(world.isOrphan(world.people()[static_cast<std::size_t>(child)])); // parents alive
    const int fatherDeath = world.kill(father, sim::CauseOfDeath::OldAge);
    (void)fatherDeath;
    world.kill(mother, sim::CauseOfDeath::OldAge);
    const sim::Person& orphan = world.people()[static_cast<std::size_t>(child)];
    CHECK(world.isOrphan(orphan));
    runDays(world, 1);
    REQUIRE(orphan.guardian >= 0);
    const auto adoptions = entriesOf(world, EventKind::Adoption);
    REQUIRE(adoptions.size() == 1);
    MESSAGE(sim::formatEntry(*adoptions.front()));
    const std::string& guardian = world.people()[static_cast<std::size_t>(orphan.guardian)].name;
    CHECK(adoptions.front()->text ==
          orphan.name + ", orphaned by the death of " + world.people()[static_cast<std::size_t>(mother)].name + ", was taken in by " + guardian + ".");
    REQUIRE_FALSE(adoptions.front()->causes.empty());
    CHECK(world.chronicle().find(adoptions.front()->causes.front())->kind == EventKind::Death);
    // The one taken in remembers it for life; both think better of each other.
    const sim::Memory* memory = memoryOf(orphan, sim::MemoryKind::Sharing);
    REQUIRE(memory != nullptr);
    CHECK(memory->major);
    CHECK(world.opinion(child, orphan.guardian) >= config.story.adoption.opinionGain);
    CHECK(world.opinion(orphan.guardian, child) >= config.story.adoption.opinionGain);
}

TEST_CASE("US-112 Story data is validated") {
    const auto folder = std::filesystem::temp_directory_path() / "odysseus-us112";
    std::filesystem::create_directories(folder);
    const auto file = folder / "story.json";
    // Copy the real file, then break one number in the sharing section.
    std::ifstream in(std::filesystem::path(ODYSSEUS_DATA_DIR) / "sim" / "story.json");
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto at = text.find("\"amount\": 15");
    REQUIRE(at != std::string::npos);
    text.replace(at, 12, "\"amount\": 0");
    std::ofstream(file) << text;
    try {
        (void)sim::loadStoryConfig(file);
        FAIL("an amount of 0 was accepted");
    } catch (const sim::DataError& error) {
        MESSAGE(std::string(error.what()));
        CHECK(std::string(error.what()).find("sharing.amount") != std::string::npos);
    }
}
