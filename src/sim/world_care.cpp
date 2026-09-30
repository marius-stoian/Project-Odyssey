// The story engine's care and kindness (M2b, US-112): wounds and sickness, nursing, sharing
// food in a famine, and taking in orphans. World member functions, in their own file.
#include "sim/world.h"

#include <algorithm>
#include <climits>
#include <format>

namespace odysseus::sim {

void World::injure(int person, int days, int cause) {
    Person& hurt = people_[static_cast<std::size_t>(person)];
    if (!hurt.alive) {
        return;
    }
    hurt.health = Health::Injured;
    hurt.healthDays = std::max(hurt.healthDays, days);
    hurt.healthEvent = cause;
}

void World::sicken(int person, int days, int cause) {
    Person& sick = people_[static_cast<std::size_t>(person)];
    if (!sick.alive || sick.health != Health::Well) {
        return; // the hurt do not also fall sick: one trouble at a time
    }
    sick.health = Health::Sick;
    sick.healthDays = days;
    sick.healthEvent = cause;
}

void World::releaseCare(Person& person) {
    if (person.carer >= 0) {
        people_[static_cast<std::size_t>(person.carer)].nursing = -1;
        person.carer = -1;
    }
    if (person.nursing >= 0) {
        people_[static_cast<std::size_t>(person.nursing)].carer = -1;
        person.nursing = -1;
    }
}

int World::kill(int person, CauseOfDeath cause, int causeEvent) {
    return die(people_[static_cast<std::size_t>(person)], cause, causeEvent);
}

void World::hurtOnHunt(Person& hunter) {
    if (hunter.health != Health::Well || storyRandom_.below(1000) >= static_cast<std::uint32_t>(config_.story.sickness.huntWoundPerMille)) {
        return;
    }
    const int span = config_.story.health.woundDaysMax - config_.story.health.woundDaysMin + 1;
    const int days = config_.story.health.woundDaysMin + static_cast<int>(storyRandom_.below(static_cast<std::uint32_t>(span)));
    const int event = chronicle_.record(date(), kImportanceSickness, EventKind::Injury, hunter.id, -1, -1, {},
                                        std::format("{} was hurt hunting.", hunter.name));
    injure(hunter.id, days, event);
}

void World::updateHealth() {
    const SicknessStory& sickness = config_.story.sickness;
    const NursingStory& nursing = config_.story.nursing;
    // 0. Whoever was left without a carer (a carer who died or fell ill) finds a new one first.
    assignCarers();
    // 1. The unwell: each day they may die of it, or mend (faster when nursed).
    for (Person& person : people_) {
        if (!person.alive || person.health == Health::Well) {
            continue;
        }
        const bool wounded = person.health == Health::Injured;
        int deathPerMille = wounded ? config_.story.health.woundDeathPerMille : sickness.deathPerMille;
        if (person.carer >= 0) {
            deathPerMille = deathPerMille * nursing.deathPercentWhenNursed / 100; // care saves lives
        }
        if (storyRandom_.below(1000) < static_cast<std::uint32_t>(deathPerMille)) {
            die(person, wounded ? CauseOfDeath::Wound : CauseOfDeath::Illness, person.healthEvent);
            continue;
        }
        person.healthDays -= 1 + (person.carer >= 0 ? nursing.extraHealPerDay : 0);
        if (person.healthDays <= 0) {
            std::vector<int> causes;
            if (person.healthEvent >= 0) {
                causes.push_back(person.healthEvent);
            }
            const std::string base = wounded ? std::format("{} recovered from the wounds", person.name) : std::format("{} got well again", person.name);
            chronicle_.record(date(), kImportanceRecovery, EventKind::Recovery, person.id, person.carer, -1, causes,
                              person.carer >= 0 ? std::format("{}, nursed by {}.", base, nameOf(person.carer)) : base + ".");
            releaseCare(person);
            person.health = Health::Well;
            person.healthDays = 0;
            person.healthEvent = -1;
        }
    }
    // 2. Sickness strikes the weak: hunger and cold make it far likelier.
    for (Person& person : people_) {
        if (!person.alive || person.health != Health::Well) {
            continue;
        }
        const bool hungry = person.needs[Need::Hunger] <= sickness.hungerBelow;
        const bool cold = person.needs[Need::Warmth] <= sickness.coldBelow;
        const int perMille = sickness.basePerMille + (hungry ? sickness.hungerPerMille : 0) + (cold ? sickness.coldPerMille : 0);
        if (storyRandom_.below(1000) >= static_cast<std::uint32_t>(perMille)) {
            continue;
        }
        std::vector<int> causes;
        std::string why = " with a fever";
        if (hungry) {
            why = ", weakened by hunger";
            const std::vector<int> stores = recentEvents(EventKind::StoreEmpty, config_.story.causes.starvationWindowDays);
            if (!stores.empty()) {
                causes.push_back(stores.front());
            }
        } else if (cold) {
            why = ", weakened by the cold";
        }
        const int days = sickness.daysMin + static_cast<int>(storyRandom_.below(static_cast<std::uint32_t>(sickness.daysMax - sickness.daysMin + 1)));
        const int event = chronicle_.record(date(), kImportanceSickness, EventKind::Sickness, person.id, -1, -1, causes,
                                            std::format("{} fell sick{}.", person.name, why));
        sicken(person.id, days, event);
    }
    // 3. The sick and hurt find someone to look after them.
    assignCarers();
}

void World::assignCarers() {
    const NursingStory& nursing = config_.story.nursing;
    const int daysPerYear = calendar_.daysPerYear();
    for (Person& patient : people_) {
        if (!patient.alive || patient.health == Health::Well || patient.carer >= 0) {
            continue;
        }
        // The best carer: a person who is well, old enough to help, not busy, and who likes the
        // patient, is kin, or is kind. Equal scores go to the lower id (strictly greater wins).
        int best = -1;
        int bestScore = INT_MIN;
        for (const Person& carer : people_) {
            if (!carer.alive || carer.id == patient.id || carer.health != Health::Well || carer.nursing >= 0 ||
                carer.ageYears(daysPerYear) < config_.actions.workAgeYears) {
                continue;
            }
            const int score = opinion(carer.id, patient.id) + (closeKin(carer, patient) || carer.partner == patient.id ? nursing.kinBonus : 0) +
                              (carer.has(Trait::Kind) ? nursing.kindBonus : 0);
            if (score >= nursing.minScore && score > bestScore) {
                best = carer.id;
                bestScore = score;
            }
        }
        if (best < 0 || !storyRandom_.chance(static_cast<std::uint32_t>(nursing.percent))) {
            continue;
        }
        Person& carer = people_[static_cast<std::size_t>(best)];
        carer.nursing = patient.id;
        patient.carer = carer.id;
        std::vector<int> causes;
        if (patient.healthEvent >= 0) {
            causes.push_back(patient.healthEvent);
        }
        const int event = chronicle_.record(date(), kImportanceNursing, EventKind::Nursing, carer.id, patient.id, -1, causes,
                                            std::format("{} nursed {} back to health.", carer.name, patient.name));
        // The patient remembers the kindness with gratitude, for life.
        remember(patient.memories, {carer.id, patient.id, MemoryKind::Nursing, today(), nursing.feeling, true, false, event},
                 config_.social.memoryLimit);
        changeOpinion(patient, carer.id, nursing.opinionGain);
        changeOpinion(carer, patient.id, nursing.carerOpinionGain);
    }
}

int World::share(int giver, int receiver) {
    Person& from = people_[static_cast<std::size_t>(giver)];
    Person& to = people_[static_cast<std::size_t>(receiver)];
    const SharingStory& story = config_.story.sharing;
    const bool desperate = to.needs[Need::Hunger] <= 10;
    const int amount = std::min(story.amount, from.needs[Need::Hunger]);
    from.needs[Need::Hunger] -= amount;
    satisfy(to.needs, Need::Hunger, amount, config_.needs.maximum);
    std::vector<int> causes;
    const std::vector<int> stores = recentEvents(EventKind::StoreEmpty, config_.story.causes.starvationWindowDays);
    if (!stores.empty()) {
        causes.push_back(stores.front());
    }
    // A giver who keeps feeding the same person through a famine is told of once, not every night.
    const bool repeat = std::any_of(to.memories.begin(), to.memories.end(), [&](const Memory& memory) {
        return memory.kind == MemoryKind::Sharing && memory.subject == giver && today() - memory.day < story.repeatDays;
    });
    if (repeat) {
        changeOpinion(to, giver, 1);
        return -1;
    }
    const int event = chronicle_.record(date(), kImportanceSharing, EventKind::Sharing, giver, receiver, -1, causes,
                                        std::format("{} shared food with hungry {}.", from.name, to.name));
    remember(to.memories, {giver, receiver, MemoryKind::Sharing, today(), story.feeling, desperate, false, event},
             config_.social.memoryLimit);
    changeOpinion(to, giver, story.opinionGain);
    return event;
}

void World::shareFood() {
    const SharingStory& story = config_.story.sharing;
    std::vector<int> hungry;
    for (const Person& person : people_) {
        if (person.alive && person.needs[Need::Hunger] <= story.receiverHungerMax) {
            hungry.push_back(person.id);
        }
    }
    // The hungriest are helped first; equal hunger: the smaller id, so the order never depends on memory layout.
    std::sort(hungry.begin(), hungry.end(), [this](int a, int b) {
        const int ha = people_[static_cast<std::size_t>(a)].needs[Need::Hunger];
        const int hb = people_[static_cast<std::size_t>(b)].needs[Need::Hunger];
        return ha != hb ? ha < hb : a < b;
    });
    std::vector<bool> gave(people_.size(), false); // a giver spares food once a day
    for (const int id : hungry) {
        const Person& receiver = people_[static_cast<std::size_t>(id)];
        int best = -1;
        int bestScore = INT_MIN;
        for (const Person& giver : people_) {
            if (!giver.alive || giver.id == id || gave[static_cast<std::size_t>(giver.id)] || giver.needs[Need::Hunger] < story.giverHungerMin ||
                giver.needs[Need::Hunger] < receiver.needs[Need::Hunger] + story.gapMin) {
                continue;
            }
            const int score = opinion(giver.id, id) + (closeKin(giver, receiver) || giver.partner == id ? story.kinBonus : 0) +
                              (giver.has(Trait::Kind) ? story.kindBonus : 0) +
                              (receiver.ageYears(calendar_.daysPerYear()) < config_.actions.workAgeYears ? story.childBonus : 0);
            if (score >= story.minScore && score > bestScore) {
                best = giver.id;
                bestScore = score;
            }
        }
        if (best >= 0) {
            gave[static_cast<std::size_t>(best)] = true;
            share(best, id);
        }
    }
}

int World::adopt(int adopter, int child) {
    Person& ward = people_[static_cast<std::size_t>(child)];
    Person& parent = people_[static_cast<std::size_t>(adopter)];
    if (!isOrphan(ward)) {
        return -1;
    }
    // What left them alone: the latest death or exile among their parents.
    int cause = -1;
    for (const int lost : {ward.mother, ward.father}) {
        if (lost < 0) {
            continue;
        }
        for (auto it = chronicle_.entries().rbegin(); it != chronicle_.entries().rend(); ++it) {
            if ((it->kind == EventKind::Death || it->kind == EventKind::Exile) && it->who == lost) {
                cause = std::max(cause, it->id);
                break;
            }
        }
    }
    std::vector<int> causes;
    std::string why = "the loss of their parents";
    if (const ChronicleEntry* entry = chronicle_.find(cause)) {
        causes.push_back(cause);
        why = entry->kind == EventKind::Death ? std::format("the death of {}", nameOf(entry->who))
                                              : std::format("the exile of {}", nameOf(entry->who));
    }
    const AdoptionStory& story = config_.story.adoption;
    ward.guardian = adopter;
    const int event = chronicle_.record(date(), kImportanceAdoption, EventKind::Adoption, adopter, child, -1, causes,
                                        std::format("{}, orphaned by {}, was taken in by {}.", ward.name, why, parent.name));
    remember(ward.memories, {adopter, child, MemoryKind::Sharing, today(), story.feeling, true, false, event}, config_.social.memoryLimit);
    changeOpinion(ward, adopter, story.opinionGain);
    changeOpinion(parent, child, story.opinionGain);
    return event;
}

void World::adoptOrphans() {
    const AdoptionStory& story = config_.story.adoption;
    const int daysPerYear = calendar_.daysPerYear();
    for (const Person& child : people_) {
        if (!child.alive || !isOrphan(child) || (child.guardian >= 0 && people_[static_cast<std::size_t>(child.guardian)].alive)) {
            continue;
        }
        int best = -1;
        int bestScore = INT_MIN;
        for (const Person& adopter : people_) {
            if (!adopter.alive || adopter.id == child.id || adopter.health != Health::Well ||
                adopter.ageYears(daysPerYear) < config_.life.adultAgeYears) {
                continue;
            }
            const int wards = static_cast<int>(std::count_if(people_.begin(), people_.end(), [&adopter](const Person& other) {
                return other.alive && other.guardian == adopter.id;
            }));
            if (wards >= story.limit) {
                continue;
            }
            const int score = opinion(adopter.id, child.id) + (closeKin(adopter, child) ? story.kinBonus : 0) +
                              (adopter.has(Trait::Kind) ? story.kindBonus : 0);
            if (score >= story.minScore && score > bestScore) {
                best = adopter.id;
                bestScore = score;
            }
        }
        if (best >= 0) {
            adopt(best, child.id);
        }
    }
}

bool World::isOrphan(const Person& person) const {
    const auto gone = [this](int parent) { return parent < 0 || !people_[static_cast<std::size_t>(parent)].alive; };
    return person.ageYears(calendar_.daysPerYear()) < config_.life.adultAgeYears && (person.mother >= 0 || person.father >= 0) &&
           gone(person.mother) && gone(person.father);
}

} // namespace odysseus::sim
