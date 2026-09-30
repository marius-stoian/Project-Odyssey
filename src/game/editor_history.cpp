#include "game/editor_history.h"

#include <algorithm>
#include <cstdlib>
#include <format>

namespace odysseus::game {

void PaintCommand::apply(Level& level) const {
    for (const CellChange& change : changes_) {
        level.set(change.x, change.y, change.after);
    }
}

void PaintCommand::undo(Level& level) const {
    // Backwards: a cell painted twice in one stroke ends as it began.
    for (auto it = changes_.rbegin(); it != changes_.rend(); ++it) {
        level.set(it->x, it->y, it->before);
    }
}

std::string PaintCommand::name() const {
    return std::format("{} ({} cells)", what_, changes_.size());
}

void CharactersCommand::apply(Level& level) const {
    level.characters = after_;
    level.nextId = std::max(level.nextId, nextIdAfter_); // an id once given is never given again
}

void CharactersCommand::undo(Level& level) const {
    level.characters = before_;
    level.nextId = std::max(level.nextId, std::max(nextIdBefore_, nextIdAfter_));
}

void PickupsCommand::apply(Level& level) const {
    level.pickups = after_;
    level.nextId = std::max(level.nextId, nextIdAfter_);
}

void PickupsCommand::undo(Level& level) const {
    level.pickups = before_;
    level.nextId = std::max(level.nextId, std::max(nextIdBefore_, nextIdAfter_));
}

void PlantsCommand::apply(Level& level) const {
    level.plants = after_;
    level.nextId = std::max(level.nextId, nextIdAfter_);
}

void PlantsCommand::undo(Level& level) const {
    level.plants = before_;
    level.nextId = std::max(level.nextId, std::max(nextIdBefore_, nextIdAfter_));
}

void History::run(std::unique_ptr<Command> command, Level& level) {
    command->apply(level);
    record(std::move(command));
}

void History::record(std::unique_ptr<Command> command) {
    undone_.clear();
    done_.push_back(std::move(command));
    if (done_.size() > limit_) {
        done_.erase(done_.begin()); // the oldest edit can no longer be undone
    }
}

bool History::undo(Level& level) {
    if (done_.empty()) return false;
    done_.back()->undo(level);
    undone_.push_back(std::move(done_.back()));
    done_.pop_back();
    return true;
}

bool History::redo(Level& level) {
    if (undone_.empty()) return false;
    undone_.back()->apply(level);
    done_.push_back(std::move(undone_.back()));
    undone_.pop_back();
    return true;
}

void History::clear() {
    done_.clear();
    undone_.clear();
}

std::vector<std::pair<int, int>> lineCells(int x0, int y0, int x1, int y1) {
    // Bresenham's line: every step moves one cell along the longer axis.
    std::vector<std::pair<int, int>> cells;
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    while (true) {
        cells.push_back({x0, y0});
        if (x0 == x1 && y0 == y1) break;
        const int twice = 2 * error;
        if (twice >= dy) {
            error += dy;
            x0 += sx;
        }
        if (twice <= dx) {
            error += dx;
            y0 += sy;
        }
    }
    return cells;
}

std::vector<CellChange> rectangleFill(const Level& level, int x0, int y0, int x1, int y1, int tile) {
    std::vector<CellChange> changes;
    for (int y = std::max(0, std::min(y0, y1)); y <= std::min(level.height - 1, std::max(y0, y1)); ++y) {
        for (int x = std::max(0, std::min(x0, x1)); x <= std::min(level.width - 1, std::max(x0, x1)); ++x) {
            if (level.at(x, y) != tile) changes.push_back({x, y, level.at(x, y), tile});
        }
    }
    return changes;
}

std::vector<CellChange> floodFill(const Level& level, int x, int y, int tile) {
    std::vector<CellChange> changes;
    if (!level.inside(x, y) || level.at(x, y) == tile) return changes;
    const int from = level.at(x, y);
    std::vector<bool> seen(level.ground.size(), false);
    std::vector<std::pair<int, int>> stack{{x, y}};
    seen[static_cast<std::size_t>(y * level.width + x)] = true;
    // An explicit stack, never recursion: a big meadow would overflow the call stack.
    while (!stack.empty()) {
        const auto [cx, cy] = stack.back();
        stack.pop_back();
        changes.push_back({cx, cy, from, tile});
        for (const auto [nx, ny] : {std::pair{cx + 1, cy}, std::pair{cx - 1, cy}, std::pair{cx, cy + 1}, std::pair{cx, cy - 1}}) {
            if (!level.inside(nx, ny)) continue;
            const auto at = static_cast<std::size_t>(ny * level.width + nx);
            if (seen[at] || level.at(nx, ny) != from) continue;
            seen[at] = true;
            stack.push_back({nx, ny});
        }
    }
    // Reading order: the same fill always lists its cells the same way.
    std::sort(changes.begin(), changes.end(), [](const CellChange& a, const CellChange& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
    return changes;
}

} // namespace odysseus::game
