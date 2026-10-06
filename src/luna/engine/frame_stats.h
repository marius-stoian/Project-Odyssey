#pragma once

#include "boundary.h"

#include <algorithm>
#include <array>
#include <cstddef>
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

// The last N timings in milliseconds (draw, tick, frame) with their average and worst: what a performance overlay shows. A ring of N numbers,
// so it never allocates and old timings fall out by themselves.
template <std::size_t N>
class TimeWindow {
public:
    void add(double milliseconds) {
        values_[next_] = milliseconds;
        next_ = (next_ + 1) % N;
        filled_ = std::min(filled_ + 1, N);
    }
    bool empty() const { return filled_ == 0; }
    double last() const { return filled_ == 0 ? 0.0 : values_[(next_ + N - 1) % N]; }
    double average() const {
        double total = 0.0;
        for (std::size_t i = 0; i < filled_; ++i) total += values_[i];
        return filled_ == 0 ? 0.0 : total / static_cast<double>(filled_);
    }
    double worst() const { return filled_ == 0 ? 0.0 : *std::max_element(values_.begin(), values_.begin() + static_cast<std::ptrdiff_t>(filled_)); }

private:
    std::array<double, N> values_{};
    std::size_t next_ = 0;
    std::size_t filled_ = 0;
};

} // namespace luna::engine
