#include "sim/npc_kind.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <set>
#include <sstream>

namespace odysseus::sim::rules {

namespace {

class KindParser {
public:
    KindParser(const JsonValue& root, const std::string& name, LoadReport& report) : root_(root), name_(name), report_(report) {}

    std::optional<NpcKind> run(const std::string& expectedKind) {
        const std::size_t before = report_.errors.size();
        NpcKind out;
        out.file = name_;
        if (!root_.isObject()) {
            error(root_.line, "the file must hold one {...} object");
            return std::nullopt;
        }
        std::set<std::string> known = {"kind", "classes", "attitude", "tags", "dialogues", "actions"};
        std::string knownText = "kind, classes, attitude, tags, dialogues, actions";
        for (const std::string& extra : extrasFieldNames()) {
            known.insert(extra);
            knownText += ", " + extra;
        }
        for (std::size_t i = 0; i < root_.keys.size(); ++i) {
            if (known.count(root_.keys[i]) == 0) {
                error(root_.keyLines[i], std::format("unknown field \"{}\" (known: {})", root_.keys[i], knownText));
            }
        }
        if (const JsonValue* kind = root_.find("kind"); kind != nullptr && kind->isString() && !kind->text.empty()) {
            out.kind = kind->text;
            if (!expectedKind.empty() && out.kind != expectedKind) error(kind->line, std::format("kind \"{}\" must match the file name \"{}\"", out.kind, expectedKind));
        } else {
            error(kind != nullptr ? kind->line : root_.line, "kind must be the name of a character kind, in quotes");
        }
        if (const JsonValue* classes = root_.find("classes")) out.layer.classes = list(classes, "classes");
        if (const JsonValue* attitude = root_.find("attitude")) {
            if (!attitude->isString() || !validAttitude(attitude->text)) {
                error(attitude->line, "attitude must be one of: friendly, neutral, wary, hostile, scared, suspicious, enchanted, lovingly, enviously");
            } else {
                out.layer.attitude = attitude->text;
            }
        }
        out.layer.tags = list(root_.find("tags"), "tags");
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
                        out.layer.dialogues.emplace_back(dialogues->keys[i], value.text);
                    }
                }
            }
        }
        if (const JsonValue* actions = root_.find("actions")) {
            if (!actions->isObject()) {
                error(actions->line, "actions must be {\"allow\": [...], \"deny\": [...]}");
            } else {
                for (std::size_t i = 0; i < actions->keys.size(); ++i) {
                    if (actions->keys[i] != "allow" && actions->keys[i] != "deny") error(actions->keyLines[i], std::format("unknown field \"{}\" (allow or deny)", actions->keys[i]));
                }
                out.layer.allow = list(actions->find("allow"), "actions.allow");
                out.layer.deny = list(actions->find("deny"), "actions.deny");
            }
        }
        out.layer.extras = parseExtras(root_, [this](int at, const std::string& message) { error(at, message); });
        if (report_.errors.size() != before) return std::nullopt;
        return out;
    }

private:
    void error(int at, const std::string& message) { report_.errors.push_back({name_, at, message}); }
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

void applyActions(std::map<std::string, ActionState>& actions, const std::vector<std::string>& allow, const std::vector<std::string>& deny) {
    for (const std::string& id : allow) actions[id] = ActionState::Allowed;
    for (const std::string& id : deny) actions[id] = ActionState::Denied; // a deny inside one layer wins over an allow of the same layer
}

} // namespace

const std::vector<std::string>& attitudeNames() {
    static const std::vector<std::string> names = [] {
        std::vector<std::string> all;
        for (std::size_t i = 0; i < kAttitudeCount; ++i) all.push_back(attitudeName(static_cast<Attitude>(i)));
        return all;
    }();
    return names;
}

bool validAttitude(const std::string& word) { return attitudeFromName(word).has_value(); }

std::optional<NpcKind> NpcKindCatalog::parse(std::string_view text, const std::string& name, LoadReport& report, const std::string& expectedKind) {
    const JsonParseResult parsed = parseJson(text);
    if (!parsed.value) {
        report.errors.push_back({name, parsed.errorLine, parsed.error});
        return std::nullopt;
    }
    return KindParser(*parsed.value, name, report).run(expectedKind);
}

NpcKindCatalog NpcKindCatalog::load(const std::filesystem::path& folder, LoadReport& report) {
    NpcKindCatalog catalog;
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
        if (auto parsed = parse(text.str(), name, report, file.stem().string())) catalog.kinds_.push_back(std::move(*parsed));
    }
    std::sort(catalog.kinds_.begin(), catalog.kinds_.end(), [](const NpcKind& a, const NpcKind& b) { return a.kind < b.kind; });
    report.loaded += static_cast<int>(catalog.kinds_.size());
    return catalog;
}

const NpcKind* NpcKindCatalog::find(std::string_view kind) const {
    for (const NpcKind& k : kinds_) {
        if (k.kind == kind) return &k;
    }
    return nullptr;
}

std::string toJson(const NpcKind& k) {
    const auto strings = [](const std::vector<std::string>& values) {
        std::string out = "[";
        for (std::size_t i = 0; i < values.size(); ++i) out += (i != 0 ? ", " : "") + quoteJson(values[i]);
        return out + "]";
    };
    std::string out = "{\n";
    out += std::format("  \"kind\": {}", quoteJson(k.kind));
    if (k.layer.classes) out += std::format(",\n  \"classes\": {}", strings(*k.layer.classes));
    if (k.layer.attitude) out += std::format(",\n  \"attitude\": {}", quoteJson(*k.layer.attitude));
    if (!k.layer.tags.empty()) out += std::format(",\n  \"tags\": {}", strings(k.layer.tags));
    if (!k.layer.dialogues.empty()) {
        out += ",\n  \"dialogues\": {";
        for (std::size_t i = 0; i < k.layer.dialogues.size(); ++i) out += std::format("{} {}: {}", i != 0 ? "," : "", quoteJson(k.layer.dialogues[i].first), quoteJson(k.layer.dialogues[i].second));
        out += " }";
    }
    if (!k.layer.allow.empty() || !k.layer.deny.empty()) out += std::format(",\n  \"actions\": {{ \"allow\": {}, \"deny\": {} }}", strings(k.layer.allow), strings(k.layer.deny));
    for (const auto& [name, text] : extrasFieldTexts(k.layer.extras)) out += std::format(",\n  \"{}\": {}", name, text);
    out += "\n}\n";
    return out;
}

ActionState ResolvedNpc::action(const std::string& id) const {
    const auto found = actions.find(id);
    return found == actions.end() ? ActionState::Unset : found->second;
}

ResolvedNpc resolveNpc(const NpcClassCatalog& classes, const NpcLayer* kind, const NpcLayer& placed) {
    ResolvedNpc out;
    // Which classes the NPC has: the placed NPC says, else the kind, else none.
    if (placed.classes) out.classes = *placed.classes;
    else if (kind != nullptr && kind->classes) out.classes = *kind->classes;
    // Attitude: placed, else kind, else neutral.
    if (placed.attitude) out.attitude = *placed.attitude;
    else if (kind != nullptr && kind->attitude) out.attitude = *kind->attitude;

    std::set<std::string> tags;
    const auto addTags = [&tags](const std::vector<std::string>& more) { tags.insert(more.begin(), more.end()); };
    // Lowest layer: the classes, in the order the NPC has them (a later class overrides an earlier one on the same partner type or action).
    for (const std::string& id : out.classes) {
        const NpcClass* npcClass = classes.find(id);
        if (npcClass == nullptr) continue; // a class without a file: nothing to add (the game warns)
        addTags(npcClass->tags);
        for (const auto& [partner, file] : npcClass->dialogues) out.dialogues[partner] = file;
        applyActions(out.actions, npcClass->allow, npcClass->deny);
        mergeExtras(out.extras, npcClass->extras);
    }
    // Then the kind, then the placed NPC.
    for (const NpcLayer* layer : {kind, &placed}) {
        if (layer == nullptr) continue;
        addTags(layer->tags);
        for (const auto& [partner, file] : layer->dialogues) out.dialogues[partner] = file;
        applyActions(out.actions, layer->allow, layer->deny);
        mergeExtras(out.extras, layer->extras);
    }
    if (!out.extras.trade.empty()) tags.insert("trader"); // any NPC with a trade profile can trade; the Trader class is only a default profile (D-54 Q7)
    out.tags.assign(tags.begin(), tags.end());
    return out;
}

} // namespace odysseus::sim::rules
