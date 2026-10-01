#include "sim/rule_expr.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <format>
#include <limits>

namespace odysseus::sim::rules {

namespace {

constexpr std::size_t kMaxSourceLength = 400; // keeps every tree shallow, so evaluating can never overflow the stack
constexpr int kMaxNesting = 32;               // parentheses inside parentheses
constexpr long long kMaxLiteral = 1'000'000'000;

// ---- tokens

enum class TokenKind { Number, Word, String, Symbol, End };

struct Token {
    TokenKind kind = TokenKind::End;
    std::string text;
    long long number = 0;
    int column = 1;
};

struct Failure {
    int column;
    std::string message;
};

bool isWordStart(char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; }
bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; }

std::vector<Token> tokenize(std::string_view s) {
    std::vector<Token> tokens;
    std::size_t i = 0;
    while (i < s.size()) {
        const char c = s[i];
        const int column = static_cast<int>(i) + 1;
        if (c == ' ' || c == '\t') {
            ++i;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            long long n = 0;
            while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
                n = n * 10 + (s[i] - '0');
                if (n > kMaxLiteral) throw Failure{column, std::format("the number is bigger than {}", kMaxLiteral)};
                ++i;
            }
            if (i < s.size() && isWordStart(s[i])) throw Failure{column, "a number cannot be followed by letters (time units belong to 'after')"};
            tokens.push_back({TokenKind::Number, {}, n, column});
        } else if (isWordStart(c)) {
            std::string word;
            while (i < s.size()) {
                if (isWordChar(s[i])) {
                    word += s[i++];
                } else if (s[i] == '.' && i + 1 < s.size() && isWordStart(s[i + 1])) { // target.state
                    word += s[i++];
                } else if (s[i] == '-' && i + 1 < s.size() && isWordStart(s[i + 1]) && !word.empty()) { // wild-berry
                    word += s[i++];
                } else {
                    break;
                }
            }
            tokens.push_back({TokenKind::Word, word, 0, column});
        } else if (c == '"') {
            std::string text;
            ++i;
            bool closed = false;
            while (i < s.size()) {
                if (s[i] == '"') {
                    closed = true;
                    ++i;
                    break;
                }
                text += s[i++];
            }
            if (!closed) throw Failure{column, "this quoted text never ends"};
            tokens.push_back({TokenKind::String, text, 0, column});
        } else {
            std::string symbol(1, c);
            if ((c == '=' || c == '!' || c == '<' || c == '>') && i + 1 < s.size() && s[i + 1] == '=') symbol += s[++i];
            ++i;
            if (symbol == "=") throw Failure{column, "use '==' to compare (a single '=' is not an operator)"};
            if (symbol == "!") throw Failure{column, "use 'not' (or '!=' to compare)"};
            if (symbol != "(" && symbol != ")" && symbol != "," && symbol != "+" && symbol != "-" && symbol != "*" && symbol != "/" &&
                symbol != "==" && symbol != "!=" && symbol != "<" && symbol != "<=" && symbol != ">" && symbol != ">=") {
                throw Failure{column, std::format("unexpected character '{}'", c)};
            }
            tokens.push_back({TokenKind::Symbol, symbol, 0, column});
        }
    }
    tokens.push_back({TokenKind::End, {}, 0, static_cast<int>(s.size()) + 1});
    return tokens;
}

// ---- parser

ExprPtr makeNode(Expr::Kind kind, std::string text, std::vector<ExprPtr> args = {}, long long number = 0) {
    auto node = std::make_shared<Expr>();
    node->kind = kind;
    node->text = std::move(text);
    node->args = std::move(args);
    node->number = number;
    return node;
}

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    ExprPtr parse() {
        ExprPtr root = orExpression();
        if (peek().kind != TokenKind::End) {
            throw Failure{peek().column, std::format("unexpected '{}' (an operator or a bracket may be missing before it)", describe(peek()))};
        }
        return root;
    }

private:
    std::vector<Token> tokens_;
    std::size_t at_ = 0;
    int nesting_ = 0;

    const Token& peek() const { return tokens_[at_]; }
    Token take() { return tokens_[at_ < tokens_.size() - 1 ? at_++ : at_]; }

    static std::string describe(const Token& t) {
        if (t.kind == TokenKind::End) return "the end";
        if (t.kind == TokenKind::Number) return std::format("{}", t.number);
        if (t.kind == TokenKind::String) return std::format("\"{}\"", t.text);
        return t.text;
    }

    bool isSymbol(const char* s) const { return peek().kind == TokenKind::Symbol && peek().text == s; }
    bool isWord(const char* s) const { return peek().kind == TokenKind::Word && peek().text == s; }

    ExprPtr orExpression() {
        ExprPtr left = andExpression();
        while (isWord("or")) {
            take();
            ExprPtr right = andExpression();
            left = makeNode(Expr::Kind::Binary, "or", {left, right});
        }
        return left;
    }

    ExprPtr andExpression() {
        ExprPtr left = notExpression();
        while (isWord("and")) {
            take();
            ExprPtr right = notExpression();
            left = makeNode(Expr::Kind::Binary, "and", {left, right});
        }
        return left;
    }

    ExprPtr notExpression() {
        if (isWord("not")) {
            take();
            if (++nesting_ > kMaxNesting) throw Failure{peek().column, "too many 'not' in a row"};
            ExprPtr inner = notExpression();
            --nesting_;
            return makeNode(Expr::Kind::Unary, "not", {inner});
        }
        return comparison();
    }

    ExprPtr comparison() {
        ExprPtr left = sum();
        for (const char* op : {"==", "!=", "<=", ">=", "<", ">"}) {
            if (isSymbol(op)) {
                take();
                ExprPtr right = sum();
                return makeNode(Expr::Kind::Binary, op, {left, right});
            }
        }
        return left;
    }

    ExprPtr sum() {
        ExprPtr left = product();
        while (isSymbol("+") || isSymbol("-")) {
            const std::string op = take().text;
            ExprPtr right = product();
            left = makeNode(Expr::Kind::Binary, op, {left, right});
        }
        return left;
    }

    ExprPtr product() {
        ExprPtr left = unary();
        while (isSymbol("*") || isSymbol("/")) {
            const std::string op = take().text;
            ExprPtr right = unary();
            left = makeNode(Expr::Kind::Binary, op, {left, right});
        }
        return left;
    }

    ExprPtr unary() {
        if (isSymbol("-") || isSymbol("+")) {
            const std::string op = take().text;
            if (++nesting_ > kMaxNesting) throw Failure{peek().column, "too many signs in a row"};
            ExprPtr inner = unary();
            --nesting_;
            return makeNode(Expr::Kind::Unary, op, {inner});
        }
        return primary();
    }

    ExprPtr primary() {
        const Token t = take();
        switch (t.kind) {
        case TokenKind::Number: return makeNode(Expr::Kind::Number, {}, {}, t.number);
        case TokenKind::String: return makeNode(Expr::Kind::Text, t.text);
        case TokenKind::Word: return word(t);
        case TokenKind::Symbol:
            if (t.text == "(") {
                if (++nesting_ > kMaxNesting) throw Failure{t.column, "brackets are nested too deeply"};
                ExprPtr inner = orExpression();
                if (!isSymbol(")")) throw Failure{peek().column, "a closing ')' is missing"};
                take();
                --nesting_;
                return inner;
            }
            throw Failure{t.column, std::format("expected a value, found '{}'", t.text)};
        case TokenKind::End: break;
        }
        throw Failure{t.column, "the expression ends where a value was expected"};
    }

    ExprPtr word(const Token& t) {
        if (t.text == "and" || t.text == "or" || t.text == "not") {
            throw Failure{t.column, std::format("'{}' is in the wrong place (it needs something on both sides)", t.text)};
        }
        if (isSymbol("(")) return call(t);
        const std::size_t dot = t.text.find('.');
        if (dot != std::string::npos) {
            const std::string root = t.text.substr(0, dot);
            if (!isPathRoot(root)) {
                throw Failure{t.column, std::format("unknown name \"{}\" (a path starts with actor, target, npc or hero)", t.text)};
            }
            return makeNode(Expr::Kind::Path, t.text);
        }
        if (isPathRoot(t.text) || isWorldValue(t.text)) return makeNode(Expr::Kind::Path, t.text);
        return makeNode(Expr::Kind::Text, t.text); // a plain word: winter, berries, ripe
    }

    ExprPtr call(const Token& nameToken) {
        const FunctionInfo* info = nullptr;
        for (const FunctionInfo& f : knownFunctions()) {
            if (nameToken.text == f.name) info = &f;
        }
        if (info == nullptr) throw Failure{nameToken.column, std::format("unknown function \"{}\"", nameToken.text)};
        take(); // (
        std::vector<ExprPtr> args;
        if (!isSymbol(")")) {
            while (true) {
                args.push_back(orExpression());
                if (isSymbol(",")) {
                    take();
                    continue;
                }
                break;
            }
        }
        if (!isSymbol(")")) throw Failure{peek().column, std::format("a closing ')' is missing after {}(...", nameToken.text)};
        take();
        const int count = static_cast<int>(args.size());
        if (count < info->minArgs || count > info->maxArgs) {
            const std::string wanted = info->minArgs == info->maxArgs ? std::format("{}", info->minArgs) : std::format("{} to {}", info->minArgs, info->maxArgs);
            throw Failure{nameToken.column, std::format("{}() takes {} argument(s), not {}", nameToken.text, wanted, count)};
        }
        return makeNode(Expr::Kind::Call, nameToken.text, std::move(args));
    }
};

// ---- whole-number arithmetic that cannot overflow

long long clampTo(long long v) { return std::clamp(v, -4'000'000'000'000'000'000LL, 4'000'000'000'000'000'000LL); }

long long addSat(long long a, long long b) { return clampTo(a + b); } // both inside +-4e18: the sum fits

long long mulSat(long long a, long long b) {
    if (a == 0 || b == 0) return 0;
    const long long limit = 4'000'000'000'000'000'000LL;
    const long long absA = a < 0 ? -a : a;
    const long long absB = b < 0 ? -b : b;
    if (absA > limit / absB) return ((a < 0) != (b < 0)) ? -limit : limit;
    return a * b;
}

bool sameKind(const Value& a, const Value& b) { return a.isText == b.isText; }

Value compare(const std::string& op, const Value& a, const Value& b) {
    bool result = false;
    if (sameKind(a, b)) {
        const int order = a.isText ? a.text.compare(b.text) : (a.number < b.number ? -1 : (a.number > b.number ? 1 : 0));
        if (op == "==") result = order == 0;
        else if (op == "!=") result = order != 0;
        else if (!a.isText) {
            if (op == "<") result = order < 0;
            else if (op == "<=") result = order <= 0;
            else if (op == ">") result = order > 0;
            else if (op == ">=") result = order >= 0;
        }
    } else {
        result = op == "!="; // a number and a text are simply different
    }
    return Value::ofNumber(result ? 1 : 0);
}

} // namespace

bool isPathRoot(std::string_view word) {
    return word == "actor" || word == "target" || word == "npc" || word == "hero";
}

bool isWorldValue(std::string_view word) {
    return word == "time" || word == "season" || word == "distance";
}

const std::vector<FunctionInfo>& knownFunctions() {
    static const std::vector<FunctionInfo> functions = {
        {"has", 2, 3, "1 when someone holds at least n of an item: has(berries, 2) for the actor, has(hero, berries, 2) for anyone"},
        {"need", 1, 1, "how much a need is missing, 0 (full) to 100 (desperate): need(hunger)"},
        {"skill", 1, 1, "the actor's skill in a profession: skill(hunter)"},
        {"trait", 1, 1, "1 when the actor has the trait, else 0: trait(diligent)"},
        {"opinion", 2, 2, "what the first thinks of the second, -100 to 100: opinion(npc, hero)"},
        {"mood", 1, 1, "one word for how someone feels about the hero (warm, friendly, neutral, wary, hostile, or hungry, tired, cold, lonely): mood(npc) == wary"},
        {"kin", 2, 2, "1 when the two are family: kin(npc, hero)"},
        {"flag", 1, 1, "a note the story has set (0 when never set): flag(met-elder)"},
        {"tag", 2, 2, "1 when a thing carries a tag: tag(target, edible)"},
    };
    return functions;
}

ParsedExpr parseExpression(std::string_view source) {
    ParsedExpr result;
    if (source.size() > kMaxSourceLength) {
        result.problem = ExprProblem{1, std::format("the expression is longer than {} characters", kMaxSourceLength)};
        return result;
    }
    try {
        std::vector<Token> tokens = tokenize(source);
        if (tokens.size() == 1) throw Failure{1, "the expression is empty"};
        result.root = Parser(std::move(tokens)).parse();
    } catch (const Failure& failure) {
        result.root = nullptr;
        result.problem = ExprProblem{failure.column, failure.message};
    }
    return result;
}

Value evaluate(const Expr& e, const RuleContext& context) {
    switch (e.kind) {
    case Expr::Kind::Number: return Value::ofNumber(e.number);
    case Expr::Kind::Text: return Value::ofText(e.text);
    case Expr::Kind::Path: return context.path(e.text);
    case Expr::Kind::Call: {
        std::vector<Value> args;
        args.reserve(e.args.size());
        for (const ExprPtr& arg : e.args) args.push_back(evaluate(*arg, context));
        return context.call(e.text, args);
    }
    case Expr::Kind::Unary: {
        const Value inner = evaluate(*e.args[0], context);
        if (e.text == "not") return Value::ofNumber(inner.truthy() ? 0 : 1);
        if (inner.isText) return Value::ofNumber(0);
        return Value::ofNumber(e.text == "-" ? -inner.number : inner.number);
    }
    case Expr::Kind::Binary: {
        if (e.text == "and") {
            if (!evaluate(*e.args[0], context).truthy()) return Value::ofNumber(0);
            return Value::ofNumber(evaluate(*e.args[1], context).truthy() ? 1 : 0);
        }
        if (e.text == "or") {
            if (evaluate(*e.args[0], context).truthy()) return Value::ofNumber(1);
            return Value::ofNumber(evaluate(*e.args[1], context).truthy() ? 1 : 0);
        }
        const Value a = evaluate(*e.args[0], context);
        const Value b = evaluate(*e.args[1], context);
        if (e.text == "==" || e.text == "!=" || e.text == "<" || e.text == "<=" || e.text == ">" || e.text == ">=") {
            return compare(e.text, a, b);
        }
        if (a.isText || b.isText) return Value::ofNumber(0);
        if (e.text == "+") return Value::ofNumber(addSat(a.number, b.number));
        if (e.text == "-") return Value::ofNumber(addSat(a.number, -b.number));
        if (e.text == "*") return Value::ofNumber(mulSat(a.number, b.number));
        if (e.text == "/") return Value::ofNumber(b.number == 0 ? 0 : a.number / b.number);
        return Value::ofNumber(0);
    }
    }
    return Value::ofNumber(0);
}

bool isTrue(const Expr& expression, const RuleContext& context) {
    return evaluate(expression, context).truthy();
}

} // namespace odysseus::sim::rules
