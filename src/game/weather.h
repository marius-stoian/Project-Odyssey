#pragma once

#include "boundary.h"

#include "core/random.h"
#include "game/catalogs.h"

#include <cstdint>
#include <string>
#include <vector>

namespace odysseus::game {

// The weather cycle (US-138, D-21): a seeded PCG32 stream (the stream "weather", Charter rule 6) picks the
// next weather by the weights of weather.json, every 60 to 120 seconds of play, and the new weather fades in
// over 3 seconds while the old one fades out. The weather is only for the eyes; nothing in play depends on it.
// The same seed gives the same weathers in the same order.
class WeatherCycle {
public:
    static constexpr int kMinTicks = 1200; // 60 s at 20 ticks per second
    static constexpr int kMaxTicks = 2400; // 120 s
    static constexpr int kFadeTicks = 60;  // 3 s

    WeatherCycle() = default;
    WeatherCycle(const std::vector<WeatherDef>& weathers, std::uint64_t seed);

    // One simulation tick.
    void update();

    // Indexes into the weather list. The game starts under "clear" (or the first entry).
    int current() const { return current_; }
    int previous() const { return previous_; }
    // How far the current weather has faded in: 0 just changed, 1 fully there.
    double fade() const { return fadeTicks_ >= kFadeTicks ? 1.0 : static_cast<double>(fadeTicks_) / kFadeTicks; }
    bool fading() const { return fadeTicks_ < kFadeTicks; }
    int ticksUntilChange() const { return untilChange_; }
    // Puts the game under this weather at once, fully faded in (for screenshots and demos: `--weather <name>`).
    void force(int index) {
        previous_ = current_ = index;
        fadeTicks_ = kFadeTicks;
    }
    // How many times the weather has changed, and every weather it has been, in order (for tests).
    int changes() const { return static_cast<int>(history_.size()); }
    const std::vector<int>& history() const { return history_; }

    // The FNV-1a hash of a text, the default seed (the level's name) when none is given.
    static std::uint64_t seedFromText(const std::string& text);

private:
    int pick();
    int nextInterval();

    std::vector<int> weights_;
    core::Pcg32 rng_{1, 9};
    int current_ = 0;
    int previous_ = 0;
    int fadeTicks_ = kFadeTicks;
    int untilChange_ = kMaxTicks;
    std::vector<int> history_;
};

} // namespace odysseus::game
