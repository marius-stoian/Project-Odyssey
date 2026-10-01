#include "game/weather.h"

#include <algorithm>
#include <numeric>

namespace odysseus::game {

WeatherCycle::WeatherCycle(const std::vector<WeatherDef>& weathers, std::uint64_t seed) : rng_(seed, 9) {
    for (const WeatherDef& weather : weathers) weights_.push_back(std::max(0, weather.weight));
    // Start under a clear sky (the entry named "clear", or else the first).
    for (std::size_t i = 0; i < weathers.size(); ++i) {
        if (weathers[i].name == "clear") current_ = static_cast<int>(i);
    }
    previous_ = current_;
    untilChange_ = nextInterval();
}

int WeatherCycle::nextInterval() {
    return kMinTicks + static_cast<int>(rng_.below(static_cast<std::uint32_t>(kMaxTicks - kMinTicks + 1)));
}

int WeatherCycle::pick() {
    const int total = std::accumulate(weights_.begin(), weights_.end(), 0);
    if (total <= 0) return current_;
    int at = static_cast<int>(rng_.below(static_cast<std::uint32_t>(total)));
    for (std::size_t i = 0; i < weights_.size(); ++i) {
        if (at < weights_[i]) return static_cast<int>(i);
        at -= weights_[i];
    }
    return current_;
}

void WeatherCycle::update() {
    if (weights_.empty()) return;
    if (fadeTicks_ < kFadeTicks) ++fadeTicks_;
    if (--untilChange_ > 0) return;
    previous_ = current_;
    current_ = pick();
    fadeTicks_ = 0;
    history_.push_back(current_);
    untilChange_ = nextInterval();
}

std::uint64_t WeatherCycle::seedFromText(const std::string& text) {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const unsigned char c : text) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    return hash;
}

} // namespace odysseus::game
