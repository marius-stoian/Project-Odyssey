#include "core/text.h"
#include "sim/quick_check.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>

namespace sim = odysseus::sim;
namespace fs = std::filesystem;

namespace {

// A copy of the data folder the test may change.
struct Folder {
    fs::path path = fs::temp_directory_path() / ("odysseus-us194-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Folder() { fs::copy(ODYSSEUS_DATA_DIR, path, fs::copy_options::recursive); }
    ~Folder() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};

} // namespace

TEST_CASE("US-194 Quick check: the same data and seed give the same summary") {
    const sim::QuickSummary first = sim::runQuickCheck(ODYSSEUS_DATA_DIR, 42, 3);
    const sim::QuickSummary second = sim::runQuickCheck(ODYSSEUS_DATA_DIR, 42, 3);
    CHECK(first.hash == second.hash);
    CHECK(first.report.alive == second.report.alive);
    CHECK(first.report.died == second.report.died);
    CHECK(first.episodes == second.episodes);
    CHECK(sim::formatQuickSummary(first, nullptr) == sim::formatQuickSummary(second, nullptr));
    CHECK(first.report.alive > 0);
    // Another seed is another history.
    CHECK(sim::runQuickCheck(ODYSSEUS_DATA_DIR, 43, 3).hash != first.hash);
}

TEST_CASE("US-194 Quick check: slices of days give the same summary as one run") {
    sim::QuickCheck sliced(ODYSSEUS_DATA_DIR, 7, 3);
    int steps = 0;
    while (!sliced.done()) {
        sliced.step(30);
        ++steps;
    }
    CHECK(steps > 2); // more than one slice
    CHECK(sliced.daysDone() == sliced.daysTotal());
    CHECK(sliced.summary().hash == sim::runQuickCheck(ODYSSEUS_DATA_DIR, 7, 3).hash);
}

TEST_CASE("US-194 Quick check: a changed hunger rate shows next to the last run") {
    Folder folder;
    const sim::QuickSummary before = sim::runQuickCheck(folder.path, 42, 4);
    // Hunger falls three times faster each day.
    const fs::path file = folder.path / "sim" / "needs.json";
    std::string text = *odysseus::core::readTextFile(file);
    const std::size_t at = text.find("\"hunger\": 30");
    REQUIRE(at != std::string::npos);
    text.replace(at, 12, "\"hunger\": 95");
    CHECK_FALSE(odysseus::core::writeTextFileSafely(file, text).has_value());
    const sim::QuickSummary after = sim::runQuickCheck(folder.path, 42, 4);
    CHECK(after.hash != before.hash);
    CHECK(after.report.averageNeeds[0] < before.report.averageNeeds[0]); // the clan is hungrier
    const std::vector<std::string> lines = sim::formatQuickSummary(after, &before);
    CHECK(std::any_of(lines.begin(), lines.end(), [](const std::string& line) { return line.starts_with("Population at the end: ") && line.find("last run") != std::string::npos; }));
    CHECK(std::any_of(lines.begin(), lines.end(), [](const std::string& line) { return line.find("of starvation") != std::string::npos; }));
}

TEST_CASE("US-194 Quick check: a mistake in the data is reported") {
    Folder folder;
    REQUIRE_FALSE(odysseus::core::writeTextFileSafely(folder.path / "sim" / "needs.json", "{ not json").has_value());
    std::string error;
    sim::runQuickCheck(folder.path, 42, 1, &error);
    CHECK_FALSE(error.empty());
}
