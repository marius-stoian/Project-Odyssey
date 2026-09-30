#include "sim/game_clock.h"

namespace odysseus::sim {

std::uint64_t GameClock::advance(std::uint64_t realNanoseconds) {
    accumulated_ += realNanoseconds * static_cast<std::uint64_t>(speed_);
    const std::uint64_t ticks = accumulated_ / kNanosecondsPerTick;
    accumulated_ %= kNanosecondsPerTick;
    return ticks;
}

} // namespace odysseus::sim
