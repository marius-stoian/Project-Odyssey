#pragma once

#include "boundary.h"

#include <cstdint>

namespace luna::engine {

// The "Fix Your Timestep!" accumulator (ADR-006). Real time goes in; a whole number of
// fixed simulation ticks comes out, so the world runs at the same speed on a 30 Hz and a
// 144 Hz monitor. The leftover time becomes alpha(), used to draw smoothly between ticks.
class FixedStepClock {
public:
    explicit FixedStepClock(int ticksPerSecond);

    // Adds real elapsed time; returns how many ticks to run now (at most kMaxTicksPerFrame,
    // so a long stall, e.g. dragging the window, never makes the game "catch up" for ages).
    int advance(std::uint64_t elapsedNanoseconds);

    // 0.0 = just after a tick, close to 1.0 = the next tick is due. For interpolation.
    double alpha() const;
    std::uint64_t totalTicks() const;

    static constexpr int kMaxTicksPerFrame = 10;

private:
    std::uint64_t tickNanoseconds_;
    std::uint64_t accumulated_ = 0;
    std::uint64_t totalTicks_ = 0;
};

} // namespace luna::engine
