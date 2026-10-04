#include "game/weather.h"

#include <algorithm>
#include <cmath>
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

namespace {
const WeatherDef* weatherAt(const std::vector<WeatherDef>& weathers, int index) {
    return index >= 0 && index < static_cast<int>(weathers.size()) ? &weathers[static_cast<std::size_t>(index)] : nullptr;
}
float tintOf(int value, double dim) { return static_cast<float>(value / 255.0 * dim); }
} // namespace

WeatherLight weatherLight(const std::vector<WeatherDef>& weathers, int previous, int current, double fade) {
    const WeatherDef* from = weatherAt(weathers, previous);
    const WeatherDef* to = weatherAt(weathers, current);
    const WeatherDef none;
    if (from == nullptr) from = &none;
    if (to == nullptr) to = &none;
    const double t = std::clamp(fade, 0.0, 1.0);
    const auto mix = [t](float a, float b) { return static_cast<float>(a + (b - a) * t); };
    WeatherLight light;
    light.red = mix(tintOf(from->tintRed, from->lightDim), tintOf(to->tintRed, to->lightDim));
    light.green = mix(tintOf(from->tintGreen, from->lightDim), tintOf(to->tintGreen, to->lightDim));
    light.blue = mix(tintOf(from->tintBlue, from->lightDim), tintOf(to->tintBlue, to->lightDim));
    return light;
}

double weatherFlashRate(const std::vector<WeatherDef>& weathers, int previous, int current, double fade) {
    const WeatherDef* from = weatherAt(weathers, previous);
    const WeatherDef* to = weatherAt(weathers, current);
    const double a = from != nullptr ? from->flashPerMinute : 0.0;
    const double b = to != nullptr ? to->flashPerMinute : 0.0;
    return a + (b - a) * std::clamp(fade, 0.0, 1.0);
}

double lightningFlash(std::uint64_t seed, double seconds, double perMinute) {
    if (perMinute <= 0.0 || seconds < 0.0) return 0.0;
    constexpr double kSlotSeconds = 0.2;
    const double slotPosition = seconds / kSlotSeconds;
    const auto slot = static_cast<std::uint64_t>(slotPosition);
    std::uint64_t x = seed * 0x9E3779B97F4A7C15ULL + (slot + 1) * 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 31;
    x *= 0x94D049BB133111EBULL;
    x ^= x >> 29;
    const double roll = static_cast<double>(x >> 11) / 9007199254740992.0; // 0 up to 1
    if (roll >= std::min(1.0, perMinute * kSlotSeconds / 60.0)) return 0.0;
    return 1.0 - (slotPosition - static_cast<double>(slot));
}

} // namespace odysseus::game
