#include "sim/world.h"

#include "core/hash.h"

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
    return config;
}

World::World(std::uint64_t seed, SimConfig config)
    : seed_(seed), config_(config), calendar_(config.calendar),
      weather_(seed, static_cast<std::uint64_t>(Stream::Weather)) {
    startDay();
}

void World::tick() {
    ++ticks_;
    if (ticks_ % static_cast<std::uint64_t>(calendar_.ticksPerDay()) == 0) {
        startDay();
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
}

std::uint64_t World::hash() const {
    core::Hasher hasher;
    hasher.add(seed_);
    hasher.add(ticks_);
    hasher.add(weather_.state());
    hasher.add(weather_.increment());
    hasher.add(temperature_);
    return hasher.value();
}

} // namespace odysseus::sim
