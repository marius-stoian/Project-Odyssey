#include "sim/quest_data.h"

#include "core/text.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <iterator>
#include <set>

namespace odysseus::sim::rules {

namespace {

bool isWord(const std::string& s) {
    if (s.empty()) return false;
    return std::all_of(s.begin(), s.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-'; });
}

// "30s", "2m", "1d": a length of time. Whole numbers only.
bool parseLength(const std::string& text, int& amount, DelayUnit& unit) {
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

const char* kindWord(QuestObjective::Kind kind) {
    switch (kind) {
    case QuestObjective::Kind::Talk: return "talk";
    case QuestObjective::Kind::Goto: return "goto";
    case QuestObjective::Kind::Gather: return "gather";
    case QuestObjective::Kind::Give: return "give";
    case QuestObjective::Kind::Craft: return "craft";
    case QuestObjective::Kind::Interact: return "interact";
    case QuestObjective::Kind::Defeat: return "defeat";
    case QuestObjective::Kind::Wait: return "wait";
    case QuestObjective::Kind::Flag: return "flag";
    }
    return "talk";
}

class QuestParser {
public:
    QuestParser(const JsonValue& root, std::string name, LoadReport& report) : root_(root), name_(std::move(name)), report_(report) {}

    std::optional<Quest> run(const std::string& expectedId) {
        const std::size_t errorsBefore = report_.errors.size();
        Quest out;
        out.file = name_;
        if (!root_.isObject()) {
            error(root_.line, "a quest file holds one JSON object { ... }");
            return std::nullopt;
        }
        static const std::set<std::string> known = {"id", "title", "giver", "requires", "start", "steps", "fail", "rewards", "journal", "offer", "turnIn", "note"};
        for (std::size_t i = 0; i < root_.keys.size(); ++i) {
            if (known.count(root_.keys[i]) == 0) error(root_.keyLines[i], std::format("unknown field \"{}\"", root_.keys[i]));
        }
        readId(out, expectedId);
        readText("title", out.title, true);
        readText("journal", out.journal, false);
        readText("offer", out.offer, false);
        readText("turnIn", out.turnIn, false);
        readText("note", out.note, false);
        readGiver(out);
        out.requires_ = readConditions("requires");
        out.fail = readConditions("fail");
        readRewards(out);
        readSteps(out);
        checkReferences(out);
        if (report_.errors.size() != errorsBefore) return std::nullopt;
        return out;
    }

private:
    const JsonValue& root_;
    std::string name_;
    LoadReport& report_;
    std::set<std::string> broken_; // steps that failed to read

    void error(int line, std::string message) { report_.errors.push_back({name_, line, std::move(message)}); }

    void readId(Quest& out, const std::string& expectedId) {
        const JsonValue* v = root_.find("id");
        if (v == nullptr) {
            error(root_.line, "the field \"id\" is missing");
            return;
        }
        if (!v->isString() || !isWord(v->text)) {
            error(v->line, "id must be one word of letters, digits, '-' or '_'");
            return;
        }
        out.id = v->text;
        if (!expectedId.empty() && v->text != expectedId) error(v->line, std::format("id \"{}\" must match the file name \"{}\"", v->text, expectedId));
    }

    void readText(const char* field, std::string& target, bool required) {
        const JsonValue* v = root_.find(field);
        if (v == nullptr) {
            if (required) error(root_.line, std::format("the field \"{}\" is missing", field));
            return;
        }
        if (!v->isString() || (required && v->text.empty())) {
            error(v->line, std::format("{} must be text", field));
            return;
        }
        target = v->text;
    }

    void readGiver(Quest& out) {
        const JsonValue* v = root_.find("giver");
        if (v == nullptr) return;
        if (!v->isString() || v->text.empty()) {
            error(v->line, "giver must be a person id, \"role:<role>\" or \"none\"");
            return;
        }
        const std::string& g = v->text;
        const bool role = g.rfind("role:", 0) == 0;
        if (role ? !isWord(g.substr(5)) : !isWord(g)) {
            error(v->line, std::format("giver \"{}\" must be a person id, \"role:<role>\" or \"none\"", g));
            return;
        }
        out.giver = g;
    }

    std::vector<QuestCondition> readConditions(const char* field) {
        std::vector<QuestCondition> result;
        const JsonValue* v = root_.find(field);
        if (v == nullptr) return result;
        if (!v->isArray()) {
            error(v->line, std::format("{} must be a list of conditions like \"flag(met-elder)\"", field));
            return result;
        }
        for (const JsonValue& item : v->items) {
            if (auto condition = readCondition(item, field)) result.push_back(std::move(*condition));
        }
        return result;
    }

    std::optional<QuestCondition> readCondition(const JsonValue& v, const char* what) {
        if (!v.isString()) {
            error(v.line, std::format("{} must be text", what));
            return std::nullopt;
        }
        ParsedExpr parsed = parseExpression(v.text);
        if (parsed.problem) {
            error(v.line, parsed.problem->message);
            return std::nullopt;
        }
        return QuestCondition{v.text, parsed.root, v.line};
    }

    void readRewards(Quest& out) {
        const JsonValue* v = root_.find("rewards");
        if (v == nullptr) return;
        if (!v->isArray()) {
            error(v->line, "rewards must be a list of effect lines like \"give hero flint 2\"");
            return;
        }
        for (const JsonValue& item : v->items) {
            if (!item.isString()) {
                error(item.line, "a reward is one line of text, like \"give hero flint 2\"");
                continue;
            }
            ParsedEffect parsed = parseEffect(item.text);
            if (parsed.problem) {
                error(item.line, parsed.problem->message);
                continue;
            }
            out.rewards.push_back(std::move(parsed.effect));
        }
    }

    void readSteps(Quest& out) {
        const JsonValue* start = root_.find("start");
        if (start == nullptr || !start->isString() || start->text.empty()) {
            error(start != nullptr ? start->line : root_.line, "start must name the first step");
        } else {
            out.start = start->text;
        }
        const JsonValue* steps = root_.find("steps");
        if (steps == nullptr || !steps->isObject()) {
            error(steps != nullptr ? steps->line : root_.line, "steps must be an object with one entry per step");
            return;
        }
        if (steps->items.empty()) error(steps->line, "steps is empty: a quest needs at least one step");
        for (std::size_t i = 0; i < steps->items.size(); ++i) {
            if (!isWord(steps->keys[i])) {
                error(steps->keyLines[i], std::format("step id \"{}\" must be one word of letters, digits, '-' or '_'", steps->keys[i]));
                continue;
            }
            if (steps->keys[i] == kQuestEnd) {
                error(steps->keyLines[i], "END is not a step; it ends the quest");
                continue;
            }
            if (std::any_of(out.steps.begin(), out.steps.end(), [&](const QuestStep& s) { return s.id == steps->keys[i]; })) {
                error(steps->keyLines[i], std::format("step \"{}\" is written twice", steps->keys[i]));
                continue;
            }
            if (auto step = readStep(steps->keys[i], steps->items[i])) out.steps.push_back(std::move(*step));
            else broken_.insert(steps->keys[i]); // already reported; do not report every step that points at it as well
        }
    }

    std::optional<QuestStep> readStep(const std::string& id, const JsonValue& v) {
        if (!v.isObject()) {
            error(v.line, std::format("step \"{}\" must be an object {{ \"text\": ..., \"objective\": ..., \"next\": ... }}", id));
            return std::nullopt;
        }
        QuestStep step;
        step.id = id;
        step.line = v.line;
        const std::size_t errorsBefore = report_.errors.size();
        static const std::set<std::string> known = {"text", "objective", "marker", "hint", "next", "branches"};
        for (std::size_t i = 0; i < v.keys.size(); ++i) {
            if (known.count(v.keys[i]) == 0) error(v.keyLines[i], std::format("unknown field \"{}\" in step \"{}\"", v.keys[i], id));
        }
        stringField(v, id, "text", step.text, true);
        stringField(v, id, "marker", step.marker, false);
        if (const JsonValue* o = v.find("objective")) {
            if (!o->isString()) {
                error(o->line, "objective must be text, like \"gather berries 3\"");
            } else {
                ParsedObjective parsed = parseObjective(o->text);
                if (!parsed.problem.empty()) error(o->line, parsed.problem);
                parsed.objective.line = o->line;
                step.objective = parsed.objective;
            }
        } else {
            error(v.line, std::format("step \"{}\" needs an \"objective\"", id));
        }
        if (const JsonValue* n = v.find("next")) {
            if (!n->isString() || n->text.empty()) {
                error(n->line, "next must be a step id or \"END\"");
            } else {
                step.next = n->text;
                step.nextLine = n->line;
            }
        } else {
            error(v.line, std::format("step \"{}\" needs a \"next\" (a step id or \"END\")", id));
        }
        if (const JsonValue* h = v.find("hint")) readHint(*h, step);
        if (const JsonValue* b = v.find("branches")) readBranches(*b, step);
        if (report_.errors.size() != errorsBefore) return std::nullopt;
        return step;
    }

    void stringField(const JsonValue& v, const std::string& id, const char* field, std::string& target, bool required) {
        const JsonValue* f = v.find(field);
        if (f == nullptr) {
            if (required) error(v.line, std::format("step \"{}\" needs a \"{}\"", id, field));
            return;
        }
        if (!f->isString() || (required && f->text.empty())) {
            error(f->line, std::format("{} must be text", field));
            return;
        }
        target = f->text;
    }

    void readHint(const JsonValue& h, QuestStep& step) {
        if (!h.isObject()) {
            error(h.line, "hint must be { \"after\": \"120s\", \"text\": \"...\" }");
            return;
        }
        const JsonValue* after = h.find("after");
        const JsonValue* text = h.find("text");
        int amount = 0;
        DelayUnit unit = DelayUnit::Seconds;
        if (after == nullptr || !after->isString() || !parseLength(after->text, amount, unit)) {
            error(after != nullptr ? after->line : h.line, "hint.after must be a time like \"120s\" or \"2m\"");
            return;
        }
        if (unit == DelayUnit::Days) {
            error(after->line, "hint.after must be seconds or minutes (\"120s\", \"2m\")");
            return;
        }
        if (text == nullptr || !text->isString() || text->text.empty()) {
            error(text != nullptr ? text->line : h.line, "hint.text must be text");
            return;
        }
        step.hint = QuestHint{unit == DelayUnit::Minutes ? amount * 60 : amount, text->text};
    }

    void readBranches(const JsonValue& b, QuestStep& step) {
        if (!b.isArray()) {
            error(b.line, "branches must be a list of { \"if\": \"...\", \"to\": \"step\" }");
            return;
        }
        for (const JsonValue& item : b.items) {
            if (!item.isObject()) {
                error(item.line, "a branch is { \"if\": \"...\", \"to\": \"step\" }");
                continue;
            }
            const JsonValue* condition = item.find("if");
            const JsonValue* to = item.find("to");
            if (condition == nullptr || to == nullptr || !to->isString() || to->text.empty()) {
                error(item.line, "a branch needs an \"if\" and a \"to\"");
                continue;
            }
            auto parsed = readCondition(*condition, "a branch condition");
            if (!parsed) continue;
            step.branches.push_back({std::move(*parsed), to->text, to->line});
        }
    }

    void checkReferences(const Quest& q) {
        const auto exists = [&](const std::string& id) { return id == kQuestEnd || q.find(id) != nullptr || broken_.count(id) != 0; };
        if (!q.start.empty() && !exists(q.start) && !q.steps.empty()) {
            const JsonValue* v = root_.find("start");
            error(v != nullptr ? v->line : root_.line, std::format("unknown step \"{}\"", q.start));
        }
        for (const QuestStep& s : q.steps) {
            if (!s.next.empty() && !exists(s.next)) error(s.nextLine, std::format("unknown step \"{}\"", s.next));
            for (const QuestBranch& b : s.branches) {
                if (!exists(b.to)) error(b.toLine, std::format("unknown step \"{}\"", b.to));
            }
        }
    }
};

std::string quoted(const std::string& text) { return quoteJson(text); }

} // namespace

const QuestStep* Quest::find(std::string_view stepId) const {
    for (const QuestStep& s : steps) {
        if (s.id == stepId) return &s;
    }
    return nullptr;
}

ParsedObjective parseObjective(std::string_view source) {
    ParsedObjective result;
    QuestObjective& o = result.objective;
    o.source = std::string(source);
    const std::vector<std::string> words = core::splitWords(source);
    if (words.empty()) {
        result.problem = "the objective is empty (write for example \"gather berries 3\")";
        return result;
    }
    const std::string& verb = words[0];
    const auto needSubject = [&](const char* what) {
        if (words.size() < 2) {
            result.problem = std::format("{} needs {}: {} <{}>", verb, what, verb, what);
            return false;
        }
        o.subject = words[1];
        return true;
    };
    const auto readCount = [&](std::size_t index) {
        if (words.size() <= index) return true; // the count is optional: 1
        const std::string& n = words[index];
        const bool digits = !n.empty() && n.size() <= 4 && std::all_of(n.begin(), n.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; });
        if (!digits || std::stoi(n) < 1) {
            result.problem = std::format("\"{}\" is not a count from 1 to 9999", n);
            return false;
        }
        o.amount = std::stoi(n);
        return true;
    };
    const std::size_t maxWords = (verb == "gather" || verb == "give" || verb == "craft" || verb == "defeat") ? 3 : 2;
    if (words.size() > maxWords) {
        result.problem = std::format("{} takes {} word(s) after it, not {}", verb, maxWords - 1, words.size() - 1);
        return result;
    }
    if (verb == "talk") {
        o.kind = QuestObjective::Kind::Talk;
        needSubject("who");
    } else if (verb == "goto") {
        o.kind = QuestObjective::Kind::Goto;
        needSubject("place");
    } else if (verb == "interact") {
        o.kind = QuestObjective::Kind::Interact;
        needSubject("interaction");
    } else if (verb == "flag") {
        o.kind = QuestObjective::Kind::Flag;
        needSubject("name");
    } else if (verb == "gather" || verb == "give" || verb == "craft") {
        o.kind = verb == "gather" ? QuestObjective::Kind::Gather : verb == "give" ? QuestObjective::Kind::Give : QuestObjective::Kind::Craft;
        if (needSubject("item")) readCount(2);
    } else if (verb == "defeat") {
        o.kind = QuestObjective::Kind::Defeat;
        if (needSubject("kind")) readCount(2);
    } else if (verb == "wait") {
        o.kind = QuestObjective::Kind::Wait;
        if (words.size() != 2) {
            result.problem = "wait needs a time: wait 30s";
        } else if (!parseLength(words[1], o.amount, o.waitUnit)) {
            result.problem = std::format("\"{}\" is not a time (write 30s, 2m or 1d)", words[1]);
        }
    } else {
        result.problem = std::format("unknown objective \"{}\" (use talk, goto, gather, give, craft, interact, defeat, wait or flag)", verb);
    }
    if (result.problem.empty() && o.kind != QuestObjective::Kind::Wait && !isWord(o.subject)) {
        result.problem = std::format("\"{}\" must be one word of letters, digits, '-' or '_'", o.subject);
    }
    return result;
}

std::optional<Quest> parseQuest(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedId) {
    const JsonParseResult json = parseJson(text);
    if (!json.value) {
        report.errors.push_back({name, json.errorLine, "not valid JSON: " + json.error});
        return std::nullopt;
    }
    return QuestParser(*json.value, name, report).run(expectedId);
}

namespace {

std::string objectiveText(const QuestObjective& o) {
    if (!o.source.empty()) {
        // Written back in the plain form: single spaces, same words.
        std::string text;
        for (const std::string& w : core::splitWords(o.source)) text += (text.empty() ? "" : " ") + w;
        return text;
    }
    std::string text = kindWord(o.kind);
    if (!o.subject.empty()) text += " " + o.subject;
    return text;
}

std::string conditionList(const std::vector<QuestCondition>& list, const char* indent) {
    std::string text = "[";
    for (std::size_t i = 0; i < list.size(); ++i) text += std::format("{}\n{}  {}", i == 0 ? "" : ",", indent, quoted(list[i].source));
    if (!list.empty()) text += std::format("\n{}", indent);
    return text + "]";
}

} // namespace

std::string writeQuest(const Quest& q) {
    std::string out = "{\n";
    out += std::format("  \"id\": {},\n  \"title\": {},\n", quoted(q.id), quoted(q.title));
    if (!q.note.empty()) out += std::format("  \"note\": {},\n", quoted(q.note));
    out += std::format("  \"giver\": {},\n", quoted(q.giver));
    out += std::format("  \"requires\": {},\n", conditionList(q.requires_, "  "));
    out += std::format("  \"start\": {},\n  \"steps\": {{\n", quoted(q.start));
    for (std::size_t i = 0; i < q.steps.size(); ++i) {
        const QuestStep& s = q.steps[i];
        out += std::format("    {}: {{\n      \"text\": {},\n      \"objective\": {},\n", quoted(s.id), quoted(s.text), quoted(objectiveText(s.objective)));
        if (!s.marker.empty()) out += std::format("      \"marker\": {},\n", quoted(s.marker));
        if (s.hint) out += std::format("      \"hint\": {{ \"after\": \"{}s\", \"text\": {} }},\n", s.hint->afterSeconds, quoted(s.hint->text));
        if (!s.branches.empty()) {
            out += "      \"branches\": [\n";
            for (std::size_t b = 0; b < s.branches.size(); ++b) {
                out += std::format("        {{ \"if\": {}, \"to\": {} }}{}\n", quoted(s.branches[b].condition.source), quoted(s.branches[b].to),
                                   b + 1 < s.branches.size() ? "," : "");
            }
            out += "      ],\n";
        }
        out += std::format("      \"next\": {}\n    }}{}\n", quoted(s.next), i + 1 < q.steps.size() ? "," : "");
    }
    out += "  },\n";
    out += std::format("  \"fail\": {},\n", conditionList(q.fail, "  "));
    out += "  \"rewards\": [";
    for (std::size_t i = 0; i < q.rewards.size(); ++i) out += std::format("{}\n    {}", i == 0 ? "" : ",", quoted(q.rewards[i].source));
    out += q.rewards.empty() ? "]" : "\n  ]";
    out += std::format(",\n  \"journal\": {}", quoted(q.journal));
    if (!q.offer.empty()) out += std::format(",\n  \"offer\": {}", quoted(q.offer));
    if (!q.turnIn.empty()) out += std::format(",\n  \"turnIn\": {}", quoted(q.turnIn));
    out += "\n}\n";
    return out;
}

std::vector<Quest> loadQuests(const std::filesystem::path& folder, LoadReport& report) {
    std::vector<Quest> quests;
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) return quests; // a game without quests is fine
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        const std::string name = entry.path().filename().string();
        const bool layout = name.size() > 12 && name.compare(name.size() - 12, 12, ".layout.json") == 0;
        if (entry.is_regular_file() && entry.path().extension() == ".json" && !layout) files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& file : files) {
        ++report.filesRead;
        const std::string name = folder.filename().generic_string() + "/" + file.filename().generic_string();
        const std::optional<std::string> text = core::readTextFile(file);
        if (!text) {
            report.errors.push_back({name, 0, "the file cannot be read"});
            continue;
        }
        if (auto quest = parseQuest(*text, name, report, file.stem().string())) quests.push_back(std::move(*quest));
    }
    std::sort(quests.begin(), quests.end(), [](const Quest& a, const Quest& b) { return a.id < b.id; });
    report.loaded += static_cast<int>(quests.size());
    return quests;
}

} // namespace odysseus::sim::rules
