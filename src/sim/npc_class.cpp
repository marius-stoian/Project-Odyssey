#include "sim/npc_class.h"

#include "sim/economy.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <fstream>
#include <set>
#include <sstream>

namespace odysseus::sim::rules {

namespace {

class ClassParser {
public:
    ClassParser(const JsonValue& root, const std::string& name, LoadReport& report) : root_(root), name_(name), report_(report) {}

    std::optional<NpcClass> run(const std::string& expectedId) {
        const std::size_t before = report_.errors.size();
        NpcClass out;
        out.file = name_;
        if (!root_.isObject()) {
            error(root_.line, "the file must hold one {...} object");
            return std::nullopt;
        }
        std::set<std::string> known = {"id", "label", "colour", "icon", "tags", "dialogues", "actions"};
        std::string knownText = "id, label, colour, icon, tags, dialogues, actions";
        for (const std::string& extra : extrasFieldNames()) {
            known.insert(extra);
            knownText += ", " + extra;
        }
        for (std::size_t i = 0; i < root_.keys.size(); ++i) {
            if (known.count(root_.keys[i]) == 0) {
                error(root_.keyLines[i], std::format("unknown field \"{}\" (known: {})", root_.keys[i], knownText));
            }
        }
        out.id = text("id", true);
        if (!out.id.empty() && !validItemId(out.id)) error(line("id"), "id must be lower-case letters, digits and - (at most 32)");
        if (!expectedId.empty() && !out.id.empty() && out.id != expectedId) error(line("id"), std::format("id \"{}\" must match the file name \"{}\"", out.id, expectedId));
        out.label = text("label", true);
        if (const JsonValue* colour = root_.find("colour")) {
            const auto value = colour->isString() ? parseColour(colour->text) : std::nullopt;
            if (!value) error(colour->line, "colour must be text like \"#d9a441\" (# and six hex digits)");
            else out.colour = *value;
        } else {
            error(root_.line, "missing field \"colour\"");
        }
        out.icon = text("icon", true);
        if (!out.icon.empty()) {
            const auto& icons = npcIconNames();
            if (std::find(icons.begin(), icons.end(), out.icon) == icons.end()) error(line("icon"), std::format("unknown icon \"{}\" (see docs/guides/npc-data.md for the icon set)", out.icon));
        }
        out.tags = list(root_.find("tags"), "tags");
        if (const JsonValue* dialogues = root_.find("dialogues")) {
            if (!dialogues->isObject()) {
                error(dialogues->line, "dialogues must be {\"player\": \"file.dlg\", ...}");
            } else {
                for (std::size_t i = 0; i < dialogues->keys.size(); ++i) {
                    const JsonValue& value = dialogues->items[i];
                    if (!validPartnerType(dialogues->keys[i])) {
                        error(dialogues->keyLines[i], std::format("unknown partner type \"{}\" (player, animal, environment or class:<id>)", dialogues->keys[i]));
                    } else if (!value.isString() || value.text.size() < 5 || value.text.substr(value.text.size() - 4) != ".dlg") {
                        error(value.line, "a dialogue must be the name of a .dlg file");
                    } else {
                        out.dialogues.emplace_back(dialogues->keys[i], value.text);
                    }
                }
            }
        }
        if (const JsonValue* actions = root_.find("actions")) {
            if (!actions->isObject()) {
                error(actions->line, "actions must be {\"allow\": [...], \"deny\": [...]}");
            } else {
                for (std::size_t i = 0; i < actions->keys.size(); ++i) {
                    if (actions->keys[i] != "allow" && actions->keys[i] != "deny") {
                        error(actions->keyLines[i], std::format("unknown field \"{}\" (allow or deny)", actions->keys[i]));
                    }
                }
                out.allow = list(actions->find("allow"), "actions.allow");
                out.deny = list(actions->find("deny"), "actions.deny");
            }
        }
        out.extras = parseExtras(root_, [this](int at, const std::string& message) { error(at, message); });
        if (report_.errors.size() != before) return std::nullopt;
        return out;
    }

private:
    void error(int at, const std::string& message) { report_.errors.push_back({name_, at, message}); }
    int line(const char* key) const {
        const JsonValue* value = root_.find(key);
        return value != nullptr ? value->line : root_.line;
    }
    std::string text(const char* key, bool required) {
        const JsonValue* value = root_.find(key);
        if (value == nullptr) {
            if (required) error(root_.line, std::format("missing field \"{}\"", key));
            return {};
        }
        if (!value->isString() || value->text.empty()) {
            error(value->line, std::format("{} must be text in quotes", key));
            return {};
        }
        return value->text;
    }
    std::vector<std::string> list(const JsonValue* value, const char* key) {
        std::vector<std::string> out;
        if (value == nullptr) return out;
        if (!value->isArray()) {
            error(value->line, std::format("{} must be a list of names", key));
            return out;
        }
        for (const JsonValue& item : value->items) {
            if (!item.isString() || item.text.empty()) error(item.line, std::format("{} must hold names in quotes", key));
            else out.push_back(item.text);
        }
        return out;
    }

    const JsonValue& root_;
    const std::string& name_;
    LoadReport& report_;
};

} // namespace

const std::vector<std::string>& npcIconNames() {
    static const std::vector<std::string> names = {"person", "coin", "crown", "shield", "sword", "bow", "heart", "skull", "star", "flame", "leaf", "paw",
                                                   "book", "cross", "hammer", "pick", "flask", "key", "eye", "moon", "sun", "drop", "tooth", "wing"};
    return names;
}


std::optional<int> parseColour(const std::string& text) {
    if (text.size() != 7 || text[0] != '#') return std::nullopt;
    int value = 0;
    for (std::size_t i = 1; i < 7; ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (std::isxdigit(c) == 0) return std::nullopt;
        value = value * 16 + (std::isdigit(c) != 0 ? c - '0' : std::tolower(c) - 'a' + 10);
    }
    return value;
}

std::string formatColour(int rgb) { return std::format("#{:06x}", rgb & 0xFFFFFF); }

std::optional<NpcClass> NpcClassCatalog::parse(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedId) {
    const JsonParseResult parsed = parseJson(text);
    if (!parsed.value) {
        report.errors.push_back({name, parsed.errorLine, parsed.error});
        return std::nullopt;
    }
    if (const std::vector<Diagnostic> mistakes = schemaDiagnostics(name, text); !mistakes.empty()) { // the schema (US-190)
        report.errors.insert(report.errors.end(), mistakes.begin(), mistakes.end());
        return std::nullopt;
    }
    return ClassParser(*parsed.value, name, report).run(expectedId);
}

NpcClassCatalog NpcClassCatalog::load(const std::filesystem::path& folder, LoadReport& report) {
    NpcClassCatalog catalog;
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) return catalog;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& file : files) {
        ++report.filesRead;
        const std::string name = folder.filename().generic_string() + "/" + file.filename().generic_string();
        std::ifstream in(file, std::ios::binary);
        if (!in) {
            report.errors.push_back({name, 0, "the file cannot be read"});
            continue;
        }
        std::stringstream text;
        text << in.rdbuf();
        if (auto parsed = parse(text.str(), name, report, file.stem().string())) catalog.classes_.push_back(std::move(*parsed));
    }
    std::sort(catalog.classes_.begin(), catalog.classes_.end(), [](const NpcClass& a, const NpcClass& b) { return a.id < b.id; });
    report.loaded += static_cast<int>(catalog.classes_.size());
    return catalog;
}

const NpcClass* NpcClassCatalog::find(std::string_view id) const {
    for (const NpcClass& c : classes_) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

std::vector<std::string> NpcClassCatalog::ids() const {
    std::vector<std::string> out;
    for (const NpcClass& c : classes_) out.push_back(c.id);
    return out;
}

std::string toJson(const NpcClass& c) {
    const auto strings = [](const std::vector<std::string>& values) {
        std::string out = "[";
        for (std::size_t i = 0; i < values.size(); ++i) out += (i != 0 ? ", " : "") + quoteJson(values[i]);
        return out + "]";
    };
    std::string out = "{\n";
    out += std::format("  \"id\": {},\n  \"label\": {},\n  \"colour\": {},\n  \"icon\": {},\n", quoteJson(c.id), quoteJson(c.label), quoteJson(formatColour(c.colour)), quoteJson(c.icon));
    out += std::format("  \"tags\": {},\n", strings(c.tags));
    out += "  \"dialogues\": {";
    for (std::size_t i = 0; i < c.dialogues.size(); ++i) out += std::format("{} {}: {}", i != 0 ? "," : "", quoteJson(c.dialogues[i].first), quoteJson(c.dialogues[i].second));
    out += std::string(c.dialogues.empty() ? "" : " ") + "},\n";
    out += std::format("  \"actions\": {{ \"allow\": {}, \"deny\": {} }}", strings(c.allow), strings(c.deny));
    for (const auto& [name, text] : extrasFieldTexts(c.extras)) out += std::format(",\n  \"{}\": {}", name, text);
    out += "\n}\n";
    return out;
}

} // namespace odysseus::sim::rules
