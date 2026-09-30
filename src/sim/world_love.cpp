// The story engine's love (M2b, US-113): courtship, rivals and partings. World member
// functions, in their own file so world.cpp stays readable.
#include "sim/world.h"

#include <algorithm>
#include <format>

namespace odysseus::sim {

void World::stopCourting(Person& person) {
    person.courting = -1;
    person.courtDays = 0;
    person.courtEvent = -1;
}

void World::dropSuitors(int beloved) {
    for (Person& suitor : people_) {
        if (suitor.courting == beloved) {
            stopCourting(suitor);
        }
    }
}

int World::pair(int a, int b) {
    Person& first = people_[static_cast<std::size_t>(a)];
    Person& second = people_[static_cast<std::size_t>(b)];
    // The courtship behind the pairing: whoever began courting the other first.
    const Person* suitor = nullptr;
    std::vector<int> causes;
    for (const Person* candidate : {&first, &second}) {
        const int loved = candidate == &first ? b : a;
        if (candidate->courting != loved || candidate->courtEvent < 0) {
            continue;
        }
        causes.push_back(candidate->courtEvent);
        if (suitor == nullptr || candidate->courtEvent < suitor->courtEvent) {
            suitor = candidate;
        }
    }
    std::sort(causes.begin(), causes.end());
    const std::string text = suitor == nullptr ? std::format("{} and {} became partners.", first.name, second.name)
                                               : std::format("{} and {} became partners after {}'s courtship.", first.name, second.name, suitor->name);
    first.partner = b;
    second.partner = a;
    stopCourting(first);
    stopCourting(second);
    const int event = chronicle_.record(date(), kImportancePairing, EventKind::Pairing, a, b, -1, causes, text);
    // Everyone else who hoped for either of them has lost.
    for (Person& rival : people_) {
        if (rival.id == a || rival.id == b || !rival.alive || rival.partner >= 0) {
            continue;
        }
        if (rival.courting == a || rival.courting == b) {
            const int beloved = rival.courting;
            makeJealous(rival, beloved == a ? b : a, beloved, event);
        }
    }
    return event;
}

void World::makeJealous(Person& rival, int winner, int beloved, int pairing) {
    const RivalStory& story = config_.story.rivals;
    const int courtEvent = rival.courtEvent;
    const int courtDays = rival.courtDays;
    stopCourting(rival);
    if (courtDays < story.minCourtDays) {
        return; // a suitor of a day or two is disappointed, not heartbroken
    }
    std::vector<int> causes;
    if (courtEvent >= 0) {
        causes.push_back(courtEvent);
    }
    causes.push_back(pairing);
    const int event = chronicle_.record(date(), kImportanceJealousy, EventKind::Jealousy, rival.id, winner, beloved, causes,
                                        std::format("{} grew jealous of {}, who won {}.", rival.name, nameOf(winner), nameOf(beloved)));
    changeOpinion(rival, winner, -story.opinionLoss);
    // The one not chosen remembers it for life.
    remember(rival.memories, {beloved, rival.id, MemoryKind::Rejection, today(), story.feeling, true, false, event},
             config_.social.memoryLimit);
    addGrudge(rival, winner, event, story.opinionLoss);
    rival.courtPauseDay = today() + story.pauseDays;
    if (storyRandom_.chance(static_cast<std::uint32_t>(story.quarrelPercent))) {
        quarrel(rival.id, winner);
    }
}

void World::turnDown(Person& suitor, int beloved, const std::string& text) {
    const CourtshipStory& love = config_.story.courtship;
    std::vector<int> causes;
    if (suitor.courtEvent >= 0) {
        causes.push_back(suitor.courtEvent);
    }
    const int event = chronicle_.record(date(), kImportanceRejection, EventKind::Rejection, suitor.id, beloved, -1, causes, text);
    changeOpinion(suitor, beloved, -love.rejectionOpinionLoss);
    remember(suitor.memories, {beloved, suitor.id, MemoryKind::Rejection, today(), love.rejectionFeeling, false, false, event},
             config_.social.memoryLimit);
    stopCourting(suitor);
    suitor.courtPauseDay = today() + love.pauseDays;
}

int World::part(int a, int b) {
    Person& first = people_[static_cast<std::size_t>(std::min(a, b))];
    Person& second = people_[static_cast<std::size_t>(std::max(a, b))];
    // Why: the heaviest grudges either holds against the other are the causes, the heaviest of all gives the reason.
    std::vector<Grudge> reasons;
    for (const Grudge& grudge : first.grudges) {
        if (grudge.about == second.id) reasons.push_back(grudge);
    }
    for (const Grudge& grudge : second.grudges) {
        if (grudge.about == first.id) reasons.push_back(grudge);
    }
    std::sort(reasons.begin(), reasons.end(), [](const Grudge& x, const Grudge& y) {
        return x.weight != y.weight ? x.weight > y.weight : x.event < y.event;
    });
    std::vector<int> causes;
    for (const Grudge& grudge : reasons) {
        if (static_cast<int>(causes.size()) < config_.story.causes.maxCauses &&
            std::find(causes.begin(), causes.end(), grudge.event) == causes.end()) {
            causes.push_back(grudge.event);
        }
    }
    std::string reason;
    if (!reasons.empty()) {
        if (const ChronicleEntry* top = chronicle_.find(reasons.front().event)) {
            reason = reasonPhrase(*top);
        }
    }
    const int event = chronicle_.record(date(), kImportanceParting, EventKind::Parting, first.id, second.id, -1, causes,
                                        std::format("{} and {} parted {}.", first.name, second.name,
                                                    reason.empty() ? std::string("as their love faded") : reason));
    first.partner = -1;
    second.partner = -1;
    first.courtPauseDay = today() + config_.story.parting.pauseDays;
    second.courtPauseDay = today() + config_.story.parting.pauseDays;
    return event;
}

void World::courtship() {
    const CourtshipStory& love = config_.story.courtship;
    const int daysPerYear = calendar_.daysPerYear();

    // 1. Partings: a partner who thinks too little of the other leaves (each pair once, smaller id first).
    for (Person& person : people_) {
        if (!person.alive || person.partner < person.id) {
            continue;
        }
        if (!people_[static_cast<std::size_t>(person.partner)].alive) {
            continue;
        }
        if (std::min(opinion(person.id, person.partner), opinion(person.partner, person.id)) < config_.story.parting.partingOpinion) {
            part(person.id, person.partner);
        }
    }

    // The unpaired adults, in id order: everything below happens among them.
    std::vector<Person*> singles;
    for (Person& person : people_) {
        if (person.alive && person.partner < 0 && person.ageYears(daysPerYear) >= config_.life.adultAgeYears) {
            singles.push_back(&person);
        } else if (person.courting >= 0) {
            stopCourting(person); // paired, dead or too young: nobody courts
        }
    }

    // 2. Each single courts the one they like best, if they like them enough. They stay faithful
    // to their love until that person is taken, or they stop liking them, or their heart is mending.
    for (Person* suitor : singles) {
        if (today() < suitor->courtPauseDay) {
            stopCourting(*suitor);
            continue;
        }
        if (suitor->courting >= 0) {
            const Person& loved = people_[static_cast<std::size_t>(suitor->courting)];
            if (!loved.alive || !courtable(*suitor, loved) || opinion(suitor->id, loved.id) < love.favourOpinion) {
                stopCourting(*suitor);
            }
        }
        if (suitor->courting >= 0) {
            continue;
        }
        const Person* best = nullptr;
        for (const Person* other : singles) {
            if (!courtable(*suitor, *other) || opinion(suitor->id, other->id) < love.favourOpinion) {
                continue;
            }
            if (best == nullptr || opinion(suitor->id, other->id) > opinion(suitor->id, best->id)) {
                best = other;
            }
        }
        if (best != nullptr) {
            suitor->courting = best->id;
            suitor->courtDays = 0;
            suitor->courtEvent = chronicle_.record(date(), kImportanceCourtship, EventKind::Courtship, suitor->id, best->id, -1, {},
                                                   std::format("{} began courting {}.", suitor->name, best->name));
        }
    }

    // 3. A day of courting: gifts and time together warm the loved one, unless they turn the
    // suitor away, or the suitor has waited in vain for too long.
    for (Person* suitor : singles) {
        if (suitor->courting < 0) {
            continue;
        }
        Person& loved = people_[static_cast<std::size_t>(suitor->courting)];
        ++suitor->courtDays;
        if (opinion(loved.id, suitor->id) < love.rejectBelow) {
            turnDown(*suitor, loved.id, std::format("{} turned down {}'s courtship.", loved.name, suitor->name));
        } else if (suitor->courtDays > love.giveUpDays) {
            turnDown(*suitor, loved.id, std::format("{} gave up courting {} after {} days.", suitor->name, loved.name, suitor->courtDays - 1));
        } else {
            changeOpinion(loved, suitor->id, love.opinionPerDay);
            changeOpinion(*suitor, loved.id, love.suitorOpinionPerDay);
        }
    }

    // 4. Both must agree: each thinks enough of the other, the suitor is courting, and the loved
    // one, who chooses among suitors the one they like best, decides today.
    for (Person* loved : singles) {
        if (loved->partner >= 0) {
            continue;
        }
        const Person* best = nullptr;
        for (const Person* suitor : singles) {
            if (suitor->courting != loved->id || suitor->partner >= 0) {
                continue;
            }
            const int hers = opinion(loved->id, suitor->id);
            const int his = opinion(suitor->id, loved->id);
            if (hers >= config_.life.pairOpinion && his >= config_.life.pairOpinion &&
                (best == nullptr || hers > opinion(loved->id, best->id))) {
                best = suitor;
            }
        }
        if (best != nullptr && lifeRandom_.chance(static_cast<std::uint32_t>(config_.life.pairPercent))) {
            pair(loved->id, best->id);
        }
    }
}

} // namespace odysseus::sim
