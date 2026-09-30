#include "sim/actions.h"

#include "sim/json_data.h"

namespace odysseus::sim {

const char* actionName(Action action) {
    switch (action) {
    case Action::Gather: return "Gather";
    case Action::Hunt: return "Hunt";
    case Action::Sleep: return "Sleep";
    case Action::WarmByFire: return "WarmByFire";
    case Action::Talk: return "Talk";
    case Action::GiveGift: return "GiveGift";
    case Action::Steal: return "Steal";
    case Action::Rest: return "Rest";
    case Action::Wander: return "Wander";
    default: return "?";
    }
}

ActionConfig loadActionConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    ActionConfig config;
    config.workAgeYears = requireInt(json, file, "workAgeYears", 1, 30);
    config.huntAgeYears = requireInt(json, file, "huntAgeYears", config.workAgeYears, 30);
    config.nightStartHour = requireInt(json, file, "nightStartHour", 13, kHoursPerDay);
    config.nightEndHour = requireInt(json, file, "nightEndHour", 1, 12);
    config.mealHour = requireInt(json, file, "mealHour", 1, kHoursPerDay);
    config.reserveDays = requireInt(json, file, "reserveDays", 0, 365);
    config.spoilPercent = requireInt(json, file, "spoilPercent", 0, 100);
    if (!json.contains("gatherYield") || !json.at("gatherYield").is_object()) {
        throw DataError(file, "gatherYield", "must be an object with spring, summer, autumn and winter");
    }
    const nlohmann::json& yield = json.at("gatherYield");
    config.gatherYield = {requireInt(yield, file, "spring", 0, 100), requireInt(yield, file, "summer", 0, 100),
                          requireInt(yield, file, "autumn", 0, 100), requireInt(yield, file, "winter", 0, 100)};
    if (!json.contains("forageDaily") || !json.at("forageDaily").is_object()) {
        throw DataError(file, "forageDaily", "must be an object with spring, summer, autumn and winter");
    }
    const nlohmann::json& forage = json.at("forageDaily");
    config.forageDaily = {requireInt(forage, file, "spring", 0, 100'000), requireInt(forage, file, "summer", 0, 100'000),
                          requireInt(forage, file, "autumn", 0, 100'000), requireInt(forage, file, "winter", 0, 100'000)};
    config.gatherSnack = requireInt(json, file, "gatherSnack", 0, 100);
    config.huntSuccessPercent = requireInt(json, file, "huntSuccessPercent", 0, 100);
    config.huntYield = requireInt(json, file, "huntYield", 0, 1000);
    config.gameDaily = requireInt(json, file, "gameDaily", 0, 1000);
    config.mammothPerMille = requireInt(json, file, "mammothPerMille", 0, 1000);
    config.mammothYield = requireInt(json, file, "mammothYield", 0, 10'000);
    config.mammothDeathPercent = requireInt(json, file, "mammothDeathPercent", 0, 100);
    config.sleepPerHour = requireInt(json, file, "sleepPerHour", 1, 100);
    config.firePerHour = requireInt(json, file, "firePerHour", 1, 100);
    config.talkPerHour = requireInt(json, file, "talkPerHour", 1, 100);
    config.restPerHour = requireInt(json, file, "restPerHour", 0, 100);
    config.nightSleepBonus = requireInt(json, file, "nightSleepBonus", 0, 1000);
    config.storePressureWeight = requireInt(json, file, "storePressureWeight", 0, 10);
    config.traitBonus = requireInt(json, file, "traitBonus", 0, 500);
    config.skillPerHours = requireInt(json, file, "skillPerHours", 1, 1000);
    return config;
}

bool isNight(const ActionConfig& config, int hour) {
    return hour > config.nightStartHour || hour <= config.nightEndHour;
}

} // namespace odysseus::sim
