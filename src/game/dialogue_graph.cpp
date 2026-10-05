#include "game/dialogue_graph.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <set>

namespace odysseus::game {

namespace {

using luna::engine::GraphNode;
using luna::engine::GraphWire;
using luna::engine::NodeGraph;
using luna::engine::UiColor;
using sim::rules::DlgChoice;
using sim::rules::DlgLine;
using sim::rules::DlgNode;
using sim::rules::DlgScript;
using sim::rules::Effect;

constexpr int kFlowIn = 0;
constexpr int kIfIn = 1;
constexpr int kDoIn = 2;
constexpr int kNextOut = 0;
constexpr int kToOut = 1;

std::string clip(const std::string& text, std::size_t width) {
    return text.size() <= width ? text : text.substr(0, width - 2) + "..";
}

const GraphNode* wireTarget(const NodeGraph& graph, int fromNode, int fromPort) {
    for (const GraphWire& wire : graph.wires()) {
        if (wire.fromNode == fromNode && wire.fromPort == fromPort) return graph.find(wire.toNode);
    }
    return nullptr;
}

// The cards of one kind wired into one port, in id order.
std::vector<const GraphNode*> cardsInto(const NodeGraph& graph, int node, int port, const char* type) {
    std::vector<const GraphNode*> found;
    for (const GraphWire& wire : graph.wires()) {
        if (wire.toNode != node || wire.toPort != port) continue;
        const GraphNode* from = graph.find(wire.fromNode);
        if (from != nullptr && from->type == type) found.push_back(from);
    }
    std::sort(found.begin(), found.end(), [](const GraphNode* a, const GraphNode* b) { return a->id < b->id; });
    return found;
}

std::string field(const GraphNode& card, std::size_t index) { return index < card.fields.size() ? card.fields[index] : std::string(); }

// A conversation reads as rows: one row per node, its lines and choices running to the right, and the conditions, effects and notes of each
// stacked above the card they belong to. The same graph always gets the same places.
void layoutRows(NodeGraph& g) {
    constexpr int kGap = 20;
    constexpr int kAuxGap = 4;
    int rowY = 0;
    for (const GraphNode& head : std::vector<GraphNode>(g.nodes())) {
        if (head.type != dlg_card::kNode) continue;
        std::vector<int> chain{head.id};
        for (const GraphNode* at = wireTarget(g, head.id, kNextOut); at != nullptr && std::find(chain.begin(), chain.end(), at->id) == chain.end();
             at = wireTarget(g, at->id, kNextOut)) {
            if (at->type != dlg_card::kLine && at->type != dlg_card::kChoice) break;
            chain.push_back(at->id);
        }
        auto auxOf = [&](int element) {
            std::vector<const GraphNode*> aux;
            {
                for (const GraphNode* c : cardsInto(g, element, kFlowIn, dlg_card::kComment)) aux.push_back(c);
            }
            for (const GraphNode* c : cardsInto(g, element, kIfIn, dlg_card::kCondition)) aux.push_back(c);
            for (const GraphNode* c : cardsInto(g, element, kDoIn, dlg_card::kEffect)) aux.push_back(c);
            return aux;
        };
        int band = 0;
        int mainHeight = 0;
        for (const int id : chain) {
            int stack = 0;
            for (const GraphNode* a : auxOf(id)) stack += a->height() + kAuxGap;
            band = std::max(band, stack);
            mainHeight = std::max(mainHeight, g.find(id)->height());
        }
        const int mainY = rowY + band;
        for (std::size_t k = 0; k < chain.size(); ++k) {
            GraphNode* card = g.find(chain[k]);
            card->x = static_cast<int>(k) * (luna::engine::kGraphNodeWidth + kGap);
            card->y = mainY;
            int top = mainY;
            for (const GraphNode* a : auxOf(chain[k])) {
                GraphNode* aux = g.find(a->id);
                top -= aux->height() + kAuxGap;
                aux->x = card->x;
                aux->y = top;
            }
        }
        rowY = mainY + mainHeight + kGap + 8;
    }
}

} // namespace

GraphNode newDialogueCard(const std::string& type) {
    GraphNode card;
    card.type = type;
    if (type == dlg_card::kNode) {
        card.header = UiColor::Selected;
        card.inputs = 1;
        card.outputs = 1;
        card.fields = {"node"};
    } else if (type == dlg_card::kLine) {
        card.header = UiColor::Hover;
        card.inputs = 2;
        card.outputs = 1;
        card.fields = {"Speaker", "Words"};
    } else if (type == dlg_card::kChoice) {
        card.header = UiColor::Gold;
        card.inputs = 3;
        card.outputs = 2;
        card.fields = {"Words", ""};
    } else if (type == dlg_card::kCondition) {
        card.header = UiColor::Red;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"time == night"};
    } else if (type == dlg_card::kEffect) {
        card.header = UiColor::Border;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"opinion npc hero 5"};
    } else if (type == dlg_card::kGoto) {
        card.header = UiColor::Grid;
        card.inputs = 1;
        card.outputs = 1;
        card.fields = {"start"};
    } else { // comment
        card.type = dlg_card::kComment;
        card.header = UiColor::Dim;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"A note"};
    }
    describeCard(card);
    return card;
}

void describeCard(GraphNode& card) {
    card.lines.clear();
    if (card.type == dlg_card::kNode) {
        card.title = "Node " + clip(field(card, 0), 10);
    } else if (card.type == dlg_card::kLine) {
        card.title = clip(field(card, 0), 14);
        const std::string text = field(card, 1);
        card.lines.push_back(clip(text, 14));
        if (text.size() > 14) card.lines.push_back(clip(text.substr(14), 14));
    } else if (card.type == dlg_card::kChoice) {
        card.title = "Choice";
        const std::string text = field(card, 0);
        card.lines.push_back(clip(text, 14));
        if (text.size() > 14) card.lines.push_back(clip(text.substr(14), 14));
    } else if (card.type == dlg_card::kCondition) {
        card.title = "If";
        card.lines.push_back(clip(field(card, 0), 14));
    } else if (card.type == dlg_card::kEffect) {
        card.title = "Do";
        for (std::size_t i = 0; i < card.fields.size() && i < 3; ++i) card.lines.push_back(clip(card.fields[i], 14));
    } else if (card.type == dlg_card::kGoto) {
        card.title = "Goto " + clip(field(card, 0), 8);
    } else {
        card.title = "Note";
        for (std::size_t i = 0; i < card.fields.size() && i < 2; ++i) card.lines.push_back(clip(card.fields[i], 14));
    }
}

DialogueGraph dialogueToGraph(const DlgScript& script, const DialogueLayout& layout) {
    DialogueGraph out;
    out.header = script;
    out.header.nodes.clear();
    NodeGraph& g = out.graph;
    std::map<int, std::string> keyOf;
    std::map<std::string, int> nodeCard;
    std::vector<std::pair<int, std::string>> pendingTargets; // choice card id, target node id

    auto make = [&](const char* type, std::vector<std::string> fields, const std::string& key) {
        GraphNode card = newDialogueCard(type);
        card.fields = std::move(fields);
        describeCard(card);
        const int id = g.add(std::move(card));
        keyOf[id] = key;
        return id;
    };
    auto notesFor = [&](const std::vector<std::string>& notes, const std::string& key, int target, int port) {
        if (notes.empty()) return;
        const int note = make(dlg_card::kComment, notes, key + "/note");
        g.connect(note, 0, target, port);
    };

    for (const DlgNode& n : script.nodes) {
        const int nodeId = make(dlg_card::kNode, {n.id}, n.id);
        nodeCard[n.id] = nodeId;
        notesFor(n.notes, n.id, nodeId, 0);
        int prev = nodeId;
        int prevPort = 0;
        int index = 0;
        for (const DlgLine& line : n.lines) {
            const std::string key = n.id + "/line" + std::to_string(index++);
            const int id = make(dlg_card::kLine, {line.speaker, line.text}, key);
            g.connect(prev, prevPort, id, kFlowIn);
            notesFor(line.notes, key, id, kFlowIn);
            if (!line.conditionSource.empty()) g.connect(make(dlg_card::kCondition, {line.conditionSource}, key + "/if"), 0, id, kIfIn);
            prev = id;
            prevPort = kNextOut;
        }
        index = 0;
        for (const DlgChoice& choice : n.choices) {
            const std::string key = n.id + "/choice" + std::to_string(index++);
            const int id = make(dlg_card::kChoice, {choice.text, choice.elseText}, key);
            g.connect(prev, prevPort, id, kFlowIn);
            notesFor(choice.notes, key, id, kFlowIn);
            if (!choice.conditionSource.empty()) g.connect(make(dlg_card::kCondition, {choice.conditionSource}, key + "/if"), 0, id, kIfIn);
            if (!choice.effects.empty()) {
                std::vector<std::string> sources;
                for (const Effect& effect : choice.effects) sources.push_back(effect.source);
                g.connect(make(dlg_card::kEffect, std::move(sources), key + "/do"), 0, id, kDoIn);
            }
            pendingTargets.emplace_back(id, choice.target);
            prev = id;
            prevPort = kNextOut;
        }
    }
    for (const auto& [choice, target] : pendingTargets) {
        const auto found = nodeCard.find(target);
        if (target != "END" && found != nodeCard.end()) g.connect(choice, kToOut, found->second, 0);
    }

    // Places: from the sidecar where it knows a card, else the layered layout.
    layoutRows(g);
    for (GraphNode& card : g.nodes()) {
        const auto it = layout.find(keyOf[card.id]);
        if (it != layout.end()) {
            card.x = it->second.first;
            card.y = it->second.second;
        }
    }
    return out;
}

namespace {

// The one walk over a graph that both the writer and the layout use: the script it stands for, and the file key of every card it reached.
struct Walk {
    DlgScript script;
    std::map<int, std::string> keys;
    std::vector<std::string> problems;
    bool ok = true;
};

Walk walkGraph(const DialogueGraph& dialogue) {
    Walk walk;
    walk.script = dialogue.header;
    walk.script.nodes.clear();
    const NodeGraph& g = dialogue.graph;
    std::set<int> used;
    std::set<std::string> ids;
    auto noteLines = [&](const GraphNode& target, const std::string& key, std::vector<std::string>& into) {
        int count = 0;
        for (const GraphNode* note : cardsInto(g, target.id, kFlowIn, dlg_card::kComment)) {
            used.insert(note->id);
            walk.keys[note->id] = key + "/note" + (count++ == 0 ? "" : std::to_string(count));
            for (const std::string& text : note->fields) into.push_back(text);
        }
    };
    for (const GraphNode& card : g.nodes()) {
        if (card.type != dlg_card::kNode) continue;
        used.insert(card.id);
        DlgNode node;
        node.id = field(card, 0);
        if (node.id.empty()) {
            walk.problems.push_back("error: a node card has no id");
            walk.ok = false;
            continue;
        }
        if (!ids.insert(node.id).second) {
            walk.problems.push_back("error: two node cards are called \"" + node.id + "\"");
            walk.ok = false;
            continue;
        }
        walk.keys[card.id] = node.id;
        // Notes above a node come into its port 0, like a flow input.
        for (const GraphNode* note : cardsInto(g, card.id, 0, dlg_card::kComment)) {
            used.insert(note->id);
            walk.keys[note->id] = node.id + "/note";
            for (const std::string& text : note->fields) node.notes.push_back(text);
        }
        std::set<int> seen;
        int index = 0;
        int choices = 0;
        for (const GraphNode* at = wireTarget(g, card.id, 0); at != nullptr; at = wireTarget(g, at->id, kNextOut)) {
            if (!seen.insert(at->id).second) {
                walk.problems.push_back("error: the chain of node \"" + node.id + "\" runs in a circle");
                walk.ok = false;
                break;
            }
            used.insert(at->id);
            const bool isLine = at->type == dlg_card::kLine;
            const bool isChoice = at->type == dlg_card::kChoice;
            if (!isLine && !isChoice) {
                walk.problems.push_back("error: node \"" + node.id + "\" leads to a " + at->type + " card; only lines and choices belong in a node");
                walk.ok = false;
                break;
            }
            const std::string key = node.id + (isLine ? "/line" : "/choice") + std::to_string(isLine ? index : choices);
            walk.keys[at->id] = key;
            const auto condition = cardsInto(g, at->id, kIfIn, dlg_card::kCondition);
            std::string conditionSource;
            if (!condition.empty()) {
                used.insert(condition.front()->id);
                walk.keys[condition.front()->id] = key + "/if";
                conditionSource = field(*condition.front(), 0);
            }
            if (isLine) {
                if (choices > 0) {
                    walk.problems.push_back("error: node \"" + node.id + "\" has a line after a choice; lines come first");
                    walk.ok = false;
                }
                DlgLine line;
                line.speaker = field(*at, 0);
                line.text = field(*at, 1);
                line.conditionSource = conditionSource;
                noteLines(*at, key, line.notes);
                node.lines.push_back(std::move(line));
                ++index;
            } else {
                DlgChoice choice;
                choice.text = field(*at, 0);
                choice.elseText = field(*at, 1);
                choice.conditionSource = conditionSource;
                noteLines(*at, key, choice.notes);
                int count = 0;
                for (const GraphNode* effect : cardsInto(g, at->id, kDoIn, dlg_card::kEffect)) {
                    used.insert(effect->id);
                    walk.keys[effect->id] = key + "/do" + (count++ == 0 ? "" : std::to_string(count));
                    for (const std::string& source : effect->fields) {
                        Effect e;
                        e.source = source;
                        choice.effects.push_back(std::move(e));
                    }
                }
                const GraphNode* to = wireTarget(g, at->id, kToOut);
                if (to == nullptr) {
                    choice.target = "END";
                } else if (to->type == dlg_card::kNode) {
                    choice.target = field(*to, 0);
                } else if (to->type == dlg_card::kGoto) {
                    used.insert(to->id);
                    walk.keys[to->id] = key + "/goto";
                    choice.target = field(*to, 0);
                } else {
                    walk.problems.push_back("error: a choice of node \"" + node.id + "\" leads to a " + to->type + " card");
                    walk.ok = false;
                }
                node.choices.push_back(std::move(choice));
                ++choices;
            }
        }
        walk.script.nodes.push_back(std::move(node));
    }
    for (const GraphNode& card : g.nodes()) {
        if (used.count(card.id) == 0) walk.problems.push_back("warning: the " + card.type + " card \"" + card.title + "\" is not connected, so it is not saved");
    }
    return walk;
}

} // namespace

std::optional<DlgScript> graphToDialogue(const DialogueGraph& dialogue, std::vector<std::string>& problems) {
    Walk walk = walkGraph(dialogue);
    problems.insert(problems.end(), walk.problems.begin(), walk.problems.end());
    if (!walk.ok) return std::nullopt;
    // The real parser has the last word: it checks every condition, effect, target and the limits, and its answer is what the game will read.
    sim::rules::LoadReport report;
    const std::string text = sim::rules::writeDialogue(walk.script);
    std::optional<DlgScript> parsed = sim::rules::parseDialogue(text, walk.script.name, walk.script.file, report);
    if (!parsed) {
        for (const auto& error : report.errors) problems.push_back("error: " + error.message + " (line " + std::to_string(error.line) + ")");
        return std::nullopt;
    }
    return parsed;
}

DialogueLayout layoutOf(const DialogueGraph& dialogue) {
    const Walk walk = walkGraph(dialogue);
    DialogueLayout layout;
    for (const auto& [id, key] : walk.keys) {
        if (const GraphNode* card = dialogue.graph.find(id)) layout[key] = {card->x, card->y};
    }
    return layout;
}

std::map<std::string, int> cardKeys(const DialogueGraph& dialogue) {
    std::map<std::string, int> keys;
    for (const auto& [id, key] : walkGraph(dialogue).keys) keys[key] = id;
    return keys;
}

std::string layoutToJson(const DialogueLayout& layout) {
    nlohmann::ordered_json cards = nlohmann::ordered_json::object();
    for (const auto& [key, place] : layout) cards[key] = nlohmann::ordered_json::array({place.first, place.second});
    nlohmann::ordered_json root;
    root["version"] = 1;
    root["cards"] = std::move(cards);
    return root.dump(2) + "\n";
}

DialogueLayout layoutFromJson(std::string_view text) {
    DialogueLayout layout;
    const nlohmann::json root = nlohmann::json::parse(text.begin(), text.end(), nullptr, false);
    if (!root.is_object() || !root.contains("cards") || !root["cards"].is_object()) return layout;
    for (const auto& [key, value] : root["cards"].items()) {
        if (value.is_array() && value.size() == 2 && value[0].is_number_integer() && value[1].is_number_integer()) {
            layout[key] = {value[0].get<int>(), value[1].get<int>()};
        }
    }
    return layout;
}

} // namespace odysseus::game
