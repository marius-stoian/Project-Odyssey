// US-170 Node-graph widget: nodes and wires on a canvas that pans, zooms, selects, drags and reports every edit.
#include "luna/engine/node_graph.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <string>
#include <vector>

namespace engine = luna::engine;
using engine::GraphNode;
using engine::NodeGraph;
using engine::NodeGraphView;
using engine::UiInput;

namespace {

UiInput tick(int x, int y, bool pressLeft = false, bool heldLeft = false, bool releaseLeft = false) {
    UiInput input;
    input.pointer.x = x;
    input.pointer.y = y;
    input.pointer.pressed[0] = pressLeft;
    input.pointer.held[0] = heldLeft;
    input.pointer.released[0] = releaseLeft;
    return input;
}

UiInput rightTick(int x, int y, bool press, bool held, bool release) {
    UiInput input;
    input.pointer.x = x;
    input.pointer.y = y;
    input.pointer.pressed[1] = press;
    input.pointer.held[1] = held;
    input.pointer.released[1] = release;
    return input;
}

UiInput wheel(int x, int y, int amount) {
    UiInput input = tick(x, y);
    input.pointer.wheel = amount;
    return input;
}

// A left drag from one point to another: press, move, release.
void drag(NodeGraphView& view, int x0, int y0, int x1, int y1) {
    view.handle(tick(x0, y0, true, true));
    view.handle(tick((x0 + x1) / 2, (y0 + y1) / 2, false, true));
    view.handle(tick(x1, y1, false, true));
    view.handle(tick(x1, y1, false, false, true));
}

struct Setup {
    NodeGraph graph;
    std::vector<std::pair<NodeGraph, NodeGraph>> edits; // before, after
    std::vector<std::string> names;
    NodeGraphView view;
    Setup()
        : view({0, 0, 300, 200}, graph, [this](const std::string& name, const NodeGraph& before, const NodeGraph& after) {
              names.push_back(name);
              edits.emplace_back(before, after);
          }) {}
    int add(int screenX, int screenY, const char* title = "Line") {
        GraphNode node;
        node.title = title;
        return view.addNodeAt(node, screenX, screenY);
    }
};

} // namespace

TEST_CASE("US-170 Edit: two nodes and a dragged wire connect them") {
    Setup s;
    const int a = s.add(10, 10);
    const int b = s.add(150, 10);
    CHECK(s.graph.nodes().size() == 2);
    // Node a is 96 wide, its output port sits at its right edge, 15 units below its top; b's input at its left edge.
    drag(s.view, 106, 25, 150, 25);
    REQUIRE(s.graph.wires().size() == 1);
    CHECK(s.graph.wires()[0] == engine::GraphWire{a, 0, b, 0});
    CHECK(s.names.back() == "connect nodes");
}

TEST_CASE("US-170 Edit: a wire dropped on nothing leaves the graph as it was") {
    Setup s;
    s.add(10, 10);
    s.add(150, 10);
    const std::size_t editsBefore = s.edits.size();
    drag(s.view, 106, 25, 200, 150);
    CHECK(s.graph.wires().empty());
    CHECK(s.edits.size() == editsBefore);
}

TEST_CASE("US-170 Edit: an output has one wire, a second drag replaces it; delete removes a node and its wires") {
    Setup s;
    const int a = s.add(10, 10);
    const int b = s.add(150, 10);
    const int c = s.add(150, 80);
    drag(s.view, 106, 25, 150, 25);
    drag(s.view, 106, 25, 150, 95);
    REQUIRE(s.graph.wires().size() == 1);
    CHECK(s.graph.wires()[0].toNode == c);
    s.view.select(c);
    CHECK(s.view.deleteSelected());
    CHECK(s.graph.wires().empty());
    CHECK(s.graph.find(b) != nullptr);
    CHECK(s.graph.find(c) == nullptr);
    CHECK(s.graph.find(a) != nullptr);
}

TEST_CASE("US-170 Edit: dragging a node moves it as one edit, a selection box picks nodes") {
    Setup s;
    const int a = s.add(10, 10);
    const int b = s.add(150, 10);
    drag(s.view, 40, 20, 60, 70); // inside a's card, not on a port
    CHECK(s.graph.find(a)->x == 30);
    CHECK(s.graph.find(a)->y == 60);
    CHECK(s.names.back() == "move nodes");
    s.view.clearSelection();
    drag(s.view, 5, 195, 295, 5); // an empty corner to past both nodes
    CHECK(s.view.selection().count(a) == 1);
    CHECK(s.view.selection().count(b) == 1);
}

TEST_CASE("US-170 Navigate: right drag pans, the wheel zooms around the cursor, within 25 and 400 percent") {
    Setup s;
    s.add(10, 10);
    s.view.handle(rightTick(50, 50, true, true, false));
    s.view.handle(rightTick(80, 70, false, true, false));
    s.view.handle(rightTick(80, 70, false, false, true));
    CHECK(s.view.panX == 30);
    CHECK(s.view.panY == 20);

    const int gx = s.view.toGraphX(200);
    const int gy = s.view.toGraphY(120);
    s.view.handle(wheel(200, 120, 1));
    CHECK(s.view.zoomPercent() == 125);
    CHECK(std::abs(s.view.toGraphX(200) - gx) <= 1);
    CHECK(std::abs(s.view.toGraphY(120) - gy) <= 1);
    for (int i = 0; i < 20; ++i) s.view.handle(wheel(200, 120, 1));
    CHECK(s.view.zoomPercent() == 400);
    for (int i = 0; i < 20; ++i) s.view.handle(wheel(200, 120, -1));
    CHECK(s.view.zoomPercent() == 25);
    CHECK(s.edits.size() == 1); // pan and zoom are not edits (only the add)
}

TEST_CASE("US-170 Navigate: frame all shows every node") {
    Setup s;
    for (int i = 0; i < 6; ++i) s.add(10 + i * 150, 10 + i * 90);
    s.view.frameAll();
    for (const GraphNode& node : s.graph.nodes()) {
        CHECK(s.view.toScreenX(node.x) >= 0);
        CHECK(s.view.toScreenY(node.y) >= 0);
        CHECK(s.view.toScreenX(node.x + engine::kGraphNodeWidth) <= 300);
        CHECK(s.view.toScreenY(node.y + node.height()) <= 200);
    }
}

TEST_CASE("US-170 Undo: ten edits walked back one by one give the empty graph again") {
    Setup s;
    for (int i = 0; i < 10; ++i) s.add(10 + i * 20, 10 + i * 10);
    REQUIRE(s.edits.size() == 10);
    CHECK(s.graph.nodes().size() == 10);
    for (int i = 9; i >= 0; --i) {
        CHECK(s.edits[static_cast<std::size_t>(i)].second == s.graph);
        s.graph = s.edits[static_cast<std::size_t>(i)].first;
    }
    CHECK(s.graph.nodes().empty());
    CHECK(s.graph.wires().empty());
}

TEST_CASE("US-170 Layout: layers run left to right, the same graph gets the same places") {
    NodeGraph g;
    GraphNode node;
    node.inputs = 1;
    node.outputs = 2;
    const int a = g.add(node);
    const int b = g.add(node);
    const int c = g.add(node);
    const int d = g.add(node);
    g.connect(a, 0, b, 0);
    g.connect(a, 1, c, 0);
    g.connect(b, 0, d, 0);
    engine::layoutLayers(g);
    CHECK(g.find(a)->x < g.find(b)->x);
    CHECK(g.find(b)->x == g.find(c)->x);
    CHECK(g.find(b)->y != g.find(c)->y);
    CHECK(g.find(d)->x > g.find(b)->x);
    NodeGraph again = g;
    engine::layoutLayers(again);
    CHECK(again == g);
}

TEST_CASE("US-170 Draw: a card paints its header colour and a selected card a gold frame") {
    NodeGraph g;
    GraphNode node;
    node.title = "Line";
    node.header = engine::UiColor::Red;
    node.x = 10;
    node.y = 10;
    g.add(node);
    NodeGraphView view({0, 0, 200, 100}, g, {});
    engine::ImageRenderer renderer(200, 100);
    const engine::Texture sheet = renderer.createTexture(engine::makeUiSheet());
    engine::UiPainter painter(renderer, sheet);
    view.draw(painter);
    const auto before = renderer.image().get(10, 30);
    CHECK(renderer.image().get(50, 12) != renderer.image().get(50, 60)); // the header differs from the empty canvas
    view.select(1);
    view.draw(painter);
    CHECK(renderer.image().get(10, 30) != before); // the frame changed colour when selected
}
