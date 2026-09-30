// The story engine's teaching and hunting parties (M2b, US-114). World member functions, in
// their own file so world.cpp stays readable.
#include "sim/world.h"

#include <algorithm>
#include <format>

namespace odysseus::sim {

void World::releaseTeaching(Person& person) {
    if (person.apprentice >= 0) {
        Person& youth = people_[static_cast<std::size_t>(person.apprentice)];
        youth.master = -1;
        youth.teachEvent = -1;
        person.apprentice = -1;
    }
    if (person.master >= 0) {
        people_[static_cast<std::size_t>(person.master)].apprentice = -1;
        person.master = -1;
        person.teachEvent = -1;
    }
}

void World::teaching() {
    const TeachingStory& teach = config_.story.teaching;
    const int daysPerYear = calendar_.daysPerYear();
    const int adultAge = config_.life.adultAgeYears;

    // 1. Lessons: the apprentice learns and grows close to the master, and graduates when they
    // have learnt what the master knows, or grow up.
    for (Person& youth : people_) {
        if (!youth.alive || youth.master < 0) {
            continue;
        }
        Person& master = people_[static_cast<std::size_t>(youth.master)];
        int& skill = youth.teachHunt ? youth.huntSkill : youth.gatherSkill;
        const int masterSkill = youth.teachHunt ? master.huntSkill : master.gatherSkill;
        if (youth.ageYears(daysPerYear) >= adultAge || skill >= masterSkill - teach.graduateGap) {
            chronicle_.record(date(), kImportanceGraduation, EventKind::Graduation, youth.id, master.id, -1, {youth.teachEvent},
                              std::format("{} finished learning {} from {}.", youth.name, youth.teachHunt ? "hunting" : "gathering", master.name));
            releaseTeaching(youth);
            continue;
        }
        skill = std::min(masterSkill, skill + teach.skillPerDay);
        changeOpinion(youth, master.id, teach.opinionPerDay);
        changeOpinion(master, youth.id, teach.opinionPerDay);
    }

    // 2. New apprentices: a skilled, well adult takes the youth they get on with best.
    for (Person& master : people_) {
        if (!master.alive || master.apprentice >= 0 || master.health != Health::Well || master.ageYears(daysPerYear) < adultAge) {
            continue;
        }
        Person* best = nullptr;
        bool bestHunt = false;
        int bestScore = 0;
        for (Person& youth : people_) {
            const int age = youth.ageYears(daysPerYear);
            if (!youth.alive || youth.master >= 0 || age < config_.actions.workAgeYears || age > teach.youthMaxYears || age >= adultAge) {
                continue;
            }
            if (opinion(master.id, youth.id) < teach.minOpinion || opinion(youth.id, master.id) < teach.minOpinion) {
                continue;
            }
            // What could be taught: the master's better skill, if they know enough and the youth is far behind.
            const int huntGap = master.huntSkill - youth.huntSkill;
            const int gatherGap = master.gatherSkill - youth.gatherSkill;
            const bool canHunt = age >= config_.actions.huntAgeYears && master.huntSkill >= teach.masterMinSkill && huntGap >= teach.skillGap;
            const bool canGather = master.gatherSkill >= teach.masterMinSkill && gatherGap >= teach.skillGap;
            if (!canHunt && !canGather) {
                continue;
            }
            const bool hunt = canHunt && (!canGather || huntGap >= gatherGap);
            const int score = opinion(master.id, youth.id) + opinion(youth.id, master.id) + (closeKin(master, youth) ? teach.kinBonus : 0);
            if (best == nullptr || score > bestScore) {
                best = &youth;
                bestHunt = hunt;
                bestScore = score;
            }
        }
        if (best == nullptr || !storyRandom_.chance(static_cast<std::uint32_t>(teach.takePercent))) {
            continue;
        }
        best->master = master.id;
        best->teachHunt = bestHunt;
        master.apprentice = best->id;
        best->teachEvent = chronicle_.record(date(), kImportanceApprentice, EventKind::Apprentice, master.id, best->id, -1, {},
                                             std::format("{} took {} as apprentice in {}.", master.name, best->name, bestHunt ? "hunting" : "gathering"));
    }
}

void World::huntMammoth(Person& sighter) {
    const HuntStory& hunt = config_.story.hunt;
    const int daysPerYear = calendar_.daysPerYear();

    // Who goes: whoever saw the mammoth, and well hunters who join. The search starts at a
    // random person, so the same few are not always the ones asked first.
    std::vector<Person*> party{&sighter};
    const std::size_t count = people_.size();
    const std::size_t start = huntRandom_.below(static_cast<std::uint32_t>(count));
    for (std::size_t i = 0; i < count && static_cast<int>(party.size()) < hunt.maxSize; ++i) {
        Person& other = people_[(start + i) % count];
        if (&other == &sighter || !other.alive || other.health != Health::Well || other.ageYears(daysPerYear) < config_.actions.huntAgeYears) {
            continue;
        }
        const int percent = hunt.joinPercent + (other.has(Trait::Brave) ? hunt.braveJoinBonus : 0) - (other.has(Trait::Timid) ? hunt.timidJoinPenalty : 0);
        if (huntRandom_.chance(static_cast<std::uint32_t>(std::clamp(percent, 0, 100)))) {
            party.push_back(&other);
        }
    }
    if (static_cast<int>(party.size()) < hunt.minSize) {
        // Too few to make a party: the sighter faces it alone, as before.
        if (huntRandom_.chance(static_cast<std::uint32_t>(config_.actions.mammothDeathPercent))) {
            die(sighter, CauseOfDeath::Hunting);
            return;
        }
        bringDownMammoth(sighter.id);
        return;
    }
    std::sort(party.begin(), party.end(), [](const Person* a, const Person* b) { return a->id < b->id; });

    // Roles: the best hunter leads; the most courageous is the hero, the least the coward.
    std::vector<int> courage;
    for (const Person* member : party) {
        courage.push_back(member->huntSkill + (member->has(Trait::Brave) ? 15 : 0) - (member->has(Trait::Timid) ? 10 : 0) +
                          static_cast<int>(huntRandom_.below(30)));
    }
    std::size_t leader = 0;
    std::size_t heroAt = 0;
    std::size_t cowardAt = 0;
    int skillSum = 0;
    for (std::size_t i = 0; i < party.size(); ++i) {
        skillSum += party[i]->huntSkill;
        if (party[i]->huntSkill > party[leader]->huntSkill) leader = i;
        if (courage[i] > courage[heroAt]) heroAt = i;
    }
    const bool hasHero = courage[heroAt] >= hunt.heroCourage;
    cowardAt = heroAt == 0 ? 1 : 0;
    for (std::size_t i = 0; i < party.size(); ++i) {
        if (i != heroAt && courage[i] < courage[cowardAt]) cowardAt = i;
    }
    const bool hasCoward = courage[cowardAt] <= hunt.cowardCourage && (!hasHero || cowardAt != heroAt);
    const int strength = skillSum / static_cast<int>(party.size());
    const int percent = std::clamp(hunt.successBase + strength + hunt.leaderBonus + (hasHero ? hunt.heroBonus : 0) -
                                       (hasCoward ? hunt.cowardPenalty : 0),
                                   0, 100);
    const bool success = huntRandom_.chance(static_cast<std::uint32_t>(percent));

    std::vector<std::string> names;
    for (const Person* member : party) {
        names.push_back(member->name);
    }
    const int partyEvent = chronicle_.record(
        date(), kImportanceHuntParty, EventKind::HuntParty, party[leader]->id, -1, -1, {},
        std::format("A hunting party of {} ({}) went after a mammoth under {}{}.", party.size(), joinNames(names), party[leader]->name,
                    success ? ", and brought it down" : ", but it got away"));

    // What the party did is remembered by every member (GD-03).
    auto remembered = [&](std::size_t who, MemoryKind kind, int feeling, int opinionChange, int event) {
        for (std::size_t i = 0; i < party.size(); ++i) {
            if (i == who) continue;
            remember(party[i]->memories, {party[who]->id, party[i]->id, kind, today(), feeling, true, false, event}, config_.social.memoryLimit);
            changeOpinion(*party[i], party[who]->id, opinionChange);
        }
    };
    if (hasHero) {
        const int event = chronicle_.record(date(), kImportanceHero, EventKind::Hero, party[heroAt]->id, -1, -1, {partyEvent},
                                            std::format("{} stood firm against the mammoth.", party[heroAt]->name));
        remembered(heroAt, MemoryKind::Heroism, hunt.heroFeeling, hunt.heroOpinion, event);
    }
    if (hasCoward) {
        const int event = chronicle_.record(date(), kImportanceCoward, EventKind::Coward, party[cowardAt]->id, -1, -1, {partyEvent},
                                            std::format("{} fled from the mammoth.", party[cowardAt]->name));
        remembered(cowardAt, MemoryKind::Cowardice, hunt.cowardFeeling, -hunt.cowardOpinionLoss, event);
    }

    // Danger: the mammoth turns on a member; the hero or the most courageous other may save them.
    for (std::size_t i = 0; i < party.size(); ++i) {
        if ((hasCoward && i == cowardAt) || !party[i]->alive || !huntRandom_.chance(static_cast<std::uint32_t>(hunt.dangerPercent))) {
            continue;
        }
        std::size_t rescuer = party.size();
        for (std::size_t j = 0; j < party.size(); ++j) {
            if (j == i || (hasCoward && j == cowardAt) || !party[j]->alive) continue;
            if (rescuer == party.size() || (hasHero && j == heroAt) || (!(hasHero && rescuer == heroAt) && courage[j] > courage[rescuer])) {
                rescuer = j;
            }
        }
        const int rescueChance = hunt.rescuePercent + (rescuer < party.size() && party[rescuer]->has(Trait::Brave) ? hunt.braveRescueBonus : 0);
        Person& endangered = *party[i];
        if (rescuer < party.size() && huntRandom_.chance(static_cast<std::uint32_t>(std::clamp(rescueChance, 0, 100)))) {
            Person& saviour = *party[rescuer];
            const int event = chronicle_.record(date(), kImportanceRescue, EventKind::Rescue, saviour.id, endangered.id, -1, {partyEvent},
                                                std::format("{} saved {} from the mammoth.", saviour.name, endangered.name));
            remember(endangered.memories, {saviour.id, endangered.id, MemoryKind::Rescue, today(), hunt.rescueFeeling, true, false, event},
                     config_.social.memoryLimit);
            changeOpinion(endangered, saviour.id, hunt.rescueOpinion);
            if (huntRandom_.chance(static_cast<std::uint32_t>(hunt.rescuerHurtPercent))) {
                injure(saviour.id, config_.story.health.woundDaysMin, event);
            }
        } else if (huntRandom_.chance(static_cast<std::uint32_t>(hunt.dangerDeathPercent))) {
            die(endangered, CauseOfDeath::Hunting, partyEvent);
        } else {
            injure(endangered.id, config_.story.health.woundDaysMax, partyEvent);
        }
    }

    if (success) {
        bringDownMammoth(party[hasHero ? heroAt : leader]->id, {partyEvent}, true);
    }
}

} // namespace odysseus::sim
