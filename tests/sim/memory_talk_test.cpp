// US-164: conversations are remembered: memories, gossip at half strength, flags, and what is saved.
#include "sim/flag_store.h"
#include "sim/save.h"
#include "sim/smalltalk.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

const sim::Memory* memoryAbout(const sim::Person& person, int subject, sim::MemoryKind kind) {
    for (const sim::Memory& memory : person.memories) {
        if (memory.subject == subject && memory.kind == kind) return &memory;
    }
    return nullptr;
}

} // namespace

TEST_CASE("US-164 Memory: an insult leaves a bad memory in the person who was insulted") {
    sim::World world(42, story_test::realConfig());
    const int hero = 0;
    const int bo = 5;
    world.personMutable(bo)->name = "Bo";
    REQUIRE(world.rememberConversation(bo, hero, "Voll told Bo to be quiet", -40));
    const sim::Person& person = world.people()[static_cast<std::size_t>(bo)];
    const sim::Memory* memory = memoryAbout(person, hero, sim::MemoryKind::Quarrel);
    REQUIRE(memory != nullptr);
    CHECK(memory->feeling == -40);
    CHECK(memory->object == bo);
    CHECK_FALSE(memory->secondHand);
    CHECK_FALSE(memory->major); // under 60 it fades like any minor memory
    REQUIRE(person.notes.size() == 1);
    CHECK(person.notes[0].text == "Voll told Bo to be quiet");
    CHECK(person.notes[0].clause);
    CHECK(person.notes[0].feeling == -40);

    // Strong feelings are kept for life, good ones are Gifts.
    world.rememberConversation(bo, hero, "", -80);
    CHECK(person.memories.back().major);
    world.rememberConversation(bo, hero, "Voll shared berries", 30);
    CHECK(person.memories.back().kind == sim::MemoryKind::Gift);
    CHECK(person.memories.back().feeling == 30);
    // Out-of-range feelings are kept in range; people who do not exist or are the same person are refused.
    world.rememberConversation(bo, hero, "", 900);
    CHECK(person.memories.back().feeling == 100);
    CHECK_FALSE(world.rememberConversation(bo, bo, "x", 10));
    CHECK_FALSE(world.rememberConversation(9999, hero, "x", 10));
    CHECK_FALSE(world.rememberConversation(bo, -1, "x", 10));
}

TEST_CASE("US-164 Gossip: a friend of the insulted person hears it at half strength") {
    sim::World world(42, story_test::realConfig());
    const int hero = 0;
    const int bo = 5;
    const int friend1 = 6;
    world.rememberConversation(bo, hero, "Voll told Bo to be quiet", -40);
    // Bo talks with his friend; talk passes on a memory now and then (the chance is the world's own). Try until it does.
    bool passed = false;
    for (int tries = 0; tries < 60 && !passed; ++tries) passed = world.talk(bo, friend1);
    REQUIRE(passed);
    const sim::Memory* heard = memoryAbout(world.people()[static_cast<std::size_t>(friend1)], hero, sim::MemoryKind::Quarrel);
    REQUIRE(heard != nullptr);
    CHECK(heard->secondHand);
    CHECK(heard->feeling == -20); // half of -40
    CHECK(heard->object == bo);
}

TEST_CASE("US-164 Gossip: two days pass and the friends of the insulted person have heard it at half strength") {
    // Person 3 is a grown, sociable member of the seed-42 clan (a child talks to few people, so gossip would take longer).
    sim::World world(42, story_test::realConfig());
    world.rememberConversation(3, 0, "Voll told Bo to be quiet", -40);
    story_test::runDays(world, 2);
    int heardBy = 0;
    for (const sim::Person& person : world.people()) {
        if (person.id == 3 || !person.alive) continue;
        const sim::Memory* memory = memoryAbout(person, 0, sim::MemoryKind::Quarrel);
        if (memory != nullptr && memory->secondHand && memory->object == 3) {
            ++heardBy;
            CHECK(memory->feeling == -20); // half strength
        }
    }
    CHECK(heardBy >= 1);
}
TEST_CASE("US-164 What was said comes up in small talk as the day it happened") {
    sim::World world(42, story_test::realConfig());
    world.personMutable(0)->name = "Voll";
    world.rememberConversation(5, 0, "Voll shared berries", 20);
    world.personMutable(5)->memories.clear(); // only the free-text memory is left
    rules::LoadReport report;
    auto data = rules::SmalltalkData::load(fs::path(ODYSSEUS_DATA_DIR) / "dialogue" / "smalltalk.json", "dialogue/smalltalk.json", report);
    REQUIRE(data.has_value());
    rules::SmallTalk talk(std::move(*data));
    odysseus::core::Pcg32 random(1, 8);
    const auto said = talk.say(world, 5, 0, random, "memory");
    REQUIRE(said.has_value());
    CHECK_MESSAGE(said->text.find("the day Voll shared berries") != std::string::npos, said->text);
}

TEST_CASE("US-164 Flags: a name with a whole number, saved and read back") {
    rules::FlagStore flags;
    CHECK(flags.get("met-elder") == 0);
    flags.set("met-elder", 1);
    flags.set("trust", 3);
    flags.set("gone", 5);
    flags.set("gone", 0); // set to 0 is never set
    CHECK(flags.get("met-elder") == 1);
    CHECK(flags.get("trust") == 3);
    CHECK(flags.all().size() == 2);
    flags.set("", 7); // no name, no flag
    CHECK(flags.all().size() == 2);

    const std::string text = flags.save();
    rules::FlagStore loaded;
    CHECK(loaded.load(text).empty());
    CHECK(loaded.get("met-elder") == 1);
    CHECK(loaded.get("trust") == 3);
    CHECK(loaded.hash() == flags.hash());
    CHECK(loaded.save() == text);
    flags.set("trust", 4);
    CHECK(loaded.hash() != flags.hash());

    // Damaged input never crashes, and what can be read is kept.
    rules::FlagStore damaged;
    CHECK_FALSE(damaged.load("{ not json").empty());
    CHECK_FALSE(damaged.load("[1, 2]").empty());
    const auto notes = damaged.load("{\"fine\": 2, \"odd\": \"text\", \"half\": 1.5}");
    CHECK(notes.size() == 2);
    CHECK(damaged.get("fine") == 2);
}

TEST_CASE("US-164 Saved: conversation memories survive a save and a load") {
    const fs::path file = fs::temp_directory_path() / "odysseus-us164" / "memories" / "clan.json";
    fs::remove_all(file.parent_path());
    fs::create_directories(file.parent_path());
    sim::World world(42, story_test::realConfig());
    world.rememberConversation(5, 0, "Voll told Bo to be quiet", -40);
    const std::uint64_t before = world.hash();
    sim::saveWorld(world, file);
    sim::LoadedWorld loaded = sim::loadWorld(file, story_test::realConfig());
    const sim::Person& bo = loaded.world.people()[5];
    REQUIRE(bo.notes.size() == 1);
    CHECK(bo.notes[0].text == "Voll told Bo to be quiet");
    CHECK(bo.notes[0].clause);
    const sim::Memory* memory = memoryAbout(bo, 0, sim::MemoryKind::Quarrel);
    REQUIRE(memory != nullptr);
    CHECK(memory->feeling == -40);
    CHECK(loaded.world.hash() == before);
}
