#pragma once

#include "boundary.h"

#include "core/random.h"
#include "sim/dialogue_script.h"
#include "sim/world.h"

#include <string>
#include <vector>

namespace odysseus::sim::rules {

// Who an NPC is, as a script's `@who` sees them (design docs/plans/M8-dialogue-design.md section 5): a name, roles and a kind.
struct WhoFacts {
    std::string name;                 // "Tok"
    std::string kind = "person";      // "person", "wolf"
    std::vector<std::string> roles;   // "elder", "hunter", "gatherer", "child"
};

// The roles of a clan member: `elder` is the oldest living member, `hunter` and `gatherer` are the members with the best hunting and
// gathering skill, `child` is anyone under 12 years. Ties go to the lowest id, so the answer is always the same for the same world.
// Children and the exiled are never elder, hunter or gatherer.
std::vector<std::string> rolesOf(const World& world, int person);

// The script that speaks for `who` when the hero talks to them, or nullptr (the plain talk then). Candidates are the scripts for a
// conversation (not barks and not pair talk) whose `@who` names this NPC by name (most specific), role or kind and whose `@when` holds.
// The most specific match wins, then the higher `@priority`; scripts still tied are chosen between by `random`. One number is drawn
// from `random` every call, tie or not, so the stream does not depend on how many scripts happen to fit.
const DlgScript* selectScript(const DialogueLibrary& library, const WhoFacts& who, const RuleContext& context, core::Pcg32& random);

// The same choice among the short greetings (`@bark`).
const DlgScript* selectBark(const DialogueLibrary& library, const WhoFacts& who, const RuleContext& context, core::Pcg32& random);

// What a greeting says: the first line of its first node whose condition holds, tokens filled. Empty when there is none.
std::string barkText(const DlgScript& script, const RuleContext& context);

} // namespace odysseus::sim::rules
