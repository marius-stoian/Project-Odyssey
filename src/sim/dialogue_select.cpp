#include "sim/dialogue_select.h"

#include <algorithm>
#include <cctype>

namespace odysseus::sim::rules {

namespace {

constexpr int kChildYears = 12;

std::string lowered(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

// 3 = the NPC's own name, 2 = a role they have, 1 = their kind, 0 = no match.
int specificity(const DlgScript& script, const WhoFacts& who) {
    int best = 0;
    for (const std::string& word : script.who) {
        const std::string w = lowered(word);
        if (w == lowered(who.name)) best = std::max(best, 3);
        else if (std::find(who.roles.begin(), who.roles.end(), w) != who.roles.end()) best = std::max(best, 2);
        else if (w == lowered(who.kind)) best = std::max(best, 1);
    }
    return best;
}

} // namespace

std::vector<std::string> rolesOf(const World& world, int person) {
    std::vector<std::string> roles;
    const auto& people = world.people();
    if (person < 0 || static_cast<std::size_t>(person) >= people.size() || !people[static_cast<std::size_t>(person)].alive) return roles;
    const int daysPerYear = world.calendar().daysPerYear();
    int oldest = -1;
    for (const Person& other : people) {
        if (other.alive && !other.exiled && (oldest < 0 || other.ageDays > people[static_cast<std::size_t>(oldest)].ageDays)) oldest = other.id;
    }
    if (oldest == person) roles.push_back("elder");
    if (people[static_cast<std::size_t>(person)].ageYears(daysPerYear) < kChildYears) roles.push_back("child");
    return roles;
}

const DlgScript* selectScript(const DialogueLibrary& library, const WhoFacts& who, const RuleContext& context) {
    const DlgScript* best = nullptr;
    int bestSpecificity = 0;
    for (const DlgScript& script : library.all()) { // sorted by name, so an exact tie keeps the first name
        if (!script.bark.empty() || !script.pair.empty()) continue;
        const int match = specificity(script, who);
        if (match == 0) continue;
        if (script.when != nullptr && !isTrue(*script.when, context)) continue;
        if (best == nullptr || match > bestSpecificity || (match == bestSpecificity && script.priority > best->priority)) {
            best = &script;
            bestSpecificity = match;
        }
    }
    return best;
}

} // namespace odysseus::sim::rules
