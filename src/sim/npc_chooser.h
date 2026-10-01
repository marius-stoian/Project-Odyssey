#pragma once

#include "boundary.h"

#include "core/random.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace odysseus::sim::rules {

// How clan members and animals choose what to do (US-154, INT-05): every interaction they could do is scored with the `npc.score`
// expression of its file, and the best score wins. Whole numbers only. Ties are broken by the caller's seeded random stream, so the
// same world always chooses the same way (Charter rule 6).

// The index of the best score that reaches `minimum`, or nothing when no score does. Among equal best scores one is picked with
// `random` (one draw, always, so the stream moves the same whether or not there was a tie).
std::optional<std::size_t> pickBest(const std::vector<int>& scores, int minimum, odysseus::core::Pcg32& random);

// Interactions an actor has just done are resting for a while (`npc.cooldown`, seconds in the file, ticks here). Keys are an actor
// number and an interaction id; ordered containers, so nothing depends on hash order.
class CooldownTable {
public:
    bool ready(int actor, const std::string& interaction, std::int64_t now) const;
    void start(int actor, const std::string& interaction, std::int64_t until);
    void forget(int actor); // an actor who is gone
    void clear() { until_.clear(); }
    std::uint64_t hash() const;

private:
    std::map<std::pair<int, std::string>, std::int64_t> until_;
};

} // namespace odysseus::sim::rules
