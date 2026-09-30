#pragma once

#include "boundary.h"

#include "calendar.h"
#include "chronicle.h"
#include "clan.h"
#include "needs.h"
#include "person.h"

#include "core/random.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace odysseus::sim {

// Every tunable number of the simulation, loaded from assets/data/sim/ (Charter rule 7).
struct SimConfig {
    CalendarConfig calendar;
    NeedsConfig needs;
    ClanConfig clan;
    NameList names;
};

SimConfig loadSimConfig(const std::filesystem::path& dataDirectory);

// Separate random streams per system (Charter rule 6): adding a random call in one system
// never changes the numbers another system sees.
enum class Stream : std::uint64_t { Weather = 1, People = 2 };

// How important chronicle entries are (0..100): deaths and births are always worth telling.
inline constexpr int kImportanceDeath = 90;

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

    // Everyone who ever lived, in id order (the dead keep their place).
    const std::vector<Person>& people() const { return people_; }
    int population() const;
    int food() const { return food_; }
    const Chronicle& chronicle() const { return chronicle_; }
    const SimConfig& config() const { return config_; }

    // One number summarising the entire state; equal worlds have equal hashes (ADR-011).
    std::uint64_t hash() const;

private:
    void startDay();
    void passHour(int hour);
    void checkSurvival(Person& person, bool winter);
    void die(Person& person, CauseOfDeath cause);

    std::uint64_t seed_;
    SimConfig config_;
    Calendar calendar_;
    std::uint64_t ticks_ = 0;
    core::Pcg32 weather_;
    core::Pcg32 peopleRandom_;
    int temperature_ = 0;
    std::vector<Person> people_;
    int food_ = 0;
    Chronicle chronicle_;
};

} // namespace odysseus::sim
