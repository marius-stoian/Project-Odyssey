#include "luna/engine/fixed_step_clock.h"

#include "core/assertions.h"

namespace luna::engine {

FixedStepClock::FixedStepClock(int ticksPerSecond)
    : tickNanoseconds_(1'000'000'000ULL / static_cast<std::uint64_t>(ticksPerSecond)) {
    ODYSSEUS_ASSERT(ticksPerSecond > 0, "a clock needs at least one tick per second");
}

int FixedStepClock::advance(std::uint64_t elapsedNanoseconds) {
    accumulated_ += elapsedNanoseconds;
    int ticks = 0;
    while (accumulated_ >= tickNanoseconds_ && ticks < kMaxTicksPerFrame) {
        accumulated_ -= tickNanoseconds_;
        ++ticks;
    }
    if (ticks == kMaxTicksPerFrame && accumulated_ >= tickNanoseconds_) {
        accumulated_ = accumulated_ % tickNanoseconds_; // drop the backlog instead of spiralling
    }
    totalTicks_ += static_cast<std::uint64_t>(ticks);
    return ticks;
}

double FixedStepClock::alpha() const {
    return static_cast<double>(accumulated_) / static_cast<double>(tickNanoseconds_);
}

std::uint64_t FixedStepClock::tickNanoseconds() const {
    return tickNanoseconds_;
}

std::uint64_t FixedStepClock::totalTicks() const {
    return totalTicks_;
}

} // namespace luna::engine
