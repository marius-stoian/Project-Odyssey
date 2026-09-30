#include "sim/report.h"

#include "sim/world.h"

#include <format>

namespace odysseus::sim {

SimReport makeReport(const World& world) {
    SimReport report;
    std::array<long long, kNeedCount> needTotals{};
    for (const Person& person : world.people()) {
        if (person.mother >= 0 || person.father >= 0) {
            ++report.born;
        } else {
            ++report.founders;
        }
        if (!person.alive) {
            ++report.died;
            ++report.deathsByCause[static_cast<std::size_t>(person.causeOfDeath)];
            continue;
        }
        ++report.alive;
        report.couples += person.partner > person.id ? 1 : 0; // count each couple once
        for (std::size_t i = 0; i < kNeedCount; ++i) {
            needTotals[i] += person.needs.values[i];
        }
    }
    for (std::size_t i = 0; i < kNeedCount; ++i) {
        report.averageNeeds[i] = report.alive > 0 ? static_cast<int>(needTotals[i] / report.alive) : 0;
    }
    report.food = world.food();
    report.feuds = static_cast<int>(world.feuds().size());
    report.mammoths = world.mammothsKilled();
    report.chronicleEntries = static_cast<int>(world.chronicle().entries().size());
    return report;
}

std::vector<std::string> formatReport(const SimReport& report) {
    std::vector<std::string> lines;
    lines.push_back(std::format("Population: {} alive ({} founders, {} born, {} died)", report.alive, report.founders, report.born,
                                report.died));
    std::string causes = "Deaths by cause:";
    for (std::size_t i = 1; i < kCauseCount; ++i) { // 0 is "nothing": the living
        causes += std::format("{} {} {}", i == 1 ? "" : ",", causeName(static_cast<CauseOfDeath>(i)), report.deathsByCause[i]);
    }
    lines.push_back(causes);
    lines.push_back(std::format("Average needs of the living: Hunger {}, Energy {}, Warmth {}, Social {}", report.averageNeeds[0],
                                report.averageNeeds[1], report.averageNeeds[2], report.averageNeeds[3]));
    lines.push_back(std::format("Food in store: {} meals; couples {}, feuds {}, mammoths {}, chronicle entries {}", report.food,
                                report.couples, report.feuds, report.mammoths, report.chronicleEntries));
    return lines;
}

} // namespace odysseus::sim
