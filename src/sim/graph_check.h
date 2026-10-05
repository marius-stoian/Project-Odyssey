#pragma once

#include "boundary.h"

#include "sim/dialogue_script.h"
#include "sim/interaction.h"
#include "sim/quest_data.h"

#include <set>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// What the owner's conversations and interactions may name (US-175, D-56 Q17). A set that is empty means "do not check that"; the game fills them from its
// data, tests fill what they need. Needs are always checked against the four needs of the game.
struct GraphCatalog {
    std::set<std::string> items;        // item ids (give, take, has)
    std::set<std::string> builtins;     // `do <name>`
    std::set<std::string> dialogues;    // `talk <name>`
    std::set<std::string> interactions; // `start <id>`
    std::set<std::string> tags;         // tag(target, x) and the target tags of an interaction
    std::set<std::string> people;       // quest words for people: names, kinds, tags and roles (US-187)
    std::set<std::string> places;       // quest words for the named places of the levels
    std::set<std::string> kinds;        // kinds of creature a quest may ask the hero to defeat
    std::set<std::string> quests;       // ids of the quests that exist
};

struct GraphFinding {
    enum class Kind { Unreachable, DeadEnd, UnknownNode, UnknownItem, UnknownNeed, UnknownTag, UnknownVerb, UnknownBuiltin, UnknownDialogue, UnknownInteraction, BadEffect, UnknownPerson, UnknownPlace, UnknownKind, UnknownQuest, Cycle, CannotFinish };
    Kind kind = Kind::BadEffect;
    bool error = true;   // an error blocks shipping; a warning only says something looks wrong (unreachable nodes load fine)
    std::string file;    // "dialogue/elder-fire.dlg"
    std::string key;     // where in the graph: the key of the card, the same the layout file uses ("start/choice1/do", "verb", "effects0"); "" for the whole file
    std::string message; // "start, choice 2: unknown item \"berrys\""
    std::string text() const { return file + ": " + message; }
};

// Breadth-first from `start` (or the first node): unreachable nodes, nodes with no choice (except in a greeting or a pair talk, which are one line by
// design), unknown targets, and every effect and condition name that the catalog does not know.
std::vector<GraphFinding> checkDialogue(const DlgScript& script, const GraphCatalog& catalog);
// The same for an interaction: its requirements, effects, NPC score and target tags.
std::vector<GraphFinding> checkInteraction(const Interaction& interaction, const GraphCatalog& catalog);

// A quest (US-187): steps nobody reaches, steps from which the quest can never end, an objective that names an item, person, place, interaction or kind the
// catalog does not know, a prerequisite or reward that names a quest that does not exist. Keys are those of the graph editor ("step:gather", "quest").
std::vector<GraphFinding> checkQuest(const Quest& quest, const GraphCatalog& catalog);
// All quests: each one on its own, and the prerequisite cycles between them (quest A needs B done and B needs A done).
std::vector<GraphFinding> checkQuests(const std::vector<Quest>& quests, const GraphCatalog& catalog);

} // namespace odysseus::sim::rules
