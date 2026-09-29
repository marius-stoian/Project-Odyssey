#pragma once

#include "boundary.h"

#include "calendar.h"

#include "core/random.h"

#include <cstdint>
#include <filesystem>

namespace odysseus::sim {

// Every tunable number of the simulation, loaded from assets/data/sim/ (Charter rule 7).
struct SimConfig {
    CalendarConfig calendar;
};

SimConfig loadSimConfig(const std::filesystem::path& dataDirectory);

// Separate random streams per system (Charter rule 6): adding a random call in one system
// never changes the numbers another system sees.
enum class Stream : std::uint64_t { Weather = 1 };

// The whole simulated world. No graphics, no operating system: it runs the same in the
// game, in the headless runner and in tests, and the same seed gives the same history.
class World {
public:
    World(std::uint64_t seed, SimConfig config);

    // One fixed step: 1/20 of a game second (ADR-006).
    void tick();
    void runTicks(std::uint64_t count);

    std::uint64_t ticks() const { return ticks_; }
    Date date() const { return calendar_.dateAt(ticks_); }
    const Calendar& calendar() const { return calendar_; }
    std::uint64_t seed() const { return seed_; }

    // Today's temperature in whole degrees Celsius, rolled each morning (weather stream).
    int temperature() const { return temperature_; }

    // One number summarising the entire state; equal worlds have equal hashes (ADR-011).
    std::uint64_t hash() const;

private:
    void startDay();

    std::uint64_t seed_;
    SimConfig config_;
    Calendar calendar_;
    std::uint64_t ticks_ = 0;
    core::Pcg32 weather_;
    int temperature_ = 0;
};

} // namespace odysseus::sim
