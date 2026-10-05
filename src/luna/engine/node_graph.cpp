#include "luna/engine/node_graph.h"

#include <algorithm>
#include <cstdlib>
#include <array>
#include <map>

namespace luna::engine {

namespace {

constexpr std::array<int, 11> kZoomSteps = {25, 33, 50, 67, 75, 100, 125, 150, 200, 300, 400};
constexpr int kPortRowHeight = kLineHeight; // graph units between ports and preview lines
constexpr int kNodeTail = 4;

int floorDiv(int value, int divisor) {
    int q = value / divisor;
    if ((value % divisor != 0) && ((value < 0) != (divisor < 0))) --q;
    return q;
}

int nextZoom(int current, bool up) {
    if (up) {
        for (const int step : kZoomSteps) {
            if (step > current) return step;
        }
        return kGraphZoomMax;
    }
    for (auto it = kZoomSteps.rbegin(); it != kZoomSteps.rend(); ++it) {
        if (*it < current) return *it;
    }
    return kGraphZoomMin;
}

Rect intersect(const Rect& a, const Rect& b) {
    const int x0 = std::max(a.x, b.x);
    const int y0 = std::max(a.y, b.y);
    const int x1 = std::min(a.x + a.width, b.x + b.width);
    const int y1 = std::min(a.y + a.height, b.y + b.height);
    return {x0, y0, std::max(0, x1 - x0), std::max(0, y1 - y0)};
}

bool overlaps(const Rect& a, const Rect& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

Rect rectBetween(int x0, int y0, int x1, int y1) {
    return {std::min(x0, x1), std::min(y0, y1), std::abs(x1 - x0) + 1, std::abs(y1 - y0) + 1};
}

} // namespace

int GraphNode::height() const {
    const int byLines = kGraphHeaderHeight + static_cast<int>(lines.size()) * kPortRowHeight + kNodeTail;
    const int byPorts = kGraphHeaderHeight + std::max(inputs, outputs) * kPortRowHeight + kNodeTail;
    return std::max(byLines, byPorts);
}

int NodeGraph::add(GraphNode node) {
    node.id = nextId_++;
    nodes_.push_back(std::move(node));
    return nodes_.back().id;
}

bool NodeGraph::remove(int id) {
    const auto it = std::find_if(nodes_.begin(), nodes_.end(), [id](const GraphNode& n) { return n.id == id; });
    if (it == nodes_.end()) return false;
    nodes_.erase(it);
    std::erase_if(wires_, [id](const GraphWire& w) { return w.fromNode == id || w.toNode == id; });
    return true;
}

bool NodeGraph::connect(int fromNode, int fromPort, int toNode, int toPort) {
    const GraphNode* from = find(fromNode);
    const GraphNode* to = find(toNode);
    if (from == nullptr || to == nullptr || fromNode == toNode) return false;
    if (fromPort < 0 || fromPort >= from->outputs || toPort < 0 || toPort >= to->inputs) return false;
    disconnect(fromNode, fromPort);
    wires_.push_back({fromNode, fromPort, toNode, toPort});
    return true;
}

bool NodeGraph::disconnect(int fromNode, int fromPort) {
    return std::erase_if(wires_, [&](const GraphWire& w) { return w.fromNode == fromNode && w.fromPort == fromPort; }) > 0;
}

GraphNode* NodeGraph::find(int id) {
    for (GraphNode& node : nodes_) {
        if (node.id == id) return &node;
    }
    return nullptr;
}

const GraphNode* NodeGraph::find(int id) const {
    for (const GraphNode& node : nodes_) {
        if (node.id == id) return &node;
    }
    return nullptr;
}

void layoutLayers(NodeGraph& graph, int gap) {
    std::map<int, int> layerOf; // node id -> column
    std::vector<int> queue;
    auto hasIncoming = [&](int id) {
        return std::any_of(graph.wires().begin(), graph.wires().end(), [id](const GraphWire& w) { return w.toNode == id; });
    };
    auto walk = [&](int root) {
        layerOf[root] = 0;
        queue.assign(1, root);
        for (std::size_t head = 0; head < queue.size(); ++head) {
            const int at = queue[head];
            for (const GraphWire& wire : graph.wires()) {
                if (wire.fromNode == at && layerOf.find(wire.toNode) == layerOf.end()) {
                    layerOf[wire.toNode] = layerOf[at] + 1;
                    queue.push_back(wire.toNode);
                }
            }
        }
    };
    for (const GraphNode& node : graph.nodes()) {
        if (!hasIncoming(node.id) && layerOf.find(node.id) == layerOf.end()) walk(node.id);
    }
    for (const GraphNode& node : graph.nodes()) { // what only a cycle reaches
        if (layerOf.find(node.id) == layerOf.end()) walk(node.id);
    }
    std::map<int, int> columnTop; // column -> next free y
    std::vector<int> order;
    for (const GraphNode& node : graph.nodes()) order.push_back(node.id);
    std::sort(order.begin(), order.end());
    for (const int id : order) {
        GraphNode* node = graph.find(id);
        const int column = layerOf[id];
        node->x = column * (kGraphNodeWidth + gap);
        node->y = columnTop[column];
        columnTop[column] += node->height() + gap;
    }
}

int NodeGraphView::toScreenX(int graphX) const { return bounds.x + panX + floorDiv(graphX * zoom_, 100); }
int NodeGraphView::toScreenY(int graphY) const { return bounds.y + panY + floorDiv(graphY * zoom_, 100); }
int NodeGraphView::toGraphX(int screenX) const { return floorDiv((screenX - bounds.x - panX) * 100, zoom_); }
int NodeGraphView::toGraphY(int screenY) const { return floorDiv((screenY - bounds.y - panY) * 100, zoom_); }

void NodeGraphView::setZoom(int percent, int aroundX, int aroundY) {
    const int next = std::clamp(percent, kGraphZoomMin, kGraphZoomMax);
    if (next == zoom_) return;
    const int fromX = aroundX - bounds.x;
    const int fromY = aroundY - bounds.y;
    panX = fromX - (fromX - panX) * next / zoom_;
    panY = fromY - (fromY - panY) * next / zoom_;
    zoom_ = next;
}

Rect NodeGraphView::nodeRect(const GraphNode& node) const {
    return {toScreenX(node.x), toScreenY(node.y), std::max(8, kGraphNodeWidth * zoom_ / 100), std::max(6, node.height() * zoom_ / 100)};
}

Point NodeGraphView::portPosition(const GraphNode& node, int port, bool output) const {
    const Rect rect = nodeRect(node);
    const int y = rect.y + (kGraphHeaderHeight + kPortRowHeight / 2 + port * kPortRowHeight) * zoom_ / 100;
    return {output ? rect.x + rect.width : rect.x, y};
}

NodeGraphView::PortHit NodeGraphView::portAt(int screenX, int screenY) const {
    const int reach = std::max(3, 4 * zoom_ / 100);
    for (auto it = graph_.nodes().rbegin(); it != graph_.nodes().rend(); ++it) {
        for (int output = 0; output < 2; ++output) {
            const int count = output != 0 ? it->outputs : it->inputs;
            for (int port = 0; port < count; ++port) {
                const Point at = portPosition(*it, port, output != 0);
                if (std::abs(screenX - at.x) <= reach && std::abs(screenY - at.y) <= reach) return {it->id, port, output != 0};
            }
        }
    }
    return {};
}

int NodeGraphView::nodeAt(int screenX, int screenY) const {
    for (auto it = graph_.nodes().rbegin(); it != graph_.nodes().rend(); ++it) { // the last drawn is on top
        if (nodeRect(*it).contains({screenX, screenY})) return it->id;
    }
    return 0;
}

void NodeGraphView::select(int id) {
    selected_.clear();
    if (graph_.find(id) != nullptr) selected_.insert(id);
}

void NodeGraphView::report(const std::string& name, const NodeGraph& before) {
    if (graph_ == before) return;
    if (onEdit_) onEdit_(name, before, graph_);
}

bool NodeGraphView::deleteSelected() {
    if (selected_.empty()) return false;
    const NodeGraph before = graph_;
    for (const int id : selected_) graph_.remove(id);
    selected_.clear();
    report("delete nodes", before);
    return !(graph_ == before);
}

int NodeGraphView::addNodeAt(GraphNode node, int screenX, int screenY) {
    const NodeGraph before = graph_;
    node.x = toGraphX(screenX);
    node.y = toGraphY(screenY);
    const int id = graph_.add(std::move(node));
    select(id);
    report("add node", before);
    return id;
}

void NodeGraphView::frameAll() {
    if (graph_.nodes().empty()) {
        panX = panY = 0;
        zoom_ = 100;
        return;
    }
    int x0 = graph_.nodes().front().x;
    int y0 = graph_.nodes().front().y;
    int x1 = x0;
    int y1 = y0;
    for (const GraphNode& node : graph_.nodes()) {
        x0 = std::min(x0, node.x);
        y0 = std::min(y0, node.y);
        x1 = std::max(x1, node.x + kGraphNodeWidth);
        y1 = std::max(y1, node.y + node.height());
    }
    constexpr int kMargin = 8;
    const int fitX = (bounds.width - 2 * kMargin) * 100 / std::max(1, x1 - x0);
    const int fitY = (bounds.height - 2 * kMargin) * 100 / std::max(1, y1 - y0);
    zoom_ = std::clamp(std::min(fitX, fitY), kGraphZoomMin, 100); // never magnify past 100% to frame a small graph
    panX = (bounds.width - (x1 - x0) * zoom_ / 100) / 2 - x0 * zoom_ / 100;
    panY = (bounds.height - (y1 - y0) * zoom_ / 100) / 2 - y0 * zoom_ / 100;
}

bool NodeGraphView::handle(const UiInput& input) {
    if (!visible) return false;
    const Pointer& p = input.pointer;
    const bool over = contains(p.x, p.y);
    bool used = false;

    if (over && p.wheel != 0 && drag_ == Drag::None) {
        setZoom(nextZoom(zoom_, p.wheel > 0), p.x, p.y);
        used = true;
    }

    if (over && drag_ == Drag::None) {
        if (p.wasPressed(PointerButton::Right)) {
            drag_ = Drag::Pan;
            used = true;
        } else if (p.wasPressed(PointerButton::Left)) {
            used = true;
            before_ = graph_;
            moved_ = false;
            startX_ = p.x;
            startY_ = p.y;
            const PortHit hit = portAt(p.x, p.y);
            if (hit.node != 0 && hit.output) {
                drag_ = Drag::Wire;
                wireFrom_ = hit;
            } else if (const int id = nodeAt(p.x, p.y); id != 0) {
                if (selected_.count(id) == 0) select(id);
                drag_ = Drag::Nodes;
            } else {
                selected_.clear();
                drag_ = Drag::Box;
            }
        }
        lastX_ = p.x;
        lastY_ = p.y;
    }

    if (drag_ == Drag::None) return used;
    used = true;

    if (drag_ == Drag::Pan) {
        if (p.inside()) {
            panX += p.x - lastX_;
            panY += p.y - lastY_;
            lastX_ = p.x;
            lastY_ = p.y;
        }
        if (p.wasReleased(PointerButton::Right) || !p.isHeld(PointerButton::Right)) drag_ = Drag::None;
        return used;
    }

    if (p.inside()) {
        lastX_ = p.x;
        lastY_ = p.y;
    }
    if (drag_ == Drag::Nodes) {
        const int dx = floorDiv((lastX_ - startX_) * 100, zoom_);
        const int dy = floorDiv((lastY_ - startY_) * 100, zoom_);
        if (dx != 0 || dy != 0) moved_ = true;
        for (const int id : selected_) {
            const GraphNode* from = before_.find(id);
            GraphNode* node = graph_.find(id);
            if (from != nullptr && node != nullptr) {
                node->x = from->x + dx;
                node->y = from->y + dy;
            }
        }
    }

    if (p.wasReleased(PointerButton::Left) || !p.isHeld(PointerButton::Left)) {
        if (drag_ == Drag::Nodes) {
            if (moved_) report("move nodes", before_);
        } else if (drag_ == Drag::Wire) {
            const PortHit hit = portAt(lastX_, lastY_);
            if (hit.node != 0 && !hit.output) graph_.connect(wireFrom_.node, wireFrom_.port, hit.node, hit.port);
            report("connect nodes", before_);
        } else if (drag_ == Drag::Box) {
            const Rect box = rectBetween(startX_, startY_, lastX_, lastY_);
            for (const GraphNode& node : graph_.nodes()) {
                if (overlaps(box, nodeRect(node))) selected_.insert(node.id);
            }
        }
        drag_ = Drag::None;
    }
    return used;
}

void NodeGraphView::draw(UiPainter& painter) const {
    if (!visible) return;
    auto fill = [&](Rect area, UiColor color) {
        const Rect clipped = intersect(area, bounds);
        if (clipped.width > 0 && clipped.height > 0) painter.fill(clipped, color);
    };
    auto text = [&](int x, int y, std::string_view s, UiColor color, int maxWidth) {
        const int fit = std::max(0, std::min(static_cast<int>(s.size()), (maxWidth + 1) / kTextAdvance));
        const std::string_view shown = s.substr(0, static_cast<std::size_t>(fit));
        if (!shown.empty() && x >= bounds.x && y >= bounds.y && x + UiPainter::textWidth(shown) <= bounds.x + bounds.width &&
            y + kGlyphHeight <= bounds.y + bounds.height) {
            painter.text(x, y, shown, color);
        }
    };
    auto line = [&](Point a, Point b, UiColor color) { // a horizontal-vertical-horizontal run
        const int mid = std::max((a.x + b.x) / 2, a.x + 4);
        fill({std::min(a.x, mid), a.y, std::abs(mid - a.x) + 1, 1}, color);
        fill({mid, std::min(a.y, b.y), 1, std::abs(b.y - a.y) + 1}, color);
        fill({std::min(mid, b.x), b.y, std::abs(b.x - mid) + 1, 1}, color);
    };

    fill(bounds, UiColor::Dark);
    for (const GraphWire& wire : graph_.wires()) {
        const GraphNode* from = graph_.find(wire.fromNode);
        const GraphNode* to = graph_.find(wire.toNode);
        if (from == nullptr || to == nullptr) continue;
        const bool lit = selected_.count(wire.fromNode) != 0 || selected_.count(wire.toNode) != 0;
        line(portPosition(*from, wire.fromPort, true), portPosition(*to, wire.toPort, false), lit ? UiColor::Gold : UiColor::Dim);
    }

    const int headerPixels = std::max(3, kGraphHeaderHeight * zoom_ / 100);
    for (const GraphNode& node : graph_.nodes()) {
        const Rect rect = nodeRect(node);
        if (!overlaps(rect, bounds)) continue;
        const bool chosen = selected_.count(node.id) != 0;
        fill(rect, UiColor::PanelLight);
        fill({rect.x, rect.y, rect.width, headerPixels}, node.header);
        fill({rect.x, rect.y, rect.width, 1}, chosen ? UiColor::Gold : UiColor::Border);
        fill({rect.x, rect.y + rect.height - 1, rect.width, 1}, chosen ? UiColor::Gold : UiColor::Border);
        fill({rect.x, rect.y, 1, rect.height}, chosen ? UiColor::Gold : UiColor::Border);
        fill({rect.x + rect.width - 1, rect.y, 1, rect.height}, chosen ? UiColor::Gold : UiColor::Border);
        if (zoom_ >= 50) text(rect.x + 3, rect.y + (headerPixels - kGlyphHeight) / 2 + 1, node.title, UiColor::Text, rect.width - 6);
        if (zoom_ >= 100) {
            for (std::size_t i = 0; i < node.lines.size(); ++i) {
                text(rect.x + 4, rect.y + headerPixels + 2 + static_cast<int>(i) * kLineHeight, node.lines[i], UiColor::Dim, rect.width - 8);
            }
        }
        for (int output = 0; output < 2; ++output) {
            const int count = output != 0 ? node.outputs : node.inputs;
            for (int port = 0; port < count; ++port) {
                const Point at = portPosition(node, port, output != 0);
                fill({at.x - 1, at.y - 1, 3, 3}, output != 0 ? UiColor::Gold : UiColor::Text);
            }
        }
    }

    if (drag_ == Drag::Box) {
        const Rect box = rectBetween(startX_, startY_, lastX_, lastY_);
        fill({box.x, box.y, box.width, 1}, UiColor::Hover);
        fill({box.x, box.y + box.height - 1, box.width, 1}, UiColor::Hover);
        fill({box.x, box.y, 1, box.height}, UiColor::Hover);
        fill({box.x + box.width - 1, box.y, 1, box.height}, UiColor::Hover);
    } else if (drag_ == Drag::Wire) {
        if (const GraphNode* from = graph_.find(wireFrom_.node)) line(portPosition(*from, wireFrom_.port, true), {lastX_, lastY_}, UiColor::Hover);
    }
    painter.fill({bounds.x, bounds.y, bounds.width, 1}, UiColor::Border);
    painter.fill({bounds.x, bounds.y + bounds.height - 1, bounds.width, 1}, UiColor::Border);
    painter.fill({bounds.x, bounds.y, 1, bounds.height}, UiColor::Border);
    painter.fill({bounds.x + bounds.width - 1, bounds.y, 1, bounds.height}, UiColor::Border);
}

} // namespace luna::engine
