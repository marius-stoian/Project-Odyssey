#pragma once

#include "boundary.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace odysseus::sim {

enum class Season { Spring, Summer, Autumn, Winter };

// The simulation's slow systems (needs, and later decisions) run once per game hour.
inline constexpr int kHoursPerDay = 24;

const char* seasonName(Season season);

// From assets/data/sim/calendar.json.
struct CalendarConfig {
    int ticksPerDay = 2400;  // 20 ticks per game second (ADR-006) x 120 s: a day lasts 2 minutes at 1x
    int daysPerSeason = 7;   // 4 seasons x 7 days = 28 days per year
};

CalendarConfig loadCalendarConfig(const std::filesystem::path& file);

// A moment in the world's calendar. All whole numbers: the calendar is exact.
struct Date {
    int year = 1;               // starts at year 1
    Season season = Season::Spring;
    int dayOfSeason = 1;        // 1..daysPerSeason
    std::int64_t day = 0;       // days since the world began (0 = the first day)
};

class Calendar {
public:
    explicit Calendar(CalendarConfig config);

    Date dateAt(std::uint64_t tick) const;
    int ticksPerDay() const { return config_.ticksPerDay; }
    int ticksPerHour() const { return config_.ticksPerDay / kHoursPerDay; }
    int daysPerYear() const { return config_.daysPerSeason * 4; }
    std::uint64_t ticksPerYear() const {
        return static_cast<std::uint64_t>(daysPerYear()) * static_cast<std::uint64_t>(config_.ticksPerDay);
    }

private:
    CalendarConfig config_;
};

// "Spring, year 3"
std::string describe(const Date& date);

} // namespace odysseus::sim
