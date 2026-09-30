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
        practise(person.huntPractice, person.huntSkill);
        if (lastMammothYear_ != date().year && huntRandom_.below(1000) < static_cast<std::uint32_t>(actions.mammothPerMille)) {
            lastMammothYear_ = date().year; // met or not, the herd moves on
            // A mammoth: a feast for the clan, if the hunter survives it.
            if (huntRandom_.chance(static_cast<std::uint32_t>(actions.mammothDeathPercent))) {
                die(person, CauseOfDeath::Hunting);
                return;
            }
            bringDownMammoth(person.id);
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
        if (taken > 0 && socialRandom_.chance(static_cast<std::uint32_t>(config_.social.witnessPercent))) {
            std::vector<int> awake;
            for (const Person& other : people_) {
                if (other.alive && other.id != person.id && other.action != Action::Sleep) {
                    awake.push_back(other.id);
                }
            }
            if (!awake.empty()) {
                recordTheft(person.id, awake[socialRandom_.below(static_cast<std::uint32_t>(awake.size()))]);
            }
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
        chronicle_.add(date(), kImportanceStoreEmpty,
                       std::format("The food store ran empty; {} went to bed hungry.", hungry == 1 ? std::string("one") : std::to_string(hungry)));
    }
    storeRanOut_ = hungry > 0;
}

bool World::closeKin(const Person& a, const Person& b) const {
    const bool parentChild = a.mother == b.id || a.father == b.id || b.mother == a.id || b.father == a.id;
    const bool siblings = (a.mother >= 0 && a.mother == b.mother) || (a.father >= 0 && a.father == b.father);
    return parentChild || siblings;
}

void World::pair(int a, int b) {
    Person& first = people_[static_cast<std::size_t>(a)];
    Person& second = people_[static_cast<std::size_t>(b)];
    first.partner = b;
    second.partner = a;
    chronicle_.add(date(), kImportancePairing, std::format("{} and {} became partners.", first.name, second.name));
}

void World::bringDownMammoth(int hunter) {
    food_ += config_.actions.mammothYield;
    ++mammoths_;
    const std::string& name = people_[static_cast<std::size_t>(hunter)].name;
    if (mammoths_ == 1) {
        chronicle_.add(date(), kImportanceFirstMammoth, std::format("{} brought down the clan's first mammoth.", name));
    } else {
        chronicle_.add(date(), kImportanceMammoth, std::format("{} brought down a mammoth.", name));
    }
}

void World::pairUp() {
    const LifeConfig& life = config_.life;
    const int daysPerYear = calendar_.daysPerYear();
    for (Person& woman : people_) {
        if (!woman.alive || woman.sex != Sex::Female || woman.partner >= 0 || woman.ageYears(daysPerYear) < life.adultAgeYears) {
            continue;
        }
        // The man she and who both like best, if they like each other enough.
        Person* best = nullptr;
        int bestBond = 0;
        for (Person& man : people_) {
            if (!man.alive || man.sex != Sex::Male || man.partner >= 0 || man.ageYears(daysPerYear) < life.adultAgeYears ||
                closeKin(woman, man)) {
                continue;
            }
            const int hers = woman.opinions[static_cast<std::size_t>(man.id)];
            const int his = man.opinions[static_cast<std::size_t>(woman.id)];
            if (hers >= life.pairOpinion && his >= life.pairOpinion && hers + his > bestBond) {
                best = &man;
                bestBond = hers + his;
            }
        }
        if (best != nullptr && lifeRandom_.chance(static_cast<std::uint32_t>(life.pairPercent))) {
            pair(woman.id, best->id);
        }
    }
}

void World::updateFeuds() {
    const int bitter = config_.life.feudOpinion;
    // Old feuds end when one side dies or both have cooled down.
    std::vector<std::pair<int, int>> still;
    for (const auto& [a, b] : feuds_) {
        const Person& first = people_[static_cast<std::size_t>(a)];
        const Person& second = people_[static_cast<std::size_t>(b)];
        if (!first.alive || !second.alive) {
            continue;
        }
        if (opinion(a, b) > 0 && opinion(b, a) > 0) {
            chronicle_.add(date(), kImportancePeace, std::format("{} and {} made peace.", first.name, second.name));
            continue;
        }
        still.push_back({a, b});
    }
    feuds_ = still;
    // New feuds: both think badly of each other.
    for (const Person& first : people_) {
        if (!first.alive) {
            continue;
        }
        for (const Person& second : people_) {
            if (second.id <= first.id || !second.alive) {
                continue;
            }
            if (opinion(first.id, second.id) <= bitter && opinion(second.id, first.id) <= bitter &&
                std::find(feuds_.begin(), feuds_.end(), std::pair<int, int>{first.id, second.id}) == feuds_.end()) {
                feuds_.push_back({first.id, second.id});
                chronicle_.add(date(), kImportanceFeud, std::format("A feud broke out between {} and {}.", first.name, second.name));
            }
        }
    }
    std::sort(feuds_.begin(), feuds_.end());
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
    chronicle_.add(date(), kImportanceBirth, std::format("{} was born to {} and {}.", child.name, father.name, mother.name));
    mother.pregnantDays = 0;
    mother.lastBirthDay = today();
    newborns.push_back(child);
    if (lifeRandom_.chance(static_cast<std::uint32_t>(life.childbirthDeathPercent))) {
        die(mother, CauseOfDeath::Childbirth, child.id);
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
    pairUp();
    updateFeuds();
}

int World::opinion(int who, int about) const {
    const Person& person = people_[static_cast<std::size_t>(who)];
    return person.opinions[static_cast<std::size_t>(about)];
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

void World::recordTheft(int thief, int witness) {
    Person& seer = people_[static_cast<std::size_t>(witness)];
    chronicle_.add(date(), kImportanceTheft,
                   std::format("{} saw {} stealing from the food store.", seer.name, people_[static_cast<std::size_t>(thief)].name));
    // The clan's store was robbed: the object of a theft is the thief's own clan (-1).
    remember(seer.memories, {thief, -1, MemoryKind::Theft, today(), config_.social.theftFeeling, true, false},
             config_.social.memoryLimit);
    changeOpinion(seer, thief, config_.social.theftOpinion);
}

bool World::talk(int speaker, int listener) {
    Person& from = people_[static_cast<std::size_t>(speaker)];
    Person& to = people_[static_cast<std::size_t>(listener)];
    satisfy(to.needs, Need::Social, config_.actions.talkPerHour / 2, config_.needs.maximum);
    changeOpinion(from, listener, config_.social.talkOpinion);
    changeOpinion(to, speaker, config_.social.talkOpinion);
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

void World::die(Person& person, CauseOfDeath cause, int other) {
    person.alive = false;
    person.causeOfDeath = cause;
    const int years = person.ageYears(calendar_.daysPerYear());
    std::string sentence;
    switch (cause) {
    case CauseOfDeath::Hunting:
        sentence = std::format("{} was killed hunting a mammoth.", person.name);
        break;
    case CauseOfDeath::OldAge:
        sentence = std::format("{} died of old age, {} years old.", person.name, years);
        break;
    case CauseOfDeath::Childbirth:
        sentence = std::format("{} died giving birth.", person.name);
        break;
    default:
        sentence = std::format("{} died of {}.", person.name, causeName(cause));
        break;
    }
    chronicle_.add(date(), kImportanceDeath, sentence);
    (void)other;
    // Close kin grieve: a memory kept for life. A widow or widower may pair again.
    for (Person& kin : people_) {
        if (!kin.alive || kin.id == person.id) {
            continue;
        }
        const bool close = kin.partner == person.id || closeKin(kin, person);
        if (close) {
            remember(kin.memories, {person.id, kin.id, MemoryKind::Death, today(), config_.life.griefFeeling, true, false},
                     config_.social.memoryLimit);
        }
        if (kin.partner == person.id) {
            kin.partner = -1;
        }
    }
}

void World::runTicks(std::uint64_t count) {
    for (std::uint64_t i = 0; i < count; ++i) {
        tick();
    }
}

void World::startDay() {
    // Each morning the weather is rolled: the season's typical value, give or take 5 degrees.
    temperature_ = seasonalTemperature(date().season) + static_cast<int>(weather_.below(11)) - 5;
    forageLeft_ = config_.actions.forageDaily[static_cast<std::size_t>(date().season)];
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
    hasher.add(mammoths_);
    hasher.add(storeRanOut_);
    hasher.add(forageLeft_);
    hasher.add(lastMammothYear_);
    hasher.add(gameLeft_);
    for (const auto& [a, b] : feuds_) {
        hasher.add(a);
        hasher.add(b);
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
        }
        for (const int value : person.opinions) {
            hasher.add(value);
        }
    }
    for (const ChronicleEntry& entry : chronicle_.entries()) {
        hasher.add(entry.date.day);
        hasher.add(entry.importance);
        hasher.add(std::string_view(entry.text));
    }
    return hasher.value();
}

} // namespace odysseus::sim
