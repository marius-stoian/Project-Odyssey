#include "sim/data_refs.h"

#include "core/text.h"
#include "sim/data_document.h"
#include "sim/json_text.h"
#include "sim/schema_index.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <functional>

namespace odysseus::sim::refs {

namespace fs = std::filesystem;

namespace {

bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-'; }

bool isPlainWord(const std::string& text) { return !text.empty() && std::all_of(text.begin(), text.end(), isWordChar); }

// The formats whose text is a rule, an effect or an objective: the places a name may be written as a word.
bool isRuleFormat(const std::string& format) { return format == "rule" || format == "effect" || format == "objective"; }

// A level has a fixed shape (no schema): which list and field of it name which catalog.
struct LevelField {
    const char* list;
    const char* field;
    const char* catalog;
};
constexpr LevelField kLevelFields[] = {
    {"characters", "kind", "characters"}, {"pickups", "weapon", "weapons"}, {"plants", "kind", "kinds"},
    {"effects", "name", "effects"},       {"lights", "kind", "lights"},     {"buildings", "kind", "building-kinds"},
};

// One file being worked on: its text and document, the edits made to the document, and the places that changed.
class Work {
public:
    Work(std::string label, fs::path path, std::string text, std::string& problem) : label_(std::move(label)), path_(std::move(path)), lines_(JsonLines::scan(text)) {
        document_ = DataDocument::fromText(std::move(text), path_, problem);
    }
    bool ok() const { return document_.has_value(); }
    const OrderedJson& root() const { return document_->root(); }
    const std::string& label() const { return label_; }
    const fs::path& path() const { return path_; }

    // Replaces the value at `path` (a link or a text); one place of the plan.
    void setValue(const std::string& path, const std::string& value, const std::string& kind, std::vector<Use>& uses) {
        std::string problem;
        if (document_->set(path, value, problem)) uses.push_back({label_, lines_.lineOf(path), path, kind});
    }
    // Renames a key of the object at `parent`; one place of the plan.
    void renameKey(const std::string& memberPath, const std::string& to, std::vector<Use>& uses) {
        const std::optional<DocPath> parsed = parsePath(memberPath);
        if (!parsed || parsed->empty() || parsed->back().index) return;
        DocPath parent = *parsed;
        const std::string from = parent.back().key;
        parent.pop_back();
        std::string problem;
        if (document_->renameMember(formatPath(parent), from, to, problem)) uses.push_back({label_, lines_.lineOf(memberPath), memberPath, "key"});
    }
    bool changed() const { return document_->dirty(); }
    std::string text() const { return document_->text(); }

private:
    std::string label_;
    fs::path path_;
    JsonLines lines_;
    std::optional<DataDocument> document_;
};

// Walks a document with its schema and calls `visit` for every text whose schema says it is a rule, an effect or an objective.
void walkFormats(const schema::Node& node, const nlohmann::json& value, const std::string& path, const std::function<void(const std::string&, const std::string&)>& visit) {
    if (value.is_string() && isRuleFormat(node.format)) visit(path, value.get<std::string>());
    if (value.is_object()) {
        for (const auto& [key, member] : value.items()) {
            const schema::Node* child = node.property(key) != nullptr ? node.property(key) : node.additional.get();
            if (child != nullptr) walkFormats(*child, member, JsonLines::childPath(path, key), visit);
        }
    } else if (value.is_array() && node.items) {
        for (std::size_t i = 0; i < value.size(); ++i) walkFormats(*node.items, value[i], JsonLines::indexPath(path, i), visit);
    }
}

// The lines of a dialogue file with the places a name may stand in: the conditions in [if ...], the effects in {...}, and the @when and @who headers.
// `edit` is called with each such piece and returns what it becomes.
std::string mapDialogue(const std::string& text, const std::function<std::string(const std::string&)>& edit) {
    std::string out;
    std::size_t at = 0;
    while (at < text.size()) {
        const std::size_t end = text.find('\n', at);
        const std::string line = text.substr(at, end == std::string::npos ? std::string::npos : end - at);
        std::string made;
        if (line.starts_with("#")) {
            made = line; // a note
        } else if (line.starts_with("@when") || line.starts_with("@who")) {
            const std::size_t space = line.find(' ');
            made = space == std::string::npos ? line : line.substr(0, space + 1) + edit(line.substr(space + 1));
        } else {
            std::size_t i = 0;
            while (i < line.size()) {
                if (line.compare(i, 4, "[if ") == 0) {
                    const std::size_t close = line.find(']', i);
                    if (close == std::string::npos) break;
                    made += "[if " + edit(line.substr(i + 4, close - i - 4)) + "]";
                    i = close + 1;
                } else if (line[i] == '{') {
                    const std::size_t close = line.find('}', i);
                    const std::string inner = close == std::string::npos ? std::string() : line.substr(i + 1, close - i - 1);
                    if (close != std::string::npos && inner.find_first_of("; ") != std::string::npos) { // an effect list; {hero} and {smalltalk.hunt} are tokens, left alone
                        made += "{" + edit(inner) + "}";
                        i = close + 1;
                    } else {
                        made += line[i++];
                    }
                } else {
                    made += line[i++];
                }
            }
            made += line.substr(std::min(i, line.size()));
        }
        out += made;
        if (end == std::string::npos) break;
        out += '\n';
        at = end + 1;
    }
    return out;
}

bool hasAny(const std::set<std::string>& catalogs, const std::string& name) { return catalogs.count(name) != 0; }

// The edits of one JSON file of the data folder (or a level): links, keys, the entry itself and the rule texts.
void editDataFile(Work& work, const schema::Schema& schemaOfFile, const std::set<std::string>& catalogs, const std::string& from, const std::string& to, bool provider,
                  std::vector<Use>& uses) {
    const nlohmann::json plain = nlohmann::json::parse(work.root().dump(), nullptr, false);
    if (plain.is_discarded()) return;
    // The entry itself: the values the file provides to these catalogs.
    if (provider) {
        for (const schema::Provide& provide : schemaOfFile.provides) {
            if (!hasAny(catalogs, provide.catalog)) continue;
            const std::size_t brackets = provide.at.find("[]");
            if (brackets != std::string::npos && provide.at.find("[]", brackets + 2) != std::string::npos) continue; // a list in a list: tags, not names
            if (provide.at.ends_with("[]")) continue;
            for (const schema::Provided& provided : schema::collectProvided(plain, provide.at)) {
                if (provided.value != from) continue;
                if (provide.at.ends_with(".*")) work.renameKey(provided.path, to, uses);
                else work.setValue(provided.path, to, "entry", uses);
            }
        }
    }
    // Links: a field with `ref`, a key of a map with `keyRef`.
    const schema::Report report = schema::check(*schemaOfFile.root, plain);
    for (const schema::RefUse& ref : report.refs) {
        if (!hasAny(catalogs, ref.catalog) || ref.value != from) continue;
        if (ref.key) work.renameKey(ref.path, to, uses);
        else work.setValue(ref.path, to, "value", uses);
    }
    // Words in rules, effects and objectives.
    walkFormats(*schemaOfFile.root, plain, std::string(), [&](const std::string& path, const std::string& text) {
        const std::string replaced = replaceWord(text, from, to);
        if (replaced != text) work.setValue(path, replaced, "text", uses);
    });
}

void editLevel(Work& work, const std::set<std::string>& catalogs, const std::string& from, const std::string& to, std::vector<Use>& uses) {
    const OrderedJson root = work.root();
    if (!root.is_object()) return;
    for (const LevelField& entry : kLevelFields) {
        if (!hasAny(catalogs, entry.catalog) || !root.contains(entry.list) || !root.at(entry.list).is_array()) continue;
        const OrderedJson& list = root.at(entry.list);
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (!list[i].is_object() || !list[i].contains(entry.field) || !list[i].at(entry.field).is_string()) continue;
            const std::string value = list[i].at(entry.field).get<std::string>();
            const std::string path = std::string(entry.list) + "[" + std::to_string(i) + "]." + entry.field;
            if (value == from) work.setValue(path, to, "value", uses);
            else if (std::string(entry.catalog) == "building-kinds" && hasAny(catalogs, "building-pieces") && value == "piece:" + from) work.setValue(path, "piece:" + to, "value", uses);
        }
    }
    if (hasAny(catalogs, "building-pieces") && root.contains("buildings")) {
        const OrderedJson& list = root.at("buildings");
        for (std::size_t i = 0; i < list.size(); ++i) {
            if (list[i].is_object() && list[i].contains("kind") && list[i].at("kind").is_string() && list[i].at("kind").get<std::string>() == "piece:" + from) {
                work.setValue("buildings[" + std::to_string(i) + "].kind", "piece:" + to, "value", uses);
            }
        }
    }
    if (hasAny(catalogs, "items") && root.contains("economy") && root.at("economy").is_object()) {
        for (const char* table : {"currencies", "prices", "resources"}) {
            if (root.at("economy").contains(table) && root.at("economy").at(table).is_object() && root.at("economy").at(table).contains(from)) {
                work.renameKey(std::string("economy.") + table + "." + from, to, uses);
            }
        }
    }
}

std::vector<fs::path> filesIn(const fs::path& folder, const std::string& extension, bool recursive) {
    std::vector<fs::path> found;
    std::error_code error;
    if (recursive) {
        for (const fs::directory_entry& entry : fs::recursive_directory_iterator(folder, error)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) found.push_back(entry.path());
        }
    } else {
        for (const fs::directory_entry& entry : fs::directory_iterator(folder, error)) {
            if (entry.is_regular_file() && entry.path().extension() == extension) found.push_back(entry.path());
        }
    }
    std::sort(found.begin(), found.end());
    return found;
}

bool isBackup(const fs::path& file) { return file.string().find(".bak") != std::string::npos; }

} // namespace

std::string replaceWord(const std::string& text, const std::string& from, const std::string& to) {
    if (from.empty() || from == to) return text;
    std::string out;
    std::size_t i = 0;
    const bool plain = isPlainWord(from);
    while (i < text.size()) {
        if (text[i] == '"') {
            const std::size_t close = text.find('"', i + 1);
            if (close == std::string::npos) {
                out += text.substr(i);
                break;
            }
            // Quoted text is prose, left alone; but a quoted name that is the whole string ("iron sword") is a name.
            const std::string inner = text.substr(i + 1, close - i - 1);
            out += "\"" + (!plain && inner == from ? to : inner) + "\"";
            i = close + 1;
            continue;
        }
        if (plain && isWordChar(text[i])) {
            std::size_t end = i;
            while (end < text.size() && isWordChar(text[end])) ++end;
            const std::string word = text.substr(i, end - i);
            out += word == from ? to : word;
            i = end;
            continue;
        }
        out += text[i++];
    }
    return out;
}

bool validEntryName(const std::string& name) {
    if (name.empty() || name.size() > 48) return false;
    if (name.front() == ' ' || name.back() == ' ') return false;
    return std::all_of(name.begin(), name.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == ' ' || c == '-' || c == '_'; });
}

namespace {

// The plan itself. With `checkName` the new name is checked first (a delete only wants the places, with a name nobody has).
Plan makePlan(const Roots& roots, const schema::SchemaSet& set, const std::set<std::string>& catalogs, const std::string& from, const std::string& to, bool checkName) {
    Plan plan;
    const schema::DataIndex index = schema::buildIndex(roots.data, set);
    if (checkName) {
        if (!validEntryName(to)) {
            plan.problem = "\"" + to + "\" is not a name: letters, digits, spaces, - and _, up to 48";
            return plan;
        }
        if (from == to) {
            plan.problem = "the new name is the old one";
            return plan;
        }
        for (const std::string& catalog : catalogs) {
            const auto found = index.catalogs.find(catalog);
            if (found != index.catalogs.end() && found->second.count(to) != 0) {
                plan.problem = "the catalog \"" + catalog + "\" has an entry called \"" + to + "\" already";
                return plan;
            }
        }
    }
    // The files of the data folder that have a schema.
    for (const std::string& relative : index.files) {
        const std::string name = relative;
        const schema::IndexEntry* entry = set.entryFor(name);
        if (entry == nullptr || entry->text) continue;
        const schema::Schema* found = set.schema(entry->schema);
        if (found == nullptr) continue;
        const fs::path path = roots.data / relative;
        const std::optional<std::string> text = core::readTextFile(path);
        if (!text) continue;
        std::string problem;
        Work work(relative, path, *text, problem);
        if (!work.ok()) continue; // a file that does not parse is not touched
        std::vector<Use> uses;
        editDataFile(work, *found, catalogs, from, to, true, uses);
        if (!work.changed()) continue;
        plan.texts[path] = work.text();
        plan.uses.insert(plan.uses.end(), uses.begin(), uses.end());
    }
    // The dialogue files.
    for (const fs::path& path : filesIn(roots.data / "dialogue", ".dlg", false)) {
        const std::optional<std::string> text = core::readTextFile(path);
        if (!text) continue;
        const std::string made = mapDialogue(*text, [&](const std::string& piece) { return replaceWord(piece, from, to); });
        if (made == *text) continue;
        plan.texts[path] = made;
        const std::vector<std::string> before = [&] {
            std::vector<std::string> lines;
            std::size_t at = 0;
            while (at <= text->size()) {
                const std::size_t end = text->find('\n', at);
                lines.push_back(text->substr(at, end == std::string::npos ? std::string::npos : end - at));
                if (end == std::string::npos) break;
                at = end + 1;
            }
            return lines;
        }();
        std::size_t lineNumber = 1;
        std::size_t at = 0;
        while (at <= made.size()) {
            const std::size_t end = made.find('\n', at);
            const std::string line = made.substr(at, end == std::string::npos ? std::string::npos : end - at);
            if (lineNumber <= before.size() && before[lineNumber - 1] != line) plan.uses.push_back({"dialogue/" + path.filename().string(), static_cast<int>(lineNumber), std::format("line {}", lineNumber), "text"});
            ++lineNumber;
            if (end == std::string::npos) break;
            at = end + 1;
        }
    }
    // The levels.
    for (const fs::path& path : filesIn(roots.levels, ".json", false)) {
        if (isBackup(path)) continue;
        const std::optional<std::string> text = core::readTextFile(path);
        if (!text) continue;
        std::string problem;
        Work work("levels/" + path.filename().string(), path, *text, problem);
        if (!work.ok()) continue;
        std::vector<Use> uses;
        editLevel(work, catalogs, from, to, uses);
        if (!work.changed()) continue;
        plan.texts[path] = work.text();
        plan.uses.insert(plan.uses.end(), uses.begin(), uses.end());
    }
    return plan;
}

} // namespace

Plan planRename(const Roots& roots, const schema::SchemaSet& set, const std::set<std::string>& catalogs, const std::string& from, const std::string& to) {
    return makePlan(roots, set, catalogs, from, to, true);
}

std::vector<Use> usesOf(const Roots& roots, const schema::SchemaSet& set, const std::set<std::string>& catalogs, const std::string& name) {
    // A rename to a name nothing has finds every place; the entry itself is the one use of kind "entry".
    const Plan plan = makePlan(roots, set, catalogs, name, name + " (renamed to find the uses)", false);
    std::vector<Use> found;
    for (const Use& use : plan.uses) {
        if (use.kind != "entry") found.push_back(use);
    }
    return found;
}

std::optional<std::string> applyPlan(const Plan& plan) {
    if (!plan.problem.empty()) return plan.problem;
    std::vector<std::pair<fs::path, std::string>> before; // what the files held, to put back
    for (const auto& [path, text] : plan.texts) {
        const std::optional<std::string> old = core::readTextFile(path);
        before.emplace_back(path, old.value_or(std::string()));
        if (const std::optional<std::string> problem = core::writeTextFileSafely(path, text, 1)) {
            for (const auto& [done, original] : before) core::writeTextFileSafely(done, original); // all or nothing
            return *problem;
        }
    }
    return std::nullopt;
}

} // namespace odysseus::sim::refs
