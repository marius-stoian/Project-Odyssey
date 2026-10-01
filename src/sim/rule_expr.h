#pragma once

#include "boundary.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::sim::rules {

// What an expression works out to: a whole number or a piece of text (Charter rule 6: no floating point).
struct Value {
    bool isText = false;
    long long number = 0;
    std::string text;

    static Value ofNumber(long long n) { return {false, n, {}}; }
    static Value ofText(std::string t) { return {true, 0, std::move(t)}; }
    bool truthy() const { return isText ? !text.empty() : number != 0; }
};

// One node of a parsed condition or score. Built once when the file loads, then evaluated as often as needed
// (ADR-019). Nodes are shared and never change, so a registry can be copied cheaply.
struct Expr {
    enum class Kind { Number, Text, Path, Call, Unary, Binary };

    Kind kind = Kind::Number;
    long long number = 0; // Number
    std::string text;     // Text: the words; Path: "target.state"; Call: the function name; Unary/Binary: the operator
    std::vector<std::shared_ptr<const Expr>> args; // Call: the arguments; Unary: 1; Binary: 2
};

using ExprPtr = std::shared_ptr<const Expr>;

struct ExprProblem {
    int column = 0; // 1-based, inside the expression text
    std::string message;
};

struct ParsedExpr {
    ExprPtr root;
    std::optional<ExprProblem> problem;
};

// The grammar, loosest to tightest (docs/guides/interaction-data.md has the plain-words version):
//   or  <  and  <  not  <  == != < <= > >=  <  + -  <  * /  <  unary -  <  ( )
// A name that is not a function, a path or time/season/distance is a word: `season != winter` compares
// the season with the text "winter". Problems are reported with the column; nothing throws.
ParsedExpr parseExpression(std::string_view source);

// The roots a path may start from: actor, target, npc, hero (alone, or as `target.state`).
bool isPathRoot(std::string_view word);
// time, season, distance: values of the world that need no parentheses.
bool isWorldValue(std::string_view word);

struct FunctionInfo {
    const char* name;
    int minArgs;
    int maxArgs;
    const char* meaning; // one line, for the guide and the tests that compare the guide with the code
};
const std::vector<FunctionInfo>& knownFunctions();

// The facts an expression can ask about. The Game implements it over the real world; tests implement it
// with small tables. Both functions answer "unknown" with the number 0 or empty text, never an error.
class RuleContext {
public:
    virtual ~RuleContext() = default;
    // "target.state", "actor.name", "time", "season", "distance", or a bare root such as "hero".
    virtual Value path(const std::string& dotted) const = 0;
    // has(...), need(...), ... with the arguments already worked out.
    virtual Value call(const std::string& name, const std::vector<Value>& args) const = 0;
};

// Mismatched kinds never fail: a number and a text are simply different, and division by zero is 0, so a
// typo in a data file can never crash the game. `and` and `or` stop as soon as the answer is known.
Value evaluate(const Expr& expression, const RuleContext& context);
bool isTrue(const Expr& expression, const RuleContext& context);

} // namespace odysseus::sim::rules
