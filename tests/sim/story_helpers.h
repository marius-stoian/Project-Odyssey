// Helpers shared by the story engine tests (M2b): small worlds and ways to read the chronicle.
#pragma once

#include "sim/chronicle.h"
#include "sim/world.h"

#include <vector>

namespace story_test {

namespace sim = odysseus::sim;

inline sim::SimConfig realConfig() {
    return sim::loadSimConfig(ODYSSEUS_DATA_DIR);
}

inline void runDays(sim::World& world, int days) {
    world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()) * static_cast<std::uint64_t>(days));
}

// Every entry of one kind, in order.
inline std::vector<const sim::ChronicleEntry*> entriesOf(const sim::World& world, sim::EventKind kind) {
    std::vector<const sim::ChronicleEntry*> found;
    for (const auto& entry : world.chronicle().entries()) {
        if (entry.kind == kind) {
            found.push_back(&entry);
        }
    }
    return found;
}

// A world where the land gives nothing, so the store empties and people starve.
inline sim::SimConfig barrenConfig() {
    sim::SimConfig config = realConfig();
    config.clan.startingFood = 0;
    config.actions.forageDaily = {0, 0, 0, 0};
    config.actions.gameDaily = 0;
    config.actions.mammothPerMille = 0;
    return config;
}

// Two people who each saw the other steal three times: bitter enough to feud.
inline void makeEnemies(sim::World& world, int a, int b) {
    for (int i = 0; i < 3; ++i) {
        world.recordTheft(a, b);
        world.recordTheft(b, a);
    }
}

inline const sim::Memory* memoryOf(const sim::Person& person, sim::MemoryKind kind) {
    for (const auto& memory : person.memories) {
        if (memory.kind == kind) {
            return &memory;
        }
    }
    return nullptr;
}

} // namespace story_test
