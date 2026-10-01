#pragma once

#include "boundary.h"

#include "sim/rule_expr.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::sim::rules {

enum class DelayUnit { Seconds, Minutes, Days };

// One effect line of an interaction or a dialogue choice: a verb and its arguments, e.g.
//   give actor berries 2        set target.state picked        after 15s set target.state ripe
// An argument is one word, number, "quoted text", call such as has(hero, berries, 1), or a (bracketed
// expression). Effects are parsed when the file loads and carried out later by the action runner (US-153).
struct Effect {
    std::string verb;
    std::vector<ExprPtr> args;               // every argument, already parsed
    std::vector<std::string> argSources;     // the same, as written
    int delayAmount = 0;                     // `after`: how long to wait
    DelayUnit delayUnit = DelayUnit::Seconds;
    std::shared_ptr<const Effect> inner;     // `after`: the effect that happens later
    std::string source;                      // the whole line, as written (the writer prints it back unchanged)
};

struct EffectProblem {
    int column = 0;
    std::string message;
};

struct ParsedEffect {
    Effect effect;
    std::optional<EffectProblem> problem;
};

struct VerbInfo {
    const char* name;
    int minArgs;
    int maxArgs;
    const char* meaning; // one line, for the guide and the tests that compare the guide with the code
};
const std::vector<VerbInfo>& knownVerbs();

// Reads one effect line. Never throws; a mistake comes back as `problem` with a message such as
// `unknown effect verb "giv"`.
ParsedEffect parseEffect(std::string_view source);

} // namespace odysseus::sim::rules
