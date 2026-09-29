// odysseus_headless.exe: runs the simulation without graphics, as fast as the computer can.
// It becomes the console clan simulator in M2 (US-015 adds the full report).
//   --seed <number>   world seed (default 42)
//   --days <number>   how many in-game days to run (default one year)
//   --data <folder>   content folder (default: the repository's assets/data)
#include "core/version.h"
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
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string_view name = argv[i];
        if (name == "--seed") {
            seed = std::stoull(argv[++i]);
        } else if (name == "--days") {
            days = std::stoll(argv[++i]);
        } else if (name == "--data") {
            dataDirectory = argv[++i];
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
                  << "World hash: " << world.hash() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
