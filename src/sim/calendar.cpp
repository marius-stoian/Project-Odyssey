#include "sim/calendar.h"

#include "sim/json_data.h"

#include <format>

namespace odysseus::sim {

const char* seasonName(Season season) {
    switch (season) {
    case Season::Spring: return "Spring";
    case Season::Summer: return "Summer";
    case Season::Autumn: return "Autumn";
    case Season::Winter: return "Winter";
    }
    return "?";
}

CalendarConfig loadCalendarConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    CalendarConfig config;
    config.ticksPerDay = requireInt(json, file, "ticksPerDay", 24, 1'728'000);
    if (config.ticksPerDay % kHoursPerDay != 0) {
        // Needs change once per game hour, so a day must split into whole hours.
        throw DataError(file, "ticksPerDay", std::format("must be a multiple of {} (hours per day)", kHoursPerDay));
    }
    config.daysPerSeason = requireInt(json, file, "daysPerSeason", 1, 90);
    return config;
}

Calendar::Calendar(CalendarConfig config) : config_(config) {}

Date Calendar::dateAt(std::uint64_t tick) const {
    Date date;
    date.day = static_cast<std::int64_t>(tick / static_cast<std::uint64_t>(config_.ticksPerDay));
    const std::int64_t dayOfYear = date.day % daysPerYear();
    date.year = static_cast<int>(date.day / daysPerYear()) + 1;
    date.season = static_cast<Season>(dayOfYear / config_.daysPerSeason);
    date.dayOfSeason = static_cast<int>(dayOfYear % config_.daysPerSeason) + 1;
    return date;
}

std::string describe(const Date& date) {
    return std::format("{}, year {}", seasonName(date.season), date.year);
}

} // namespace odysseus::sim
