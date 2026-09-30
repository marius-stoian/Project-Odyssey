#pragma once

#include "boundary.h"

#include <cstdint>

namespace odysseus::core {

// PCG32: a small, fast, high-quality random number generator (pcg-random.org, O'Neill 2014).
// The same seed and stream always give the same numbers on every computer, which is what
// determinism needs (Charter rule 6). Give each system its own stream (weather, AI, births)
// so adding a random call in one system never changes the numbers another system sees.
class Pcg32 {
public:
    Pcg32(std::uint64_t seed, std::uint64_t stream);

    std::uint32_t next();

    // A number in [0, bound), without the bias of "next() % bound".
    std::uint32_t below(std::uint32_t bound);

    // True with the given chance in percent (0..100).
    bool chance(std::uint32_t percent);

    // The internal state, for saving and for the world hash.
    std::uint64_t state() const { return state_; }
    std::uint64_t increment() const { return increment_; }
    void restore(std::uint64_t state, std::uint64_t increment);

private:
    std::uint64_t state_ = 0;
    std::uint64_t increment_ = 0;
};

} // namespace odysseus::core
