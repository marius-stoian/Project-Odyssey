#pragma once

#include "boundary.h"

#include "sim/interaction.h"
#include "sim/rule_effect.h"
#include "sim/rule_expr.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// A conversation written as plain text (US-160, SDC-04): `assets/data/dialogue/<name>.dlg`. The format is documented in
// docs/guides/dialogue-format.md. Conditions and effects are the language of the interaction files (ADR-019).

// One thing the NPC says. It is shown when its condition holds (or it has none).
struct DlgLine {
    std::string speaker;
    std::string text; // with {tokens}
    std::string conditionSource;
    ExprPtr condition;
    std::vector<std::string> notes; // `#` lines just before it, kept
    int line = 0;
};

// One thing the player may answer. A false condition hides it, or greys it out with `elseText` when there is one.
struct DlgChoice {
    std::string text;
    std::string conditionSource;
    ExprPtr condition;
    std::string elseText;
    std::vector<Effect> effects; // run when it is chosen
    std::string target;          // a node id, or "END"
    std::vector<std::string> notes;
    int line = 0;
};

struct DlgNode {
    std::string id;
    std::vector<DlgLine> lines;
    std::vector<DlgChoice> choices;
    std::vector<std::string> notes;
    int line = 0;
};

struct DlgScript {
    std::string name;                // the file name without `.dlg`
    std::string file;                // "dialogue/elder-fire.dlg"
    std::vector<std::string> who;    // @who: a placed character's id or name, a role, or a kind
    std::string whenSource;          // @when
    ExprPtr when;
    int priority = 0;                // @priority
    std::string bark;                // @bark: this is a short greeting (US-162)
    std::vector<std::string> pair;   // @pair a b: for two NPCs talking to each other (US-165)
    std::vector<std::string> headerNotes; // `#` lines before the first header or node
    std::vector<DlgNode> nodes;
    std::vector<std::string> endNotes; // `#` lines after the last element
    // The node a conversation begins at: "start" when there is one, else the first.
    const DlgNode* startNode() const;
    const DlgNode* find(std::string_view id) const;
};

constexpr int kMaxVisibleChoices = 5; // D-38: the panel shows at most 5 numbered choices

// Reads one script. Mistakes are added to `report` as "file:line: message"; the result is empty when there are any.
std::optional<DlgScript> parseDialogue(std::string_view text, const std::string& name, const std::string& file, LoadReport& report);

// The canonical text of a script: what the Editor saves. Reading it again gives the same script, and a shipped file is kept in this form.
std::string writeDialogue(const DlgScript& script);

// Every `.dlg` of a folder, each on its own (one with mistakes is left out and listed).
class DialogueLibrary {
public:
    static DialogueLibrary load(const std::filesystem::path& folder, LoadReport& report);
    const std::vector<DlgScript>& all() const { return scripts_; }
    const DlgScript* find(std::string_view name) const;

private:
    std::vector<DlgScript> scripts_; // sorted by name
};

} // namespace odysseus::sim::rules
