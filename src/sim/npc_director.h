#pragma once

#include "boundary.h"

#include "sim/npc_population.h"
#include "sim/npc_schedule.h"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace odysseus::sim {

// A named spot of a level (US-290): where a schedule sends a person, and where the environment interactions happen (a tag such as forage, shelter, water, shrine).
struct Place {
    std::string name;
    int x = 0; // world pixels
    int y = 0;
    std::vector<std::string> tags;
    friend bool operator==(const Place&, const Place&) = default;
};

// The life of the persons of an NpcPopulation (M9c, D-54 Q9-Q13). It decides where each person is and what they do: they follow their schedule hour by hour while they are
// near the hero, and in one coarse step a day while they are far (ADR-022); hunger, danger and fighting interrupt the schedule and then it resumes. It holds only what
// the population does not: the schedule of each person, their home, their mode. Deterministic: whole numbers, ordered containers, no wall clock; saved as npc-life.json.
//
// Call tick() once after every NpcPopulation::tick(). The cost of a tick does not grow with the crowd: the far persons are visited in slices, one person in every
// ticksPerDay, at a tick that depends only on their index.
class NpcDirector {
public:
    enum class Mode : std::uint8_t { Scheduled, Eating, Fleeing, Fighting, Dead };

    explicit NpcDirector(rules::ScheduleConfig config = {});

    const rules::ScheduleConfig& config() const { return config_; }

    // The named places of the level (a person's own "home" is built in).
    void setPlaces(std::vector<Place> places);
    const std::vector<Place>& places() const { return places_; }
    const Place* place(const std::string& name) const;

    // A person's schedule, home and danger, by their index in the population. The storage grows to fit the index.
    void setSchedule(int index, const rules::Schedule& schedule);
    const rules::Schedule* schedule(int index) const; // null: no schedule
    void setHome(int index, int x, int y);
    int homeX(int index) const;
    int homeY(int index) const;
    // Danger near a person (the game sets it while a hostile is within reach): they run home, and resume when it is gone.
    void setDanger(int index, bool danger);

    Mode mode(int index) const;
    // The schedule block in force for a person at this moment of the population's clock, or null.
    const rules::ScheduleBlock* currentBlock(const NpcPopulation& population, int index) const;
    // The activity a person is doing now: the word of the block, "eat" or "flee" while interrupted, "fight" in a fight, "dead".
    std::string activity(const NpcPopulation& population, int index) const;

    // One game tick, after NpcPopulation::tick(): on the hour the persons near the focus take up their schedule (or are interrupted); every tick a slice of the far ones does.
    void tick(NpcPopulation& population);

    // The saved state (versioned JSON, run-length coded: a crowd with one schedule saves in a few hundred bytes) and its reader. A damaged text is a DataError.
    std::string toText() const;
    static NpcDirector fromText(std::string_view text, rules::ScheduleConfig config);
    std::uint64_t hash() const;
    static constexpr int kSaveVersion = 1;

private:
    void ensure(std::size_t size);
    int internSchedule(const rules::Schedule& schedule);
    // The point a person goes to for a place name: "home", a place of the level (scattered a little so a crowd does not stand on one spot), or home when the name is unknown.
    std::pair<int, int> pointFor(const NpcPopulation& population, int index, const std::string& placeName) const;
    void goTo(NpcPopulation& population, int index, const std::string& placeName);
    // The hour of a person near the hero (interruptions and the schedule), and of a far person (the schedule only).
    void stepNear(NpcPopulation& population, int index, int hour);
    void stepFar(NpcPopulation& population, int index, int hour);
    void followSchedule(NpcPopulation& population, int index, int hour, bool restoreNeeds);
    void farSlice(NpcPopulation& population);

    rules::ScheduleConfig config_;
    std::vector<Place> places_;
    std::vector<rules::Schedule> schedules_; // [0] is "no schedule"
    std::vector<std::uint16_t> scheduleOf_;
    std::vector<std::uint8_t> modes_;
    std::vector<std::uint8_t> dangers_;
    std::vector<std::int32_t> homeX_;
    std::vector<std::int32_t> homeY_;
    std::size_t adopted_ = 0; // the persons below this index have been given a home
};

} // namespace odysseus::sim
