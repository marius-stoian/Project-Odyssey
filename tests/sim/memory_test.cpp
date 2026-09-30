#include "sim/memory.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

using odysseus::sim::Memory;
using odysseus::sim::MemoryKind;
using odysseus::sim::World;
namespace sim = odysseus::sim;

namespace {

sim::SimConfig realConfig() {
    return sim::loadSimConfig(ODYSSEUS_DATA_DIR);
}

const Memory* find(const std::vector<Memory>& memories, int subject, MemoryKind kind, std::int64_t day) {
    const auto it = std::find_if(memories.begin(), memories.end(), [&](const Memory& m) {
        return m.subject == subject && m.kind == kind && m.day == day;
    });
    return it == memories.end() ? nullptr : &*it;
}

} // namespace

TEST_CASE("US-013 Memory") {
    const sim::SimConfig config = realConfig();
    World world(42, config);
    const auto& giver = world.people()[0];
    const auto& receiver = world.people()[1];
    CHECK(world.opinion(1, 0) == 0);

    world.giveGift(0, 1);
    const Memory* memory = find(receiver.memories, 0, MemoryKind::Gift, world.date().day);
    REQUIRE(memory != nullptr);
    CHECK(memory->subject == 0);                          // who
    CHECK(memory->object == 1);                           // to whom
    CHECK(memory->kind == MemoryKind::Gift);              // what
    CHECK(memory->day == world.date().day);               // when
    CHECK(memory->feeling == config.social.giftFeeling);  // a feeling
    CHECK_FALSE(memory->secondHand);
    CHECK(world.opinion(1, 0) == config.social.giftOpinion); // and it changes what they think
    MESSAGE(receiver.name, " remembers: ", giver.name, " gave a ", std::string(sim::memoryKindName(memory->kind)), " on day ",
            memory->day, ", feeling ", memory->feeling);
}

TEST_CASE("US-013 Gossip") {
    sim::SimConfig config = realConfig();
    config.social.gossipPercent = 100; // "may": here always, so the test is exact
    config.social.talkativeGossipPercent = 100;
    World world(42, config);
    world.giveGift(0, 1);
    world.recordTheft(3, 1);

    // Person 1 talks to person 2, who knows nothing yet: the strongest story passes on,
    // as a weaker, second-hand copy.
    CHECK(world.talk(1, 2));
    const auto& listener = world.people()[2];
    const Memory* heard = find(listener.memories, 3, MemoryKind::Theft, world.date().day);
    REQUIRE(heard != nullptr);
    CHECK(heard->secondHand);
    CHECK(heard->feeling == config.social.theftFeeling / 2);
    CHECK(world.opinion(2, 3) == config.social.theftFeeling / 2 / 4); // the thief's name suffers
    // Talking again passes the next story; then nothing is left that 2 lacks.
    CHECK(world.talk(1, 2));
    CHECK(find(listener.memories, 0, MemoryKind::Gift, world.date().day) != nullptr);
    CHECK_FALSE(world.talk(1, 2));
    CHECK(listener.memories.size() == 2); // never the same event twice

    // With gossip switched off, a talk passes nothing on.
    sim::SimConfig quiet = realConfig();
    quiet.social.gossipPercent = 0;
    quiet.social.talkativeGossipPercent = 0;
    World silent(42, quiet);
    silent.giveGift(0, 1);
    CHECK_FALSE(silent.talk(1, 2));
    CHECK(silent.people()[2].memories.empty());
}

TEST_CASE("US-013 Forgetting") {
    const int lifetime = realConfig().social.minorMemoryDays;
    std::vector<Memory> memories{{0, 1, MemoryKind::Gift, 0, 40, false, false}, {3, -1, MemoryKind::Theft, 0, -60, true, false}};
    CHECK(sim::forgetOldMemories(memories, lifetime, lifetime) == 0); // exactly the lifetime: still there
    CHECK(sim::forgetOldMemories(memories, lifetime + 1, lifetime) == 1); // older: the minor one goes
    REQUIRE(memories.size() == 1);
    CHECK(memories.front().major); // the important one is kept

    // In the world, at the end of a day.
    World world(42, realConfig());
    world.giveGift(0, 1);
    world.recordTheft(3, 1);
    world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()) * static_cast<std::uint64_t>(lifetime + 1));
    const auto& person = world.people()[1];
    REQUIRE(person.alive);
    CHECK(find(person.memories, 0, MemoryKind::Gift, 0) == nullptr); // minor, older than 60 days: forgotten
    CHECK(find(person.memories, 3, MemoryKind::Theft, 0) != nullptr); // major: kept for life
}

TEST_CASE("US-013 A full memory makes room by forgetting the oldest minor memory") {
    std::vector<Memory> memories;
    sim::remember(memories, {1, 0, MemoryKind::Theft, 1, -60, true, false}, 3);
    sim::remember(memories, {2, 0, MemoryKind::Gift, 2, 40, false, false}, 3);
    sim::remember(memories, {3, 0, MemoryKind::Gift, 3, 40, false, false}, 3);
    sim::remember(memories, {4, 0, MemoryKind::Gift, 4, 40, false, false}, 3);
    REQUIRE(memories.size() == 3);
    CHECK(memories[0].subject == 1); // the major memory stays
    CHECK(memories[1].subject == 3); // the oldest minor one (from 2) made room
    CHECK(memories[2].subject == 4);
}

TEST_CASE("US-013 Gifts, thefts and gossip happen in a living clan") {
    World world(42, realConfig());
    world.runTicks(world.calendar().ticksPerYear());
    int gifts = 0;
    int thefts = 0;
    int secondHand = 0;
    for (const auto& person : world.people()) {
        for (const Memory& memory : person.memories) {
            gifts += memory.kind == MemoryKind::Gift ? 1 : 0;
            thefts += memory.kind == MemoryKind::Theft ? 1 : 0;
            secondHand += memory.secondHand ? 1 : 0;
        }
    }
    MESSAGE("after a year: ", gifts, " gift memories, ", thefts, " theft memories, ", secondHand, " heard second-hand");
    CHECK(gifts > 0);
    CHECK(thefts > 0);
    CHECK(secondHand > 0);
}
