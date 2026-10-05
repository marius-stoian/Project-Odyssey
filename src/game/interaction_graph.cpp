#include "game/interaction_graph.h"

#include <algorithm>
#include <charconv>
#include <cstdlib>
#include <set>
#include <sstream>

namespace odysseus::game {

namespace {

using luna::engine::GraphNode;
using luna::engine::GraphWire;
using luna::engine::NodeGraph;
using luna::engine::UiColor;
using sim::rules::Effect;
using sim::rules::Interaction;
using sim::rules::NpcRule;
using sim::rules::Requirement;

constexpr int kActorOut = 0;
constexpr int kTargetOut = 0;
constexpr int kRequiresIn = 1;
constexpr int kDoIn = 2;
constexpr int kNpcIn = 3;
constexpr int kChronicleIn = 4;

std::string clip(const std::string& text, std::size_t width) { return text.size() <= width ? text : text.substr(0, width - 2) + ".."; }
std::string field(const GraphNode& card, std::size_t index) { return index < card.fields.size() ? card.fields[index] : std::string(); }

std::string joinWords(const std::vector<std::string>& words) {
    std::string out;
    for (const std::string& word : words) out += (out.empty() ? "" : " ") + word;
    return out;
}

std::vector<std::string> splitWords(const std::string& text) {
    std::vector<std::string> words;
    std::istringstream in(text);
    for (std::string word; in >> word;) words.push_back(word);
    return words;
}

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

struct Parts {
    const GraphNode* verb = nullptr;
    const GraphNode* actor = nullptr;
    const GraphNode* target = nullptr;
    std::vector<const GraphNode*> requirements;
    std::vector<const GraphNode*> effects;
    const GraphNode* npc = nullptr;
    const GraphNode* chronicle = nullptr;
};

// The cards that belong to the verb, by the wires; the one place both the writer and the layout look at the graph.
Parts partsOf(const NodeGraph& g, std::vector<std::string>* problems) {
    Parts p;
    int verbs = 0;
    for (const GraphNode& card : g.nodes()) {
        if (card.type == rule_card::kVerb) {
            ++verbs;
            if (p.verb == nullptr) p.verb = &card;
        }
    }
    if (verbs != 1 && problems != nullptr) problems->push_back("error: an interaction needs exactly one verb card (this graph has " + std::to_string(verbs) + ")");
    if (p.verb == nullptr) return p;
    const auto actors = cardsInto(g, p.verb->id, 0, rule_card::kActor);
    p.actor = actors.empty() ? nullptr : actors.front();
    p.target = wireTarget(g, p.verb->id, kTargetOut);
    if (p.target != nullptr && p.target->type != rule_card::kTarget) {
        if (problems != nullptr) problems->push_back("error: the verb leads to a " + p.target->type + " card, not a target card");
        p.target = nullptr;
    }
    p.requirements = cardsInto(g, p.verb->id, kRequiresIn, rule_card::kRequirement);
    p.effects = cardsInto(g, p.verb->id, kDoIn, rule_card::kEffects);
    const auto npc = cardsInto(g, p.verb->id, kNpcIn, rule_card::kNpcRule);
    p.npc = npc.empty() ? nullptr : npc.front();
    const auto chronicle = cardsInto(g, p.verb->id, kChronicleIn, rule_card::kChronicle);
    p.chronicle = chronicle.empty() ? nullptr : chronicle.front();
    return p;
}

// The stable name of each card in the file, for the layout sidecar.
std::vector<std::pair<int, std::string>> keysOf(const Parts& p) {
    std::vector<std::pair<int, std::string>> keys;
    if (p.verb == nullptr) return keys;
    keys.emplace_back(p.verb->id, "verb");
    if (p.actor != nullptr) keys.emplace_back(p.actor->id, "actor");
    if (p.target != nullptr) keys.emplace_back(p.target->id, "target");
    for (std::size_t i = 0; i < p.requirements.size(); ++i) keys.emplace_back(p.requirements[i]->id, "requirement" + std::to_string(i));
    for (std::size_t i = 0; i < p.effects.size(); ++i) keys.emplace_back(p.effects[i]->id, "effects" + std::to_string(i));
    if (p.npc != nullptr) keys.emplace_back(p.npc->id, "npcrule");
    if (p.chronicle != nullptr) keys.emplace_back(p.chronicle->id, "chronicle");
    return keys;
}

// Actor, verb and target on one row; what hangs off the verb stacked above (requirements) and below (effects, the NPC rule, the chronicle line).
void layoutRow(NodeGraph& g) {
    constexpr int kGap = 20;
    constexpr int kAuxGap = 4;
    const Parts p = partsOf(g, nullptr);
    if (p.verb == nullptr) return;
    int above = 0;
    for (const GraphNode* c : p.requirements) above += c->height() + kAuxGap;
    const int rowY = above;
    const int verbX = luna::engine::kGraphNodeWidth + kGap;
    g.find(p.verb->id)->x = verbX;
    g.find(p.verb->id)->y = rowY;
    if (p.actor != nullptr) {
        g.find(p.actor->id)->x = 0;
        g.find(p.actor->id)->y = rowY;
    }
    if (p.target != nullptr) {
        g.find(p.target->id)->x = 2 * verbX;
        g.find(p.target->id)->y = rowY;
    }
    int top = rowY;
    for (const GraphNode* c : p.requirements) {
        top -= c->height() + kAuxGap;
        g.find(c->id)->x = verbX;
        g.find(c->id)->y = top;
    }
    int below = rowY + p.verb->height() + kGap;
    auto place = [&](const GraphNode* c) {
        if (c == nullptr) return;
        g.find(c->id)->x = verbX;
        g.find(c->id)->y = below;
        below += c->height() + kAuxGap;
    };
    for (const GraphNode* c : p.effects) place(c);
    place(p.npc);
    place(p.chronicle);
}

} // namespace

std::string milliToText(int milli) {
    std::string out = std::to_string(milli / 1000);
    const int fraction = std::abs(milli) % 1000;
    if (fraction != 0) {
        std::string digits = std::to_string(fraction + 1000).substr(1);
        while (!digits.empty() && digits.back() == '0') digits.pop_back();
        out += "." + digits;
    }
    return out;
}

std::optional<int> textToMilli(const std::string& text) {
    if (text.empty()) return std::nullopt;
    std::size_t at = 0;
    bool negative = false;
    if (text[0] == '-') {
        negative = true;
        at = 1;
    }
    long long whole = 0;
    std::size_t digits = 0;
    while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
        whole = whole * 10 + (text[at] - '0');
        if (whole > 1000000) return std::nullopt;
        ++at;
        ++digits;
    }
    if (digits == 0) return std::nullopt;
    long long milli = whole * 1000;
    if (at < text.size()) {
        if (text[at] != '.') return std::nullopt;
        ++at;
        int scale = 100;
        std::size_t fractionDigits = 0;
        while (at < text.size() && text[at] >= '0' && text[at] <= '9') {
            if (scale == 0) return std::nullopt; // more than three decimals
            milli += (text[at] - '0') * scale;
            scale /= 10;
            ++at;
            ++fractionDigits;
        }
        if (at != text.size() || fractionDigits == 0) return std::nullopt;
    }
    return static_cast<int>(negative ? -milli : milli);
}

std::string leadingComments(const std::string& text) {
    std::string out;
    std::istringstream in(text);
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            if (out.empty()) continue;
            break;
        }
        if (line.compare(start, 2, "//") != 0) break;
        out += line + "\n";
    }
    return out;
}

GraphNode newRuleCard(const std::string& type) {
    GraphNode card;
    card.type = type;
    if (type == rule_card::kActor) {
        card.header = UiColor::Selected;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"hero person"};
    } else if (type == rule_card::kVerb) {
        card.header = UiColor::Gold;
        card.inputs = 5;
        card.outputs = 1;
        card.fields = {"verb", "Do it", "", "1.5", "0", "100", ""};
    } else if (type == rule_card::kTarget) {
        card.header = UiColor::Hover;
        card.inputs = 1;
        card.outputs = 0;
        card.fields = {"edible", ""};
    } else if (type == rule_card::kRequirement) {
        card.header = UiColor::Red;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"target.state == ripe", "Not yet"};
    } else if (type == rule_card::kEffects) {
        card.header = UiColor::Border;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"set target.state picked"};
    } else if (type == rule_card::kNpcRule) {
        card.header = UiColor::Grid;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"need(hunger) * 2", "60"};
    } else { // chronicle
        card.type = rule_card::kChronicle;
        card.header = UiColor::Dim;
        card.inputs = 0;
        card.outputs = 1;
        card.fields = {"{actor.name} did it"};
    }
    describeRuleCard(card);
    return card;
}

void describeRuleCard(GraphNode& card) {
    card.lines.clear();
    if (card.type == rule_card::kActor) {
        card.title = "Actor";
        card.lines.push_back(clip(field(card, 0), 14));
    } else if (card.type == rule_card::kVerb) {
        card.title = "Verb " + clip(field(card, 0), 9);
        card.lines.push_back(clip(field(card, 1), 14));
        card.lines.push_back("range " + clip(field(card, 3), 7));
    } else if (card.type == rule_card::kTarget) {
        card.title = "Target";
        card.lines.push_back(clip(field(card, 0), 14));
        if (!field(card, 1).empty()) card.lines.push_back(clip("kind " + field(card, 1), 14));
    } else if (card.type == rule_card::kRequirement) {
        card.title = "Needs";
        card.lines.push_back(clip(field(card, 0), 14));
    } else if (card.type == rule_card::kEffects) {
        card.title = "Effects";
        for (std::size_t i = 0; i < card.fields.size() && i < 3; ++i) card.lines.push_back(clip(card.fields[i], 14));
    } else if (card.type == rule_card::kNpcRule) {
        card.title = "NPC rule";
        card.lines.push_back(clip(field(card, 0), 14));
    } else {
        card.title = "Chronicle";
        card.lines.push_back(clip(field(card, 0), 14));
    }
}

NodeGraph interactionToGraph(const Interaction& i, const DialogueLayout& layout) {
    NodeGraph g;
    std::map<int, std::string> keyOf;
    auto make = [&](const char* type, std::vector<std::string> fields, const std::string& key) {
        GraphNode card = newRuleCard(type);
        card.fields = std::move(fields);
        describeRuleCard(card);
        const int id = g.add(std::move(card));
        keyOf[id] = key;
        return id;
    };
    const int verb = make(rule_card::kVerb, {i.id, i.label, i.note, milliToText(i.rangeMilli), milliToText(i.durationMilli), std::to_string(i.order), i.menu}, "verb");
    const int actor = make(rule_card::kActor, {joinWords(i.actors)}, "actor");
    g.connect(actor, kActorOut, verb, 0);
    const int target = make(rule_card::kTarget, {joinWords(i.targetTags), joinWords(i.targetKinds)}, "target");
    g.connect(verb, kTargetOut, target, 0);
    for (std::size_t k = 0; k < i.requires_.size(); ++k) {
        g.connect(make(rule_card::kRequirement, {i.requires_[k].source, i.requires_[k].otherwise}, "requirement" + std::to_string(k)), 0, verb, kRequiresIn);
    }
    if (!i.effects.empty()) {
        std::vector<std::string> sources;
        for (const Effect& effect : i.effects) sources.push_back(effect.source);
        g.connect(make(rule_card::kEffects, std::move(sources), "effects0"), 0, verb, kDoIn);
    }
    if (i.npc) g.connect(make(rule_card::kNpcRule, {i.npc->scoreSource, std::to_string(i.npc->cooldownSeconds)}, "npcrule"), 0, verb, kNpcIn);
    if (i.chronicle) g.connect(make(rule_card::kChronicle, {*i.chronicle}, "chronicle"), 0, verb, kChronicleIn);

    layoutRow(g);
    for (GraphNode& card : g.nodes()) {
        const auto it = layout.find(keyOf[card.id]);
        if (it != layout.end()) {
            card.x = it->second.first;
            card.y = it->second.second;
        }
    }
    return g;
}

std::optional<Interaction> graphToInteraction(const NodeGraph& g, std::vector<std::string>& problems, std::string& json) {
    const Parts p = partsOf(g, &problems);
    if (p.verb == nullptr) return std::nullopt;
    std::set<int> used{p.verb->id};
    for (const auto& [id, key] : keysOf(p)) used.insert(id);
    Interaction i;
    bool ok = problems.empty();
    i.id = field(*p.verb, 0);
    i.label = field(*p.verb, 1);
    i.note = field(*p.verb, 2);
    const auto range = textToMilli(field(*p.verb, 3));
    const auto duration = textToMilli(field(*p.verb, 4));
    if (!range || *range < 0) {
        problems.push_back("error: the range \"" + field(*p.verb, 3) + "\" is not a number of metres");
        ok = false;
    } else {
        i.rangeMilli = *range;
    }
    if (!duration || *duration < 0) {
        problems.push_back("error: the duration \"" + field(*p.verb, 4) + "\" is not a number of seconds");
        ok = false;
    } else {
        i.durationMilli = *duration;
    }
    const std::string order = field(*p.verb, 5);
    char* end = nullptr;
    const long parsedOrder = std::strtol(order.c_str(), &end, 10);
    if (order.empty() || end == nullptr || *end != '\0') {
        problems.push_back("error: the order \"" + order + "\" is not a whole number");
        ok = false;
    } else {
        i.order = static_cast<int>(parsedOrder);
    }
    i.menu = field(*p.verb, 6);
    if (p.actor == nullptr) {
        problems.push_back("error: the verb has no actor card");
        ok = false;
    } else {
        i.actors = splitWords(field(*p.actor, 0));
    }
    if (p.target == nullptr) {
        problems.push_back("error: the verb leads to no target card");
        ok = false;
    } else {
        i.targetTags = splitWords(field(*p.target, 0));
        i.targetKinds = splitWords(field(*p.target, 1));
    }
    for (const GraphNode* card : p.requirements) {
        Requirement r;
        r.source = field(*card, 0);
        r.otherwise = field(*card, 1);
        i.requires_.push_back(std::move(r));
    }
    for (const GraphNode* card : p.effects) {
        for (const std::string& source : card->fields) {
            Effect e;
            e.source = source;
            i.effects.push_back(std::move(e));
        }
    }
    if (p.npc != nullptr) {
        NpcRule rule;
        rule.scoreSource = field(*p.npc, 0);
        const std::string cooldown = field(*p.npc, 1);
        char* cooldownEnd = nullptr;
        rule.cooldownSeconds = static_cast<int>(std::strtol(cooldown.c_str(), &cooldownEnd, 10));
        if (cooldown.empty() || cooldownEnd == nullptr || *cooldownEnd != '\0') {
            problems.push_back("error: the NPC cooldown \"" + cooldown + "\" is not a whole number of seconds");
            ok = false;
        }
        i.npc = std::move(rule);
    }
    if (p.chronicle != nullptr) i.chronicle = field(*p.chronicle, 0);
    for (const GraphNode& card : g.nodes()) {
        if (used.count(card.id) == 0) problems.push_back("warning: the " + card.type + " card \"" + card.title + "\" is not connected, so it is not saved");
    }
    if (!ok) return std::nullopt;
    // The real loader has the last word, as for dialogue.
    const std::string text = sim::rules::toJson(i);
    sim::rules::LoadReport report;
    std::optional<Interaction> parsed = sim::rules::InteractionRegistry::parse(text, "interactions/" + i.id + ".json", report, i.id);
    if (!parsed) {
        for (const auto& error : report.errors) problems.push_back("error: " + error.message);
        return std::nullopt;
    }
    json = sim::rules::toJson(*parsed);
    return parsed;
}

DialogueLayout interactionLayoutOf(const NodeGraph& g) {
    DialogueLayout layout;
    for (const auto& [id, key] : keysOf(partsOf(g, nullptr))) {
        if (const GraphNode* card = g.find(id)) layout[key] = {card->x, card->y};
    }
    return layout;
}

} // namespace odysseus::game
