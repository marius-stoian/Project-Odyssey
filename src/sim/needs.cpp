#include "sim/needs.h"

#include "sim/json_data.h"

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
    config.sleepValue = requireInt(json, file, "sleepValue", 1, config.maximum);
    config.fireWarmth = requireInt(json, file, "fireWarmth", 1, config.maximum);
    config.talkSocial = requireInt(json, file, "talkSocial", 1, config.maximum);
    if (!json.contains("daysAtZeroBeforeDeath") || !json.at("daysAtZeroBeforeDeath").is_object()) {
        throw DataError(file, "daysAtZeroBeforeDeath", "must be an object with hunger and warmth");
    }
    const nlohmann::json& death = json.at("daysAtZeroBeforeDeath");
    config.hungerDaysBeforeDeath = requireInt(death, file, "hunger", 1, 365);
    config.warmthDaysBeforeDeath = requireInt(death, file, "warmth", 1, 365);
    return config;
}

} // namespace odysseus::sim
