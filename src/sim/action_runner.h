#pragma once

#include "boundary.h"

#include "sim/interaction.h"
#include "sim/rule_effect.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// What an action is done to, named by a kind (the game's own numbering) and a stable id, never by pointer (Charter rule 6, brief section 6).
struct ThingRef {
    int kind = 0;
    int id = -1;
    friend bool operator==(const ThingRef&, const ThingRef&) = default;
};

// What the runner asks the game to do. The runner keeps the time (durations, delays, ordering); the game keeps the world.
class EffectHost {
public:
    virtual ~EffectHost() = default;
    // `set target.state picked`.
    virtual void setState(const ThingRef& target, const std::string& state) = 0;
    // Every other effect verb: do, give, take, flag, say, fx...
    virtual void apply(const Effect& effect, int actor, const ThingRef& target) = 0;
    // For `after 1d ...`.
    virtual int ticksPerDay() const = 0;
};

// An interaction under way (US-153): a timed action. At most one per actor.
struct RunningAction {
    int actor = 0; // 0 is the hero
    std::string interaction;
    ThingRef target;
    std::int64_t startTick = 0;
    std::int64_t endTick = 0;
};

// An effect waiting for its time: `after 15s set target.state ripe`.
struct PendingEffect {
    std::int64_t dueTick = 0;
    std::int64_t seq = 0; // creation order: two effects due in the same tick happen in the order they were made
    int actor = 0;
    ThingRef target;
    Effect effect;
};

// Runs interactions over time (ADR-006: 20 ticks a second). Instant ones act at once; timed ones act when their duration is over and
// give nothing if stopped before; `after` effects wait in a queue. Everything is ordered by whole numbers (ticks, then creation order,
// then actor id), so the same inputs always give the same world (Charter rule 6).
class ActionRunner {
public:
    static constexpr int kTicksPerSecond = 20;

    // A change of an interaction for one thing (US-173): the game gives a function that returns the interaction as it is for this target, or nothing when it
    // is not changed. It is asked when an action starts and again when it finishes.
    using Adjuster = std::function<std::optional<Interaction>(const Interaction&, const ThingRef&)>;
    void setAdjuster(Adjuster adjuster) { adjuster_ = std::move(adjuster); }

    // Told each time an interaction has done its effects (US-181), with who did it, so quests can count what the hero finishes.
    using FinishObserver = std::function<void(const std::string& interactionId, int actor, const ThingRef& target)>;
    void setFinishObserver(FinishObserver observer) { finished_ = std::move(observer); }

    // Starts `interaction` for `actor` on `target`. An instant one (duration 0) carries out its effects now. False when the actor is
    // already busy with another action.
    bool start(const Interaction& interaction, int actor, const ThingRef& target, std::int64_t now, EffectHost& host);

    // Carries out effects now, in order, for an actor on a target: the same path an interaction's effects take (fter waits in the queue,
    // set target.state reaches the host's setState). A conversation's choices use it (US-161).
    void runEffects(const std::vector<Effect>& effects, int actor, const ThingRef& target, std::int64_t now, EffectHost& host);

    // Stops an action: no effects, nothing given (US-153 "Interrupt").
    void cancel(int actor);

    // Moves time to `now`: effects whose time has come happen, in order; then actions whose duration is over finish. The registry is
    // asked again for each finishing action (the data may have been reloaded meanwhile); one that no longer exists is dropped.
    void tick(std::int64_t now, const InteractionRegistry& registry, EffectHost& host);

    const RunningAction* running(int actor) const;
    // 0..100 for the ring over the target; -1 when the actor is not doing anything.
    int progress(int actor, std::int64_t now) const;
    const std::vector<PendingEffect>& pending() const { return pending_; }

    // Pending effects as JSON text, with their times relative to `now` so a saved game does not care what the clock said. Running
    // actions are not saved: loading drops them, and since their effects had not happened yet nothing is lost but the work in progress.
    std::string savePending(std::int64_t now) const;
    // Replaces the pending effects with the saved ones, due relative to `now`. Returns the notes of anything that could not be read.
    std::vector<std::string> loadPending(const std::string& text, std::int64_t now);

    // One number for the state of the runner, for the determinism check.
    std::uint64_t hash() const;
    void clear();

private:
    void run(const Effect& effect, int actor, const ThingRef& target, std::int64_t now, EffectHost& host);

    std::vector<RunningAction> running_; // sorted by actor
    std::vector<PendingEffect> pending_; // sorted by (dueTick, seq)
    std::int64_t nextSeq_ = 0;
    Adjuster adjuster_;
    FinishObserver finished_;
};

} // namespace odysseus::sim::rules
