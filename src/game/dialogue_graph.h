#pragma once

#include "boundary.h"

#include "luna/engine/node_graph.h"
#include "sim/dialogue_script.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// A conversation as a graph (US-171, SDC-06, D-56): the cards of a `.dlg` file as nodes and wires, and back. The graph is only a view; the file
// stays the one truth, written by `sim::rules::writeDialogue` in its canonical form with every `#` note kept.
//
// Cards (the `type` of a graph node) and what they keep in `fields`:
//   node       fields {id}                  in 0 "from", out 0 "first" (the chain of lines then choices)
//   line       fields {speaker, text}       in 0 flow, in 1 "if"; out 0 next
//   choice     fields {text, elseText}      in 0 flow, in 1 "if", in 2 "do"; out 0 next choice, out 1 "to" (a node card, or a goto card; no wire = END)
//   condition  fields {expression}          out 0 -> an "if" port
//   effect     fields {one effect each...}  out 0 -> a "do" port
//   goto       fields {target node id}      in 0, out 0 -> a node card (a long wire saved as a name)
//   comment    fields {one note line each}  out 0 -> the flow port or node port of what the note is above
namespace dlg_card {
inline constexpr const char* kNode = "node";
inline constexpr const char* kLine = "line";
inline constexpr const char* kChoice = "choice";
inline constexpr const char* kCondition = "condition";
inline constexpr const char* kEffect = "effect";
inline constexpr const char* kGoto = "goto";
inline constexpr const char* kComment = "comment";
} // namespace dlg_card

// Where the owner left each card: keyed by what the card is in the file ("start/line0", "start/choice1/do"), not by an id that changes.
using DialogueLayout = std::map<std::string, std::pair<int, int>>;

struct DialogueGraph {
    luna::engine::NodeGraph graph;
    sim::rules::DlgScript header; // @who, @when, @priority, @bark, @pair and the notes above the first node and below the last (no nodes)
};

// The file's cards and wires. A card without a place in `layout` gets one from `luna::engine::layoutLayers`.
DialogueGraph dialogueToGraph(const sim::rules::DlgScript& script, const DialogueLayout& layout);

// The script a graph stands for, or nothing when it cannot be written (`problems` says why, one line each). Cards no wire leads to (a loose
// condition, a comment that points nowhere) are not in the file; each one is named in `problems` as a warning that starts with "warning:".
std::optional<sim::rules::DlgScript> graphToDialogue(const DialogueGraph& dialogue, std::vector<std::string>& problems);

// The key and place of every card the file holds, for the `.dlg.layout.json` beside it.
DialogueLayout layoutOf(const DialogueGraph& dialogue);

std::string layoutToJson(const DialogueLayout& layout);
// Reads the sidecar; a missing or damaged one gives an empty layout (the graph is laid out again, nothing is lost).
DialogueLayout layoutFromJson(std::string_view text);

// Puts the cards in rows again (one row per node, notes, conditions and effects above their card): the editor's Tidy button.
void arrangeDialogue(luna::engine::NodeGraph& graph);

// Every card the file holds by its key ("start", "start/choice1", "start/choice1/do"): where a finding from the check points to.
std::map<std::string, int> cardKeys(const DialogueGraph& dialogue);

// The preview a card shows (title and lines) from its fields: call it after any field changed.
void describeCard(luna::engine::GraphNode& card);
// A new empty card of a kind, with its ports and header colour.
luna::engine::GraphNode newDialogueCard(const std::string& type);

} // namespace odysseus::game
