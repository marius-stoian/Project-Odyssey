// US-114 Teach the young and hunt together (M2b story engine).
#include "sim/chronicle.h"
#include "sim/data.h"
#include "sim/save.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

using odysseus::sim::EventKind;
using odysseus::sim::World;
namespace sim = odysseus::sim;

using namespace story_test;

namespace {

// Quiet worlds: nothing but the story under test stirs.
sim::SimConfig quietConfig() {
    sim::SimConfig config = realConfig();
    config.story.quarrel.calmPercent = 0;
    config.story.quarrel.irritablePercent = 0;
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerPerMille = 0;
    config.story.sickness.coldPerMille = 0;
    config.story.sickness.huntWoundPerMille = 0;
    config.social.witnessPercent = 0;
    config.social.talkOpinion = 0;
    return config;
}

// Founders aged 13 to 45: some are youths of teaching age.
sim::SimConfig teachingConfig(int takePercent) {
    sim::SimConfig config = quietConfig();
    config.clan.minimumStartingAgeYears = 13;
    config.story.teaching.takePercent = takePercent;
    return config;
}

// A hungry clan with mammoths everywhere: hunters go out at once and meet one.
sim::SimConfig huntConfig() {
    sim::SimConfig config = barrenConfig();
    config.actions.mammothPerMille = 1000;
    config.story.sickness.basePerMille = 0;
    config.story.sickness.hungerPerMille = 0;
    config.story.sickness.coldPerMille = 0;
    config.story.sickness.huntWoundPerMille = 0;
    config.story.hunt.joinPercent = 100;
    config.story.hunt.dangerPercent = 0;
    return config;
}

const sim::ChronicleEntry* first(const World& world, EventKind kind) {
    const auto entries = entriesOf(world, kind);
    return entries.empty() ? nullptr : entries.front();
}

// Days until a party has hunted (at most `limit`).
bool runToHunt(World& world, int limit) {
    for (int day = 0; day < limit && first(world, EventKind::HuntParty) == nullptr; ++day) {
        runDays(world, 1);
    }
    return first(world, EventKind::HuntParty) != nullptr;
}

// The first seed whose clan has a master who takes a youth within three days.
std::uint64_t seedWithApprentice() {
    for (std::uint64_t seed = 1; seed < 60; ++seed) {
        World world(seed, teachingConfig(100));
        runDays(world, 3);
        if (first(world, EventKind::Apprentice) != nullptr) {
            return seed;
        }
    }
    FAIL("no clan had a master and a youth in 60 seeds");
    return 0;
}

} // namespace

TEST_CASE("US-114 Apprenticeship") {
    const std::uint64_t seed = seedWithApprentice();
    World taught(seed, teachingConfig(100));
    World untaught(seed, teachingConfig(0)); // the same clan, but nobody takes an apprentice
    runDays(taught, 3);
    runDays(untaught, 3);
    const sim::ChronicleEntry* start = first(taught, EventKind::Apprentice);
    REQUIRE(start != nullptr);
    MESSAGE(sim::formatEntry(*start));
    const int master = start->who;
    const int youth = start->other;
    const bool hunting = start->text.find("hunting") != std::string::npos;
    CHECK(start->text == taught.people()[static_cast<std::size_t>(master)].name + " took " +
                             taught.people()[static_cast<std::size_t>(youth)].name + " as apprentice in " +
                             (hunting ? "hunting." : "gathering."));
    CHECK(taught.people()[static_cast<std::size_t>(youth)].master == master);
    CHECK(taught.people()[static_cast<std::size_t>(master)].apprentice == youth);
    runDays(taught, 8);
    runDays(untaught, 8);
    // The youth's skill grew faster than practice alone grows it, and they grew close.
    auto skill = [hunting](const World& world, int id) {
        const sim::Person& person = world.people()[static_cast<std::size_t>(id)];
        return hunting ? person.huntSkill : person.gatherSkill;
    };
    MESSAGE("skill with lessons ", skill(taught, youth), ", without ", skill(untaught, youth));
    CHECK(skill(taught, youth) >= skill(untaught, youth) + 5);
    CHECK(taught.opinion(master, youth) > untaught.opinion(master, youth));
    CHECK(taught.opinion(youth, master) > untaught.opinion(youth, master));
    // A graduation follows: the youth has learnt what the master knows, or has grown up.
    runDays(taught, 120);
    const sim::ChronicleEntry* done = first(taught, EventKind::Graduation);
    REQUIRE(done != nullptr);
    MESSAGE(sim::formatEntry(*done));
    CHECK(done->text.find("finished learning") != std::string::npos);
    CHECK(done->causes.size() == 1);
    CHECK(taught.people()[static_cast<std::size_t>(youth)].master == -1);
    CHECK(taught.people()[static_cast<std::size_t>(master)].apprentice == -1);
}

TEST_CASE("US-114 Saved apprenticeships") {
    const std::filesystem::path folder = std::filesystem::temp_directory_path() / "odysseus-us114";
    std::filesystem::remove_all(folder);
    std::filesystem::create_directories(folder);
    const std::filesystem::path file = folder / "clan.json";
    World world(seedWithApprentice(), teachingConfig(100));
    runDays(world, 3);
    sim::saveWorld(world, file);
    sim::LoadedWorld loaded = sim::loadWorld(file, teachingConfig(100));
    CHECK(loaded.world.hash() == world.hash());
    runDays(world, 20);
    runDays(loaded.world, 20);
    CHECK(loaded.world.hash() == world.hash());
}

TEST_CASE("US-114 Hunting party") {
    sim::SimConfig config = huntConfig();
    config.story.hunt.heroCourage = 0;     // somebody always stands firm...
    config.story.hunt.cowardCourage = 100; // ...and somebody always flees
    World world(42, config);
    REQUIRE(runToHunt(world, 10));
    const sim::ChronicleEntry* party = first(world, EventKind::HuntParty);
    MESSAGE(sim::formatEntry(*party));
    const std::string opening = "A hunting party of ";
    CHECK(party->text.find(opening) == 0);
    const char size = party->text[opening.size()]; // three to five hunters
    CHECK(size >= '3');
    CHECK(size <= '5');
    const sim::ChronicleEntry* hero = first(world, EventKind::Hero);
    const sim::ChronicleEntry* coward = first(world, EventKind::Coward);
    REQUIRE(hero != nullptr);
    REQUIRE(coward != nullptr);
    MESSAGE(sim::formatEntry(*hero));
    MESSAGE(sim::formatEntry(*coward));
    CHECK(hero->who != coward->who);
    CHECK(std::find(hero->causes.begin(), hero->causes.end(), party->id) != hero->causes.end());
    // Each member remembers what the others did.
    int remembered = 0;
    for (const auto& person : world.people()) {
        if (person.id == hero->who || person.id == coward->who) {
            continue;
        }
        const sim::Memory* deed = memoryOf(person, sim::MemoryKind::Heroism);
        const sim::Memory* shame = memoryOf(person, sim::MemoryKind::Cowardice);
        if (deed != nullptr && shame != nullptr) {
            ++remembered;
            CHECK(deed->subject == hero->who);
            CHECK(deed->feeling > 0);
            CHECK(shame->subject == coward->who);
            CHECK(shame->feeling < 0);
            CHECK(world.opinion(person.id, hero->who) > world.opinion(person.id, coward->who));
        }
    }
    CHECK(remembered >= 1);
}

TEST_CASE("US-114 The party decides the hunt") {
    for (const bool strong : {true, false}) {
        sim::SimConfig config = huntConfig();
        config.story.hunt.successBase = strong ? 200 : -200; // the party's strength swamps or is swamped
        World world(42, config);
        REQUIRE(runToHunt(world, 10));
        const sim::ChronicleEntry* party = first(world, EventKind::HuntParty);
        MESSAGE(sim::formatEntry(*party));
        const sim::ChronicleEntry* feast = first(world, EventKind::Mammoth);
        if (strong) {
            CHECK(party->text.find("brought it down") != std::string::npos);
            REQUIRE(feast != nullptr);
            CHECK(std::find(feast->causes.begin(), feast->causes.end(), party->id) != feast->causes.end());
            CHECK(world.mammothsKilled() == 1);
        } else {
            CHECK(party->text.find("got away") != std::string::npos);
            CHECK(feast == nullptr);
            CHECK(world.mammothsKilled() == 0);
        }
    }
}

TEST_CASE("US-114 Rescue") {
    sim::SimConfig config = huntConfig();
    config.story.hunt.dangerPercent = 100;
    config.story.hunt.rescuePercent = 100;
    config.story.hunt.braveRescueBonus = 0;
    World world(42, config);
    REQUIRE(runToHunt(world, 10));
    const auto rescues = entriesOf(world, EventKind::Rescue);
    REQUIRE_FALSE(rescues.empty());
    const sim::ChronicleEntry* rescue = rescues.front();
    MESSAGE(sim::formatEntry(*rescue));
    const sim::Person& saved = world.people()[static_cast<std::size_t>(rescue->other)];
    const sim::Person& saviour = world.people()[static_cast<std::size_t>(rescue->who)];
    CHECK(rescue->text == saviour.name + " saved " + saved.name + " from the mammoth.");
    CHECK(rescue->who != rescue->other);
    // The rescued owes a debt of gratitude, kept for life.
    const sim::Memory* debt = memoryOf(saved, sim::MemoryKind::Rescue);
    REQUIRE(debt != nullptr);
    CHECK(debt->subject == saviour.id);
    CHECK(debt->major);
    CHECK(debt->feeling > 0);
    CHECK(debt->event == rescue->id);
    CHECK(world.opinion(saved.id, saviour.id) >= config.story.hunt.rescueOpinion);
}

TEST_CASE("US-114 Nobody to save them") {
    sim::SimConfig config = huntConfig();
    config.story.hunt.dangerPercent = 100;
    config.story.hunt.rescuePercent = 0;
    config.story.hunt.dangerDeathPercent = 100;
    World world(42, config);
    REQUIRE(runToHunt(world, 10));
    CHECK(entriesOf(world, EventKind::Rescue).empty());
    const sim::ChronicleEntry* party = first(world, EventKind::HuntParty);
    bool died = false;
    for (const auto* death : entriesOf(world, EventKind::Death)) {
        if (world.people()[static_cast<std::size_t>(death->who)].causeOfDeath == sim::CauseOfDeath::Hunting) {
            died = true;
            CHECK(std::find(death->causes.begin(), death->causes.end(), party->id) != death->causes.end());
        }
    }
    CHECK(died);
}
