#include "sim/interaction.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <iterator>
#include <sstream>

namespace odysseus::sim::rules {

namespace {

constexpr int kMaxRangeMilli = 30'000;      // 30 m
constexpr int kMaxDurationMilli = 600'000;  // 10 minutes

struct PendingStart {
    std::string interactionId;
    std::string target; // the id named by a `start` effect
    int line;
};

const std::set<std::string>& knownFields() {
    static const std::set<std::string> fields = {"id", "label", "actors", "target", "range", "duration", "requires", "effects", "npc", "chronicle", "order", "note"};
    return fields;
}

class FileParser {
public:
    FileParser(const JsonValue& root, const std::string& name, LoadReport& report, std::vector<PendingStart>* pending, const LoadOptions* options)
        : root_(root), name_(name), report_(report), pending_(pending), options_(options) {}

    std::optional<Interaction> run(const std::string& expectedId) {
        const std::size_t errorsBefore = report_.errors.size();
        Interaction out;
        out.file = name_;
        if (!root_.isObject()) {
            error(root_.line, "the file must hold one {...} object");
            return std::nullopt;
        }
        for (std::size_t i = 0; i < root_.keys.size(); ++i) {
            if (knownFields().count(root_.keys[i]) == 0) {
                error(root_.keyLines[i], std::format("unknown field \"{}\" (known: id, label, actors, target, range, duration, requires, effects, npc, chronicle, order, note)", root_.keys[i]));
            }
        }
        readId(out, expectedId);
        readLabel(out);
        readActors(out);
        readTarget(out);
        out.rangeMilli = readMilli("range", out.rangeMilli, 0, kMaxRangeMilli, "metres");
        out.durationMilli = readMilli("duration", 0, 0, kMaxDurationMilli, "seconds");
        readOrder(out);
        readRequires(out);
        readEffects(out);
        readNpc(out);
        readChronicle(out);
        if (const JsonValue* note = root_.find("note")) {
            if (note->isString()) out.note = note->text;
            else error(note->line, "note must be text");
        }
        if (report_.errors.size() != errorsBefore) return std::nullopt;
        return out;
    }

private:
    const JsonValue& root_;
    std::string name_;
    LoadReport& report_;
    std::vector<PendingStart>* pending_;
    const LoadOptions* options_;
    std::string id_;

    void error(int line, std::string message) { report_.errors.push_back({name_, line, std::move(message)}); }
    void warning(int line, std::string message) { report_.warnings.push_back({name_, line, std::move(message)}); }

    const JsonValue* need(const char* field) {
        const JsonValue* v = root_.find(field);
        if (v == nullptr) error(root_.line, std::format("the field \"{}\" is missing", field));
        return v;
    }

    static bool isWord(const std::string& s) {
        if (s.empty()) return false;
        return std::all_of(s.begin(), s.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-'; });
    }

    void readId(Interaction& out, const std::string& expectedId) {
        const JsonValue* v = need("id");
        if (v == nullptr) return;
        if (!v->isString() || !isWord(v->text)) {
            error(v->line, "id must be one word of letters, digits, '-' or '_'");
            return;
        }
        out.id = v->text;
        id_ = v->text;
        if (!expectedId.empty() && v->text != expectedId) {
            error(v->line, std::format("id \"{}\" must match the file name \"{}\"", v->text, expectedId));
        }
    }

    // {target.name}: every token must be a name an actor, target, npc or hero has.
    void checkTokens(const std::string& text, int line, const char* what) {
        std::size_t at = 0;
        while ((at = text.find('{', at)) != std::string::npos) {
            const std::size_t close = text.find('}', at);
            if (close == std::string::npos) {
                error(line, std::format("{}: a '{{' is never closed", what));
                return;
            }
            const std::string token = text.substr(at + 1, close - at - 1);
            const std::string root = token.substr(0, token.find('.'));
            if (!isPathRoot(root)) {
                error(line, std::format("{}: unknown token \"{{{}}}\" (use {{actor.name}}, {{target.name}}, {{npc.name}} or {{hero.name}})", what, token));
            }
            at = close + 1;
        }
    }

    void readLabel(Interaction& out) {
        const JsonValue* v = need("label");
        if (v == nullptr) return;
        if (!v->isString() || v->text.empty()) {
            error(v->line, "label must be text, like \"Gather {target.name}\"");
            return;
        }
        out.label = v->text;
        checkTokens(v->text, v->line, "label");
    }

    std::vector<std::string> readStrings(const JsonValue& list, const char* what) {
        std::vector<std::string> out;
        if (!list.isArray()) {
            error(list.line, std::format("{} must be a list of words, like [\"a\", \"b\"]", what));
            return out;
        }
        for (const JsonValue& item : list.items) {
            if (!item.isString() || !isWord(item.text)) {
                error(item.line, std::format("{} holds words of letters, digits, '-' or '_'", what));
                continue;
            }
            out.push_back(item.text);
        }
        return out;
    }

    void readActors(Interaction& out) {
        const JsonValue* v = need("actors");
        if (v == nullptr) return;
        out.actors = readStrings(*v, "actors");
        if (v->isArray() && v->items.empty()) error(v->line, "actors must name at least one: hero, person, animal or a kind");
    }

    void readTarget(Interaction& out) {
        const JsonValue* v = need("target");
        if (v == nullptr) return;
        if (!v->isObject()) {
            error(v->line, "target must be like { \"tags\": [\"edible\", \"plant\"] }");
            return;
        }
        const std::size_t before = report_.errors.size();
        for (std::size_t i = 0; i < v->keys.size(); ++i) {
            if (v->keys[i] != "tags" && v->keys[i] != "kinds") error(v->keyLines[i], std::format("unknown field \"{}\" in target (known: tags, kinds)", v->keys[i]));
        }
        if (const JsonValue* tags = v->find("tags")) {
            out.targetTags = readStrings(*tags, "tags");
            if (options_ != nullptr && !options_->knownTags.empty() && tags->isArray()) {
                for (const JsonValue& item : tags->items) {
                    if (item.isString() && options_->knownTags.count(item.text) == 0) {
                        warning(item.line, std::format("unknown tag \"{}\": no catalog uses it, so this will never match", item.text));
                    }
                }
            }
        }
        if (const JsonValue* kinds = v->find("kinds")) out.targetKinds = readStrings(*kinds, "kinds");
        if (out.targetTags.empty() && out.targetKinds.empty() && report_.errors.size() == before) {
            error(v->line, "target needs tags or kinds, or it would match everything");
        }
    }

    int readMilli(const char* field, int fallback, int minimum, int maximum, const char* unit) {
        const JsonValue* v = root_.find(field);
        if (v == nullptr) return fallback;
        if (!v->isNumber()) {
            error(v->line, std::format("{} must be a number of {}", field, unit));
            return fallback;
        }
        const auto milli = parseMilli(v->text);
        if (!milli) {
            error(v->line, std::format("{} \"{}\" is not a plain number with at most three decimals", field, v->text));
            return fallback;
        }
        if (*milli < minimum || *milli > maximum) {
            error(v->line, std::format("{} must be between {} and {} {} (is {})", field, formatMilli(minimum), formatMilli(maximum), unit, v->text));
            return fallback;
        }
        return static_cast<int>(*milli);
    }

    void readOrder(Interaction& out) {
        const JsonValue* v = root_.find("order");
        if (v == nullptr) return;
        const auto milli = v->isNumber() ? parseMilli(v->text) : std::nullopt;
        if (!milli || *milli % 1000 != 0 || *milli < 0 || *milli > 1'000'000) {
            error(v->line, "order must be a whole number from 0 to 1000");
            return;
        }
        out.order = static_cast<int>(*milli / 1000);
    }

    ExprPtr readExpression(const JsonValue& v, const char* what) {
        if (!v.isString()) {
            error(v.line, std::format("{} must be text", what));
            return nullptr;
        }
        ParsedExpr parsed = parseExpression(v.text);
        if (parsed.problem) {
            error(v.line, parsed.problem->message);
            return nullptr;
        }
        return parsed.root;
    }

    void readRequires(Interaction& out) {
        const JsonValue* v = root_.find("requires");
        if (v == nullptr) return;
        if (!v->isArray()) {
            error(v->line, "requires must be a list of { \"if\": \"...\", \"else\": \"why not\" }");
            return;
        }
        for (const JsonValue& item : v->items) {
            Requirement r;
            r.otherwise = "Not possible now";
            const JsonValue* condition = &item;
            if (item.isObject()) {
                for (std::size_t i = 0; i < item.keys.size(); ++i) {
                    if (item.keys[i] != "if" && item.keys[i] != "else") error(item.keyLines[i], std::format("unknown field \"{}\" in a requirement (known: if, else)", item.keys[i]));
                }
                condition = item.find("if");
                if (condition == nullptr) {
                    error(item.line, "a requirement needs an \"if\"");
                    continue;
                }
                if (const JsonValue* otherwise = item.find("else")) {
                    if (otherwise->isString()) r.otherwise = otherwise->text;
                    else error(otherwise->line, "else must be text");
                }
            }
            r.source = condition->isString() ? condition->text : std::string();
            r.condition = readExpression(*condition, "a condition");
            if (r.condition) out.requires_.push_back(std::move(r));
        }
    }

    void collectStarts(const Effect& effect, int line) {
        if (effect.verb == "start" && pending_ != nullptr && !effect.args.empty() && effect.args[0]->kind == Expr::Kind::Text) {
            pending_->push_back({id_, effect.args[0]->text, line});
        }
        if (effect.inner) collectStarts(*effect.inner, line);
    }

    void readEffects(Interaction& out) {
        const JsonValue* v = need("effects");
        if (v == nullptr) return;
        if (!v->isArray()) {
            error(v->line, "effects must be a list of lines like \"give actor berries 2\"");
            return;
        }
        if (v->items.empty()) error(v->line, "effects is empty: say what happens (for example \"say \\\"Nothing to do\\\"\")");
        for (const JsonValue& item : v->items) {
            if (!item.isString()) {
                error(item.line, "an effect is one line of text, like \"give actor berries 2\"");
                continue;
            }
            ParsedEffect parsed = parseEffect(item.text);
            if (parsed.problem) {
                error(item.line, parsed.problem->message);
                continue;
            }
            collectStarts(parsed.effect, item.line);
            out.effects.push_back(std::move(parsed.effect));
        }
    }

    void readNpc(Interaction& out) {
        const JsonValue* v = root_.find("npc");
        if (v == nullptr) return;
        if (!v->isObject()) {
            error(v->line, "npc must be like { \"score\": \"need(hunger) * 2\", \"cooldown\": 60 }");
            return;
        }
        for (std::size_t i = 0; i < v->keys.size(); ++i) {
            if (v->keys[i] != "score" && v->keys[i] != "cooldown") error(v->keyLines[i], std::format("unknown field \"{}\" in npc (known: score, cooldown)", v->keys[i]));
        }
        NpcRule rule;
        const JsonValue* score = v->find("score");
        if (score == nullptr) {
            error(v->line, "npc needs a \"score\"");
            return;
        }
        rule.scoreSource = score->isString() ? score->text : std::string();
        rule.score = readExpression(*score, "the score");
        if (const JsonValue* cooldown = v->find("cooldown")) {
            const auto milli = cooldown->isNumber() ? parseMilli(cooldown->text) : std::nullopt;
            if (!milli || *milli % 1000 != 0 || *milli < 0 || *milli > 86'400'000) {
                error(cooldown->line, "cooldown must be a whole number of seconds (0 to 86400)");
            } else {
                rule.cooldownSeconds = static_cast<int>(*milli / 1000);
            }
        }
        if (rule.score) out.npc = std::move(rule);
    }

    void readChronicle(Interaction& out) {
        const JsonValue* v = root_.find("chronicle");
        if (v == nullptr || v->kind == JsonValue::Kind::Null) return;
        if (!v->isString()) {
            error(v->line, "chronicle must be null or a line of text");
            return;
        }
        out.chronicle = v->text;
        checkTokens(v->text, v->line, "chronicle");
    }
};

std::string readWholeFile(const std::filesystem::path& file, bool& ok) {
    std::ifstream in(file, std::ios::binary);
    ok = static_cast<bool>(in);
    if (!ok) return {};
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::optional<Interaction> parseText(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedId,
                                     std::vector<PendingStart>* pending, const LoadOptions* options) {
    const JsonParseResult json = parseJson(text);
    if (!json.value) {
        report.errors.push_back({name, json.errorLine, "not valid JSON: " + json.error});
        return std::nullopt;
    }
    return FileParser(*json.value, name, report, pending, options).run(expectedId);
}

bool matchesActor(const Interaction& interaction, const ThingInfo& actor) {
    for (const std::string& allowed : interaction.actors) {
        if (allowed == actor.kind) return true;
        if (std::find(actor.tags.begin(), actor.tags.end(), allowed) != actor.tags.end()) return true;
    }
    return false;
}

bool matchesTarget(const Interaction& interaction, const ThingInfo& target) {
    for (const std::string& tag : interaction.targetTags) {
        if (std::find(target.tags.begin(), target.tags.end(), tag) == target.tags.end()) return false;
    }
    if (!interaction.targetKinds.empty() &&
        std::find(interaction.targetKinds.begin(), interaction.targetKinds.end(), target.kind) == interaction.targetKinds.end()) {
        return false;
    }
    return true;
}

} // namespace

std::optional<Interaction> InteractionRegistry::parse(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedId) {
    return parseText(text, name, report, expectedId, nullptr, nullptr);
}

InteractionRegistry InteractionRegistry::load(const std::filesystem::path& folder, LoadReport& report, const LoadOptions& options) {
    InteractionRegistry registry;
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) {
        report.errors.push_back({folder.filename().generic_string(), 0, "the interactions folder cannot be found"});
        return registry;
    }
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());

    std::vector<PendingStart> pending;
    for (const std::filesystem::path& file : files) {
        ++report.filesRead;
        const std::string name = folder.filename().generic_string() + "/" + file.filename().generic_string();
        bool ok = false;
        const std::string text = readWholeFile(file, ok);
        if (!ok) {
            report.errors.push_back({name, 0, "the file cannot be read"});
            continue;
        }
        if (auto interaction = parseText(text, name, report, file.stem().string(), &pending, &options)) {
            registry.interactions_.push_back(std::move(*interaction));
        }
    }

    // `start` must name an interaction that exists; one that does not takes its own interaction out.
    std::set<std::string> ids = options.extraIds;
    for (const Interaction& i : registry.interactions_) ids.insert(i.id);
    std::set<std::string> broken;
    for (const PendingStart& p : pending) {
        if (ids.count(p.target) == 0) {
            const auto owner = std::find_if(registry.interactions_.begin(), registry.interactions_.end(), [&](const Interaction& i) { return i.id == p.interactionId; });
            if (owner != registry.interactions_.end()) {
                report.errors.push_back({owner->file, p.line, std::format("start names \"{}\", which is not an interaction", p.target)});
                broken.insert(p.interactionId);
            }
        }
    }
    std::erase_if(registry.interactions_, [&](const Interaction& i) { return broken.count(i.id) != 0; });

    std::stable_sort(registry.interactions_.begin(), registry.interactions_.end(), [](const Interaction& a, const Interaction& b) {
        return a.order != b.order ? a.order < b.order : a.id < b.id;
    });
    report.loaded = static_cast<int>(registry.interactions_.size());
    return registry;
}

const Interaction* InteractionRegistry::find(std::string_view id) const {
    for (const Interaction& i : interactions_) {
        if (i.id == id) return &i;
    }
    return nullptr;
}

std::vector<Offer> InteractionRegistry::offered(const ThingInfo& actor, const ThingInfo& target, long long distanceMilli, const RuleContext& context) const {
    std::vector<Offer> offers;
    for (const Interaction& interaction : interactions_) {
        if (!matchesActor(interaction, actor) || !matchesTarget(interaction, target)) continue;
        Offer offer;
        offer.interaction = &interaction;
        if (distanceMilli > interaction.rangeMilli) {
            offer.enabled = false;
            offer.reason = "Too far away";
        } else {
            for (const Requirement& r : interaction.requires_) {
                if (!isTrue(*r.condition, context)) {
                    offer.enabled = false;
                    offer.reason = r.otherwise;
                    break;
                }
            }
        }
        offers.push_back(std::move(offer));
    }
    return offers;
}

std::string fillTokens(const std::string& text, const RuleContext& context) {
    std::string out;
    std::size_t at = 0;
    while (at < text.size()) {
        const std::size_t open = text.find('{', at);
        if (open == std::string::npos) {
            out += text.substr(at);
            break;
        }
        const std::size_t close = text.find('}', open);
        if (close == std::string::npos) {
            out += text.substr(at);
            break;
        }
        out += text.substr(at, open - at);
        const std::string token = text.substr(open + 1, close - open - 1);
        if (isPathRoot(token.substr(0, token.find('.')))) {
            const Value v = context.path(token);
            out += v.isText ? v.text : std::format("{}", v.number);
        } else {
            out += text.substr(open, close - open + 1);
        }
        at = close + 1;
    }
    return out;
}

std::string toJson(const Interaction& i) {
    const auto list = [](const std::vector<std::string>& words) {
        std::string out = "[";
        for (std::size_t k = 0; k < words.size(); ++k) out += (k ? ", " : "") + quoteJson(words[k]);
        return out + "]";
    };
    std::string out = "{\n";
    out += std::format("  \"id\": {},\n", quoteJson(i.id));
    out += std::format("  \"label\": {},\n", quoteJson(i.label));
    if (!i.note.empty()) out += std::format("  \"note\": {},\n", quoteJson(i.note));
    out += std::format("  \"actors\": {},\n", list(i.actors));
    out += "  \"target\": {";
    bool first = true;
    if (!i.targetTags.empty()) {
        out += std::format(" \"tags\": {}", list(i.targetTags));
        first = false;
    }
    if (!i.targetKinds.empty()) out += std::format("{} \"kinds\": {}", first ? "" : ",", list(i.targetKinds));
    out += " },\n";
    out += std::format("  \"range\": {},\n", formatMilli(i.rangeMilli));
    out += std::format("  \"duration\": {},\n", formatMilli(i.durationMilli));
    out += std::format("  \"order\": {},\n", i.order);
    out += "  \"requires\": [";
    for (std::size_t k = 0; k < i.requires_.size(); ++k) {
        out += std::format("{}\n    {{ \"if\": {}, \"else\": {} }}", k ? "," : "", quoteJson(i.requires_[k].source), quoteJson(i.requires_[k].otherwise));
    }
    out += i.requires_.empty() ? "],\n" : "\n  ],\n";
    out += "  \"effects\": [";
    for (std::size_t k = 0; k < i.effects.size(); ++k) out += std::format("{}\n    {}", k ? "," : "", quoteJson(i.effects[k].source));
    out += i.effects.empty() ? "]" : "\n  ]";
    if (i.npc) out += std::format(",\n  \"npc\": {{ \"score\": {}, \"cooldown\": {} }}", quoteJson(i.npc->scoreSource), i.npc->cooldownSeconds);
    if (i.chronicle) out += std::format(",\n  \"chronicle\": {}", quoteJson(*i.chronicle));
    out += "\n}\n";
    return out;
}

} // namespace odysseus::sim::rules
