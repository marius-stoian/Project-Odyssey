#pragma once

#include "boundary.h"

#include "actions.h"
#include "person.h"

#include "core/random.h"

#include <array>
#include <string>

namespace odysseus::sim {

// What a person's surroundings allow this hour, worked out by the World.
struct Situation {
    int hour = 12;             // 1..24
    Season season = Season::Spring;
    int ageYears = 20;
    int storePressure = 0;     // 0..100: how badly the clan's store needs filling
    bool someoneToTalkTo = true;
    bool fireLit = true;
    bool canGiveGift = true; // someone is awake and no gift was given today
    bool canSteal = false;   // there is food in the store and the last theft is long enough ago
    bool unwell = false;     // sick or injured: no work, no hunting, no stealing (M2b)
    // US-196: what the active block of the person's profession routine makes of each action, in whole percent (see scoreActions). Only used when `routined`.
    bool routined = false;
    std::array<int, kActionCount> routinePercent{};
};

// Utility AI (US-012): every action gets a whole-number score from the person's needs,
// traits, skills and situation; the highest available score wins. Unavailable actions
// score 0 and can never be chosen. Rest and Wander are always possible, so nobody ever
// "freezes" with nothing to do.
std::array<bool, kActionCount> availableActions(const Situation& situation, const ActionConfig& config);

std::array<int, kActionCount> scoreActions(const Person& person, const Situation& situation,
                                           const std::array<bool, kActionCount>& available, const ActionConfig& config);

// Picks the highest score; ties are broken by the seeded random stream (determinism).
Decision decide(const Person& person, const Situation& situation, const std::array<bool, kActionCount>& available,
                const ActionConfig& config, core::Pcg32& random);

// "Tok (24, Brave): Hunger 12, Energy 70, ... -> Gather 172 (chosen), Hunt 150, Sleep 27, ..."
std::string describeDecision(const Person& person, int daysPerYear);

// How urgent a need is, 0..100: it grows with the square of what is missing, so a nearly
// empty need is far more urgent than a half-empty one.
int urgency(int needValue);

} // namespace odysseus::sim
