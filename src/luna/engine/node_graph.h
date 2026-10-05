#pragma once

#include "boundary.h"

#include "ui.h"

#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <vector>

namespace luna::engine {

// Luna's node-graph widget (US-170): nodes with ports, wires between them, and a canvas to pan, zoom, select,
// drag, connect and delete on. Game-agnostic: the Editor builds dialogue and interaction graphs on it (M9).

inline constexpr int kGraphNodeWidth = 96;   // graph units
inline constexpr int kGraphHeaderHeight = 11;
inline constexpr int kGraphZoomMin = 25;     // percent (owner, D-56 Q7)
inline constexpr int kGraphZoomMax = 400;

struct GraphNode {
    int id = 0;
    std::string type;                  // "line", "choice"... the host gives it meaning
    std::string title;
    std::vector<std::string> lines;    // preview text under the header
    std::vector<std::string> fields;   // what the host keeps in the card (a line's speaker and text...); part of the value, so an edit of it can be undone
    UiColor header = UiColor::Selected; // the card's header colour, one per type
    int x = 0;                         // graph units
    int y = 0;
    int inputs = 1;                    // ports on the left
    int outputs = 1;                   // ports on the right

    int height() const;                // graph units
    friend bool operator==(const GraphNode&, const GraphNode&) = default;
};

// One wire runs from an output port to an input port. An output port has at most one wire.
struct GraphWire {
    int fromNode = 0;
    int fromPort = 0;
    int toNode = 0;
    int toPort = 0;
    friend bool operator==(const GraphWire&, const GraphWire&) = default;
};

// The model: a plain value, so a Command can keep a copy from before and after an edit.
class NodeGraph {
public:
    // Adds a node and returns its id. Ids are never reused.
    int add(GraphNode node);
    bool remove(int id);                                   // also removes its wires
    // Wires `fromNode.fromPort` to `toNode.toPort`; the output's old wire goes. False for a bad port, the same node, or no such node.
    bool connect(int fromNode, int fromPort, int toNode, int toPort);
    bool disconnect(int fromNode, int fromPort);

    GraphNode* find(int id);
    const GraphNode* find(int id) const;
    const std::vector<GraphNode>& nodes() const { return nodes_; }
    std::vector<GraphNode>& nodes() { return nodes_; }
    const std::vector<GraphWire>& wires() const { return wires_; }
    int nextId() const { return nextId_; }
    void setNextId(int id) { nextId_ = id; }

    friend bool operator==(const NodeGraph&, const NodeGraph&) = default;

private:
    std::vector<GraphNode> nodes_;
    std::vector<GraphWire> wires_;
    int nextId_ = 1;
};

// Puts every node in a column by its distance from the nodes nobody points at, left to right, and stacks each column
// from the top (Q8, D-56). The same graph always gets the same places.
void layoutLayers(NodeGraph& graph, int gap = 24);

// What a drag or a key changed: reported after the graph already holds `after`.
using GraphEdit = std::function<void(const std::string& name, const NodeGraph& before, const NodeGraph& after)>;

class NodeGraphView final : public Widget {
public:
    NodeGraphView(Rect bounds, NodeGraph& graph, GraphEdit onEdit) : Widget(bounds), graph_(graph), onEdit_(std::move(onEdit)) {}

    // View state (not edits): graph origin in pixels from the view's top-left corner, and the zoom in percent.
    int panX = 0;
    int panY = 0;
    int zoomPercent() const { return zoom_; }
    void setZoom(int percent, int aroundX, int aroundY); // clamped to 25..400; the graph point under (aroundX, aroundY) stays put

    // Graph units to screen pixels and back (floor division, so both are exact inverses at whole zoom steps).
    int toScreenX(int graphX) const;
    int toScreenY(int graphY) const;
    int toGraphX(int screenX) const;
    int toGraphY(int screenY) const;

    const std::set<int>& selection() const { return selected_; }
    void select(int id);                 // only this node
    void clearSelection() { selected_.clear(); }
    bool deleteSelected();               // the host calls it on the Delete key
    void frameAll();                     // pan and zoom so every node shows
    // Adds a node at a screen point; the host calls it from its "add" menu.
    int addNodeAt(GraphNode node, int screenX, int screenY);

    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;

    // The id of the node at a screen point, or 0.
    int nodeAt(int screenX, int screenY) const;

private:
    struct PortHit {
        int node = 0;
        int port = 0;
        bool output = false;
    };
    Point portPosition(const GraphNode& node, int port, bool output) const; // screen
    PortHit portAt(int screenX, int screenY) const;
    Rect nodeRect(const GraphNode& node) const; // screen
    void report(const std::string& name, const NodeGraph& before);

    NodeGraph& graph_;
    GraphEdit onEdit_;
    int zoom_ = 100;

    enum class Drag { None, Pan, Nodes, Box, Wire };
    Drag drag_ = Drag::None;
    NodeGraph before_;           // the graph when a drag began
    bool moved_ = false;
    int lastX_ = 0;
    int lastY_ = 0;
    int startX_ = 0;             // screen point a box or a wire began at
    int startY_ = 0;
    PortHit wireFrom_;
    std::set<int> selected_;
};

} // namespace luna::engine
