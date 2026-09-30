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
    config.clan = loadClanConfig(dataDirectory / "sim" / "clan.json");
    config.names = loadNameList(dataDirectory / "sim" / "names.json");
    return config;
}

World::World(std::uint64_t seed, SimConfig config)
    : seed_(seed), config_(config), calendar_(config.calendar),
      weather_(seed, static_cast<std::uint64_t>(Stream::Weather)),
      peopleRandom_(seed, static_cast<std::uint64_t>(Stream::People)),
      people_(makeStartingClan(config_.clan, config_.names, calendar_.daysPerYear(), peopleRandom_)),
      food_(config_.clan.startingFood) {
    startDay();
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
        }
    }
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
    }
    for (const ChronicleEntry& entry : chronicle_.entries()) {
        hasher.add(entry.date.day);
        hasher.add(entry.importance);
        hasher.add(std::string_view(entry.text));
    }
    return hasher.value();
}

} // namespace odysseus::sim
