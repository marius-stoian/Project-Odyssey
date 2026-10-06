#include "sim/schema.h"

#include "sim/schema_install.h"

#include "core/log.h"
#include "core/text.h"
#include "sim/data.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <map>

namespace odysseus::sim::schema {

using OrderedJson = nlohmann::ordered_json;

// ---- The nodes

const Node* Node::property(const std::string& name) const {
    for (const Property& property : properties) {
        if (property.name == name) return property.node.get();
    }
    return nullptr;
}

bool Node::isRequired(const std::string& name) const { return std::find(required.begin(), required.end(), name) != required.end(); }

std::string Issue::text(const std::filesystem::path& file) const {
    std::string where = file.generic_string();
    if (line > 0) where += ":" + std::to_string(line);
    return std::format("{}: {}: {}", where, path.empty() ? "(file)" : path, message);
}

bool Report::hasErrors() const {
    return std::any_of(issues.begin(), issues.end(), [](const Issue& issue) { return !issue.warning; });
}

std::vector<const Issue*> Report::errors() const {
    std::vector<const Issue*> found;
    for (const Issue& issue : issues) {
        if (!issue.warning) found.push_back(&issue);
    }
    return found;
}

namespace {

// Members every data file may carry whatever its schema says: notes for the human reader.
bool isNoteKey(const std::string& key) { return key == "note" || key == "comment" || key.starts_with("_") || key.starts_with("//"); }

// ---- Reading a schema

Type typeFromName(const std::string& name, bool& known) {
    known = true;
    if (name == "object") return Type::Object;
    if (name == "array") return Type::Array;
    if (name == "string") return Type::String;
    if (name == "number") return Type::Number;
    if (name == "integer") return Type::Integer;
    if (name == "boolean") return Type::Boolean;
    known = false;
    return Type::Any;
}

[[noreturn]] void bad(const std::filesystem::path& file, const std::string& path, const std::string& problem) { throw DataError(file, path.empty() ? "(root)" : path, problem); }

std::string textOf(const OrderedJson& value, const std::filesystem::path& file, const std::string& path) {
    if (!value.is_string()) bad(file, path, "must be text");
    return value.get<std::string>();
}

int wholeOf(const OrderedJson& value, const std::filesystem::path& file, const std::string& path) {
    if (!value.is_number_integer()) bad(file, path, "must be a whole number");
    return value.get<int>();
}

NodePtr parseNode(const OrderedJson& json, const std::filesystem::path& file, const std::string& path, bool root) {
    if (!json.is_object()) bad(file, path, "must be an object");
    auto node = std::make_shared<Node>();
    for (const auto& [key, value] : json.items()) {
        const std::string at = path.empty() ? key : path + "." + key;
        if (key == "type") {
            bool known = false;
            node->type = typeFromName(textOf(value, file, at), known);
            if (!known) bad(file, at, "must be object, array, string, number, integer or boolean");
        } else if (key == "properties") {
            if (!value.is_object()) bad(file, at, "must be an object");
            for (const auto& [name, child] : value.items()) node->properties.push_back({name, parseNode(child, file, at + "." + name, false)});
        } else if (key == "additionalProperties") {
            if (value.is_boolean()) node->anyMember = value.get<bool>();
            else node->additional = parseNode(value, file, at, false);
        } else if (key == "items") {
            node->items = parseNode(value, file, at, false);
        } else if (key == "required") {
            if (!value.is_array()) bad(file, at, "must be a list of field names");
            for (const OrderedJson& name : value) node->required.push_back(textOf(name, file, at));
        } else if (key == "minimum" || key == "maximum") {
            if (!value.is_number()) bad(file, at, "must be a number");
            (key == "minimum" ? node->minimum : node->maximum) = value.get<double>();
        } else if (key == "minItems") {
            node->minItems = wholeOf(value, file, at);
        } else if (key == "maxItems") {
            node->maxItems = wholeOf(value, file, at);
        } else if (key == "maxLength") {
            node->maxLength = wholeOf(value, file, at);
        } else if (key == "enum") {
            if (!value.is_array() || value.empty()) bad(file, at, "must be a list of the allowed values");
            for (const OrderedJson& choice : value) node->choices.push_back(nlohmann::json::parse(choice.dump()));
        } else if (key == "default") {
            node->defaultText = value.dump();
        } else if (key == "description") {
            node->description = textOf(value, file, at);
        } else if (key == "example") {
            node->example = textOf(value, file, at);
        } else if (key == "ref") {
            node->ref = textOf(value, file, at);
            if (!node->ref.starts_with("catalog:") || node->ref.size() == 8) bad(file, at, "must read \"catalog:<name>\"");
        } else if (key == "keyRef") {
            node->keyRef = textOf(value, file, at);
            if (!node->keyRef.starts_with("catalog:") || node->keyRef.size() == 8) bad(file, at, "must read \"catalog:<name>\"");
        } else if (key == "format") {
            node->format = textOf(value, file, at);
        } else if (isNoteKey(key) || key == "title") {
            // a note, or the title of the root (read by the caller)
        } else if (root && (key == "provides" || key == "loaders" || key == "externalFields")) {
            // root keys, read by the caller
        } else {
            bad(file, at, "is not a key a schema node knows");
        }
    }
    return node;
}

Schema parseSchema(const std::string& name, const std::filesystem::path& file) {
    const std::optional<std::string> text = core::readTextFile(file);
    if (!text) bad(file, "(file)", "cannot be opened");
    OrderedJson json;
    try {
        json = OrderedJson::parse(*text, nullptr, true, true);
    } catch (const OrderedJson::parse_error& error) {
        bad(file, "(syntax)", error.what());
    }
    Schema schema;
    schema.name = name;
    schema.file = file;
    schema.root = parseNode(json, file, "", true);
    if (json.contains("title")) schema.title = textOf(json.at("title"), file, "title");
    if (json.contains("provides")) {
        if (!json.at("provides").is_array()) bad(file, "provides", "must be a list of { catalog, at }");
        for (const OrderedJson& entry : json.at("provides")) {
            if (!entry.is_object() || !entry.contains("catalog") || !entry.contains("at")) bad(file, "provides", "each entry needs \"catalog\" and \"at\"");
            schema.provides.push_back({textOf(entry.at("catalog"), file, "provides.catalog"), textOf(entry.at("at"), file, "provides.at")});
        }
    }
    for (const char* key : {"loaders", "externalFields"}) {
        if (!json.contains(key)) continue;
        if (!json.at(key).is_array()) bad(file, key, "must be a list of text");
        std::vector<std::string>& into = std::string(key) == "loaders" ? schema.loaders : schema.externalFields;
        for (const OrderedJson& entry : json.at(key)) into.push_back(textOf(entry, file, key));
    }
    return schema;
}

// ---- Checking a document

std::string numberText(double number) {
    if (std::floor(number) == number && std::abs(number) < 1e15) return std::format("{}", static_cast<long long>(number));
    return std::format("{}", number);
}

const char* describe(const nlohmann::json& value) {
    if (value.is_object()) return "an object";
    if (value.is_array()) return "a list";
    if (value.is_string()) return "text";
    if (value.is_boolean()) return "true or false";
    if (value.is_null()) return "null";
    return "a number";
}

const char* typeWords(Type type) {
    switch (type) {
    case Type::Object: return "an object";
    case Type::Array: return "a list";
    case Type::String: return "text";
    case Type::Number: return "a number";
    case Type::Integer: return "a whole number";
    case Type::Boolean: return "true or false";
    case Type::Any: break;
    }
    return "a value";
}

bool isHexColour(const std::string& text) {
    if (text.size() != 7 || text[0] != '#') return false;
    return std::all_of(text.begin() + 1, text.end(), [](unsigned char c) { return std::isxdigit(c) != 0; });
}

class Checker {
public:
    Checker(Report& report, const JsonLines* lines) : report_(report), lines_(lines) {}

    void node(const Node& schema, const nlohmann::json& value, const std::string& path) {
        if (!typeMatches(schema.type, value)) {
            error(path, std::format("must be {} (is {})", typeWords(schema.type), describe(value)));
            return;
        }
        if (value.is_number()) numberRange(schema, value, path);
        if (!schema.choices.empty()) choice(schema, value, path);
        if (value.is_string()) text(schema, value.get<std::string>(), path);
        if (value.is_object()) object(schema, value, path);
        if (value.is_array()) array(schema, value, path);
    }

private:
    static bool typeMatches(Type type, const nlohmann::json& value) {
        switch (type) {
        case Type::Any: return true;
        case Type::Object: return value.is_object();
        case Type::Array: return value.is_array();
        case Type::String: return value.is_string();
        case Type::Number: return value.is_number();
        case Type::Integer: return value.is_number_integer();
        case Type::Boolean: return value.is_boolean();
        }
        return true;
    }

    void issue(bool warning, const std::string& path, std::string message) {
        report_.issues.push_back({warning, path, lines_ != nullptr ? lines_->lineOf(path) : 0, std::move(message)});
    }
    void error(const std::string& path, std::string message) { issue(false, path, std::move(message)); }

    void numberRange(const Node& schema, const nlohmann::json& value, const std::string& path) {
        const double number = value.get<double>();
        const bool low = schema.minimum && number < *schema.minimum;
        const bool high = schema.maximum && number > *schema.maximum;
        if (!low && !high) return;
        if (schema.minimum && schema.maximum) error(path, std::format("must be between {} and {} (is {})", numberText(*schema.minimum), numberText(*schema.maximum), value.dump()));
        else if (schema.minimum) error(path, std::format("must be at least {} (is {})", numberText(*schema.minimum), value.dump()));
        else error(path, std::format("must be at most {} (is {})", numberText(*schema.maximum), value.dump()));
    }

    void choice(const Node& schema, const nlohmann::json& value, const std::string& path) {
        if (std::find(schema.choices.begin(), schema.choices.end(), value) != schema.choices.end()) return;
        std::string allowed;
        for (const nlohmann::json& option : schema.choices) allowed += (allowed.empty() ? "" : ", ") + (option.is_string() ? option.get<std::string>() : option.dump());
        error(path, std::format("must be one of: {} (is {})", allowed, value.is_string() ? value.get<std::string>() : value.dump()));
    }

    void text(const Node& schema, const std::string& value, const std::string& path) {
        if (schema.maxLength && static_cast<int>(value.size()) > *schema.maxLength) error(path, std::format("is too long: at most {} letters (has {})", *schema.maxLength, value.size()));
        if (schema.format == "colour" && !value.empty() && !isHexColour(value)) error(path, std::format("must be a colour such as #3a9a4c (is {})", value));
        if (schema.ref.starts_with("catalog:") && !value.empty()) report_.refs.push_back({schema.ref.substr(8), value, path, lines_ != nullptr ? lines_->lineOf(path) : 0});
    }

    void object(const Node& schema, const nlohmann::json& value, const std::string& path) {
        for (const std::string& name : schema.required) {
            if (!value.contains(name)) error(JsonLines::childPath(path, name), "is missing");
        }
        for (const auto& [key, member] : value.items()) {
            const std::string at = JsonLines::childPath(path, key);
            if (const Node* known = schema.property(key)) {
                node(*known, member, at);
            } else if (schema.additional) {
                if (schema.keyRef.starts_with("catalog:")) report_.refs.push_back({schema.keyRef.substr(8), key, at, lines_ != nullptr ? lines_->lineOf(at) : 0, true});
                node(*schema.additional, member, at);
            } else if (!schema.anyMember && !isNoteKey(key) && !schema.properties.empty()) { // an object with no named fields is free-form
                issue(true, at, "is not a field this file knows (a typo? a note field starts with note, comment or _)");
            }
        }
    }

    void array(const Node& schema, const nlohmann::json& value, const std::string& path) {
        const int count = static_cast<int>(value.size());
        if (schema.minItems && count < *schema.minItems) error(path, std::format("needs at least {} entries (has {})", *schema.minItems, count));
        if (schema.maxItems && count > *schema.maxItems) error(path, std::format("allows at most {} entries (has {})", *schema.maxItems, count));
        if (!schema.items) return;
        for (std::size_t i = 0; i < value.size(); ++i) node(*schema.items, value[i], JsonLines::indexPath(path, i));
    }

    Report& report_;
    const JsonLines* lines_;
};

// ---- The installed set

std::shared_ptr<const SchemaSet> gSet;
std::filesystem::path gRoot;

void walkProvided(const nlohmann::json& node, const std::vector<std::string>& segments, std::size_t i, const std::string& path, std::vector<Provided>& out) {
    if (i == segments.size()) {
        if (node.is_string()) {
            out.push_back({node.get<std::string>(), path});
        } else if (node.is_array()) {
            for (std::size_t k = 0; k < node.size(); ++k) {
                if (node[k].is_string()) out.push_back({node[k].get<std::string>(), JsonLines::indexPath(path, k)});
            }
        }
        return;
    }
    const std::string& segment = segments[i];
    if (segment == "*") {
        if (!node.is_object()) return;
        for (const auto& [key, member] : node.items()) {
            const std::string at = JsonLines::childPath(path, key);
            if (i + 1 == segments.size()) out.push_back({key, at});
            else walkProvided(member, segments, i + 1, at, out);
        }
        return;
    }
    const bool each = segment.ends_with("[]");
    const std::string name = each ? segment.substr(0, segment.size() - 2) : segment;
    const nlohmann::json* next = &node;
    std::string at = path;
    if (!name.empty()) {
        if (!node.is_object() || !node.contains(name)) return;
        next = &node.at(name);
        at = JsonLines::childPath(path, name);
    }
    if (!each) {
        walkProvided(*next, segments, i + 1, at, out);
        return;
    }
    if (!next->is_array()) return;
    for (std::size_t k = 0; k < next->size(); ++k) walkProvided((*next)[k], segments, i + 1, JsonLines::indexPath(at, k), out);
}

} // namespace

Report check(const Node& root, const nlohmann::json& data, const JsonLines* lines) {
    Report report;
    Checker(report, lines).node(root, data, "");
    return report;
}

std::vector<Provided> collectProvided(const nlohmann::json& data, std::string_view at) {
    std::vector<std::string> segments;
    std::string current;
    for (const char c : at) {
        if (c == '.') {
            segments.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    segments.push_back(current);
    std::vector<Provided> out;
    walkProvided(data, segments, 0, "", out);
    return out;
}

// ---- The set

const Schema* SchemaSet::schema(const std::string& name) const {
    for (const Schema& entry : schemas_) {
        if (entry.name == name) return &entry;
    }
    return nullptr;
}

const IndexEntry* SchemaSet::entryFor(const std::string& relativePath) const {
    for (const IndexEntry& entry : index_) {
        if (globMatches(entry.match, relativePath)) return &entry;
    }
    return nullptr;
}

const Schema* SchemaSet::schemaFor(const std::string& relativePath) const {
    const IndexEntry* entry = entryFor(relativePath);
    return entry != nullptr && !entry->text ? schema(entry->schema) : nullptr;
}

SchemaSet SchemaSet::load(const std::filesystem::path& folder) {
    SchemaSet set;
    set.folder_ = folder;
    const std::filesystem::path indexFile = folder / "index.json";
    const std::optional<std::string> text = core::readTextFile(indexFile);
    if (!text) bad(indexFile, "(file)", "cannot be opened");
    OrderedJson index;
    try {
        index = OrderedJson::parse(*text, nullptr, true, true);
    } catch (const OrderedJson::parse_error& error) {
        bad(indexFile, "(syntax)", error.what());
    }
    if (!index.is_object() || !index.contains("files") || !index.at("files").is_array()) bad(indexFile, "files", "must be a list of { match, schema }");
    for (std::size_t i = 0; i < index.at("files").size(); ++i) {
        const OrderedJson& entry = index.at("files")[i];
        const std::string at = std::format("files[{}]", i);
        if (!entry.is_object() || !entry.contains("match")) bad(indexFile, at, "needs \"match\"");
        IndexEntry made;
        made.match = textOf(entry.at("match"), indexFile, at + ".match");
        if (entry.contains("text") && entry.at("text").is_boolean()) made.text = entry.at("text").get<bool>();
        if (!made.text) {
            if (!entry.contains("schema")) bad(indexFile, at, "needs \"schema\" (or \"text\": true for a text file)");
            made.schema = textOf(entry.at("schema"), indexFile, at + ".schema");
            if (set.schema(made.schema) == nullptr) {
                const std::filesystem::path file = folder / (made.schema + ".schema.json");
                set.schemas_.push_back(parseSchema(made.schema, file));
            }
        }
        set.index_.push_back(std::move(made));
    }
    return set;
}

bool globMatches(std::string_view pattern, std::string_view path) {
    if (pattern.empty()) return path.empty();
    if (pattern.front() == '*') {
        // A star takes any run of letters except a slash, then the rest of the pattern must match what follows.
        for (std::size_t take = 0; take <= path.size(); ++take) {
            if (globMatches(pattern.substr(1), path.substr(take))) return true;
            if (take < path.size() && path[take] == '/') break;
        }
        return false;
    }
    return !path.empty() && pattern.front() == path.front() && globMatches(pattern.substr(1), path.substr(1));
}

void install(std::shared_ptr<const SchemaSet> set, std::filesystem::path dataRoot) {
    gSet = std::move(set);
    gRoot = std::move(dataRoot);
}

void uninstall() {
    gSet.reset();
    gRoot.clear();
}

const SchemaSet* installed() { return gSet.get(); }

std::optional<std::string> installFromFolder(const std::filesystem::path& dataRoot) {
    try {
        install(std::make_shared<SchemaSet>(SchemaSet::load(dataRoot / "schemas")), dataRoot);
        return std::nullopt;
    } catch (const std::exception& problem) {
        uninstall();
        return std::string(problem.what());
    }
}

std::string relativeDataPath(const std::filesystem::path& file) {
    if (!gSet) return {};
    const std::filesystem::path relative = std::filesystem::absolute(file).lexically_normal().lexically_relative(std::filesystem::absolute(gRoot).lexically_normal());
    const std::string text = relative.generic_string();
    if (text.empty() || text.starts_with("..")) return {};
    return text;
}

Report checkFile(const std::filesystem::path& file, const nlohmann::json& data, std::string_view text) {
    const std::string relative = relativeDataPath(file);
    if (relative.empty()) return {};
    const Schema* schema = gSet->schemaFor(relative);
    if (schema == nullptr) return {};
    if (text.empty()) return check(*schema->root, data, nullptr);
    const JsonLines lines = JsonLines::scan(text);
    return check(*schema->root, data, &lines);
}

std::vector<Issue> issuesForText(const std::string& relativeName, std::string_view text) {
    if (!gSet) return {};
    const Schema* schema = gSet->schemaFor(relativeName);
    if (schema == nullptr) return {};
    nlohmann::json data = nlohmann::json::parse(text.begin(), text.end(), nullptr, false, true);
    if (data.is_discarded()) return {};
    const JsonLines lines = JsonLines::scan(text);
    return check(*schema->root, data, &lines).issues;
}

void checkLoaded(const std::filesystem::path& file, const nlohmann::json& data, std::string_view text) {
    const Report report = checkFile(file, data, text);
    static std::set<std::string> warned;
    for (const Issue& issue : report.issues) {
        if (issue.warning) {
            if (warned.insert(file.generic_string()).second) core::logWarning("Data: " + issue.text(file));
            continue;
        }
        // The first mistake stops the load of this file: file:line, then the field and what is allowed.
        std::string where = file.generic_string();
        if (issue.line > 0) where += ":" + std::to_string(issue.line);
        throw DataError(where, issue.path.empty() ? "(file)" : issue.path, issue.message);
    }
}

} // namespace odysseus::sim::schema
