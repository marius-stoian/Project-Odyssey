#include "sim/life.h"

#include "sim/json_data.h"

namespace odysseus::sim {

LifeConfig loadLifeConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    LifeConfig config;
    config.adultAgeYears = requireInt(json, file, "adultAgeYears", 10, 30);
    config.pairOpinion = requireInt(json, file, "pairOpinion", 0, 100);
    config.courtingBonus = requireInt(json, file, "courtingBonus", 0, 200);
    config.pairPercent = requireInt(json, file, "pairPercent", 0, 100);
    config.fertileFromYears = requireInt(json, file, "fertileFromYears", 10, 60);
    config.fertileToYears = requireInt(json, file, "fertileToYears", config.fertileFromYears, 60);
    config.conceptionPerMille = requireInt(json, file, "conceptionPerMille", 0, 1000);
    config.pregnancyDays = requireInt(json, file, "pregnancyDays", 1, 365);
    config.childbirthDeathPercent = requireInt(json, file, "childbirthDeathPercent", 0, 100);
    config.birthSpacingYears = requireInt(json, file, "birthSpacingYears", 0, 20);
    config.famineStorePressure = requireInt(json, file, "famineStorePressure", 0, 100);
    config.oldAgeFromYears = requireInt(json, file, "oldAgeFromYears", 20, 120);
    config.oldAgePerMillePerYear = requireInt(json, file, "oldAgePerMillePerYear", 0, 1000);
    config.feudOpinion = requireInt(json, file, "feudOpinion", -100, 0);
    config.parentChildOpinion = requireInt(json, file, "parentChildOpinion", 0, 100);
    config.griefFeeling = requireInt(json, file, "griefFeeling", -100, 0);
    return config;
}

} // namespace odysseus::sim
