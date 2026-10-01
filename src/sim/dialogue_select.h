#pragma once

#include "boundary.h"

#include "sim/dialogue_script.h"
#include "sim/world.h"

#include <string>
#include <vector>

namespace odysseus::sim::rules {

// Who an NPC is, as a script's `@who` sees them (design docs/plans/M8-dialogue-design.md section 5): a name, roles and a kind.
struct WhoFacts {
    std::string name;                 // "Tok"
    std::string kind = "person";      // "person", "wolf"
    std::vector<std::string> roles;   // "elder", "child"
};

// The roles of a clan member: `elder` is the oldest living member (the lowest id when two are the same age), `child` is under 12 years.
std::vector<std::string> rolesOf(const World& world, int person);

// The script that speaks for `who`, or nullptr (the NPC then falls back to the plain Talk). Candidates are the scripts for a conversation
// (not barks and not pair talk) whose `@who` names this NPC by name (most specific), role or kind and whose `@when` holds. The most
// specific match wins, then the higher `@priority`, then the name of the file, so the choice is always the same for the same world.
// US-162 adds the seeded tie-break and barks.
const DlgScript* selectScript(const DialogueLibrary& library, const WhoFacts& who, const RuleContext& context);

} // namespace odysseus::sim::rules
