#pragma once

#include "boundary.h"

#include "needs.h"
#include "person.h"

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace odysseus::sim {

class World;

inline constexpr std::size_t kCauseCount = static_cast<std::size_t>(CauseOfDeath::Childbirth) + 1;

// The numbers a soak test prints (US-015): how the clan is doing after a long run.
struct SimReport {
    int alive = 0;
    int founders = 0;
    int born = 0;
    int died = 0;
    std::array<int, kCauseCount> deathsByCause{};  // indexed by CauseOfDeath
    std::array<int, kNeedCount> averageNeeds{};     // of the living, 0..100
    int food = 0;
    int couples = 0;
    int feuds = 0;
    int mammoths = 0;
    int chronicleEntries = 0;
};

SimReport makeReport(const World& world);

// The report as lines of text, for the console.
std::vector<std::string> formatReport(const SimReport& report);

} // namespace odysseus::sim
