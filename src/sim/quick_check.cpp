#include "sim/quick_check.h"

#include "sim/episodes.h"

#include <algorithm>
#include <format>

namespace odysseus::sim {

QuickCheck::QuickCheck(const std::filesystem::path& dataFolder, std::uint64_t seed, int years) : seed_(seed), years_(years) {
    try {
        world_.emplace(seed, loadSimConfig(dataFolder));
        daysTotal_ = years * world_->calendar().daysPerYear();
    } catch (const std::exception& problem) {
        error_ = problem.what();
    }
}

void QuickCheck::step(int days) {
    if (done()) return;
    const int run = std::min(days, daysTotal_ - daysDone_);
    world_->runTicks(static_cast<std::uint64_t>(run) * static_cast<std::uint64_t>(world_->calendar().ticksPerDay()));
    daysDone_ += run;
}

QuickSummary QuickCheck::summary() const {
    QuickSummary summary;
    summary.seed = seed_;
    summary.years = years_;
    if (!world_) return summary;
    summary.report = makeReport(*world_);
    summary.episodes = static_cast<int>(findEpisodes(*world_).size());
    summary.hash = world_->hash();
    return summary;
}

QuickSummary runQuickCheck(const std::filesystem::path& dataFolder, std::uint64_t seed, int years, std::string* error) {
    QuickCheck check(dataFolder, seed, years);
    while (!check.done()) check.step(1000);
    if (error != nullptr) *error = check.error();
    return check.summary();
}

std::vector<std::string> formatQuickSummary(const QuickSummary& now, const QuickSummary* last) {
    std::vector<std::string> lines;
    const auto row = [&](const std::string& label, int value, int before) {
        lines.push_back(last == nullptr ? std::format("{}: {}", label, value) : std::format("{}: {}   (last run {}, {:+})", label, value, before, value - before));
    };
    const SimReport none;
    const SimReport& before = last != nullptr ? last->report : none;
    lines.push_back(std::format("Seed {}, {} years", now.seed, now.years));
    row("Population at the end", now.report.alive, before.alive);
    row("Born", now.report.born, before.born);
    row("Died", now.report.died, before.died);
    for (std::size_t i = 1; i < kCauseCount; ++i) {
        row(std::format("  of {}", causeName(static_cast<CauseOfDeath>(i))), now.report.deathsByCause[i], before.deathsByCause[i]);
    }
    row("Feuds", now.report.feuds, before.feuds);
    row("Episodes of the story", now.episodes, last != nullptr ? last->episodes : 0);
    lines.push_back(std::format("World hash {}", now.hash));
    return lines;
}

} // namespace odysseus::sim
