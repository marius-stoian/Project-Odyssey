#pragma once

#include "boundary.h"

#include <array>
#include <cstddef>
#include <filesystem>

namespace odysseus::sim {

// The four Age 1 needs (D-03). 100 = fully satisfied, 0 = desperate.
enum class Need { Hunger, Energy, Warmth, Social, Count };

inline constexpr std::size_t kNeedCount = static_cast<std::size_t>(Need::Count);

const char* needName(Need need);

// A person's needs: one whole number per need. Plain data (a "component").
struct Needs {
    std::array<int, kNeedCount> values{100, 100, 100, 100};

    int& operator[](Need need) { return values[static_cast<std::size_t>(need)]; }
    int operator[](Need need) const { return values[static_cast<std::size_t>(need)]; }
};

// From assets/data/sim/needs.json.
struct NeedsConfig {
    int maximum = 100;
    std::array<int, kNeedCount> dailyDecay{30, 35, 20, 15};
    int winterWarmthDecay = 45;
    int mealValue = 40;
    int sleepValue = 60;
    int fireWarmth = 50;
    int talkSocial = 25;
    int hungerDaysBeforeDeath = 3;
    int warmthDaysBeforeDeath = 3;
};

NeedsConfig loadNeedsConfig(const std::filesystem::path& file);

// How much a need drops in hour `hour` (1..24) of a day with daily rate `dailyRate`. The
// 24 hourly drops add up to exactly the daily rate (30 a day -> 1, 1, 2, 1, 1, 2, ...), so
// whole numbers never drift: after H hours the total is dailyRate x H / 24, rounded down.
int hourlyDrop(int dailyRate, int hour);

// The daily rate for a need today: Warmth drops faster in winter.
int dailyRate(const NeedsConfig& config, Need need, bool winter);

// One game hour passes: every need drops by its share of the daily rate, never below 0.
void decayForHour(Needs& needs, const NeedsConfig& config, bool winter, int hour);

// Something satisfies a need (a meal, sleep, a fire, a talk): it rises, capped at the maximum.
void satisfy(Needs& needs, Need need, int amount, int maximum);

} // namespace odysseus::sim
