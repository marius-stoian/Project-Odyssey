#include "sim/world.h"

#include "core/hash.h"

#include <algorithm>
#include <cstdlib>
#include <format>

namespace odysseus::sim {

namespace {

// Typical daytime temperature per season, in degrees Celsius.
int seasonalTemperature(Season season) {
    switch (season) {
    case Season::Spring: return 10;
    case Season::Summer: return 20;
    case Season::Autumn: return 8;
    case Season::Winter: return -8;
    }
    return 10;
}

} // namespace

SimConfig loadSimConfig(const std::filesystem::path& dataDirectory) {
    SimConfig config;
    config.calendar = loadCalendarConfig(dataDirectory / "sim" / "calendar.json");
    config.needs = loadNeedsConfig(dataDirectory / "sim" / "needs.json");
    config.actions = loadActionConfig(dataDirectory / "sim" / "actions.json");
    config.clan = loadClanConfig(dataDirectory / "sim" / "clan.json");
    config.names = loadNameList(dataDirectory / "sim" / "names.json");
    config.social = loadSocialConfig(dataDirectory / "sim" / "social.json");
    config.life = loadLifeConfig(dataDirectory / "sim" / "life.json");
    config.story = loadStoryConfig(dataDirectory / "sim" / "story.json");
    return config;
}

World::World(std::uint64_t seed, SimConfig config)
    : seed_(seed), config_(config), calendar_(config.calendar),
      weather_(seed, static_cast<std::uint64_t>(Stream::Weather)),
      peopleRandom_(seed, static_cast<std::uint64_t>(Stream::People)),
      decisionRandom_(seed, static_cast<std::uint64_t>(Stream::Decisions)),
      huntRandom_(seed, static_cast<std::uint64_t>(Stream::Hunting)),
      socialRandom_(seed, static_cast<std::uint64_t>(Stream::Social)),
      lifeRandom_(seed, static_cast<std::uint64_t>(Stream::Life)),
      storyRandom_(seed, static_cast<std::uint64_t>(Stream::Story)),
      people_(makeStartingClan(config_.clan, config_.names, calendar_.daysPerYear(), peopleRandom_)),
      food_(config_.clan.startingFood) {
    for (Person& person : people_) {
        person.opinions.assign(people_.size(), 0); // strangers to nobody, friends of nobody yet
    }
    startDay();
    decideAll(1);
}

void World::tick() {
    ++ticks_;
    const auto ticksPerHour = static_cast<std::uint64_t>(calendar_.ticksPerHour());
    if (ticks_ % ticksPerHour == 0) {
        // Hours 1..24 of the day that is ending or running; hour 24 closes the day.
        const auto hour = static_cast<int>((ticks_ / ticksPerHour - 1) % kHoursPerDay) + 1;
        passHour(hour);
    }
    if (ticks_ % static_cast<std::uint64_t>(calendar_.ticksPerDay()) == 0) {
        startDay();
    }
}

void World::satisfyPersonNeed(int personId, Need need, int amount) {
    if (personId < 0 || static_cast<std::size_t>(personId) >= people_.size() || !people_[static_cast<std::size_t>(personId)].alive || amount <= 0) return;
    satisfy(people_[static_cast<std::size_t>(personId)].needs, need, amount, config_.needs.maximum);
}

void World::drainPersonNeed(int personId, Need need, int amount) {
    if (personId < 0 || static_cast<std::size_t>(personId) >= people_.size() || !people_[static_cast<std::size_t>(personId)].alive || amount <= 0) return;
    int& value = people_[static_cast<std::size_t>(personId)].needs[need];
    value = std::max(0, value - amount);
}

int World::population() const {
    return static_cast<int>(std::count_if(people_.begin(), people_.end(), [](const Person& p) { return p.alive; }));
}

void World::passHour(int hour) {
    // The hour that just passed belongs to the day before a new morning.
    const bool winter = calendar_.dateAt(ticks_ - 1).season == Season::Winter;
    for (Person& person : people_) {
        if (person.alive) {
            decayForHour(person.needs, config_.needs, winter, hour);
            if (dailyLife_) {
                doAction(person); // what they chose for this hour now pays off
            }
        }
    }
    if (!dailyLife_) {
        return;
    }
    if (hour == config_.actions.mealHour) {
        eatTogether();
    }
    decideAll(hour % kHoursPerDay + 1);
}

int World::storePressure() const {
    // The clan wants reserveDays of meals in store (twice that in autumn, before winter).
    const bool autumn = date().season == Season::Autumn;
    const int wanted = population() * config_.actions.reserveDays * (autumn ? 2 : 1);
    if (wanted <= 0 || food_ >= wanted) {
        return 0;
    }
    return (wanted - food_) * 100 / wanted;
}

Situation World::situationOf(const Person& person) const {
    Situation situation;
    situation.hour = hour_;
    situation.season = date().season;
    situation.ageYears = person.ageYears(calendar_.daysPerYear());
    situation.storePressure = storePressure();
    situation.someoneToTalkTo = std::any_of(people_.begin(), people_.end(), [&person](const Person& other) {
        return other.alive && other.id != person.id && other.action != Action::Sleep;
    });
    situation.canGiveGift = situation.someoneToTalkTo && person.lastGiftDay != today();
    situation.unwell = person.health != Health::Well;
    situation.canSteal = food_ > 0 && (person.lastTheftDay < 0 || today() - person.lastTheftDay >= config_.social.theftCooldownDays);
    return situation;
}

void World::decideAll(int nextHour) {
    hour_ = nextHour;
    for (Person& person : people_) {
        if (!person.alive) {
            continue;
        }
        const Situation situation = situationOf(person);
        person.lastDecision = decide(person, situation, availableActions(situation, config_.actions), config_.actions, decisionRandom_);
        person.action = person.lastDecision.chosen;
    }
}

void World::practise(int& practice, int& skill) {
    if (++practice >= config_.actions.skillPerHours) {
        practice = 0;
        skill = std::min(100, skill + 1);
    }
}

void World::doAction(Person& person) {
    const ActionConfig& actions = config_.actions;
    const int maximum = config_.needs.maximum;
    switch (person.action) {
    case Action::Gather: {
        // The land offers only so much each day: many gatherers share it (carrying capacity).
        const int yield = actions.gatherYield[static_cast<std::size_t>(calendar_.dateAt(ticks_ - 1).season)];
        const int found = std::min(forageLeft_, yield + (yield > 0 && person.gatherSkill >= 50 ? 1 : 0));
        forageLeft_ -= found;
        food_ += found;
        if (found > 0) {
            satisfy(person.needs, Need::Hunger, actions.gatherSnack, maximum); // a nibble of what was found
        }
        practise(person.gatherPractice, person.gatherSkill);
        break;
    }
    case Action::Hunt: {
        hurtOnHunt(person); // a hunt can end in a wound (US-112)
        practise(person.huntPractice, person.huntSkill);
        if (lastMammothYear_ != date().year && huntRandom_.below(1000) < static_cast<std::uint32_t>(actions.mammothPerMille)) {
            lastMammothYear_ = date().year; // met or not, the herd moves on
            huntMammoth(person);            // a party, or a lone hunter, goes after it (US-114)
        } else if (gameLeft_ > 0 &&
                   huntRandom_.chance(static_cast<std::uint32_t>(std::min(100, actions.huntSuccessPercent + person.huntSkill / 5)))) {
            --gameLeft_;
            food_ += actions.huntYield;
        }
        break;
    }
    case Action::Sleep:
        satisfy(person.needs, Need::Energy, actions.sleepPerHour, maximum);
        break;
    case Action::WarmByFire:
        satisfy(person.needs, Need::Warmth, actions.firePerHour, maximum);
        break;
    case Action::Talk:
        satisfy(person.needs, Need::Social, actions.talkPerHour, maximum);
        if (Person* partner = favouriteAwake(person, true)) {
            talk(person.id, partner->id);
        }
        break;
    case Action::GiveGift:
        if (Person* friendly = favouriteAwake(person)) {
            giveGift(person.id, friendly->id);
            satisfy(person.needs, Need::Social, actions.talkPerHour, maximum);
        }
        break;
    case Action::Steal: {
        // A handful from the store, eaten in secret; someone may see it.
        const int taken = std::min(food_, config_.social.theftMeals);
        food_ -= taken;
        person.lastTheftDay = today();
        satisfy(person.needs, Need::Hunger, taken * config_.needs.mealValue / 2, maximum);
        if (taken > 0) {
            // Someone may see it; either way the theft is an event, so a later empty store
            // can name the thief (US-110).
            int witness = -1;
            if (socialRandom_.chance(static_cast<std::uint32_t>(config_.social.witnessPercent))) {
                std::vector<int> awake;
                for (const Person& other : people_) {
                    if (other.alive && other.id != person.id && other.action != Action::Sleep) {
                        awake.push_back(other.id);
                    }
                }
                if (!awake.empty()) {
                    witness = awake[socialRandom_.below(static_cast<std::uint32_t>(awake.size()))];
                }
            }
            recordTheft(person.id, witness);
        }
        break;
    }
    case Action::Rest:
        satisfy(person.needs, Need::Energy, actions.restPerHour, maximum);
        break;
    default:
        break; // Wander: time passes
    }
}

void World::eatTogether() {
    // The evening meal: the hungriest eat first (children and the weak), one meal each, while
    // the store lasts. Ties go by id so the order never depends on memory layout.
    std::vector<Person*> eaters;
    for (Person& person : people_) {
        if (person.alive) {
            eaters.push_back(&person);
        }
    }
    std::sort(eaters.begin(), eaters.end(), [](const Person* a, const Person* b) {
        return a->needs[Need::Hunger] != b->needs[Need::Hunger] ? a->needs[Need::Hunger] < b->needs[Need::Hunger] : a->id < b->id;
    });
    int hungry = 0;
    for (Person* person : eaters) {
        if (food_ <= 0) {
            ++hungry;
            continue;
        }
        --food_;
        satisfy(person->needs, Need::Hunger, config_.needs.mealValue, config_.needs.maximum);
    }
    // The first evening the store runs dry is news; the next ones are not, until it refills.
    if (hungry > 0 && !storeRanOut_) {
        // Why it ran empty: the thefts of the last weeks and the failed harvest, if any (US-110).
        std::vector<int> causes = recentEvents(EventKind::Theft, config_.story.causes.theftWindowDays);
        std::vector<std::string> thieves;
        for (const int theft : causes) {
            const std::string& thief = nameOf(chronicle_.find(theft)->who);
            if (std::find(thieves.begin(), thieves.end(), thief) == thieves.end()) {
                thieves.push_back(thief);
            }
        }
        std::string reason;
        if (leanEvent_ >= 0) {
            causes.push_back(leanEvent_);
            reason = " after the failed harvest";
        }
        if (!thieves.empty()) {
            reason += std::string(reason.empty() ? " after" : " and") + " the thefts of " + joinNames(thieves);
        }
        chronicle_.record(date(), kImportanceStoreEmpty, EventKind::StoreEmpty, -1, -1, -1, causes,
                          std::format("The food store ran empty{}; {} went to bed hungry.", reason,
                                      hungry == 1 ? std::string("one") : std::to_string(hungry)));
    }
    storeRanOut_ = hungry > 0;
    if (hungry > 0) {
        shareFood(); // in a famine the better fed spare food for the hungriest (US-112)
    }
}

bool World::closeKin(const Person& a, const Person& b) const {
    const bool parentChild = a.mother == b.id || a.father == b.id || b.mother == a.id || b.father == a.id;
    const bool siblings = (a.mother >= 0 && a.mother == b.mother) || (a.father >= 0 && a.father == b.father);
    const bool guardian = a.guardian == b.id || b.guardian == a.id; // an adopted child is family too
    return parentChild || siblings || guardian;
}

int World::bringDownMammoth(int hunter, std::vector<int> causes, bool withParty) {
    food_ += config_.actions.mammothYield;
    ++mammoths_;
    const std::string& name = people_[static_cast<std::size_t>(hunter)].name;
    const std::string party = withParty ? " with the hunting party" : "";
    if (mammoths_ == 1) {
        return chronicle_.record(date(), kImportanceFirstMammoth, EventKind::Mammoth, hunter, -1, -1, std::move(causes),
                                 std::format("{} brought down the clan's first mammoth{}.", name, party));
    }
    return chronicle_.record(date(), kImportanceMammoth, EventKind::Mammoth, hunter, -1, -1, std::move(causes),
                             std::format("{} brought down a mammoth{}.", name, party));
}

std::vector<std::pair<int, int>> World::feuds() const {
    std::vector<std::pair<int, int>> pairs;
    for (const FeudRecord& feud : feuds_) {
        pairs.push_back({feud.a, feud.b});
    }
    return pairs;
}

void World::updateFeuds() {
    const int bitter = config_.life.feudOpinion;
    // Old feuds end when one side dies or both have cooled down.
    std::vector<FeudRecord> still;
    for (const FeudRecord& feud : feuds_) {
        const Person& first = people_[static_cast<std::size_t>(feud.a)];
        const Person& second = people_[static_cast<std::size_t>(feud.b)];
        if (!first.alive || !second.alive) {
            continue;
        }
        if (opinion(feud.a, feud.b) > 0 && opinion(feud.b, feud.a) > 0) {
            std::vector<int> causes;
            if (feud.event >= 0) {
                causes.push_back(feud.event);
            }
            chronicle_.record(date(), kImportancePeace, EventKind::Peace, feud.a, feud.b, -1, causes,
                              std::format("{} and {} made peace.", first.name, second.name));
            continue;
        }
        still.push_back(feud);
    }
    feuds_ = still;
    // New feuds: both think badly of each other, and say why: the strongest grudges they hold
    // against each other are the causes, the strongest of all gives the reason.
    for (const Person& first : people_) {
        if (!first.alive) {
            continue;
        }
        for (const Person& second : people_) {
            if (second.id <= first.id || !second.alive) {
                continue;
            }
            const bool known = std::any_of(feuds_.begin(), feuds_.end(), [&](const FeudRecord& feud) { return feud.a == first.id && feud.b == second.id; });
            if (opinion(first.id, second.id) <= bitter && opinion(second.id, first.id) <= bitter && !known) {
                std::vector<Grudge> reasons;
                for (const Grudge& grudge : first.grudges) {
                    if (grudge.about == second.id) reasons.push_back(grudge);
                }
                for (const Grudge& grudge : second.grudges) {
                    if (grudge.about == first.id) reasons.push_back(grudge);
                }
                // Heaviest first; equal weights: the earlier event (a total order, so no ties remain).
                std::sort(reasons.begin(), reasons.end(), [](const Grudge& a, const Grudge& b) {
                    return a.weight != b.weight ? a.weight > b.weight : a.event < b.event;
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
                const int event =
                    chronicle_.record(date(), kImportanceFeud, EventKind::Feud, first.id, second.id, -1, causes,
                                      reason.empty() ? std::format("A feud broke out between {} and {}.", first.name, second.name)
                                                     : std::format("A feud broke out between {} and {} {}.", first.name, second.name, reason));
                feuds_.push_back({first.id, second.id, event, today(), -1});
            }
        }
    }
    std::sort(feuds_.begin(), feuds_.end(), [](const FeudRecord& x, const FeudRecord& y) { return x.a != y.a ? x.a < y.a : x.b < y.b; });
}

void World::giveBirth(Person& mother, std::vector<Person>& newborns) {
    const LifeConfig& life = config_.life;
    Person child;
    child.id = static_cast<int>(people_.size() + newborns.size());
    child.sex = lifeRandom_.chance(50) ? Sex::Female : Sex::Male;
    child.mother = mother.id;
    child.father = mother.childFather;
    child.gatherSkill = 5;
    child.huntSkill = 5;
    std::vector<Person> living(people_);
    living.insert(living.end(), newborns.begin(), newborns.end());
    child.name = pickName(config_.names, child.sex, living, lifeRandom_);
    // Traits run in families: one from a parent, sometimes something new.
    const Person& father = people_[static_cast<std::size_t>(mother.childFather)];
    const Person& source = lifeRandom_.chance(50) ? mother : father;
    std::vector<Trait> inherited;
    for (std::size_t i = 0; i < kTraitCount; ++i) {
        if (source.has(static_cast<Trait>(i))) {
            inherited.push_back(static_cast<Trait>(i));
        }
    }
    if (!inherited.empty() && !lifeRandom_.chance(20)) {
        child.give(inherited[lifeRandom_.below(static_cast<std::uint32_t>(inherited.size()))]);
    } else {
        giveRandomTraits(child, lifeRandom_);
    }
    const int birthEvent = chronicle_.record(date(), kImportanceBirth, EventKind::Birth, child.id, mother.id, father.id, {},
                                             std::format("{} was born to {} and {}.", child.name, father.name, mother.name));
    mother.pregnantDays = 0;
    mother.lastBirthDay = today();
    newborns.push_back(child);
    if (lifeRandom_.chance(static_cast<std::uint32_t>(life.childbirthDeathPercent))) {
        die(mother, CauseOfDeath::Childbirth, birthEvent);
    }
}

void World::lifeEvents() {
    const LifeConfig& life = config_.life;
    const int daysPerYear = calendar_.daysPerYear();
    std::vector<Person> newborns; // added after the loop: the vector must not grow while we walk it
    for (Person& person : people_) {
        if (!person.alive) {
            continue;
        }
        const int age = person.ageYears(daysPerYear);
        if (age >= life.oldAgeFromYears) {
            const int perMille = (age - life.oldAgeFromYears + 1) * life.oldAgePerMillePerYear;
            if (lifeRandom_.below(1000) < static_cast<std::uint32_t>(perMille)) {
                die(person, CauseOfDeath::OldAge);
                continue;
            }
        }
        if (person.pregnantDays > 0 && ++person.pregnantDays > life.pregnancyDays) {
            giveBirth(person, newborns);
            continue;
        }
        const bool fertile = person.sex == Sex::Female && age >= life.fertileFromYears && age <= life.fertileToYears;
        // Mothers nurse for a while between children, and hungry times bring fewer births.
        const bool nursing = person.lastBirthDay >= 0 && today() - person.lastBirthDay < life.birthSpacingYears * daysPerYear;
        const bool famine = storePressure() > life.famineStorePressure;
        if (fertile && !nursing && !famine && person.pregnantDays == 0 && person.partner >= 0 &&
            people_[static_cast<std::size_t>(person.partner)].alive &&
            lifeRandom_.below(1000) < static_cast<std::uint32_t>(life.conceptionPerMille)) {
            person.pregnantDays = 1;
            person.childFather = person.partner;
        }
    }
    for (Person& child : newborns) {
        for (Person& other : people_) {
            other.opinions.push_back(0);
        }
        child.opinions.assign(people_.size() + 1, 0);
        people_.push_back(child);
        Person& born = people_.back();
        // Parents and child love each other from the start.
        for (const int parent : {born.mother, born.father}) {
            if (parent >= 0) {
                people_[static_cast<std::size_t>(parent)].opinions[static_cast<std::size_t>(born.id)] = life.parentChildOpinion;
                born.opinions[static_cast<std::size_t>(parent)] = life.parentChildOpinion;
            }
        }
    }
    updateHealth();
    adoptOrphans();
    teaching();
    encounters();
    courtship();
    updateFeuds();
    considerRevenge();
}

int World::opinion(int who, int about) const {
    const Person& person = people_[static_cast<std::size_t>(who)];
    return person.opinions[static_cast<std::size_t>(about)];
}

void World::adjustOpinion(int who, int about, int delta) {
    if (who < 0 || about < 0 || static_cast<std::size_t>(who) >= people_.size() || static_cast<std::size_t>(about) >= people_.size()) return;
    changeOpinion(people_[static_cast<std::size_t>(who)], about, delta);
}

int World::note(const std::string& text, int importance, EventKind kind, int who, int other) {
    return chronicle_.record(date(), importance, kind, who, other, -1, {}, text);
}

void World::changeOpinion(Person& who, int about, int change) {
    int& value = who.opinions[static_cast<std::size_t>(about)];
    value = std::clamp(value + change, -100, 100);
}

bool World::courtable(const Person& a, const Person& b) const {
    const int daysPerYear = calendar_.daysPerYear();
    return a.sex != b.sex && a.partner < 0 && b.partner < 0 && a.ageYears(daysPerYear) >= config_.life.adultAgeYears &&
           b.ageYears(daysPerYear) >= config_.life.adultAgeYears && !closeKin(a, b);
}

Person* World::favouriteAwake(const Person& person, bool courting) {
    // The awake person they like best; among equals the social stream picks. When talking,
    // unpaired adults lean towards unpaired adults of the other sex (courting).
    std::vector<Person*> best;
    int bestOpinion = -1000;
    for (Person& other : people_) {
        if (!other.alive || other.id == person.id || other.action == Action::Sleep) {
            continue;
        }
        const int value = person.opinions[static_cast<std::size_t>(other.id)] +
                          (courting && courtable(person, other) ? config_.life.courtingBonus : 0);
        if (value > bestOpinion) {
            bestOpinion = value;
            best.clear();
        }
        if (value == bestOpinion) {
            best.push_back(&other);
        }
    }
    if (best.empty()) {
        return nullptr;
    }
    return best[socialRandom_.below(static_cast<std::uint32_t>(best.size()))];
}

void World::giveGift(int giver, int receiver) {
    Person& from = people_[static_cast<std::size_t>(giver)];
    Person& to = people_[static_cast<std::size_t>(receiver)];
    from.lastGiftDay = today();
    chronicle_.add(date(), kImportanceGift, std::format("{} gave {} a gift.", from.name, to.name));
    remember(to.memories, {giver, receiver, MemoryKind::Gift, today(), config_.social.giftFeeling, false, false},
             config_.social.memoryLimit);
    changeOpinion(to, giver, config_.social.giftOpinion);
}

int World::recordTheft(int thief, int witness) {
    const std::string text = witness >= 0 ? std::format("{} stole from the food store; {} saw it.", nameOf(thief), nameOf(witness))
                                          : std::format("{} stole from the food store.", nameOf(thief));
    const int event = chronicle_.record(date(), kImportanceTheft, EventKind::Theft, thief, witness, -1, {}, text);
    if (witness >= 0) {
        Person& seer = people_[static_cast<std::size_t>(witness)];
        // The clan's store was robbed: the object of a theft is the thief's own clan (-1).
        remember(seer.memories, {thief, -1, MemoryKind::Theft, today(), config_.social.theftFeeling, true, false, event},
                 config_.social.memoryLimit);
        changeOpinion(seer, thief, config_.social.theftOpinion);
        addGrudge(seer, thief, event, -config_.social.theftOpinion);
    }
    return event;
}

void World::addGrudge(Person& owner, int about, int event, int weight) {
    if (weight <= 0 || about == owner.id) {
        return;
    }
    std::vector<Grudge>& grudges = owner.grudges;
    for (Grudge& known : grudges) {
        if (known.about == about && known.event == event) {
            known.weight = std::max(known.weight, weight);
            return;
        }
    }
    grudges.push_back({about, event, weight});
    if (static_cast<int>(grudges.size()) > config_.story.causes.grudgeLimit) {
        // The weakest reason goes; among equals the oldest (min_element returns the first).
        grudges.erase(std::min_element(grudges.begin(), grudges.end(), [](const Grudge& a, const Grudge& b) { return a.weight < b.weight; }));
    }
}

std::string World::reasonPhrase(const ChronicleEntry& event) const {
    switch (event.kind) {
    case EventKind::Theft: return "over stolen meat";
    case EventKind::Quarrel: return "after a bitter quarrel";
    case EventKind::Blame: return event.aux >= 0 ? std::format("over the death of {}", nameOf(event.aux)) : "over a death";
    case EventKind::Death: return std::format("over the death of {}", nameOf(event.who));
    case EventKind::Revenge: return "after the fight";
    case EventKind::Jealousy: return event.aux >= 0 ? std::format("over their love for {}", nameOf(event.aux)) : "out of jealousy";
    default: return "";
    }
}

std::vector<int> World::recentEvents(EventKind kind, int days) const {
    std::vector<int> found;
    const std::int64_t earliest = today() - days;
    const auto& entries = chronicle_.entries();
    for (auto it = entries.rbegin(); it != entries.rend() && it->date.day >= earliest; ++it) {
        if (it->kind == kind) {
            found.push_back(it->id);
            if (static_cast<int>(found.size()) >= config_.story.causes.maxCauses) {
                break;
            }
        }
    }
    return found;
}

std::string World::seasonPhrase() const {
    switch (date().season) {
    case Season::Winter: return "the hard winter";
    case Season::Autumn: return "the lean autumn";
    case Season::Spring: return "the hungry spring";
    case Season::Summer: return "the dry summer";
    }
    return "a hard time";
}

bool World::rememberConversation(int holder, int other, const std::string& text, int feeling) {
    const auto count = static_cast<int>(people_.size());
    if (holder < 0 || other < 0 || holder >= count || other >= count || holder == other || !people_[static_cast<std::size_t>(holder)].alive) return false;
    Person& person = people_[static_cast<std::size_t>(holder)];
    feeling = std::clamp(feeling, -100, 100);
    const MemoryKind kind = feeling >= 0 ? MemoryKind::Gift : MemoryKind::Quarrel;
    remember(person.memories, {other, holder, kind, today(), feeling, std::abs(feeling) >= 60, false, -1}, config_.social.memoryLimit);
    if (!text.empty()) {
        person.notes.push_back({text, today(), feeling, false, true});
        constexpr std::size_t kMostNotes = 20; // a person keeps at most this many free-text memories, the oldest go first
        if (person.notes.size() > kMostNotes) person.notes.erase(person.notes.begin());
    }
    return true;
}

bool World::talk(int speaker, int listener) {
    Person& from = people_[static_cast<std::size_t>(speaker)];
    Person& to = people_[static_cast<std::size_t>(listener)];
    satisfy(to.needs, Need::Social, config_.actions.talkPerHour / 2, config_.needs.maximum);
    changeOpinion(from, listener, config_.social.talkOpinion);
    changeOpinion(to, speaker, config_.social.talkOpinion);
    talks_.push_back({speaker, listener});
    if (talks_.size() > 32) talks_.erase(talks_.begin());
    const int chance = from.has(Trait::Talkative) ? config_.social.talkativeGossipPercent : config_.social.gossipPercent;
    if (!socialRandom_.chance(static_cast<std::uint32_t>(chance))) {
        return false;
    }
    // Gossip: the strongest memory the listener lacks (and is not about the listener).
    const Memory* story = nullptr;
    for (const Memory& memory : from.memories) {
        if (memory.subject == listener) {
            continue;
        }
        const bool known = std::any_of(to.memories.begin(), to.memories.end(),
                                       [&memory](const Memory& theirs) { return theirs.sameEvent(memory); });
        if (!known && (story == nullptr || std::abs(memory.feeling) > std::abs(story->feeling))) {
            story = &memory;
        }
    }
    if (story == nullptr) {
        return false;
    }
    Memory copy = *story;
    copy.feeling /= 2; // "slightly weaker" (GD-03): heard, not lived
    copy.secondHand = true;
    const int subject = copy.subject;
    const int opinionChange = copy.feeling / 4;
    remember(to.memories, copy, config_.social.memoryLimit);
    changeOpinion(to, subject, opinionChange);
    addGrudge(to, subject, copy.event, -opinionChange); // a bad story heard is still a reason (US-110)
    return true;
}

const Person* World::findPerson(const std::string& nameOrId) const {
    for (const Person& person : people_) {
        if (person.name == nameOrId || std::to_string(person.id) == nameOrId) {
            return &person;
        }
    }
    return nullptr;
}

void World::checkSurvival(Person& person, bool winter) {
    // A need that stays at 0 for the configured number of days kills (D-02): the counter
    // grows at each morning that finds the need empty. Hunger damage builds up: a fed
    // morning only takes one day off the count, so half rations still wear people down.
    person.daysAtZeroHunger = person.needs[Need::Hunger] == 0 ? person.daysAtZeroHunger + 1 : std::max(0, person.daysAtZeroHunger - 1);
    person.daysAtZeroWarmth = (winter && person.needs[Need::Warmth] == 0) ? person.daysAtZeroWarmth + 1 : 0;
    if (person.daysAtZeroHunger > config_.needs.hungerDaysBeforeDeath) {
        die(person, CauseOfDeath::Starvation);
    } else if (person.daysAtZeroWarmth > config_.needs.warmthDaysBeforeDeath) {
        die(person, CauseOfDeath::Cold);
    }
}

int World::die(Person& person, CauseOfDeath cause, int causeEvent) {
    person.alive = false;
    person.causeOfDeath = cause;
    stopCourting(person);         // the dead court nobody, and nobody courts the dead
    dropSuitors(person.id);
    releaseTeaching(person);      // a master's death ends the lessons, and so does an apprentice's
    releaseCare(person);          // nobody nurses the dead, and a carer who dies frees their patient
    person.health = Health::Well; // nothing more to heal
    const int years = person.ageYears(calendar_.daysPerYear());
    std::string sentence;
    std::vector<int> causes;
    int behind = -1; // the person behind the death (a thief, the one who struck the blow), if any
    switch (cause) {
    case CauseOfDeath::Starvation: {
        // Hunger kills after an empty store: name the store's cause, a thief if there was one (US-110).
        sentence = std::format("{} died of hunger in {}", person.name, seasonPhrase());
        const std::vector<int> stores = recentEvents(EventKind::StoreEmpty, config_.story.causes.starvationWindowDays);
        if (!stores.empty()) {
            const ChronicleEntry& store = *chronicle_.find(stores.front());
            causes.push_back(store.id);
            for (const int earlier : store.causes) {
                const ChronicleEntry* entry = chronicle_.find(earlier);
                if (entry != nullptr && entry->kind == EventKind::Theft && behind < 0) {
                    behind = entry->who;
                    causes.push_back(entry->id);
                }
            }
            const bool failedHarvest = std::find_if(store.causes.begin(), store.causes.end(), [this](int id) {
                                           return chronicle_.find(id)->kind == EventKind::Lean;
                                       }) != store.causes.end();
            if (behind >= 0) {
                sentence += std::format(", after {} stole from the store", nameOf(behind));
            } else {
                sentence += failedHarvest ? ", after the failed harvest" : ", when the store ran empty";
            }
        }
        sentence += ".";
        break;
    }
    case CauseOfDeath::Cold:
        sentence = std::format("{} died of the cold in the hard winter.", person.name);
        break;
    case CauseOfDeath::Hunting:
        sentence = std::format("{} was killed hunting a mammoth.", person.name);
        break;
    case CauseOfDeath::OldAge:
        sentence = std::format("{} died of old age, {} years old.", person.name, years);
        break;
    case CauseOfDeath::Childbirth:
        sentence = std::format("{} died giving birth.", person.name);
        break;
    case CauseOfDeath::Fight: {
        // A fight is the event `causeEvent` (a revenge): the other one in it struck the blow.
        const ChronicleEntry* fight = chronicle_.find(causeEvent);
        if (fight != nullptr) {
            behind = fight->who == person.id ? fight->other : fight->who;
        }
        sentence = behind >= 0 ? std::format("{} was killed by {} in a fight.", person.name, nameOf(behind))
                               : std::format("{} was killed in a fight.", person.name);
        break;
    }
    case CauseOfDeath::Wound: {
        const ChronicleEntry* hurt = chronicle_.find(causeEvent);
        if (hurt != nullptr && hurt->kind == EventKind::Revenge) {
            behind = hurt->who == person.id ? hurt->other : hurt->who;
            sentence = std::format("{} died of the wounds taken in the fight with {}.", person.name, nameOf(behind));
        } else {
            sentence = std::format("{} died of the wounds.", person.name);
        }
        break;
    }
    case CauseOfDeath::Illness: {
        // What weakened them, as the sickness event told it ("weakened by hunger").
        const ChronicleEntry* sick = chronicle_.find(causeEvent);
        const std::string reason = sick == nullptr                                           ? ""
                                   : sick->text.find("weakened by hunger") != std::string::npos ? ", weakened by hunger"
                                   : sick->text.find("weakened by the cold") != std::string::npos ? ", weakened by the cold"
                                                                                                  : "";
        sentence = std::format("{} died of the sickness{}.", person.name, reason);
        break;
    }
    default:
        sentence = std::format("{} died of {}.", person.name, causeName(cause));
        break;
    }
    if (causeEvent >= 0) {
        causes.push_back(causeEvent);
    }
    const int deathEvent = chronicle_.record(date(), kImportanceDeath, EventKind::Death, person.id, behind, -1, causes, sentence);
    // Close kin grieve: a memory kept for life. A widow or widower may pair again.
    std::vector<int> grievers;
    for (Person& kin : people_) {
        if (!kin.alive || kin.id == person.id) {
            continue;
        }
        const bool close = kin.partner == person.id || closeKin(kin, person);
        if (close) {
            remember(kin.memories, {person.id, kin.id, MemoryKind::Death, today(), config_.life.griefFeeling, true, false, deathEvent},
                     config_.social.memoryLimit);
            grievers.push_back(kin.id);
        }
        if (kin.partner == person.id) {
            kin.partner = -1;
        }
    }
    // Grief looks for someone to blame (US-111): the thief whose theft they know of, the one
    // who struck the blow.
    if (behind >= 0 && behind != person.id) {
        for (const int griever : grievers) {
            if (griever == behind) {
                continue;
            }
            if (cause == CauseOfDeath::Starvation) {
                const Person& kin = people_[static_cast<std::size_t>(griever)];
                const bool knows = std::any_of(kin.memories.begin(), kin.memories.end(),
                                               [behind](const Memory& m) { return m.kind == MemoryKind::Theft && m.subject == behind; });
                if (knows) {
                    blame(griever, behind, person.id, deathEvent, std::format("because {} had stolen from the store", nameOf(behind)));
                }
            } else if (cause == CauseOfDeath::Fight || cause == CauseOfDeath::Wound) {
                blame(griever, behind, person.id, deathEvent, std::format("because {} struck the blow", nameOf(behind)));
            }
        }
    }
    return deathEvent;
}

void World::runTicks(std::uint64_t count) {
    for (std::uint64_t i = 0; i < count; ++i) {
        tick();
    }
}

void World::startDay() {
    // Each morning the weather is rolled: the season's typical value, give or take 5 degrees.
    temperature_ = seasonalTemperature(date().season) + static_cast<int>(weather_.below(11)) - 5;
    // Once a year, on the first morning of autumn, the harvest may fail: a lean autumn is the
    // root of many hard winters (US-110). The failure lasts until the next spring.
    if (date().dayOfSeason == 1) {
        if (date().season == Season::Spring) {
            leanEvent_ = -1;
        } else if (date().season == Season::Autumn && weather_.chance(static_cast<std::uint32_t>(config_.story.season.leanAutumnPercent))) {
            leanEvent_ = chronicle_.record(date(), kImportanceLean, EventKind::Lean, -1, -1, -1, {},
                                           std::format("The autumn harvest failed: the land gave only {}% of its usual food.",
                                                       config_.story.season.leanForagePercent));
        }
    }
    const int share = (date().season == Season::Autumn && leanEvent_ >= 0) ? config_.story.season.leanForagePercent : 100;
    forageLeft_ = config_.actions.forageDaily[static_cast<std::size_t>(date().season)] * share / 100;
    gameLeft_ = config_.actions.gameDaily;
    if (ticks_ == 0) {
        return; // the first morning: everyone starts rested and fed
    }
    const bool winterNight = calendar_.dateAt(ticks_ - 1).season == Season::Winter;
    food_ -= food_ * config_.actions.spoilPercent / 100; // some of the store spoils every day
    for (Person& person : people_) {
        if (!person.alive) {
            continue;
        }
        ++person.ageDays;
        forgetOldMemories(person.memories, today(), config_.social.minorMemoryDays);
        checkSurvival(person, winterNight);
    }
    if (dailyLife_) {
        lifeEvents();
    }
}

std::uint64_t World::hash() const {
    core::Hasher hasher;
    hasher.add(seed_);
    hasher.add(ticks_);
    hasher.add(weather_.state());
    hasher.add(weather_.increment());
    hasher.add(temperature_);
    hasher.add(peopleRandom_.state());
    hasher.add(decisionRandom_.state());
    hasher.add(huntRandom_.state());
    hasher.add(socialRandom_.state());
    hasher.add(lifeRandom_.state());
    hasher.add(storyRandom_.state());
    hasher.add(leanEvent_);
    hasher.add(mammoths_);
    hasher.add(storeRanOut_);
    hasher.add(forageLeft_);
    hasher.add(lastMammothYear_);
    hasher.add(gameLeft_);
    for (const FeudRecord& feud : feuds_) {
        hasher.add(feud.a);
        hasher.add(feud.b);
        hasher.add(feud.event);
        hasher.add(feud.sinceDay);
        hasher.add(feud.lastRevengeDay);
    }
    hasher.add(hour_);
    hasher.add(food_);
    for (const Person& person : people_) {
        hasher.add(person.id);
        hasher.add(std::string_view(person.name));
        hasher.add(static_cast<int>(person.sex));
        hasher.add(person.ageDays);
        hasher.add(person.alive);
        hasher.add(static_cast<int>(person.causeOfDeath));
        hasher.add(person.exiled);
        hasher.add(static_cast<int>(person.health));
        hasher.add(person.healthDays);
        hasher.add(person.healthEvent);
        hasher.add(person.carer);
        hasher.add(person.nursing);
        hasher.add(person.guardian);
        for (const int value : person.needs.values) {
            hasher.add(value);
        }
        hasher.add(person.daysAtZeroHunger);
        hasher.add(person.daysAtZeroWarmth);
        hasher.add(static_cast<int>(person.traits));
        hasher.add(person.gatherSkill);
        hasher.add(person.huntSkill);
        hasher.add(person.gatherPractice);
        hasher.add(person.huntPractice);
        hasher.add(static_cast<int>(person.action));
        hasher.add(person.lastGiftDay);
        hasher.add(person.lastTheftDay);
        hasher.add(person.mother);
        hasher.add(person.father);
        hasher.add(person.partner);
        hasher.add(person.courting);
        hasher.add(person.courtDays);
        hasher.add(person.courtEvent);
        hasher.add(person.courtPauseDay);
        hasher.add(person.master);
        hasher.add(person.apprentice);
        hasher.add(person.teachHunt);
        hasher.add(person.teachEvent);
        hasher.add(person.pregnantDays);
        hasher.add(person.childFather);
        hasher.add(person.lastBirthDay);
        for (const Memory& memory : person.memories) {
            hasher.add(memory.subject);
            hasher.add(memory.object);
            hasher.add(static_cast<int>(memory.kind));
            hasher.add(memory.day);
            hasher.add(memory.feeling);
            hasher.add(memory.major);
            hasher.add(memory.secondHand);
            hasher.add(memory.event);
        }
        for (const MemoryNote& note : person.notes) { // none unless a conversation or the Game gave one
            for (const char c : note.text) hasher.add(static_cast<int>(c));
            hasher.add(note.day);
            hasher.add(note.feeling);
            hasher.add(note.secondHand);
            hasher.add(note.clause);
        }
        for (const int value : person.opinions) {
            hasher.add(value);
        }
        for (const Grudge& grudge : person.grudges) {
            hasher.add(grudge.about);
            hasher.add(grudge.event);
            hasher.add(grudge.weight);
        }
    }
    for (const ChronicleEntry& entry : chronicle_.entries()) {
        hasher.add(entry.date.day);
        hasher.add(entry.importance);
        hasher.add(std::string_view(entry.text));
        hasher.add(static_cast<int>(entry.kind));
        hasher.add(entry.who);
        hasher.add(entry.other);
        hasher.add(entry.aux);
        for (const int cause : entry.causes) {
            hasher.add(cause);
        }
    }
    return hasher.value();
}

} // namespace odysseus::sim
