#include "core/random.h"

namespace odysseus::core {

Pcg32::Pcg32(std::uint64_t seed, std::uint64_t stream) {
    // The official PCG32 seeding procedure.
    state_ = 0U;
    increment_ = (stream << 1U) | 1U;
    next();
    state_ += seed;
    next();
}

std::uint32_t Pcg32::next() {
    const std::uint64_t old = state_;
    state_ = old * 6364136223846793005ULL + increment_;
    const auto xorShifted = static_cast<std::uint32_t>(((old >> 18U) ^ old) >> 27U);
    const auto rotation = static_cast<std::uint32_t>(old >> 59U);
    return (xorShifted >> rotation) | (xorShifted << ((0U - rotation) & 31U));
}

std::uint32_t Pcg32::below(std::uint32_t bound) {
    if (bound == 0U) {
        return 0U;
    }
    // Reject the few values that would make low numbers slightly more likely.
    const std::uint32_t threshold = (0U - bound) % bound;
    for (;;) {
        const std::uint32_t value = next();
        if (value >= threshold) {
            return value % bound;
        }
    }
}

bool Pcg32::chance(std::uint32_t percent) {
    return below(100U) < percent;
}

void Pcg32::restore(std::uint64_t state, std::uint64_t increment) {
    state_ = state;
    increment_ = increment;
}

} // namespace odysseus::core
