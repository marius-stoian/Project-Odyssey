#include "sim/conversation.h"

#include "sim/interaction.h"

#include <format>

namespace odysseus::sim::rules {

namespace {

// Opinion bands of the mood word. Plain numbers here, because a mood is a way of reading a number for the panel, not a rule of the world.
constexpr int kWarm = 40;
constexpr int kFriendly = 15;
constexpr int kWary = -15;
constexpr int kHostile = -40;
// A need this low (of 100) is pressing enough to show instead of the feeling about the hero.
constexpr int kPressing = 25;

std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) text.replace(at, from.size(), to);
    return text;
}

std::string textOf(const Value& value) { return value.isText ? value.text : std::format("{}", value.number); }

bool holds(const ExprPtr& condition, const RuleContext& context) { return condition == nullptr || isTrue(*condition, context); }

} // namespace

Conversation::Conversation(DlgScript script, int actor, ThingRef target) : script_(std::move(script)), actor_(actor), target_(target) {
    if (const DlgNode* start = script_.startNode()) node_ = start->id;
    else finished_ = true;
}

std::string fillDialogueTokens(const std::string& text, const RuleContext& context) {
    std::string filled = replaceAll(text, "{hero}", textOf(context.path("hero.name")));
    filled = replaceAll(filled, "{npc}", textOf(context.path("npc.name")));
    return fillTokens(filled, context);
}

ConversationView Conversation::view(const RuleContext& context) const {
    ConversationView out;
    // `{smalltalk.topic}` is asked of the source once for each node entered; the answer is kept until the talk moves on.
    const auto speak = [&](const std::string& text) {
        std::string result = text;
        for (std::size_t at = result.find("{smalltalk."); smalltalk_ && at != std::string::npos; at = result.find("{smalltalk.", at)) {
            const std::size_t close = result.find('}', at);
            if (close == std::string::npos) break;
            const std::string topic = result.substr(at + 11, close - at - 11);
            auto found = spoken_.find(topic);
            if (found == spoken_.end()) found = spoken_.emplace(topic, smalltalk_(topic)).first;
            result.replace(at, close - at + 1, found->second);
            at += found->second.size();
        }
        return fillDialogueTokens(result, context);
    };
    const DlgNode* node = finished_ ? nullptr : script_.find(node_);
    if (node == nullptr) return out;
    for (const DlgLine& line : node->lines) {
        if (holds(line.condition, context)) out.lines.push_back({line.speaker, speak(line.text)});
    }
    for (std::size_t i = 0; i < node->choices.size() && static_cast<int>(out.choices.size()) < kMaxVisibleChoices; ++i) {
        const DlgChoice& choice = node->choices[i];
        if (holds(choice.condition, context)) {
            out.choices.push_back({static_cast<int>(i), speak(choice.text), true, {}});
        } else if (!choice.elseText.empty()) {
            out.choices.push_back({static_cast<int>(i), speak(choice.text), false, speak(choice.elseText)});
        } // else: hidden
    }
    return out;
}

bool Conversation::choose(int visibleIndex, const RuleContext& context, ActionRunner& runner, std::int64_t now, EffectHost& host) {
    if (finished_) return false;
    const DlgNode* node = script_.find(node_);
    if (node == nullptr) return false;
    const ConversationView shown = view(context);
    if (visibleIndex < 0 || static_cast<std::size_t>(visibleIndex) >= shown.choices.size() || !shown.choices[static_cast<std::size_t>(visibleIndex)].enabled) return false;
    // Copy the choice first: an effect may change the world, and nothing in the script changes, but the node is read again below.
    const DlgChoice choice = node->choices[static_cast<std::size_t>(shown.choices[static_cast<std::size_t>(visibleIndex)].index)];
    runner.runEffects(choice.effects, actor_, target_, now, host);
    if (choice.target == "END" || script_.find(choice.target) == nullptr) {
        finished_ = true;
    } else {
        node_ = choice.target;
        spoken_.clear(); // a node entered again says something new
    }
    return true;
}

DlgScript smalltalkScript(const std::string& speaker, const std::string& line) {
    DlgScript script;
    script.name = "smalltalk";
    script.file = "smalltalk";
    DlgNode node;
    node.id = "start";
    node.lines.push_back({speaker, line, {}, nullptr, {}, 0});
    DlgChoice friendly;
    friendly.text = "Thank you";
    friendly.target = "END";
    DlgChoice rude;
    rude.text = "Be quiet";
    rude.target = "END";
    const ParsedEffect insult = parseEffect("opinion npc hero -10");
    if (!insult.problem) rude.effects.push_back(insult.effect);
    node.choices.push_back(std::move(friendly));
    node.choices.push_back(std::move(rude));
    script.nodes.push_back(std::move(node));
    return script;
}

std::string moodWord(int opinion, const Needs& needs) {
    if (opinion <= kHostile) return "hostile";
    // The most pressing need, the lowest one; ties go to the first in the list (a total order, so the word never flickers).
    int worst = -1;
    for (std::size_t n = 0; n < kNeedCount; ++n) {
        if (needs.values[n] < kPressing && (worst < 0 || needs.values[n] < needs.values[static_cast<std::size_t>(worst)])) worst = static_cast<int>(n);
    }
    if (worst >= 0) {
        switch (static_cast<Need>(worst)) {
        case Need::Hunger: return "hungry";
        case Need::Energy: return "tired";
        case Need::Warmth: return "cold";
        case Need::Social: return "lonely";
        default: break;
        }
    }
    if (opinion >= kWarm) return "warm";
    if (opinion >= kFriendly) return "friendly";
    if (opinion > kWary) return "neutral";
    return "wary";
}

} // namespace odysseus::sim::rules
