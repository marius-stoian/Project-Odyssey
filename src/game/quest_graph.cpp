#include "game/quest_graph.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace odysseus::game {

namespace {

using luna::engine::GraphNode;
using luna::engine::GraphWire;
using luna::engine::NodeGraph;
using luna::engine::UiColor;
using sim::rules::Quest;
using sim::rules::QuestBranch;
using sim::rules::QuestCondition;
using sim::rules::QuestStep;

constexpr int kRequiresIn = 0;
constexpr int kFailIn = 1;
constexpr int kRewardsIn = 2;
constexpr int kBranchesIn = 1;

std::string clip(const std::string& text, std::size_t width) { return text.size() <= width ? text : text.substr(0, width - 2) + ".."; }
std::string field(const GraphNode& card, std::size_t index) { return index < card.fields.size() ? card.fields[index] : std::string(); }

const GraphNode* wireTarget(const NodeGraph& graph, int fromNode, int fromPort) {
    for (const GraphWire& wire : graph.wires()) {
        if (wire.fromNode == fromNode && wire.fromPort == fromPort) return graph.find(wire.toNode);
    }
    return nullptr;
}

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

// "120s" or "2m" as seconds; 0 when it is neither.
int secondsOf(const std::string& text) {
    if (text.size() < 2) return 0;
    long long n = 0;
    for (std::size_t i = 0; i + 1 < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return 0;
        n = n * 10 + (text[i] - '0');
        if (n > 100000) return 0;
    }
    if (text.back() == 's') return static_cast<int>(n);
    if (text.back() == 'm') return static_cast<int>(n * 60);
    return 0;
}

// The name a card goes by in the file: the step id, or a position among its kind.
std::vector<std::pair<int, std::string>> keysOf(const NodeGraph& g) {
    std::vector<std::pair<int, std::string>> keys;
    int needs = 0;
    int fail = 0;
    int rewards = 0;
    int ends = 0;
    std::map<int, int> branchCount; // by owning step card
    for (const GraphNode& card : g.nodes()) {
        if (card.type == quest_card::kQuest) keys.emplace_back(card.id, "quest");
        else if (card.type == quest_card::kStep) keys.emplace_back(card.id, "step:" + field(card, 0));
        else if (card.type == quest_card::kRequires) keys.emplace_back(card.id, "requires" + std::to_string(needs++));
        else if (card.type == quest_card::kFail) keys.emplace_back(card.id, "fail" + std::to_string(fail++));
        else if (card.type == quest_card::kRewards) keys.emplace_back(card.id, "rewards" + std::to_string(rewards++));
        else if (card.type == quest_card::kEnd) keys.emplace_back(card.id, "end" + std::to_string(ends++));
        else if (card.type == quest_card::kBranch) {
            const GraphNode* owner = wireTarget(g, card.id, 0);
            const std::string ownerKey = owner != nullptr ? field(*owner, 0) : std::string("loose");
            keys.emplace_back(card.id, "branch:" + ownerKey + ":" + std::to_string(branchCount[owner != nullptr ? owner->id : 0]++));
        }
    }
    return keys;
}

} // namespace

GraphNode newQuestCard(const std::string& type) {
    GraphNode card;
    card.type = type;
    if (type == quest_card::kQuest) {
        card.header = UiColor::Gold;
        card.inputs = 3;
        card.outputs = 1;
        card.fields = {"new-quest", "A new quest", "none", "", "", "", ""};
    } else if (type == quest_card::kStep) {
        card.header = UiColor::Selected;
        card.inputs = 2;
        card.outputs = 1;
        card.fields = {"step", "What the player must do.", "talk elder", "", "", ""};
    } else if (type == quest_card::kBranch) {
        card.header = UiColor::Hover;
        card.inputs = 0;
        card.outputs = 2;
        card.fields = {"flag(x)"};
    } else if (type == quest_card::kEnd) {
        card.header = UiColor::Dim;
        card.inputs = 1;
        card.outputs = 0;
    } else if (type == quest_card::kFail) {
        card.header = UiColor::Red;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"hero.dead"};
    } else if (type == quest_card::kRewards) {
        card.header = UiColor::Border;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"give hero flint 1"};
    } else { // needs
        card.type = quest_card::kRequires;
        card.header = UiColor::Grid;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"flag(x)"};
    }
    describeQuestCard(card);
    return card;
}

void describeQuestCard(GraphNode& card) {
    card.lines.clear();
    if (card.type == quest_card::kQuest) {
        card.title = "Quest " + clip(field(card, 0), 8);
        card.lines.push_back(clip(field(card, 1), 14));
        card.lines.push_back("giver " + clip(field(card, 2), 8));
    } else if (card.type == quest_card::kStep) {
        card.title = "Step " + clip(field(card, 0), 9);
        card.lines.push_back(clip(field(card, 2), 14));
        card.lines.push_back(clip(field(card, 1), 14));
    } else if (card.type == quest_card::kBranch) {
        card.title = "If";
        card.lines.push_back(clip(field(card, 0), 14));
    } else if (card.type == quest_card::kEnd) {
        card.title = "End";
        card.lines.push_back("quest done");
    } else {
        card.title = card.type == quest_card::kFail ? "Fails when" : card.type == quest_card::kRewards ? "Rewards" : "Requires";
        for (std::size_t i = 0; i < card.fields.size() && i < 3; ++i) card.lines.push_back(clip(card.fields[i], 14));
    }
}

NodeGraph questToGraph(const Quest& q, const DialogueLayout& layout) {
    NodeGraph g;
    std::map<int, std::string> keyOf;
    auto make = [&](const char* type, std::vector<std::string> fields, const std::string& key) {
        GraphNode card = newQuestCard(type);
        if (!fields.empty() || std::string(type) == quest_card::kEnd) card.fields = std::move(fields);
        describeQuestCard(card);
        const int id = g.add(std::move(card));
        keyOf[id] = key;
        return id;
    };
    const int header = make(quest_card::kQuest, {q.id, q.title, q.giver, q.note, q.journal, q.offer, q.turnIn}, "quest");
    std::map<std::string, int> stepCard;
    for (const QuestStep& s : q.steps) {
        stepCard[s.id] = make(quest_card::kStep, {s.id, s.text, s.objective.source, s.marker, s.hint ? std::to_string(s.hint->afterSeconds) + "s" : std::string(), s.hint ? s.hint->text : std::string()},
                              "step:" + s.id);
    }
    int endCard = 0;
    const auto targetOf = [&](const std::string& id) {
        if (id == sim::rules::kQuestEnd) {
            if (endCard == 0) endCard = make(quest_card::kEnd, {}, "end0");
            return endCard;
        }
        const auto it = stepCard.find(id);
        return it == stepCard.end() ? 0 : it->second;
    };
    if (const auto first = stepCard.find(q.start); first != stepCard.end()) g.connect(header, 0, first->second, 0);
    for (const QuestStep& s : q.steps) {
        const int card = stepCard[s.id];
        if (const int to = targetOf(s.next); to != 0) g.connect(card, 0, to, 0);
        for (std::size_t b = 0; b < s.branches.size(); ++b) {
            const int branch = make(quest_card::kBranch, {s.branches[b].condition.source}, "branch:" + s.id + ":" + std::to_string(b));
            g.connect(branch, 0, card, kBranchesIn);
            if (const int to = targetOf(s.branches[b].to); to != 0) g.connect(branch, 1, to, 0);
        }
    }
    const auto list = [&](const char* type, std::vector<std::string> lines, int port, const std::string& key) {
        if (!lines.empty()) g.connect(make(type, std::move(lines), key), 0, header, port);
    };
    std::vector<std::string> needs;
    for (const QuestCondition& c : q.requires_) needs.push_back(c.source);
    std::vector<std::string> fail;
    for (const QuestCondition& c : q.fail) fail.push_back(c.source);
    std::vector<std::string> rewards;
    for (const sim::rules::Effect& e : q.rewards) rewards.push_back(e.source);
    list(quest_card::kRequires, std::move(needs), kRequiresIn, "requires0");
    list(quest_card::kFail, std::move(fail), kFailIn, "fail0");
    list(quest_card::kRewards, std::move(rewards), kRewardsIn, "rewards0");

    luna::engine::layoutLayers(g);
    for (GraphNode& card : g.nodes()) {
        const auto it = layout.find(keyOf[card.id]);
        if (it != layout.end()) {
            card.x = it->second.first;
            card.y = it->second.second;
        }
    }
    return g;
}

std::optional<Quest> graphToQuest(const NodeGraph& g, std::vector<std::string>& problems, std::string& json) {
    const GraphNode* header = nullptr;
    int headers = 0;
    for (const GraphNode& card : g.nodes()) {
        if (card.type == quest_card::kQuest) {
            ++headers;
            if (header == nullptr) header = &card;
        }
    }
    if (headers != 1) {
        problems.push_back("error: a quest needs exactly one quest card (this graph has " + std::to_string(headers) + ")");
        return std::nullopt;
    }
    bool ok = true;
    Quest q;
    q.id = field(*header, 0);
    q.title = field(*header, 1);
    q.giver = field(*header, 2).empty() ? "none" : field(*header, 2);
    q.note = field(*header, 3);
    q.journal = field(*header, 4);
    q.offer = field(*header, 5);
    q.turnIn = field(*header, 6);

    std::set<int> used{header->id};
    std::set<std::string> seen;
    const auto idOf = [&](const GraphNode* target) -> std::string {
        if (target == nullptr) return {};
        if (target->type == quest_card::kEnd) {
            used.insert(target->id);
            return sim::rules::kQuestEnd;
        }
        return target->type == quest_card::kStep ? field(*target, 0) : std::string();
    };
    for (const GraphNode& card : g.nodes()) {
        if (card.type != quest_card::kStep) continue;
        used.insert(card.id);
        QuestStep s;
        s.id = field(card, 0);
        if (s.id.empty() || !seen.insert(s.id).second) {
            problems.push_back("error: step id \"" + s.id + "\" is empty or used twice");
            ok = false;
            continue;
        }
        s.text = field(card, 1);
        s.objective.source = field(card, 2);
        s.marker = field(card, 3);
        if (!field(card, 4).empty() || !field(card, 5).empty()) {
            const int after = secondsOf(field(card, 4));
            if (after <= 0) {
                problems.push_back("error: step \"" + s.id + "\": the hint time \"" + field(card, 4) + "\" must be like 120s or 2m");
                ok = false;
            } else {
                s.hint = sim::rules::QuestHint{after, field(card, 5)};
            }
        }
        s.next = idOf(wireTarget(g, card.id, 0));
        if (s.next.empty()) {
            problems.push_back("error: step \"" + s.id + "\" leads nowhere: wire it to a step or an end card");
            ok = false;
        }
        for (const GraphNode* branch : cardsInto(g, card.id, kBranchesIn, quest_card::kBranch)) {
            used.insert(branch->id);
            QuestBranch b;
            b.condition.source = field(*branch, 0);
            b.to = idOf(wireTarget(g, branch->id, 1));
            if (b.to.empty()) {
                problems.push_back("error: a branch of step \"" + s.id + "\" leads nowhere");
                ok = false;
                continue;
            }
            s.branches.push_back(std::move(b));
        }
        q.steps.push_back(std::move(s));
    }
    q.start = idOf(wireTarget(g, header->id, 0));
    if (q.start.empty() || q.start == sim::rules::kQuestEnd) {
        problems.push_back("error: the quest card leads to no first step");
        ok = false;
    }
    const auto lines = [&](int port, const char* type) {
        std::vector<std::string> all;
        for (const GraphNode* card : cardsInto(g, header->id, port, type)) {
            used.insert(card->id);
            for (const std::string& line : card->fields) {
                if (!line.empty()) all.push_back(line);
            }
        }
        return all;
    };
    for (const std::string& line : lines(kRequiresIn, quest_card::kRequires)) q.requires_.push_back({line, nullptr, 0});
    for (const std::string& line : lines(kFailIn, quest_card::kFail)) q.fail.push_back({line, nullptr, 0});
    for (const std::string& line : lines(kRewardsIn, quest_card::kRewards)) {
        sim::rules::Effect e;
        e.source = line;
        q.rewards.push_back(std::move(e));
    }
    for (const GraphNode& card : g.nodes()) {
        if (used.count(card.id) == 0) problems.push_back("warning: the " + card.type + " card \"" + card.title + "\" is not connected, so it is not saved");
    }
    if (!ok) return std::nullopt;
    // The real loader has the last word, as for dialogue.
    const std::string text = sim::rules::writeQuest(q);
    sim::rules::LoadReport report;
    std::optional<Quest> parsed = sim::rules::parseQuest(text, "quests/" + q.id + ".json", report, q.id);
    if (!parsed) {
        for (const auto& error : report.errors) problems.push_back("error: " + error.message);
        return std::nullopt;
    }
    json = sim::rules::writeQuest(*parsed);
    return parsed;
}

void arrangeQuest(NodeGraph& graph) { luna::engine::layoutLayers(graph); }

std::map<std::string, int> questCardKeys(const NodeGraph& g) {
    std::map<std::string, int> keys;
    for (const auto& [id, key] : keysOf(g)) keys[key] = id;
    return keys;
}

DialogueLayout questLayoutOf(const NodeGraph& g) {
    DialogueLayout layout;
    for (const auto& [id, key] : keysOf(g)) {
        if (const GraphNode* card = g.find(id)) layout[key] = {card->x, card->y};
    }
    return layout;
}

} // namespace odysseus::game
