#pragma once

#include "boundary.h"

#include "sim/action_runner.h"
#include "sim/quest_data.h"
#include "sim/rule_expr.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// Where a quest stands (US-180). Locked: its prerequisites do not hold yet. Available: they hold, and it waits to be given. Active: the player is on
// a step. Done and Failed are final.
enum class QuestStatus { Locked, Available, Active, Done, Failed };
const char* statusWord(QuestStatus status); // "locked", "available", "active", "done", "failed"

struct QuestState {
    QuestStatus status = QuestStatus::Locked;
    std::string step;                // the current step while Active (or the last one reached)
    std::int64_t stepStartTick = 0;  // when the player got to this step
    std::int64_t lastProgressTick = 0; // the last time the objective moved (the hint waits from here)
    int progress = 0;                // how much of the objective is done (gather 2 of 3)
};

// Something that happened in the world that a quest may be waiting for (US-181). The Game reports each one where it already knows the fact.
struct QuestEvent {
    QuestObjective::Kind kind = QuestObjective::Kind::Talk;
    std::string subject; // who, place, item, interaction or kind of enemy
    int amount = 1;      // how many items or enemies this one event stands for
    std::int64_t tick = 0; // when it happened; an event older than a step's start never counts for that step
};

struct QuestChange {
    enum class Kind { Available, Started, Step, Done, Failed };
    Kind kind = Kind::Available;
    std::string quest;
    std::string step; // Started and Step: the step reached
};

// Runs the quests (US-180): the state machine of each quest, kept as whole numbers and ids, so the same inputs always give the same quests (Charter rule 6).
class QuestBook {
public:
    QuestBook() = default;
    explicit QuestBook(std::vector<Quest> quests);

    // New data after a reload (F5): states of quests that still exist are kept, the rest are dropped.
    void replaceQuests(std::vector<Quest> quests);

    const std::vector<Quest>& quests() const { return quests_; }
    const Quest* find(std::string_view id) const;
    const QuestState* state(std::string_view id) const;
    QuestStatus status(std::string_view id) const; // Locked for a quest that does not exist
    // The current step's id of an Active quest, otherwise empty.
    std::string activeStep(std::string_view id) const;

    // The player (or a dialogue effect) takes an Available quest on. False, and nothing changes, for any other.
    bool start(std::string_view id, std::int64_t now);
    // Forces a status; for the debugger (US-186) and the effects `quest complete|fail`. They run no rewards unless `rewards` is true.
    bool complete(std::string_view id, std::int64_t now, ActionRunner* runner, EffectHost* host, const ThingRef& self = {});
    bool fail(std::string_view id);
    // Puts a quest back to Locked (the debugger).
    bool reset(std::string_view id);
    // Jumps an Active (or Available, which it starts) quest to a step (the debugger).
    bool jumpTo(std::string_view id, std::string_view step, std::int64_t now);

    // The world did something. It is counted at the next update().
    void notify(QuestEvent event) { events_.push_back(std::move(event)); }

    // Moves the quests on: counts the events, checks waits, flags, failures and prerequisites, moves steps, gives rewards through the runner (as the hero,
    // actor 0). `world` answers the questions of the rule language; quest(id) and step(id) are answered here. `ticksPerDay` turns `wait 1d` into ticks.
    // Quests are visited in id order. Returns what changed, in order.
    // `self` is the thing the rewards are aimed at (the hero).
    std::vector<QuestChange> update(const RuleContext& world, std::int64_t now, int ticksPerDay, ActionRunner& runner, EffectHost& host, const ThingRef& self = {});

    // True when the active step has a hint and the player has not moved the objective for as long as the hint says.
    bool hintDue(std::string_view id, std::int64_t now) const;

    // The quests as JSON text, with times relative to `now` so a saved game does not care what the clock said (as the action runner does), and back.
    // `load` keeps what it can read and returns the notes of the rest; a quest the data no longer has is dropped with a note.
    std::string save(std::int64_t now) const;
    std::vector<std::string> load(const std::string& text, std::int64_t now);

    // One number for the state of the book, for the determinism check.
    std::uint64_t hash() const;
    void clearStates();

private:
    std::vector<Quest> quests_; // sorted by id
    std::map<std::string, QuestState> states_; // by id, ordered (never hash order)
    std::vector<QuestEvent> events_;

    QuestState* stateOf(std::string_view id);
    void enter(const Quest& quest, QuestState& state, const std::string& step, std::int64_t now);
    void finish(const Quest& quest, QuestState& state, std::int64_t now, ActionRunner* runner, EffectHost* host, const ThingRef& self = {});
    bool objectiveMet(const Quest& quest, QuestState& state, const RuleContext& world, std::int64_t now, int ticksPerDay);
};

} // namespace odysseus::sim::rules
