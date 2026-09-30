#include "sim/report.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <numeric>
#include <string>

namespace sim = odysseus::sim;

TEST_CASE("US-015 The report adds up") {
    sim::World world(7, sim::loadSimConfig(ODYSSEUS_DATA_DIR));
    world.runTicks(world.calendar().ticksPerYear() * 10);
    const sim::SimReport report = sim::makeReport(world);
    CHECK(report.alive == world.population());
    CHECK(report.founders == world.config().clan.startingPeople);
    CHECK(report.founders + report.born == static_cast<int>(world.people().size()));
    CHECK(report.alive + report.died == static_cast<int>(world.people().size()));
    CHECK(std::accumulate(report.deathsByCause.begin(), report.deathsByCause.end(), 0) == report.died);
    CHECK(report.deathsByCause[0] == 0); // nobody dies of "nothing"
    for (const int average : report.averageNeeds) {
        CHECK(average >= 0);
        CHECK(average <= 100);
    }
    const auto lines = sim::formatReport(report);
    REQUIRE(lines.size() == 4);
    CHECK(lines[0].rfind("Population: ", 0) == 0);
    CHECK(lines[1].rfind("Deaths by cause: starvation ", 0) == 0);
    CHECK(lines[2].rfind("Average needs of the living: Hunger ", 0) == 0);
    for (const std::string& line : lines) {
        MESSAGE(line);
    }
}
