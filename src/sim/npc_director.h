#pragma once

#include "boundary.h"

#include "sim/interaction.h"
#include "sim/npc_actions.h"
#include "sim/npc_chooser.h"
#include "sim/npc_context.h"
#include "sim/npc_events.h"
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

    // ---- What a person does on their own (US-291, D-54 Q11): when a person near the hero is free (idle, or on a block of work) they choose among the actions of their class, their
    // own custom actions and the events on offer, by the `npc` score of the interaction files, and carry the best out. The interactions are the registry's (the same files the hero
    // uses); the registry belongs to the game and must outlive the director.
    void setInteractions(const rules::InteractionRegistry* registry) { interactions_ = registry; }
    void setEvents(EventCatalog catalog) { events_ = std::move(catalog); }
    const EventCatalog& events() const { return events_; }
    void setSeed(std::uint64_t seed) { seed_ = seed; } // the world seed: every choice of a tie is a roll of (seed, tick, person)
    void setProfile(int index, const NpcProfile& profile);
    const NpcProfile* profile(int index) const; // null: nothing known about the person
    // A world event happens at (x, y) (a fire starts): it is on offer for as long as its definitions say, and the persons it concerns, within reach, answer at once.
    void postEvent(NpcPopulation& population, const std::string& trigger, int x, int y);
    const EventBoard& board() const { return board_; }
    // The interaction a person last chose for themselves ("" when none yet).
    std::string lastAction(int index) const;

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
    int internProfile(const NpcProfile& profile);
    // The person is free to do something of their own now: nothing in their schedule holds them (a person with no schedule is always free).
    bool isFree(const NpcPopulation& population, int index, int hour) const;
    // The person chooses what to do among their candidates (events only when `eventsOnly`); true when they chose something and did it. Bounded by maxPerHour.
    bool chooseAction(NpcPopulation& population, int index, int hour, bool eventsOnly);
    void carryOut(NpcPopulation& population, int index, const rules::Interaction& interaction, const ActionTarget& target);
    // A spot near a point, different for each person (so a crowd answering one event does not stand on one pixel).
    std::pair<int, int> pointNear(const NpcPopulation& population, int index, int x, int y) const;

    rules::ScheduleConfig config_;
    std::vector<Place> places_;
    std::vector<rules::Schedule> schedules_; // [0] is "no schedule"
    std::vector<std::uint16_t> scheduleOf_;
    std::vector<std::uint8_t> modes_;
    std::vector<std::uint8_t> dangers_;
    std::vector<std::int32_t> homeX_;
    std::vector<std::int32_t> homeY_;
    std::size_t adopted_ = 0; // the persons below this index have been given a home

    // US-291.
    std::vector<NpcProfile> profiles_; // [0] is "nothing known"
    std::vector<std::uint16_t> profileOf_;
    std::vector<std::int16_t> lastAction_; // an index into actionNames_, -1: none yet
    std::vector<std::string> actionNames_;
    EventCatalog events_;
    EventBoard board_;
    ActionSources sources_ = ActionSources::standard();
    rules::CooldownTable cooldowns_;
    const rules::InteractionRegistry* interactions_ = nullptr;
    std::uint64_t seed_ = 0;
    int chosenThisHour_ = 0;
};

} // namespace odysseus::sim
