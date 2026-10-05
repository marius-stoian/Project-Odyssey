#pragma once

#include "boundary.h"

#include "game/dialogue_graph.h"
#include "luna/engine/node_graph.h"
#include "sim/quest_data.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// A quest file as a graph (US-184, EDT-06, D-57): the steps as cards, the `next` of each step as a wire, the branches as small cards between a step and
// the step they lead to. Like the dialogue graph it is only a view: the file is written by `sim::rules::writeQuest` and checked by the real loader.
//
// Cards (the `type` of a graph node) and what they keep in `fields`:
//   quest     fields {id, title, giver, note, journal, offer, turnIn}   in 0 requires, 1 fail, 2 rewards; out 0 -> the first step
//   step      fields {id, text, objective, marker, hint after, hint text}   in 0 flow, 1 branches; out 0 "next" -> a step, or an end card
//   branch    fields {condition}                  out 0 -> the step it belongs to (in 1), out 1 -> the step it leads to (or an end card)
//   end       no fields                           in 0: the quest is finished when a wire reaches it
//   requires  fields {one condition each...}      out 0 -> the quest card's in 0
//   fail      fields {one condition each...}      out 0 -> the quest card's in 1
//   rewards   fields {one effect each...}         out 0 -> the quest card's in 2
namespace quest_card {
inline constexpr const char* kQuest = "quest";
inline constexpr const char* kStep = "step";
inline constexpr const char* kBranch = "branch";
inline constexpr const char* kEnd = "end";
inline constexpr const char* kRequires = "requires";
inline constexpr const char* kFail = "fail";
inline constexpr const char* kRewards = "rewards";
} // namespace quest_card

// The cards and wires of a quest. A card without a place in `layout` gets one from `luna::engine::layoutLayers`.
luna::engine::NodeGraph questToGraph(const sim::rules::Quest& quest, const DialogueLayout& layout);

// The quest a graph stands for, checked by the real loader, or nothing (`problems` says why, "error: ..." and "warning: ..."). `json` gets the canonical text.
std::optional<sim::rules::Quest> graphToQuest(const luna::engine::NodeGraph& graph, std::vector<std::string>& problems, std::string& json);

// Where each card sits, keyed by what the card is in the file ("quest", "step:gather", "branch:fire:0"), for the `.quest.layout.json` beside it.
DialogueLayout questLayoutOf(const luna::engine::NodeGraph& graph);

// Cards in columns again: the editor's Tidy button.
void arrangeQuest(luna::engine::NodeGraph& graph);

// Every card the file holds by its key: where a finding from the check points to.
std::map<std::string, int> questCardKeys(const luna::engine::NodeGraph& graph);

void describeQuestCard(luna::engine::GraphNode& card);
luna::engine::GraphNode newQuestCard(const std::string& type);

} // namespace odysseus::game
