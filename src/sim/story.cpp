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

    config.sickness.basePerMille = requireInt(json, file, "sickness", "basePerMille", 0, 1000);
    config.sickness.hungerBelow = requireInt(json, file, "sickness", "hungerBelow", 0, 100);
    config.sickness.hungerPerMille = requireInt(json, file, "sickness", "hungerPerMille", 0, 1000);
    config.sickness.coldBelow = requireInt(json, file, "sickness", "coldBelow", 0, 100);
    config.sickness.coldPerMille = requireInt(json, file, "sickness", "coldPerMille", 0, 1000);
    config.sickness.daysMin = requireInt(json, file, "sickness", "daysMin", 1, 365);
    config.sickness.daysMax = requireInt(json, file, "sickness", "daysMax", config.sickness.daysMin, 365);
    config.sickness.deathPerMille = requireInt(json, file, "sickness", "deathPerMille", 0, 1000);
    config.sickness.huntWoundPerMille = requireInt(json, file, "sickness", "huntWoundPerMille", 0, 1000);

    config.nursing.minScore = requireInt(json, file, "nursing", "minScore", -100, 300);
    config.nursing.kinBonus = requireInt(json, file, "nursing", "kinBonus", 0, 200);
    config.nursing.kindBonus = requireInt(json, file, "nursing", "kindBonus", 0, 200);
    config.nursing.percent = requireInt(json, file, "nursing", "percent", 0, 100);
    config.nursing.extraHealPerDay = requireInt(json, file, "nursing", "extraHealPerDay", 0, 10);
    config.nursing.deathPercentWhenNursed = requireInt(json, file, "nursing", "deathPercentWhenNursed", 0, 100);
    config.nursing.opinionGain = requireInt(json, file, "nursing", "opinionGain", 0, 100);
    config.nursing.carerOpinionGain = requireInt(json, file, "nursing", "carerOpinionGain", 0, 100);
    config.nursing.feeling = requireInt(json, file, "nursing", "feeling", 0, 100);

    config.sharing.giverHungerMin = requireInt(json, file, "sharing", "giverHungerMin", 0, 100);
    config.sharing.gapMin = requireInt(json, file, "sharing", "gapMin", 0, 100);
    config.sharing.receiverHungerMax = requireInt(json, file, "sharing", "receiverHungerMax", 0, 100);
    config.sharing.amount = requireInt(json, file, "sharing", "amount", 1, 100);
    config.sharing.repeatDays = requireInt(json, file, "sharing", "repeatDays", 0, 3650);
    config.sharing.minScore = requireInt(json, file, "sharing", "minScore", -100, 300);
    config.sharing.kinBonus = requireInt(json, file, "sharing", "kinBonus", 0, 200);
    config.sharing.kindBonus = requireInt(json, file, "sharing", "kindBonus", 0, 200);
    config.sharing.childBonus = requireInt(json, file, "sharing", "childBonus", 0, 200);
    config.sharing.opinionGain = requireInt(json, file, "sharing", "opinionGain", 0, 100);
    config.sharing.feeling = requireInt(json, file, "sharing", "feeling", 0, 100);

    config.adoption.minScore = requireInt(json, file, "adoption", "minScore", -100, 300);
    config.adoption.kinBonus = requireInt(json, file, "adoption", "kinBonus", 0, 200);
    config.adoption.kindBonus = requireInt(json, file, "adoption", "kindBonus", 0, 200);
    config.adoption.limit = requireInt(json, file, "adoption", "limit", 1, 20);
    config.adoption.opinionGain = requireInt(json, file, "adoption", "opinionGain", 0, 100);
    config.adoption.feeling = requireInt(json, file, "adoption", "feeling", 0, 100);
    return config;
}

} // namespace odysseus::sim
