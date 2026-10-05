#pragma once

#include "boundary.h"

#include "game/editor_history.h"
#include "luna/engine/node_graph.h"

#include <memory>
#include <string>

namespace odysseus::game {

// One edit of a node graph (US-170) as a Command, so it lives in the same History as the map edits (D-56 Q19): Ctrl+Z walks back
// through both. It keeps the graph before and after; the level it is handed is not its business and stays as it is.
class GraphEditCommand final : public Command {
public:
    GraphEditCommand(std::string what, std::shared_ptr<luna::engine::NodeGraph> target, luna::engine::NodeGraph before, luna::engine::NodeGraph after)
        : what_(std::move(what)), target_(std::move(target)), before_(std::move(before)), after_(std::move(after)) {}
    void apply(Level& /*level*/) const override { *target_ = after_; }
    void undo(Level& /*level*/) const override { *target_ = before_; }
    std::string name() const override { return what_; }

private:
    std::string what_;
    std::shared_ptr<luna::engine::NodeGraph> target_;
    luna::engine::NodeGraph before_;
    luna::engine::NodeGraph after_;
};

} // namespace odysseus::game
