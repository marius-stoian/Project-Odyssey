#pragma once

#include "boundary.h"

#include "sim/report.h"
#include "sim/world.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim {

// What a Quick check says about a clan after some years (US-194): the report of the headless runner plus the episodes of the story.
struct QuickSummary {
    std::uint64_t seed = 0;
    int years = 0;
    SimReport report;
    int episodes = 0;
    std::uint64_t hash = 0; // the world hash: the same data and seed give the same one (ADR-011)
};

// Runs the clan simulation with the data in a folder, a slice of days at a time, so a screen can stay alive between slices without a thread (ADR-007, D-60 Q9).
// The headless program and the Editor's Quick check both use it; nothing in it reads a clock, so the same data and seed give the same summary.
class QuickCheck {
public:
    // Reads the data folder: a mistake in it is `error()` and the check is done at once.
    QuickCheck(const std::filesystem::path& dataFolder, std::uint64_t seed, int years);

    bool done() const { return !error_.empty() || daysDone_ >= daysTotal_; }
    const std::string& error() const { return error_; }
    int daysDone() const { return daysDone_; }
    int daysTotal() const { return daysTotal_; }
    // Runs at most `days` more days.
    void step(int days);
    // The summary so far (the whole of it once done()).
    QuickSummary summary() const;

private:
    std::optional<World> world_;
    std::uint64_t seed_;
    int years_;
    int daysDone_ = 0;
    int daysTotal_ = 0;
    std::string error_;
};

QuickSummary runQuickCheck(const std::filesystem::path& dataFolder, std::uint64_t seed, int years, std::string* error = nullptr);

// The summary as lines of "label: now (last)": population, births, deaths by cause, episodes. `last` is the run before, when there was one.
std::vector<std::string> formatQuickSummary(const QuickSummary& now, const QuickSummary* last);

} // namespace odysseus::sim
