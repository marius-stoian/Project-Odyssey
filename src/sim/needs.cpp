#include "sim/needs.h"

#include "sim/calendar.h"
#include "sim/json_data.h"

#include <algorithm>

namespace odysseus::sim {

const char* needName(Need need) {
    switch (need) {
    case Need::Hunger: return "Hunger";
    case Need::Energy: return "Energy";
    case Need::Warmth: return "Warmth";
    case Need::Social: return "Social";
    default: return "?";
    }
}

int hourlyDrop(int dailyRate, int hour) {
    return dailyRate * hour / kHoursPerDay - dailyRate * (hour - 1) / kHoursPerDay;
}

int dailyRate(const NeedsConfig& config, Need need, bool winter) {
    if (need == Need::Warmth && winter) {
        return config.winterWarmthDecay;
    }
    return config.dailyDecay[static_cast<std::size_t>(need)];
}

void decayForHour(Needs& needs, const NeedsConfig& config, bool winter, int hour) {
    for (std::size_t i = 0; i < kNeedCount; ++i) {
        const Need need = static_cast<Need>(i);
        needs[need] = std::max(0, needs[need] - hourlyDrop(dailyRate(config, need, winter), hour));
    }
}

void satisfy(Needs& needs, Need need, int amount, int maximum) {
    needs[need] = std::min(maximum, needs[need] + amount);
}

NeedsConfig loadNeedsConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    NeedsConfig config;
    config.maximum = requireInt(json, file, "maximum", 10, 1000);
    if (!json.contains("dailyDecay") || !json.at("dailyDecay").is_object()) {
        throw DataError(file, "dailyDecay", "must be an object with hunger, energy, warmth and social");
    }
    const nlohmann::json& decay = json.at("dailyDecay");
    config.dailyDecay[static_cast<std::size_t>(Need::Hunger)] = requireInt(decay, file, "hunger", 0, config.maximum);
    config.dailyDecay[static_cast<std::size_t>(Need::Energy)] = requireInt(decay, file, "energy", 0, config.maximum);
    config.dailyDecay[static_cast<std::size_t>(Need::Warmth)] = requireInt(decay, file, "warmth", 0, config.maximum);
    config.dailyDecay[static_cast<std::size_t>(Need::Social)] = requireInt(decay, file, "social", 0, config.maximum);
    config.winterWarmthDecay = requireInt(json, file, "winterWarmthDecay", 0, config.maximum);
    config.mealValue = requireInt(json, file, "mealValue", 1, config.maximum);
    if (!json.contains("daysAtZeroBeforeDeath") || !json.at("daysAtZeroBeforeDeath").is_object()) {
        throw DataError(file, "daysAtZeroBeforeDeath", "must be an object with hunger and warmth");
    }
    const nlohmann::json& death = json.at("daysAtZeroBeforeDeath");
    config.hungerDaysBeforeDeath = requireInt(death, file, "hunger", 1, 365);
    config.warmthDaysBeforeDeath = requireInt(death, file, "warmth", 1, 365);
    return config;
}

} // namespace odysseus::sim
