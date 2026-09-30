#pragma once

#include "boundary.h"

#include "calendar.h"

#include <array>
#include <cstddef>
#include <filesystem>

namespace odysseus::sim {

// What a person can spend a game hour doing (US-012). Eating happens at the clan's
// evening meal, so the food actions are Gather and Hunt: they fill the shared store.
enum class Action { Gather, Hunt, Sleep, WarmByFire, Talk, GiveGift, Steal, Rest, Wander, Count };

inline constexpr std::size_t kActionCount = static_cast<std::size_t>(Action::Count);

const char* actionName(Action action);

// From assets/data/sim/actions.json: when people may do what, how much each hour of it
// brings, and the weights of the scores. All whole numbers (Charter rule 6).
struct ActionConfig {
    int workAgeYears = 12;       // gathering from this age
    int huntAgeYears = 15;       // hunting from this age
    int nightStartHour = 22;     // hours 22..24 and 1..6 are night
    int nightEndHour = 6;
    int mealHour = 19;           // the clan eats together at the end of this hour
    int reserveDays = 10;        // the store feels "low" below this many days of meals
    int spoilPercent = 2;        // of the store lost each day
    std::array<int, 4> gatherYield{2, 3, 2, 0}; // meals per hour of gathering, by season
    std::array<int, 4> forageDaily{50, 80, 60, 0}; // meals the land offers per day, by season
    int gatherSnack = 4;         // Hunger a gatherer gains by nibbling
    int huntSuccessPercent = 30; // chance per hour of hunting to catch small game
    int huntYield = 6;           // meals from small game
    int gameDaily = 4;           // small game the valley offers per day
    int mammothPerMille = 2;     // chance per hour of hunting to meet a mammoth, in thousandths
    int mammothYield = 120;
    int mammothDeathPercent = 10; // chance a mammoth kills the hunter
    int sleepPerHour = 6;        // Energy
    int firePerHour = 10;        // Warmth
    int talkPerHour = 8;         // Social
    int restPerHour = 2;         // Energy
    int nightSleepBonus = 150;   // added to Sleep's score at night
    int storePressureWeight = 1; // how much a low store pushes people to work
    int traitBonus = 30;         // Brave hunts, Diligent works, Talkative talks...
    int skillPerHours = 8;       // one skill point per this many hours of practice
};

ActionConfig loadActionConfig(const std::filesystem::path& file);

// Is this hour of the day (1..24) night?
bool isNight(const ActionConfig& config, int hour);

} // namespace odysseus::sim
