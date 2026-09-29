#pragma once

#include "boundary.h"

#include <cstdint>

namespace luna::engine {

// Counts frames and the time they took, to report the average frame rate (US-020).
class FrameStats {
public:
    void addFrame(std::uint64_t frameNanoseconds);

    std::uint64_t frames() const;
    double seconds() const;
    double averageFps() const; // 0 when nothing was measured yet

private:
    std::uint64_t frames_ = 0;
    std::uint64_t nanoseconds_ = 0;
};

} // namespace luna::engine
