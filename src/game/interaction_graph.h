#pragma once

#include "boundary.h"

#include "game/dialogue_graph.h"
#include "luna/engine/node_graph.h"
#include "sim/interaction.h"

#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// An interaction file as a graph (US-172, INT-06, D-56 Q12): one graph per file, actor -> verb -> target on one row, with the requirements, effects,
// the NPC rule and the chronicle line hanging off the verb. Like the dialogue graph it is only a view: the file is written by `sim::rules::toJson`
// and checked by the real loader before anything touches the disk.
//
// Cards (the `type` of a graph node) and what they keep in `fields`:
//   actor        fields {actors, separated by spaces}     out 0 -> the verb's in 0
//   verb         fields {id, label, note, range (m), duration (s), order, menu}
//                  in 0 actor, 1 "requires", 2 "do", 3 "npc", 4 "chronicle"; out 0 -> the target
//   target       fields {tags, kinds: words separated by spaces}   in 0
//   requirement  fields {condition, otherwise}            out 0 -> the verb's in 1 (one card per requirement)
//   effects      fields {one effect each...}              out 0 -> the verb's in 2
//   npcrule      fields {score, cooldown in seconds}      out 0 -> the verb's in 3
//   chronicle    fields {the line for the chronicle}      out 0 -> the verb's in 4
namespace rule_card {
inline constexpr const char* kActor = "actor";
inline constexpr const char* kVerb = "verb";
inline constexpr const char* kTarget = "target";
inline constexpr const char* kRequirement = "requirement";
inline constexpr const char* kEffects = "effects";
inline constexpr const char* kNpcRule = "npcrule";
inline constexpr const char* kChronicle = "chronicle";
} // namespace rule_card

// The cards and wires of an interaction. A card without a place in `layout` is placed by the row layout.
luna::engine::NodeGraph interactionToGraph(const sim::rules::Interaction& interaction, const DialogueLayout& layout);

// The interaction a graph stands for, checked by the real loader, or nothing (`problems` says why, "error: ..." and "warning: ..." as for dialogue).
// `json` gets the canonical text the Editor saves.
std::optional<sim::rules::Interaction> graphToInteraction(const luna::engine::NodeGraph& graph, std::vector<std::string>& problems, std::string& json);

// Where each card of the graph sits, keyed by what the card is in the file ("verb", "requirement1"), for the `.json.layout.json` beside it.
DialogueLayout interactionLayoutOf(const luna::engine::NodeGraph& graph);

void describeRuleCard(luna::engine::GraphNode& card);
luna::engine::GraphNode newRuleCard(const std::string& type);

// A number of metres or seconds as the files write it ("1.5", "2") and back; nothing for text that is not a number with at most three decimals.
std::string milliToText(int milli);
std::optional<int> textToMilli(const std::string& text);

// The comment lines (`// ...`) at the top of an interaction file, which the canonical text would lose, and the text without them.
std::string leadingComments(const std::string& fileText);

} // namespace odysseus::game
