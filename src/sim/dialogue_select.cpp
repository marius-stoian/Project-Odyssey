#include "sim/dialogue_select.h"

#include "sim/conversation.h"

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

struct Candidate {
    const DlgScript* script = nullptr;
    int specificity = 0;
    int priority = 0;
};

// Best by specificity, then priority; the ones left tied are chosen between with one draw.
const DlgScript* choose(const DialogueLibrary& library, const WhoFacts& who, const RuleContext& context, core::Pcg32& random, bool barks) {
    const std::uint32_t draw = random.next(); // one draw every time, tie or not (as the NPC chooser does)
    std::vector<Candidate> candidates;
    for (const DlgScript& script : library.all()) { // sorted by name, so the order of ties is fixed
        if (!script.pair.empty() || script.bark.empty() == barks) continue; // a talk takes the scripts without @bark, a greeting the ones with it
        const int match = specificity(script, who);
        if (match == 0) continue;
        if (script.when != nullptr && !isTrue(*script.when, context)) continue;
        candidates.push_back({&script, match, script.priority});
    }
    if (candidates.empty()) return nullptr;
    const auto better = [](const Candidate& a, const Candidate& b) {
        return a.specificity != b.specificity ? a.specificity < b.specificity : a.priority < b.priority;
    };
    const Candidate& best = *std::ranges::max_element(candidates, better);
    std::vector<const DlgScript*> tied;
    for (const Candidate& c : candidates) {
        if (!better(c, best) && !better(best, c)) tied.push_back(c.script);
    }
    return tied[draw % tied.size()];
}

} // namespace

std::vector<std::string> rolesOf(const World& world, int person) {
    std::vector<std::string> roles;
    const auto& people = world.people();
    if (person < 0 || static_cast<std::size_t>(person) >= people.size() || !people[static_cast<std::size_t>(person)].alive) return roles;
    const int daysPerYear = world.calendar().daysPerYear();
    // The best of the adults who are in the clan by one measure; the first of equals wins (people are listed by id).
    const auto bestBy = [&](auto measure) {
        int best = -1;
        for (const Person& other : people) {
            if (!other.alive || other.exiled || other.ageYears(daysPerYear) < kChildYears) continue;
            if (best < 0 || measure(other) > measure(people[static_cast<std::size_t>(best)])) best = other.id;
        }
        return best;
    };
    int oldest = -1;
    for (const Person& other : people) {
        if (other.alive && !other.exiled && (oldest < 0 || other.ageDays > people[static_cast<std::size_t>(oldest)].ageDays)) oldest = other.id;
    }
    if (oldest == person) roles.push_back("elder");
    if (bestBy([](const Person& p) { return p.huntSkill; }) == person) roles.push_back("hunter");
    if (bestBy([](const Person& p) { return p.gatherSkill; }) == person) roles.push_back("gatherer");
    if (people[static_cast<std::size_t>(person)].ageYears(daysPerYear) < kChildYears) roles.push_back("child");
    return roles;
}

const DlgScript* selectScript(const DialogueLibrary& library, const WhoFacts& who, const RuleContext& context, core::Pcg32& random) {
    return choose(library, who, context, random, false);
}

const DlgScript* selectBark(const DialogueLibrary& library, const WhoFacts& who, const RuleContext& context, core::Pcg32& random) {
    return choose(library, who, context, random, true);
}

std::string barkText(const DlgScript& script, const RuleContext& context) {
    for (const DlgNode& node : script.nodes) {
        for (const DlgLine& line : node.lines) {
            if (line.condition == nullptr || isTrue(*line.condition, context)) return fillDialogueTokens(line.text, context);
        }
    }
    return {};
}

} // namespace odysseus::sim::rules
