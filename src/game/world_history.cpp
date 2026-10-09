#include "game/world_history.h"

namespace odysseus::game {

void WorldHistory::record(WorldCommand command) {
    if (command.changes.empty() && command.placed.empty()) return;
    steps_.resize(next_); // what could have been redone is gone
    steps_.push_back(std::move(command));
    if (steps_.size() > kMaxSteps) steps_.erase(steps_.begin());
    next_ = steps_.size();
}

const WorldCommand* WorldHistory::undo() {
    if (!canUndo()) return nullptr;
    return &steps_[--next_];
}

const WorldCommand* WorldHistory::redo() {
    if (!canRedo()) return nullptr;
    return &steps_[next_++];
}

} // namespace odysseus::game
