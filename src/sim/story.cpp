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

    config.courtship.favourOpinion = requireInt(json, file, "courtship", "favourOpinion", -100, 100);
    config.courtship.opinionPerDay = requireInt(json, file, "courtship", "opinionPerDay", 0, 100);
    config.courtship.suitorOpinionPerDay = requireInt(json, file, "courtship", "suitorOpinionPerDay", 0, 100);
    config.courtship.rejectBelow = requireInt(json, file, "courtship", "rejectBelow", -100, 100);
    config.courtship.giveUpDays = requireInt(json, file, "courtship", "giveUpDays", 1, 3650);
    config.courtship.rejectionOpinionLoss = requireInt(json, file, "courtship", "rejectionOpinionLoss", 0, 100);
    config.courtship.rejectionFeeling = requireInt(json, file, "courtship", "rejectionFeeling", -100, 0);
    config.courtship.pauseDays = requireInt(json, file, "courtship", "pauseDays", 0, 3650);

    config.rivals.minCourtDays = requireInt(json, file, "rivals", "minCourtDays", 0, 3650);
    config.rivals.opinionLoss = requireInt(json, file, "rivals", "opinionLoss", 0, 100);
    config.rivals.feeling = requireInt(json, file, "rivals", "feeling", -100, 0);
    config.rivals.quarrelPercent = requireInt(json, file, "rivals", "quarrelPercent", 0, 100);
    config.rivals.pauseDays = requireInt(json, file, "rivals", "pauseDays", 0, 3650);

    config.parting.partingOpinion = requireInt(json, file, "parting", "partingOpinion", -100, 100);
    config.parting.pauseDays = requireInt(json, file, "parting", "pauseDays", 0, 3650);

    config.teaching.masterMinSkill = requireInt(json, file, "teaching", "masterMinSkill", 0, 100);
    config.teaching.skillGap = requireInt(json, file, "teaching", "skillGap", 0, 100);
    config.teaching.takePercent = requireInt(json, file, "teaching", "takePercent", 0, 100);
    config.teaching.minOpinion = requireInt(json, file, "teaching", "minOpinion", -100, 100);
    config.teaching.kinBonus = requireInt(json, file, "teaching", "kinBonus", 0, 200);
    config.teaching.youthMaxYears = requireInt(json, file, "teaching", "youthMaxYears", 1, 40);
    config.teaching.skillPerDay = requireInt(json, file, "teaching", "skillPerDay", 0, 20);
    config.teaching.opinionPerDay = requireInt(json, file, "teaching", "opinionPerDay", 0, 100);
    config.teaching.graduateGap = requireInt(json, file, "teaching", "graduateGap", 0, 100);

    HuntStory& hunt = config.hunt;
    hunt.minSize = requireInt(json, file, "hunt", "minSize", 1, 20);
    hunt.maxSize = requireInt(json, file, "hunt", "maxSize", hunt.minSize, 20);
    hunt.joinPercent = requireInt(json, file, "hunt", "joinPercent", 0, 100);
    hunt.braveJoinBonus = requireInt(json, file, "hunt", "braveJoinBonus", 0, 100);
    hunt.timidJoinPenalty = requireInt(json, file, "hunt", "timidJoinPenalty", 0, 100);
    hunt.successBase = requireInt(json, file, "hunt", "successBase", -1000, 1000);
    hunt.leaderBonus = requireInt(json, file, "hunt", "leaderBonus", 0, 100);
    hunt.heroBonus = requireInt(json, file, "hunt", "heroBonus", 0, 100);
    hunt.cowardPenalty = requireInt(json, file, "hunt", "cowardPenalty", 0, 100);
    hunt.heroCourage = requireInt(json, file, "hunt", "heroCourage", 0, 200);
    hunt.cowardCourage = requireInt(json, file, "hunt", "cowardCourage", 0, 200);
    hunt.dangerPercent = requireInt(json, file, "hunt", "dangerPercent", 0, 100);
    hunt.rescuePercent = requireInt(json, file, "hunt", "rescuePercent", 0, 100);
    hunt.braveRescueBonus = requireInt(json, file, "hunt", "braveRescueBonus", 0, 100);
    hunt.rescuerHurtPercent = requireInt(json, file, "hunt", "rescuerHurtPercent", 0, 100);
    hunt.dangerDeathPercent = requireInt(json, file, "hunt", "dangerDeathPercent", 0, 100);
    hunt.heroFeeling = requireInt(json, file, "hunt", "heroFeeling", 0, 100);
    hunt.heroOpinion = requireInt(json, file, "hunt", "heroOpinion", 0, 100);
    hunt.cowardFeeling = requireInt(json, file, "hunt", "cowardFeeling", -100, 0);
    hunt.cowardOpinionLoss = requireInt(json, file, "hunt", "cowardOpinionLoss", 0, 100);
    hunt.rescueFeeling = requireInt(json, file, "hunt", "rescueFeeling", 0, 100);
    hunt.rescueOpinion = requireInt(json, file, "hunt", "rescueOpinion", 0, 100);
    return config;
}

} // namespace odysseus::sim
