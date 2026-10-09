#pragma once

#include "boundary.h"

#include "sim/region.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// One tile of a world edit (US-202): the hand edit it had before and has now. No value means no edit, the seed's own land.
struct TileChange {
    int x = 0;
    int y = 0;
    std::optional<sim::Biome> before;
    std::optional<sim::Biome> after;
    friend bool operator==(const TileChange&, const TileChange&) = default;
};

// A step of Undo in the world tools: a whole stroke, rectangle or fill is one step, as in the level editor.
struct WorldCommand {
    std::string label;
    std::vector<TileChange> changes;
};

// The way back for the world tools (D-61 Q5): one history for all of them, up to kMaxSteps steps; a new step drops what could have been redone.
class WorldHistory {
public:
    static constexpr std::size_t kMaxSteps = 500;

    void record(WorldCommand command);
    bool canUndo() const { return next_ > 0; }
    bool canRedo() const { return next_ < steps_.size(); }
    // The step to take back (the caller applies each change's `before`), and the step to do again (each change's `after`). Nothing when there is none.
    const WorldCommand* undo();
    const WorldCommand* redo();
    std::string undoLabel() const { return canUndo() ? steps_[next_ - 1].label : std::string(); }
    std::size_t size() const { return steps_.size(); }
    void clear() {
        steps_.clear();
        next_ = 0;
    }

private:
    std::vector<WorldCommand> steps_;
    std::size_t next_ = 0; // steps_[0 .. next_) are done
};

} // namespace odysseus::game
