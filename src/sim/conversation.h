#pragma once

#include "boundary.h"

#include "sim/action_runner.h"
#include "sim/dialogue_script.h"
#include "sim/needs.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// A conversation under way (US-161, SDC-03, design docs/plans/M8-dialogue-design.md section 4): one script, the node the talk is at, and
// what to show now. The script is copied in (expressions are shared, so this is cheap), which means a reload of the data files while a
// panel is open cannot pull the words out from under the player.

struct ConversationLine {
    std::string speaker;
    std::string text; // tokens already filled
};

struct ConversationChoice {
    int index = 0;        // its place in the node, kept so the Editor and the tests can point at it
    std::string text;
    bool enabled = true;  // false: shown greyed out with `reason` (the script's [else ...])
    std::string reason;
};

struct ConversationView {
    std::vector<ConversationLine> lines;       // what the NPC says now: every line of the node whose condition holds, in order
    std::vector<ConversationChoice> choices;   // at most kMaxVisibleChoices; a choice whose condition fails and has no [else] is not here
};

class Conversation {
public:
    // `actor` is who talks (0 is the hero), `target` is the NPC the effects are aimed at.
    Conversation(DlgScript script, int actor, ThingRef target);

    const DlgScript& script() const { return script_; }
    const std::string& nodeId() const { return node_; }
    const ThingRef& target() const { return target_; }
    bool finished() const { return finished_; }

    // What the panel shows at this node. Conditions are asked of `context` each time, so the screen can never show a stale choice.
    ConversationView view(const RuleContext& context) const;

    // The player picks the n-th choice of view() (0-based). Its effects run in order through the runner (so `after 5s ...` waits as it
    // does for an interaction), then the talk moves to the choice's node, or ends at END. False when there is no such choice or it is
    // greyed out; nothing happens then.
    bool choose(int visibleIndex, const RuleContext& context, ActionRunner& runner, std::int64_t now, EffectHost& host);

    // The player walks away (Esc): the talk ends, no effects.
    void leave() { finished_ = true; }

    // Where `{smalltalk.hunt}` gets its line (US-163): asked once per token each time a node is entered, so what is said does not change from one
    // frame to the next. Without a source the token stays as written.
    using SmalltalkSource = std::function<std::string(const std::string& topic)>;
    void setSmalltalk(SmalltalkSource source) { smalltalk_ = std::move(source); }

private:
    DlgScript script_;
    int actor_ = 0;
    ThingRef target_;
    std::string node_;
    bool finished_ = false;
    SmalltalkSource smalltalk_;
    mutable std::map<std::string, std::string> spoken_; // the {smalltalk.topic} lines of the node the talk is at
};

// The conversation a person has when no script was written for them (US-163): their line, then a friendly answer and a rude one (D-38).
// The rude one costs 10 opinion and leaves a bad memory (-40).
DlgScript smalltalkScript(const std::string& speaker, const std::string& line);

// The one word the panel shows next to the NPC's name (D-38): how they feel about the hero, unless a need is pressing and they are not
// hostile. Opinion is -100..100; each need is 0 (empty) to 100 (full), as in Needs.
std::string moodWord(int opinion, const Needs& needs);

// Fills {hero}, {npc} and {npc.name} (the hero's name and the NPC's name), then the other tokens of the rule language ({target.name}).
std::string fillDialogueTokens(const std::string& text, const RuleContext& context);

} // namespace odysseus::sim::rules
