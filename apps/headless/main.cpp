// odysseus_headless.exe: the console clan simulator (M2). Runs the simulation without
// graphics, as fast as the computer can, and reports how the clan fared (US-015).
//   --seed <number>          world seed (default 42)
//   --years <number>         how many in-game years to run (default 1)
//   --days <number>          or how many in-game days
//   --data <folder>          content folder (default: the repository's assets/data)
//   --inspect <name or id>   print that person's last decision: every action's score (US-012)
//   --story                  print the clan's story: the episodes, then births, deaths, pairings and feuds (US-115)
//   --chronicle [year]       print the chronicle (one year, or all years) (US-014)
//   --threshold <0..100>     the importance an event needs to be printed (default 50)
//   --why <event id>         print that event and the earlier events that caused it (US-110)
//   --load <file>            continue a saved world instead of founding a new one (US-016)
//   --save <file>            save the world at the end (safely, with 3 backups)
//   --help                   print this list
#include "core/version.h"
#include "sim/chronicle.h"
#include "sim/episodes.h"
#include "sim/ai.h"
#include "sim/report.h"
#include "sim/save.h"
#include "sim/world.h"

#include <charconv>
#include <chrono>
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

constexpr int kBadUsage = 2; // exit code for a wrong command line

struct Options {
    std::uint64_t seed = 42;
    long long years = -1;
    long long days = -1;
    std::string dataDirectory = ODYSSEUS_DATA_DIR;
    std::string inspect;
    bool chronicle = false;
    bool story = false;
    int chronicleYear = 0;
    int threshold = odysseus::sim::kDefaultChronicleThreshold;
    int why = -1;
    bool help = false;
    std::string loadFile;
    std::string saveFile;
};

void printUsage(std::ostream& out) {
    out << "Usage: odysseus_headless [--seed N] [--years N | --days N] [--data FOLDER]\n"
           "                         [--inspect NAME] [--story] [--chronicle [YEAR]] [--threshold 0..100]\n"
           "                         [--why EVENT] [--load FILE] [--save FILE]\n"
           "  --years and --days take a whole number from 1 to 10000; --seed a whole number 0 or more.\n"
           "Example: odysseus_headless --seed 7 --years 100\n";
}

// Reads a whole number in [minimum, maximum]; anything else (letters, "-5", "3.5") is refused.
// std::from_chars does not accept a leading '+' or spaces and reports where parsing stopped,
// so "12abc" is caught too.
std::optional<long long> readNumber(std::string_view text, long long minimum, long long maximum) {
    long long value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size() || value < minimum || value > maximum) {
        return std::nullopt;
    }
    return value;
}

// Fills `options` from the command line; returns an error message, or nothing when all is well.
std::optional<std::string> parse(int argc, char* argv[], Options& options) {
    for (int i = 1; i < argc; ++i) {
        const std::string_view flag = argv[i];
        const bool hasValue = i + 1 < argc;
        auto value = [&]() { return std::string_view(argv[++i]); };
        if (flag == "--help") {
            options.help = true;
        } else if (flag == "--story") {
            options.story = true;
        } else if (flag == "--chronicle") {
            options.chronicle = true;
            if (hasValue && readNumber(argv[i + 1], 1, 1'000'000)) {
                options.chronicleYear = static_cast<int>(*readNumber(value(), 1, 1'000'000)); // the optional year
            }
        } else if (!hasValue) {
            return std::format("{} needs a value", flag);
        } else if (flag == "--seed") {
            const auto number = readNumber(value(), 0, INT64_MAX);
            if (!number) return std::format("--seed must be a whole number 0 or more (got {})", argv[i]);
            options.seed = static_cast<std::uint64_t>(*number);
        } else if (flag == "--years" || flag == "--days") {
            const auto number = readNumber(value(), 1, 10'000);
            if (!number) return std::format("{} must be a whole number from 1 to 10000 (got {})", flag, argv[i]);
            (flag == "--years" ? options.years : options.days) = *number;
        } else if (flag == "--data") {
            options.dataDirectory = value();
        } else if (flag == "--inspect") {
            options.inspect = value();
        } else if (flag == "--load") {
            options.loadFile = value();
        } else if (flag == "--save") {
            options.saveFile = value();
        } else if (flag == "--why") {
            const auto number = readNumber(value(), 0, 100'000'000);
            if (!number) return std::format("--why must be an event number (got {})", argv[i]);
            options.why = static_cast<int>(*number);
        } else if (flag == "--threshold") {
            const auto number = readNumber(value(), 0, 100);
            if (!number) return std::format("--threshold must be a whole number from 0 to 100 (got {})", argv[i]);
            options.threshold = static_cast<int>(*number);
        } else {
            return std::format("unknown option {}", flag);
        }
    }
    if (options.years > 0 && options.days > 0) {
        return std::string("give --years or --days, not both");
    }
    return std::nullopt;
}

} // namespace

int main(int argc, char* argv[]) {
    Options options;
    if (const auto problem = parse(argc, argv, options)) {
        std::cerr << "Error: " << *problem << "\n\n";
        printUsage(std::cerr);
        return kBadUsage;
    }
    if (options.help) {
        printUsage(std::cout);
        return 0;
    }

    try {
        const odysseus::sim::SimConfig config = odysseus::sim::loadSimConfig(options.dataDirectory);
        odysseus::sim::World world(options.seed, config);
        if (!options.loadFile.empty()) {
            odysseus::sim::LoadedWorld loaded = odysseus::sim::loadWorld(options.loadFile, config);
            for (const std::string& note : loaded.notes) {
                std::cout << "Load: " << note << '\n';
            }
            std::cout << "Loaded " << loaded.loadedFrom.string() << ": " << odysseus::sim::describe(loaded.world.date())
                      << ", world hash " << loaded.world.hash() << '\n';
            world = std::move(loaded.world);
            options.seed = world.seed();
        }
        const long long days = options.days > 0 ? options.days : (options.years > 0 ? options.years : 1) * world.calendar().daysPerYear();
        const auto ticks = static_cast<std::uint64_t>(days) * static_cast<std::uint64_t>(world.calendar().ticksPerDay());

        // The only clock in the program: it measures the run, it never feeds the simulation.
        const auto start = std::chrono::steady_clock::now();
        world.runTicks(ticks);
        const std::chrono::duration<double> seconds = std::chrono::steady_clock::now() - start;

        std::cout << "Project Odyssey headless runner " << odysseus::core::versionString() << '\n'
                  << std::format("Seed {}, {} days ({} years, {} ticks): now {}, day {}, {} C\n", options.seed, days,
                                 days / world.calendar().daysPerYear(), ticks, odysseus::sim::describe(world.date()),
                                 world.date().dayOfSeason, world.temperature());
        for (const std::string& line : odysseus::sim::formatReport(odysseus::sim::makeReport(world))) {
            std::cout << line << '\n';
        }
        std::cout << std::format("Tick time: {:.3f} microseconds per tick ({:.2f} s in total)\n",
                                 ticks > 0 ? seconds.count() * 1e6 / static_cast<double>(ticks) : 0.0, seconds.count())
                  << "World hash: " << world.hash() << '\n';

        if (options.story) {
            std::cout << '\n';
            for (const std::string& line : odysseus::sim::formatStory(world, options.threshold)) {
                std::cout << line << '\n';
            }
        }
        if (options.chronicle) {
            std::cout << "\nChronicle" << (options.chronicleYear > 0 ? " of year " + std::to_string(options.chronicleYear) : std::string())
                      << " (importance " << options.threshold << " and above):\n";
            for (const auto& entry : world.chronicle().select(options.chronicleYear, options.threshold)) {
                std::cout << std::format("  [#{}] ", entry.id) << odysseus::sim::formatEntry(entry) << '\n';
            }
        }
        if (options.why >= 0) {
            const auto lines = odysseus::sim::explainEvent(world.chronicle(), options.why);
            if (lines.empty()) {
                std::cerr << "No event #" << options.why << " (the chronicle has " << world.chronicle().entries().size() << " events)\n";
                return 1;
            }
            std::cout << "\nWhy:\n";
            for (const std::string& line : lines) {
                std::cout << "  " << line << '\n';
            }
        }
        if (!options.saveFile.empty()) {
            odysseus::sim::saveWorld(world, options.saveFile);
            std::cout << "Saved " << options.saveFile << " (world hash " << world.hash() << ")\n";
        }
        if (!options.inspect.empty()) {
            const odysseus::sim::Person* person = world.findPerson(options.inspect);
            if (person == nullptr) {
                std::cerr << "No person named " << options.inspect << '\n';
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
