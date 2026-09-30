// The story engine's daily interactions (M2b): quarrels, blame, revenge, wounds. They are
// World member functions, kept in their own file so world.cpp stays readable.
#include "sim/world.h"

#include <algorithm>
#include <format>

namespace odysseus::sim {

int World::quarrel(int a, int b) {
    Person& first = people_[static_cast<std::size_t>(a)];
    Person& second = people_[static_cast<std::size_t>(b)];
    const QuarrelStory& story = config_.story.quarrel;
    // Why they quarrel: the heaviest grudge either of them holds against the other.
    const Grudge* top = heaviestGrudge(second, a, heaviestGrudge(first, b));
    std::vector<int> causes;
    std::string reason;
    if (top != nullptr) {
        causes.push_back(top->event);
        if (const ChronicleEntry* entry = chronicle_.find(top->event)) {
            reason = entry->kind == EventKind::Quarrel ? " again" : " " + reasonPhrase(*entry);
        }
    }
    const int low = story.irritableBelow;
    const bool hungry = first.needs[Need::Hunger] < low || second.needs[Need::Hunger] < low;
    const bool tired = first.needs[Need::Energy] < low || second.needs[Need::Energy] < low;
    const std::string mood = hungry ? " while hungry" : (tired ? " while worn out" : "");
    const int event = chronicle_.record(date(), kImportanceQuarrel, EventKind::Quarrel, a, b, -1, causes,
                                        std::format("{} and {} quarrelled{}{}.", first.name, second.name, reason, mood));
    for (Person* side : {&first, &second}) {
        const int other = side->id == a ? b : a;
        changeOpinion(*side, other, -story.opinionLoss);
        remember(side->memories, {other, side->id, MemoryKind::Quarrel, today(), story.feeling, false, false, event},
                 config_.social.memoryLimit);
        addGrudge(*side, other, event, story.opinionLoss);
    }
    return event;
}

int World::blame(int griever, int blamed, int dead, int deathEvent, const std::string& why) {
    if (griever == blamed || griever < 0 || blamed < 0) {
        return -1;
    }
    // One blame per griever per death: the newest entries are the only place it can be.
    const auto& entries = chronicle_.entries();
    for (auto it = entries.rbegin(); it != entries.rend() && it->id >= deathEvent; ++it) {
        if (it->kind == EventKind::Blame && it->who == griever && it->aux == dead) {
            return -1;
        }
    }
    Person& mourner = people_[static_cast<std::size_t>(griever)];
    const int event = chronicle_.record(date(), kImportanceBlame, EventKind::Blame, griever, blamed, dead, {deathEvent},
                                        std::format("{} blamed {} for {}'s death, {}.", mourner.name, nameOf(blamed), nameOf(dead), why));
    changeOpinion(mourner, blamed, -config_.story.blame.opinionLoss);
    remember(mourner.memories, {blamed, griever, MemoryKind::Blame, today(), config_.story.blame.feeling, true, false, event},
             config_.social.memoryLimit);
    addGrudge(mourner, blamed, event, config_.story.blame.opinionLoss);
    return event;
}

int World::exile(Person& person, int victim, std::vector<int> causes, const std::string& text) {
    person.alive = false;
    person.exiled = true;
    stopCourting(person);
    dropSuitors(person.id);
    releaseTeaching(person);
    releaseCare(person);
    person.health = Health::Well;
    const int event = chronicle_.record(date(), kImportanceExile, EventKind::Exile, person.id, victim, -1, std::move(causes), text);
    // The clan remembers an exile like a loss: kin and partner grieve, a partner is free again.
    for (Person& kin : people_) {
        if (!kin.alive || kin.id == person.id) {
            continue;
        }
        if (kin.partner == person.id || closeKin(kin, person)) {
            remember(kin.memories, {person.id, kin.id, MemoryKind::Death, today(), config_.life.griefFeeling / 2, true, false, event},
                     config_.social.memoryLimit);
        }
        if (kin.partner == person.id) {
            kin.partner = -1;
        }
    }
    return event;
}

int World::takeRevenge(int aggressor, int victim) {
    Person& attacker = people_[static_cast<std::size_t>(aggressor)];
    Person& target = people_[static_cast<std::size_t>(victim)];
    const RevengeStory& story = config_.story.revenge;
    // The feud this comes from (its record remembers the day), and the grudge behind it.
    int feudEvent = -1;
    for (FeudRecord& feud : feuds_) {
        if (feud.a == std::min(aggressor, victim) && feud.b == std::max(aggressor, victim)) {
            feud.lastRevengeDay = today();
            feudEvent = feud.event;
        }
    }
    std::vector<int> causes;
    if (feudEvent >= 0) {
        causes.push_back(feudEvent);
    }
    std::string reason;
    if (const Grudge* grudge = heaviestGrudge(attacker, victim)) {
        if (std::find(causes.begin(), causes.end(), grudge->event) == causes.end()) {
            causes.push_back(grudge->event);
        }
        if (const ChronicleEntry* entry = chronicle_.find(grudge->event)) {
            reason = " " + reasonPhrase(*entry);
        }
    }
    // The clan's view of the aggressor decides between a fight and an exile.
    int total = 0;
    int counted = 0;
    for (const Person& other : people_) {
        if (other.alive && other.id != aggressor && other.id != victim) {
            total += opinion(other.id, aggressor);
            ++counted;
        }
    }
    if (counted > 0 && total / counted <= story.exileClanOpinion) {
        return exile(attacker, victim, causes, std::format("The clan drove {} out for attacking {}{}.", attacker.name, target.name, reason));
    }
    // A fight: strength is skill and courage, plus the luck of the day.
    auto strength = [this](const Person& person) {
        return person.huntSkill + (person.has(Trait::Brave) ? 15 : 0) - (person.has(Trait::Timid) ? 10 : 0) +
               static_cast<int>(storyRandom_.below(30));
    };
    const bool attackerWins = strength(attacker) >= strength(target);
    Person& winner = attackerWins ? attacker : target;
    Person& loser = attackerWins ? target : attacker;
    const bool loserDies = storyRandom_.chance(static_cast<std::uint32_t>(story.fightDeathPercent));
    const bool winnerHurt = storyRandom_.chance(static_cast<std::uint32_t>(story.winnerHurtPercent));
    const int span = config_.story.health.woundDaysMax - config_.story.health.woundDaysMin + 1;
    const int days = config_.story.health.woundDaysMin + static_cast<int>(storyRandom_.below(static_cast<std::uint32_t>(span)));
    const int event = chronicle_.record(date(), kImportanceRevenge, EventKind::Revenge, aggressor, victim, -1, causes,
                                        std::format("{} attacked {}{}: {} lost the fight and {}.", attacker.name, target.name, reason,
                                                    loser.name, loserDies ? "was killed" : "was badly hurt"));
    if (winnerHurt && !loserDies) {
        injure(winner.id, std::max(1, days / 2), event);
    }
    if (loserDies) {
        die(loser, CauseOfDeath::Fight, event);
    } else {
        injure(loser.id, days, event);
    }
    if (target.alive) {
        changeOpinion(target, aggressor, -story.victimOpinionLoss);
        remember(target.memories, {aggressor, victim, MemoryKind::Fight, today(), story.feeling, true, false, event},
                 config_.social.memoryLimit);
        addGrudge(target, aggressor, event, story.victimOpinionLoss);
    }
    if (attackerWins && attacker.alive) {
        changeOpinion(attacker, victim, story.satisfaction); // revenge is sweet
    }
    return event;
}

void World::considerRevenge() {
    const RevengeStory& story = config_.story.revenge;
    for (std::size_t i = 0; i < feuds_.size(); ++i) {
        const int a = feuds_[i].a;
        const int b = feuds_[i].b;
        if (!people_[static_cast<std::size_t>(a)].alive || !people_[static_cast<std::size_t>(b)].alive) {
            continue;
        }
        if (today() - feuds_[i].sinceDay < story.minFeudDays ||
            (feuds_[i].lastRevengeDay >= 0 && today() - feuds_[i].lastRevengeDay < story.cooldownDays)) {
            continue;
        }
        const int ofA = opinion(a, b);
        const int ofB = opinion(b, a);
        if (std::min(ofA, ofB) > story.revengeOpinion || !storyRandom_.chance(static_cast<std::uint32_t>(story.percentPerDay))) {
            continue;
        }
        // The one who hates more strikes first (a tie: the smaller id).
        if (ofA <= ofB) {
            takeRevenge(a, b);
        } else {
            takeRevenge(b, a);
        }
    }
}

void World::encounters() {
    const QuarrelStory& story = config_.story.quarrel;
    std::vector<int> living;
    for (const Person& person : people_) {
        if (person.alive) {
            living.push_back(person.id);
        }
    }
    if (living.size() < 2) {
        return;
    }
    std::vector<bool> quarrelled(people_.size(), false); // at most one quarrel a day each
    for (const int id : living) {
        const Person& person = people_[static_cast<std::size_t>(id)];
        int other = -1;
        // Often the one they meet is someone they hold a grudge against: old wounds draw people together.
        if (!person.grudges.empty() && storyRandom_.chance(static_cast<std::uint32_t>(story.grudgeMeetPercent))) {
            std::vector<int> foes;
            for (const Grudge& grudge : person.grudges) {
                if (people_[static_cast<std::size_t>(grudge.about)].alive &&
                    std::find(foes.begin(), foes.end(), grudge.about) == foes.end()) {
                    foes.push_back(grudge.about);
                }
            }
            std::sort(foes.begin(), foes.end());
            if (!foes.empty()) {
                other = foes[storyRandom_.below(static_cast<std::uint32_t>(foes.size()))];
            }
        }
        if (other < 0) {
            // Anyone else, all equally likely: draw from the others (skipping oneself).
            const auto pick = static_cast<std::size_t>(storyRandom_.below(static_cast<std::uint32_t>(living.size() - 1)));
            const auto self = static_cast<std::size_t>(std::find(living.begin(), living.end(), id) - living.begin());
            other = living[pick >= self ? pick + 1 : pick];
        }
        if (quarrelled[static_cast<std::size_t>(id)] || quarrelled[static_cast<std::size_t>(other)]) {
            continue;
        }
        if (opinion(id, other) > story.dislikeOpinion || opinion(other, id) > story.dislikeOpinion) {
            continue;
        }
        const Person& second = people_[static_cast<std::size_t>(other)];
        const bool irritable = std::min(person.needs[Need::Hunger], person.needs[Need::Energy]) < story.irritableBelow ||
                               std::min(second.needs[Need::Hunger], second.needs[Need::Energy]) < story.irritableBelow;
        if (storyRandom_.chance(static_cast<std::uint32_t>(irritable ? story.irritablePercent : story.calmPercent))) {
            quarrel(id, other);
            quarrelled[static_cast<std::size_t>(id)] = true;
            quarrelled[static_cast<std::size_t>(other)] = true;
        }
    }
}

} // namespace odysseus::sim
