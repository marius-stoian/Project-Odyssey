#include "luna/engine/frame_stats.h"

namespace luna::engine {

void FrameStats::addFrame(std::uint64_t frameNanoseconds) {
    ++frames_;
    nanoseconds_ += frameNanoseconds;
}

std::uint64_t FrameStats::frames() const {
    return frames_;
}

double FrameStats::seconds() const {
    return static_cast<double>(nanoseconds_) / 1e9;
}

double FrameStats::averageFps() const {
    return nanoseconds_ == 0 ? 0.0 : static_cast<double>(frames_) / seconds();
}

} // namespace luna::engine
