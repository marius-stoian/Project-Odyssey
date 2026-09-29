#include "luna/engine/fixed_step_clock.h"
#include "luna/engine/frame_stats.h"

#include <doctest/doctest.h>

#include <cstdint>

using luna::engine::FixedStepClock;

namespace {

constexpr std::uint64_t kSecond = 1'000'000'000ULL;

// Plays `seconds` of frames at `hertz` (like a monitor refreshing) and returns the ticks run.
std::uint64_t ticksAt(int hertz, int seconds) {
    FixedStepClock clock(20);
    const std::uint64_t frame = kSecond / static_cast<std::uint64_t>(hertz);
    const std::uint64_t frames = static_cast<std::uint64_t>(hertz) * static_cast<std::uint64_t>(seconds);
    std::uint64_t ticks = 0;
    for (std::uint64_t i = 0; i < frames; ++i) {
        ticks += static_cast<std::uint64_t>(clock.advance(frame));
        CHECK(clock.alpha() >= 0.0);
        CHECK(clock.alpha() < 1.0);
    }
    return ticks;
}

} // namespace

TEST_CASE("US-020 Steady") {
    SUBCASE("simulation speed is the same at 30 Hz, 60 Hz and 144 Hz") {
        // 60 seconds at 20 ticks per second = 1200 ticks, whatever the monitor.
        // (1/30 s and 1/144 s are not whole nanoseconds, so a frame may lose a fraction:
        // at most one tick in a minute.)
        const std::uint64_t at30 = ticksAt(30, 60);
        const std::uint64_t at60 = ticksAt(60, 60);
        const std::uint64_t at144 = ticksAt(144, 60);
        CHECK(at30 >= 1199);
        CHECK(at30 <= 1200);
        CHECK(at60 >= 1199);
        CHECK(at60 <= 1200);
        CHECK(at144 >= 1199);
        CHECK(at144 <= 1200);
    }

    SUBCASE("a long stall does not make the game race to catch up") {
        FixedStepClock clock(20);
        CHECK(clock.advance(10 * kSecond) == FixedStepClock::kMaxTicksPerFrame);
        CHECK(clock.advance(0) == 0); // the backlog was dropped
    }

    SUBCASE("frame statistics report the average frame rate") {
        luna::engine::FrameStats stats;
        for (int i = 0; i < 600; ++i) {
            stats.addFrame(kSecond / 60);
        }
        CHECK(stats.frames() == 600);
        CHECK(stats.averageFps() == doctest::Approx(60.0).epsilon(0.001));
    }
}
