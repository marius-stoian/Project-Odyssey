#include "sim/world.h"

#include "core/hash.h"

#include <algorithm>
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
    return config;
}

World::World(std::uint64_t seed, SimConfig config)
    : seed_(seed), config_(config), calendar_(config.calendar),
      weather_(seed, static_cast<std::uint64_t>(Stream::Weather)),
      peopleRandom_(seed, static_cast<std::uint64_t>(Stream::People)),
      decisionRandom_(seed, static_cast<std::uint64_t>(Stream::Decisions)),
      huntRandom_(seed, static_cast<std::uint64_t>(Stream::Hunting)),
      people_(makeStartingClan(config_.clan, config_.names, calendar_.daysPerYear(), peopleRandom_)),
      food_(config_.clan.startingFood) {
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
        const int yield = actions.gatherYield[static_cast<std::size_t>(calendar_.dateAt(ticks_ - 1).season)];
        food_ += yield + (yield > 0 && person.gatherSkill >= 50 ? 1 : 0);
        satisfy(person.needs, Need::Hunger, actions.gatherSnack, maximum);
        practise(person.gatherPractice, person.gatherSkill);
        break;
    }
    case Action::Hunt: {
        practise(person.huntPractice, person.huntSkill);
        if (huntRandom_.below(1000) < static_cast<std::uint32_t>(actions.mammothPerMille)) {
            // A mammoth: a feast for the clan, if the hunter survives it.
            if (huntRandom_.chance(static_cast<std::uint32_t>(actions.mammothDeathPercent))) {
                die(person, CauseOfDeath::Hunting);
                return;
            }
            food_ += actions.mammothYield;
        } else if (huntRandom_.chance(static_cast<std::uint32_t>(std::min(100, actions.huntSuccessPercent + person.huntSkill / 5)))) {
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
        break;
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
    for (Person* person : eaters) {
        if (food_ <= 0) {
            break;
        }
        --food_;
        satisfy(person->needs, Need::Hunger, config_.needs.mealValue, config_.needs.maximum);
    }
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
    // A need that stays at 0 for the configured number of whole days kills (D-02): the
    // counter grows at each morning that finds the need empty, and death comes when the
    // need has been empty for that many days.
    person.daysAtZeroHunger = person.needs[Need::Hunger] == 0 ? person.daysAtZeroHunger + 1 : 0;
    person.daysAtZeroWarmth = (winter && person.needs[Need::Warmth] == 0) ? person.daysAtZeroWarmth + 1 : 0;
    if (person.daysAtZeroHunger > config_.needs.hungerDaysBeforeDeath) {
        die(person, CauseOfDeath::Starvation);
    } else if (person.daysAtZeroWarmth > config_.needs.warmthDaysBeforeDeath) {
        die(person, CauseOfDeath::Cold);
    }
}

void World::die(Person& person, CauseOfDeath cause) {
    person.alive = false;
    person.causeOfDeath = cause;
    chronicle_.add(date(), kImportanceDeath, std::format("{} died of {}.", person.name, causeName(cause)));
}

void World::runTicks(std::uint64_t count) {
    for (std::uint64_t i = 0; i < count; ++i) {
        tick();
    }
}

void World::startDay() {
    // Each morning the weather is rolled: the season's typical value, give or take 5 degrees.
    temperature_ = seasonalTemperature(date().season) + static_cast<int>(weather_.below(11)) - 5;
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
        checkSurvival(person, winterNight);
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
    }
    for (const ChronicleEntry& entry : chronicle_.entries()) {
        hasher.add(entry.date.day);
        hasher.add(entry.importance);
        hasher.add(std::string_view(entry.text));
    }
    return hasher.value();
}

} // namespace odysseus::sim
