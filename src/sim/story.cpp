#include "sim/story.h"

#include "sim/json_data.h"

namespace odysseus::sim {

StoryConfig loadStoryConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    StoryConfig config;
    config.season.leanAutumnPercent = requireInt(json, file, "season", "leanAutumnPercent", 0, 100);
    config.season.leanForagePercent = requireInt(json, file, "season", "leanForagePercent", 0, 100);
    config.causes.theftWindowDays = requireInt(json, file, "causes", "theftWindowDays", 1, 3650);
    config.causes.starvationWindowDays = requireInt(json, file, "causes", "starvationWindowDays", 1, 3650);
    config.causes.maxCauses = requireInt(json, file, "causes", "maxCauses", 1, 20);
    config.causes.grudgeLimit = requireInt(json, file, "causes", "grudgeLimit", 1, 200);

    config.quarrel.grudgeMeetPercent = requireInt(json, file, "quarrel", "grudgeMeetPercent", 0, 100);
    config.quarrel.dislikeOpinion = requireInt(json, file, "quarrel", "dislikeOpinion", -100, 100);
    config.quarrel.irritableBelow = requireInt(json, file, "quarrel", "irritableBelow", 0, 100);
    config.quarrel.irritablePercent = requireInt(json, file, "quarrel", "irritablePercent", 0, 100);
    config.quarrel.calmPercent = requireInt(json, file, "quarrel", "calmPercent", 0, 100);
    config.quarrel.opinionLoss = requireInt(json, file, "quarrel", "opinionLoss", 0, 100);
    config.quarrel.feeling = requireInt(json, file, "quarrel", "feeling", -100, 0);

    config.blame.opinionLoss = requireInt(json, file, "blame", "opinionLoss", 0, 100);
    config.blame.feeling = requireInt(json, file, "blame", "feeling", -100, 0);

    config.revenge.minFeudDays = requireInt(json, file, "revenge", "minFeudDays", 0, 3650);
    config.revenge.revengeOpinion = requireInt(json, file, "revenge", "revengeOpinion", -100, 0);
    config.revenge.percentPerDay = requireInt(json, file, "revenge", "percentPerDay", 0, 100);
    config.revenge.cooldownDays = requireInt(json, file, "revenge", "cooldownDays", 0, 3650);
    config.revenge.exileClanOpinion = requireInt(json, file, "revenge", "exileClanOpinion", -100, 100);
    config.revenge.fightDeathPercent = requireInt(json, file, "revenge", "fightDeathPercent", 0, 100);
    config.revenge.winnerHurtPercent = requireInt(json, file, "revenge", "winnerHurtPercent", 0, 100);
    config.revenge.satisfaction = requireInt(json, file, "revenge", "satisfaction", 0, 100);
    config.revenge.victimOpinionLoss = requireInt(json, file, "revenge", "victimOpinionLoss", 0, 100);
    config.revenge.feeling = requireInt(json, file, "revenge", "feeling", -100, 0);

    config.health.woundDaysMin = requireInt(json, file, "health", "woundDaysMin", 1, 365);
    config.health.woundDaysMax = requireInt(json, file, "health", "woundDaysMax", config.health.woundDaysMin, 365);
    config.health.woundDeathPerMille = requireInt(json, file, "health", "woundDeathPerMille", 0, 1000);
    return config;
}

} // namespace odysseus::sim
