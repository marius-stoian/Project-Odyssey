#include "sim/rule_effect.h"

#include <cctype>
#include <format>

namespace odysseus::sim::rules {

namespace {

constexpr int kMaxAfterDepth = 4; // after 5s after 5s after 5s ...: more than this is a mistake
constexpr std::size_t kMaxLineLength = 600;

struct Term {
    std::string text;
    std::size_t offset; // where it starts in the line
};

// Splits a line into terms at white space, but not inside "quotes" or brackets.
std::optional<EffectProblem> splitTerms(std::string_view s, std::vector<Term>& terms) {
    std::size_t i = 0;
    while (i < s.size()) {
        if (s[i] == ' ' || s[i] == '\t') {
            ++i;
            continue;
        }
        const std::size_t start = i;
        int depth = 0;
        bool inQuote = false;
        while (i < s.size()) {
            const char c = s[i];
            if (inQuote) {
                if (c == '"') inQuote = false;
            } else if (c == '"') {
                inQuote = true;
            } else if (c == '(') {
                ++depth;
            } else if (c == ')') {
                if (depth == 0) return EffectProblem{static_cast<int>(i) + 1, "a ')' has no matching '('"};
                --depth;
            } else if ((c == ' ' || c == '\t') && depth == 0) {
                break;
            }
            ++i;
        }
        if (inQuote) return EffectProblem{static_cast<int>(start) + 1, "this quoted text never ends"};
        if (depth != 0) return EffectProblem{static_cast<int>(start) + 1, "a '(' is never closed"};
        terms.push_back({std::string(s.substr(start, i - start)), start});
    }
    return std::nullopt;
}

bool parseDelay(const std::string& text, int& amount, DelayUnit& unit) {
    if (text.size() < 2) return false;
    const char suffix = text.back();
    if (suffix == 's') unit = DelayUnit::Seconds;
    else if (suffix == 'm') unit = DelayUnit::Minutes;
    else if (suffix == 'd') unit = DelayUnit::Days;
    else return false;
    long long n = 0;
    for (std::size_t i = 0; i + 1 < text.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(text[i]))) return false;
        n = n * 10 + (text[i] - '0');
        if (n > 100000) return false;
    }
    amount = static_cast<int>(n);
    return n > 0;
}

ParsedEffect parseAt(std::string_view source, int depth);

ParsedEffect failure(int column, std::string message) {
    ParsedEffect result;
    result.problem = EffectProblem{column, std::move(message)};
    return result;
}

ParsedEffect parseAt(std::string_view source, int depth) {
    if (source.size() > kMaxLineLength) return failure(1, std::format("the effect is longer than {} characters", kMaxLineLength));
    std::vector<Term> terms;
    if (const auto problem = splitTerms(source, terms)) return failure(problem->column, problem->message);
    if (terms.empty()) return failure(1, "the effect is empty");

    const std::string& verb = terms[0].text;
    const VerbInfo* info = nullptr;
    for (const VerbInfo& v : knownVerbs()) {
        if (verb == v.name) info = &v;
    }
    if (info == nullptr) return failure(static_cast<int>(terms[0].offset) + 1, std::format("unknown effect verb \"{}\"", verb));

    ParsedEffect result;
    Effect& effect = result.effect;
    effect.verb = verb;
    effect.source = std::string(source);

    if (verb == "after") {
        if (terms.size() < 3) return failure(static_cast<int>(terms[0].offset) + 1, "after needs a time and an effect: after 15s set target.state ripe");
        if (!parseDelay(terms[1].text, effect.delayAmount, effect.delayUnit)) {
            return failure(static_cast<int>(terms[1].offset) + 1, std::format("\"{}\" is not a time (write 15s, 2m or 1d)", terms[1].text));
        }
        if (depth >= kMaxAfterDepth) return failure(static_cast<int>(terms[0].offset) + 1, "after is nested too deeply");
        ParsedEffect inner = parseAt(source.substr(terms[2].offset), depth + 1);
        if (inner.problem) {
            inner.problem->column += static_cast<int>(terms[2].offset);
            return inner;
        }
        effect.inner = std::make_shared<const Effect>(std::move(inner.effect));
        return result;
    }

    const int argCount = static_cast<int>(terms.size()) - 1;
    if (argCount < info->minArgs || argCount > info->maxArgs) {
        const std::string wanted = info->minArgs == info->maxArgs ? std::format("{}", info->minArgs) : std::format("{} to {}", info->minArgs, info->maxArgs);
        return failure(static_cast<int>(terms[0].offset) + 1, std::format("{} takes {} argument(s), not {}", verb, wanted, argCount));
    }
    for (std::size_t i = 1; i < terms.size(); ++i) {
        ParsedExpr arg = parseExpression(terms[i].text);
        if (arg.problem) return failure(static_cast<int>(terms[i].offset) + arg.problem->column, arg.problem->message);
        effect.args.push_back(arg.root);
        effect.argSources.push_back(terms[i].text);
    }

    // The few verbs whose first argument must be a certain kind of thing.
    const auto isPath = [&](std::size_t index, bool needsDot) {
        const Expr& e = *effect.args[index];
        return e.kind == Expr::Kind::Path && (!needsDot || e.text.find('.') != std::string::npos);
    };
    if (verb == "set" && !isPath(0, true)) {
        return failure(static_cast<int>(terms[1].offset) + 1, "set needs something to change, like target.state");
    }
    if ((verb == "give" || verb == "take") && !isPath(0, false)) {
        return failure(static_cast<int>(terms[1].offset) + 1, std::format("{} needs who first: actor, target, npc or hero", verb));
    }
    if (verb == "opinion" && (!isPath(0, false) || !isPath(1, false))) {
        return failure(static_cast<int>(terms[1].offset) + 1, "opinion needs two people: opinion npc hero +5");
    }
    return result;
}

} // namespace

const std::vector<VerbInfo>& knownVerbs() {
    static const std::vector<VerbInfo> verbs = {
        {"give", 3, 3, "put items in someone's hands: give actor berries 2"},
        {"take", 3, 3, "remove items from someone: take hero berries 1"},
        {"set", 2, 2, "change a state of a thing: set target.state picked"},
        {"flag", 1, 2, "set a story note (1 unless a value is given): flag met-elder"},
        {"opinion", 3, 3, "change what the first thinks of the second: opinion npc hero +5"},
        {"remember", 2, 3, "give someone a memory with a feeling: remember npc \"shared berries\" 20"},
        {"start", 1, 2, "begin another interaction: start inspect target"},
        {"talk", 1, 1, "open a conversation: talk elder-fire"},
        {"say", 1, 1, "show a speech bubble: say \"Hello, {hero.name}\""},
        {"fx", 1, 1, "play a visual effect from effects.json: fx leaves"},
        {"sound", 1, 1, "play a sound: sound pop"},
        {"after", 3, 1000, "do an effect later (s, m or d): after 15s set target.state ripe"},
        {"chronicle", 1, 1, "write a line in the clan's chronicle: chronicle \"{actor.name} shared berries\""},
    };
    return verbs;
}

ParsedEffect parseEffect(std::string_view source) {
    return parseAt(source, 0);
}

} // namespace odysseus::sim::rules
