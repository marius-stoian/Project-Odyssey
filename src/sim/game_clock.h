#pragma once

#include "boundary.h"

#include <cstdint>

namespace odysseus::sim {

// Pausable real time with speed control (D-01, MVP-12): pause, 1x, 2x, 4x.
enum class Speed { Paused = 0, Normal = 1, Fast = 2, Fastest = 4 };

// Turns real time into simulation ticks at 20 ticks per game second (ADR-006), times the
// speed. Whole nanoseconds only, so no rounding drift: at 4x, one real second is exactly
// 80 ticks, which is 4 game seconds.
class GameClock {
public:
    static constexpr std::uint64_t kTicksPerGameSecond = 20;

    void setSpeed(Speed speed) { speed_ = speed; }
    Speed speed() const { return speed_; }

    // Adds real elapsed time; returns how many ticks the world should run now.
    std::uint64_t advance(std::uint64_t realNanoseconds);

private:
    static constexpr std::uint64_t kNanosecondsPerTick = 1'000'000'000ULL / kTicksPerGameSecond;
    Speed speed_ = Speed::Normal;
    std::uint64_t accumulated_ = 0; // game nanoseconds not yet turned into a tick
};

} // namespace odysseus::sim
