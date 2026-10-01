#include "sim/dialogue_script.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <iterator>
#include <set>

namespace odysseus::sim::rules {

namespace {

std::string trim(std::string_view text) {
    std::size_t a = 0;
    std::size_t b = text.size();
    while (a < b && (text[a] == ' ' || text[a] == '\t')) ++a;
    while (b > a && (text[b - 1] == ' ' || text[b - 1] == '\t' || text[b - 1] == '\r')) --b;
    return std::string(text.substr(a, b - a));
}

bool startsWith(std::string_view s, std::string_view prefix) { return s.substr(0, prefix.size()) == prefix; }

bool isWord(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-'; });
}

// The text inside the last bracket group of `s` that ends with `close`, removed from `s`. Works backwards over matching pairs, so
// `{remember npc "{hero} shared"}` comes out whole. Empty when `s` does not end with `close` or the bracket never opens.
std::optional<std::string> takeSuffix(std::string& s, char open, char close) {
    s = trim(s);
    if (s.empty() || s.back() != close) return std::nullopt;
    int depth = 0;
    for (std::size_t i = s.size(); i-- > 0;) {
        if (s[i] == close) ++depth;
        else if (s[i] == open && --depth == 0) {
            std::string inside = s.substr(i + 1, s.size() - i - 2);
            s = trim(s.substr(0, i));
            return inside;
        }
    }
    return std::nullopt;
}

// `a; b "c; d"; e(1, 2)` -> the pieces between the semicolons that are not inside quotes or brackets.
std::vector<std::string> splitEffects(const std::string& text) {
    std::vector<std::string> pieces;
    std::string current;
    bool quoted = false;
    int depth = 0;
    for (const char c : text) {
        if (c == '"') quoted = !quoted;
        if (!quoted) {
            if (c == '(' || c == '{') ++depth;
            if (c == ')' || c == '}') --depth;
        }
        if (c == ';' && !quoted && depth == 0) {
            pieces.push_back(trim(current));
            current.clear();
        } else {
            current += c;
        }
    }
    pieces.push_back(trim(current));
    return pieces;
}

class Parser {
public:
    Parser(std::string_view text, const std::string& name, const std::string& file, LoadReport& report) : text_(text), name_(name), file_(file), report_(report) {}

    std::optional<DlgScript> run() {
        const std::size_t errorsBefore = report_.errors.size();
        script_.name = name_;
        script_.file = file_;
        std::size_t start = 0;
        int lineNumber = 0;
        while (start <= text_.size()) {
            std::size_t end = text_.find('\n', start);
            if (end == std::string_view::npos) end = text_.size();
            ++lineNumber;
            parseLine(trim(text_.substr(start, end - start)), lineNumber);
            if (end >= text_.size()) break;
            start = end + 1;
        }
        script_.endNotes = std::move(pending_);
        validate();
        if (report_.errors.size() != errorsBefore) return std::nullopt;
        return std::move(script_);
    }

private:
    std::string_view text_;
    std::string name_;
    std::string file_;
    LoadReport& report_;
    DlgScript script_;
    std::vector<std::string> pending_; // notes waiting for the element they belong to
    bool headersDone_ = false;         // the first node has begun
    bool inChoices_ = false;           // the current node has had a choice

    void error(int line, std::string message) { report_.errors.push_back({file_, line, std::move(message)}); }
    void warning(int line, std::string message) { report_.warnings.push_back({file_, line, std::move(message)}); }

    // {tokens} in what a character says: the same names as the interaction labels, plus {smalltalk.topic}, {season} and {time}.
    void checkTokens(const std::string& text, int line) {
        std::size_t at = 0;
        while ((at = text.find('{', at)) != std::string::npos) {
            const std::size_t close = text.find('}', at);
            if (close == std::string::npos) {
                error(line, "a '{' is never closed");
                return;
            }
            const std::string token = text.substr(at + 1, close - at - 1);
            const std::string root = token.substr(0, token.find('.'));
            const bool smalltalk = root == "smalltalk" && token.size() > 10 && isWord(token.substr(10));
            if (!isPathRoot(root) && !smalltalk && token != "season" && token != "time") {
                error(line, std::format("unknown token \"{{{}}}\" (use {{hero}}, {{npc.name}}, {{target.name}}, {{smalltalk.topic}}, {{season}} or {{time}})", token));
            }
            at = close + 1;
        }
    }

    ExprPtr condition(const std::string& source, int line) {
        ParsedExpr parsed = parseExpression(source);
        if (parsed.problem) {
            error(line, parsed.problem->message);
            return nullptr;
        }
        return parsed.root;
    }

    void parseLine(const std::string& line, int number) {
        if (line.empty()) return;
        if (line[0] == '#') {
            std::string note = line.substr(1);
            if (!note.empty() && note[0] == ' ') note.erase(0, 1);
            pending_.push_back(note);
            return;
        }
        if (line[0] == '@') {
            parseHeader(line, number);
            return;
        }
        if (startsWith(line, "===")) {
            parseNode(line, number);
            return;
        }
        if (script_.nodes.empty()) {
            error(number, "this line is before the first node (start one with \"=== start\")");
            return;
        }
        if (startsWith(line, "->")) {
            parseChoice(line, number);
            return;
        }
        parseSpeech(line, number);
    }

    void parseHeader(const std::string& line, int number) {
        if (headersDone_) {
            error(number, "headers (@who, @when...) must come before the first node");
            return;
        }
        if (script_.headerNotes.empty() && script_.who.empty() && script_.whenSource.empty()) {
            script_.headerNotes = std::move(pending_); // the notes above the first header
            pending_.clear();
        }
        const std::size_t space = line.find(' ');
        const std::string key = line.substr(0, space);
        const std::string rest = space == std::string::npos ? std::string() : trim(line.substr(space));
        std::vector<std::string> words;
        {
            std::string word;
            for (const char c : rest + " ") {
                if (c == ' ' || c == '\t') {
                    if (!word.empty()) words.push_back(word);
                    word.clear();
                } else {
                    word += c;
                }
            }
        }
        if (key == "@who") {
            if (words.empty()) error(number, "@who needs a character, a role or a kind (for example: @who elder)");
            for (const std::string& w : words) {
                if (!isWord(w)) error(number, std::format("\"{}\" is not a word of letters, digits, '-' or '_'", w));
            }
            script_.who = words;
        } else if (key == "@when") {
            script_.whenSource = rest;
            script_.when = condition(rest, number);
        } else if (key == "@priority") {
            const auto milli = parseMilli(rest);
            if (!milli || *milli % 1000 != 0 || *milli < -1'000'000 || *milli > 1'000'000) error(number, "@priority needs a whole number (for example: @priority 10)");
            else script_.priority = static_cast<int>(*milli / 1000);
        } else if (key == "@bark") {
            if (words.size() != 1 || !isWord(words[0])) error(number, "@bark needs one word, the kind of greeting (for example: @bark hello)");
            else script_.bark = words[0];
        } else if (key == "@pair") {
            if (words.size() != 2 || !isWord(words[0]) || !isWord(words[1])) error(number, "@pair needs two words, who talks to whom (for example: @pair elder child)");
            else script_.pair = words;
        } else {
            error(number, std::format("unknown header \"{}\" (known: @who, @when, @priority, @bark, @pair)", key));
        }
    }

    void parseNode(const std::string& line, int number) {
        headersDone_ = true;
        const std::string id = trim(line.substr(3));
        DlgNode node;
        node.line = number;
        node.notes = std::move(pending_);
        pending_.clear();
        if (!isWord(id)) {
            error(number, "a node needs a name of letters, digits, '-' or '_' (for example: === start)");
        } else if (script_.find(id) != nullptr) {
            error(number, std::format("the node \"{}\" is defined twice", id));
        }
        node.id = id;
        script_.nodes.push_back(std::move(node));
        inChoices_ = false;
    }

    void parseSpeech(std::string line, int number) {
        const std::size_t colon = line.find(':');
        const std::string speaker = colon == std::string::npos ? std::string() : trim(line.substr(0, colon));
        if (!isWord(speaker)) {
            error(number, "expected \"Speaker: words\", \"-> choice => node\", \"=== node\" or a \"# note\"");
            return;
        }
        if (inChoices_) {
            error(number, "a line the character says must come before the choices of its node");
            return;
        }
        DlgLine said;
        said.line = number;
        said.speaker = speaker;
        std::string rest = trim(line.substr(colon + 1));
        if (auto cond = takeSuffix(rest, '[', ']')) {
            if (!startsWith(*cond, "if ")) {
                error(number, "a bracket at the end of a line must be [if condition]");
                return;
            }
            said.conditionSource = trim(cond->substr(3));
            said.condition = condition(said.conditionSource, number);
        }
        if (rest.empty()) error(number, "the character says nothing here");
        said.text = rest;
        checkTokens(said.text, number);
        said.notes = std::move(pending_);
        pending_.clear();
        script_.nodes.back().lines.push_back(std::move(said));
    }

    void parseChoice(const std::string& line, int number) {
        inChoices_ = true;
        std::string rest = trim(line.substr(2));
        const std::size_t arrow = rest.rfind("=>");
        DlgChoice choice;
        choice.line = number;
        if (arrow == std::string::npos) {
            error(number, "a choice needs \"=> node\" or \"=> END\" at its end");
            pending_.clear();
            return;
        }
        choice.target = trim(rest.substr(arrow + 2));
        if (choice.target != "END" && !isWord(choice.target)) {
            error(number, "after \"=>\" write the name of a node, or END");
            pending_.clear();
            return;
        }
        std::string left = trim(rest.substr(0, arrow));
        bool hadEffects = false;
        bool hadIf = false;
        bool hadElse = false;
        while (true) {
            if (!left.empty() && left.back() == '}') {
                auto inside = takeSuffix(left, '{', '}');
                if (!inside || hadEffects) {
                    error(number, hadEffects ? "a choice has one {effects} part" : "a '{' is missing");
                    break;
                }
                hadEffects = true;
                for (const std::string& piece : splitEffects(*inside)) {
                    if (piece.empty()) continue;
                    ParsedEffect parsed = parseEffect(piece);
                    if (parsed.problem) error(number, parsed.problem->message);
                    else choice.effects.push_back(std::move(parsed.effect));
                }
            } else if (!left.empty() && left.back() == ']') {
                auto inside = takeSuffix(left, '[', ']');
                if (!inside) {
                    error(number, "a '[' is missing");
                    break;
                }
                if (startsWith(*inside, "if ")) {
                    if (hadIf) error(number, "a choice has one [if ...]");
                    hadIf = true;
                    choice.conditionSource = trim(inside->substr(3));
                    choice.condition = condition(choice.conditionSource, number);
                } else if (startsWith(*inside, "else ")) {
                    if (hadElse) error(number, "a choice has one [else ...]");
                    hadElse = true;
                    choice.elseText = trim(inside->substr(5));
                } else {
                    error(number, "a bracket in a choice must be [if condition] or [else reason]");
                }
            } else {
                break;
            }
        }
        if (hadElse && !hadIf) error(number, "[else ...] needs an [if ...] before it");
        choice.text = left;
        if (choice.text.empty()) error(number, "a choice needs its words (\"-> Ask about the hunt => hunt\")");
        checkTokens(choice.text, number);
        choice.notes = std::move(pending_);
        pending_.clear();
        script_.nodes.back().choices.push_back(std::move(choice));
    }

    // After the whole file: what only makes sense with every node known.
    void validate() {
        if (script_.nodes.empty()) {
            error(1, "the script has no nodes (start one with \"=== start\")");
            return;
        }
        for (const DlgNode& node : script_.nodes) {
            if (static_cast<int>(node.choices.size()) > kMaxVisibleChoices) {
                error(node.line, std::format("node \"{}\" has {} choices; the panel shows at most {}", node.id, node.choices.size(), kMaxVisibleChoices));
            }
            for (const DlgChoice& choice : node.choices) {
                if (choice.target != "END" && script_.find(choice.target) == nullptr) error(choice.line, std::format("unknown node \"{}\"", choice.target));
            }
        }
        // Nodes nobody can reach are probably a mistake; they load, with a warning.
        std::set<std::string> reached;
        std::vector<const DlgNode*> todo = {script_.startNode()};
        while (!todo.empty()) {
            const DlgNode* node = todo.back();
            todo.pop_back();
            if (node == nullptr || !reached.insert(node->id).second) continue;
            for (const DlgChoice& choice : node->choices) todo.push_back(script_.find(choice.target));
        }
        for (const DlgNode& node : script_.nodes) {
            if (reached.count(node.id) == 0) warning(node.line, std::format("node \"{}\" is never reached", node.id));
        }
    }
};

void writeNotes(std::string& out, const std::vector<std::string>& notes) {
    for (const std::string& note : notes) out += note.empty() ? "#\n" : "# " + note + "\n";
}

} // namespace

const DlgNode* DlgScript::find(std::string_view id) const {
    for (const DlgNode& node : nodes) {
        if (node.id == id) return &node;
    }
    return nullptr;
}

const DlgNode* DlgScript::startNode() const {
    if (const DlgNode* start = find("start")) return start;
    return nodes.empty() ? nullptr : &nodes.front();
}

std::optional<DlgScript> parseDialogue(std::string_view text, const std::string& name, const std::string& file, LoadReport& report) {
    return Parser(text, name, file, report).run();
}

std::string writeDialogue(const DlgScript& s) {
    std::string out;
    writeNotes(out, s.headerNotes);
    if (!s.who.empty()) {
        out += "@who";
        for (const std::string& w : s.who) out += " " + w;
        out += "\n";
    }
    if (!s.whenSource.empty()) out += "@when " + s.whenSource + "\n";
    if (s.priority != 0) out += std::format("@priority {}\n", s.priority);
    if (!s.bark.empty()) out += "@bark " + s.bark + "\n";
    if (s.pair.size() == 2) out += "@pair " + s.pair[0] + " " + s.pair[1] + "\n";
    for (const DlgNode& node : s.nodes) {
        if (!out.empty()) out += "\n";
        writeNotes(out, node.notes);
        out += "=== " + node.id + "\n";
        for (const DlgLine& line : node.lines) {
            writeNotes(out, line.notes);
            out += line.speaker + ": " + line.text;
            if (!line.conditionSource.empty()) out += "   [if " + line.conditionSource + "]";
            out += "\n";
        }
        for (const DlgChoice& choice : node.choices) {
            writeNotes(out, choice.notes);
            out += "-> " + choice.text;
            if (!choice.conditionSource.empty()) out += " [if " + choice.conditionSource + "]";
            if (!choice.elseText.empty()) out += " [else " + choice.elseText + "]";
            if (!choice.effects.empty()) {
                out += " {";
                for (std::size_t i = 0; i < choice.effects.size(); ++i) out += (i ? "; " : "") + choice.effects[i].source;
                out += "}";
            }
            out += " => " + choice.target + "\n";
        }
    }
    if (!s.endNotes.empty()) {
        out += "\n";
        writeNotes(out, s.endNotes);
    }
    return out;
}

DialogueLibrary DialogueLibrary::load(const std::filesystem::path& folder, LoadReport& report) {
    DialogueLibrary library;
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) {
        report.errors.push_back({folder.filename().generic_string(), 0, "the dialogue folder cannot be found"});
        return library;
    }
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dlg") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& path : files) {
        ++report.filesRead;
        const std::string file = folder.filename().generic_string() + "/" + path.filename().generic_string();
        std::ifstream in(path, std::ios::binary);
        if (!in) {
            report.errors.push_back({file, 0, "the file cannot be read"});
            continue;
        }
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (auto script = parseDialogue(text, path.stem().string(), file, report)) library.scripts_.push_back(std::move(*script));
    }
    report.loaded = static_cast<int>(library.scripts_.size());
    return library;
}

const DlgScript* DialogueLibrary::find(std::string_view name) const {
    for (const DlgScript& script : scripts_) {
        if (script.name == name) return &script;
    }
    return nullptr;
}

} // namespace odysseus::sim::rules
