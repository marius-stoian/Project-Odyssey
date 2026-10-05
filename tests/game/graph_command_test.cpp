// US-170 Node-graph widget, Undo: graph edits go through the same History as the map edits (D-56 Q19).
#include "game/editor_history.h"
#include "game/graph_commands.h"
#include "game/level.h"
#include "luna/engine/node_graph.h"

#include <doctest/doctest.h>

#include <memory>

using luna::engine::GraphNode;
using luna::engine::NodeGraph;
using luna::engine::NodeGraphView;
using odysseus::game::GraphEditCommand;
using odysseus::game::History;
using odysseus::game::Level;

TEST_CASE("US-170 Undo: ten graph edits and ten Ctrl+Z give the graph as it was") {
    auto graph = std::make_shared<NodeGraph>();
    History history;
    Level level;
    NodeGraphView view({0, 0, 300, 200}, *graph, [&](const std::string& name, const NodeGraph& before, const NodeGraph& after) {
        history.record(std::make_unique<GraphEditCommand>(name, graph, before, after));
    });
    for (int i = 0; i < 10; ++i) {
        GraphNode node;
        node.title = "Line";
        view.addNodeAt(node, 10 + i * 20, 10 + i * 10);
    }
    REQUIRE(graph->nodes().size() == 10);
    REQUIRE(history.size() == 10);
    for (int i = 0; i < 10; ++i) CHECK(history.undo(level));
    CHECK(graph->nodes().empty());
    CHECK_FALSE(history.canUndo());
    for (int i = 0; i < 10; ++i) CHECK(history.redo(level));
    CHECK(graph->nodes().size() == 10);
}
