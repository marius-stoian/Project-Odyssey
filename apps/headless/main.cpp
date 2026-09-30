// odysseus_headless.exe: runs the simulation without graphics, as fast as the computer can.
// It becomes the console clan simulator in M2 (US-015 adds the full report).
//   --seed <number>   world seed (default 42)
//   --days <number>   how many in-game days to run (default one year)
//   --data <folder>   content folder (default: the repository's assets/data)
//   --inspect <name or id>  print that person's last decision: every action's score (US-012)
//   --chronicle [year]       print the chronicle (one year, or all years) (US-014)
//   --threshold <0..100>     the importance an event needs to be printed (default 50)
#include "core/version.h"
#include "sim/ai.h"
#include "sim/world.h"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

int main(int argc, char* argv[]) {
    std::uint64_t seed = 42;
    long long days = -1;
    std::string dataDirectory = ODYSSEUS_DATA_DIR;
    std::string inspect;
    bool chronicle = false;
    int chronicleYear = 0;
    int threshold = odysseus::sim::kDefaultChronicleThreshold;
    for (int i = 1; i < argc; ++i) {
        const std::string_view flag = argv[i];
        if (flag == "--chronicle") {
            chronicle = true;
            // An optional year may follow.
            if (i + 1 < argc && std::string_view(argv[i + 1]).find_first_not_of("0123456789") == std::string_view::npos) {
                chronicleYear = std::stoi(argv[++i]);
            }
        }
    }
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string_view name = argv[i];
        if (name == "--seed") {
            seed = std::stoull(argv[++i]);
        } else if (name == "--days") {
            days = std::stoll(argv[++i]);
        } else if (name == "--data") {
            dataDirectory = argv[++i];
        } else if (name == "--inspect") {
            inspect = argv[++i];
        } else if (name == "--threshold") {
            threshold = std::stoi(argv[++i]);
        }
    }

    try {
        odysseus::sim::World world(seed, odysseus::sim::loadSimConfig(dataDirectory));
        if (days < 0) {
            days = world.calendar().daysPerYear();
        }
        world.runTicks(static_cast<std::uint64_t>(days) * static_cast<std::uint64_t>(world.calendar().ticksPerDay()));
        std::cout << "Project Odyssey headless runner " << odysseus::core::versionString() << '\n'
                  << "Seed " << seed << ", " << days << " days: now " << odysseus::sim::describe(world.date())
                  << ", day " << world.date().dayOfSeason << ", " << world.temperature() << " C\n"
                  << "Population " << world.population() << ", food in store " << world.food() << " meals\n"
                  << "World hash: " << world.hash() << '\n';
        if (chronicle) {
            std::cout << "\nChronicle" << (chronicleYear > 0 ? " of year " + std::to_string(chronicleYear) : std::string())
                      << " (importance " << threshold << " and above):\n";
            for (const auto& entry : world.chronicle().select(chronicleYear, threshold)) {
                std::cout << "  " << odysseus::sim::formatEntry(entry) << '\n';
            }
        }
        if (!inspect.empty()) {
            const odysseus::sim::Person* person = world.findPerson(inspect);
            if (person == nullptr) {
                std::cerr << "No person named " << inspect << '\n';
                return 1;
            }
            std::cout << "Last decision: " << odysseus::sim::describeDecision(*person, world.calendar().daysPerYear()) << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
