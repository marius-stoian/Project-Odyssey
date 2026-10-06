#include "sim/schema_index.h"

#include "core/text.h"

#include <algorithm>
#include <format>

namespace odysseus::sim::schema {

namespace {

namespace fs = std::filesystem;

bool isIdentifierChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; }

bool isMemberName(const std::string& text) {
    if (text.empty() || std::isalpha(static_cast<unsigned char>(text.front())) == 0) return false;
    return std::all_of(text.begin(), text.end(), [](char c) { return isIdentifierChar(c) || c == '-'; });
}

// A backup the writer left behind, or a file the build tools made: not data.
bool isIgnored(const std::string& relative) {
    if (relative.starts_with("schemas/")) return true;
    const std::size_t dot = relative.rfind('.');
    if (dot == std::string::npos) return false;
    const std::string extension = relative.substr(dot);
    return extension.starts_with(".bak") || extension == ".tmp";
}

void addFolderStems(std::map<std::string, std::set<std::string>>& catalogs, const std::string& catalog, const fs::path& folder, const std::string& extension) {
    std::error_code error;
    if (!fs::is_directory(folder, error)) return;
    for (const fs::directory_entry& entry : fs::directory_iterator(folder, error)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) catalogs[catalog].insert(entry.path().stem().string());
    }
}

} // namespace

bool DataIndex::clean() const {
    return brokenLinks.empty() && std::none_of(issues.begin(), issues.end(), [](const FileIssue& issue) { return !issue.issue.warning; });
}

std::vector<std::string> DataIndex::usesOf(const std::string& catalog, const std::string& value) const {
    std::vector<std::string> found;
    for (const CatalogUse& use : uses) {
        if (use.catalog == catalog && use.value == value) found.push_back(std::format("{}:{}: {}", use.file, use.line, use.path));
    }
    return found;
}

DataIndex buildIndex(const fs::path& dataRoot, const SchemaSet& set, const std::map<std::string, std::set<std::string>>& extra) {
    DataIndex index;
    index.catalogs = extra;
    addFolderStems(index.catalogs, "dialogues", dataRoot / "dialogue", ".dlg");
    addFolderStems(index.catalogs, "levels", dataRoot.parent_path() / "levels", ".json");

    std::vector<std::string> files;
    std::error_code error;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(dataRoot, error)) {
        if (!entry.is_regular_file()) continue;
        const std::string relative = entry.path().lexically_relative(dataRoot).generic_string();
        if (!isIgnored(relative)) files.push_back(relative);
    }
    std::sort(files.begin(), files.end()); // directory order is the file system's; the index must not depend on it
    index.files = files;

    for (const std::string& relative : files) {
        const IndexEntry* entry = set.entryFor(relative);
        if (entry == nullptr) {
            index.issues.push_back({relative, {false, "", 0, "has no schema: add it to assets/data/schemas/index.json"}});
            continue;
        }
        if (entry->text) continue; // a text file has its own parser
        const fs::path file = dataRoot / relative;
        const std::optional<std::string> text = core::readTextFile(file);
        if (!text) {
            index.issues.push_back({relative, {false, "", 0, "cannot be opened"}});
            continue;
        }
        nlohmann::json data;
        try {
            data = nlohmann::json::parse(*text, nullptr, true, true);
        } catch (const nlohmann::json::parse_error& problem) {
            index.issues.push_back({relative, {false, "", 0, std::string("is not valid JSON: ") + problem.what()}});
            continue;
        }
        const Schema* schema = set.schema(entry->schema);
        const JsonLines lines = JsonLines::scan(*text);
        const Report report = check(*schema->root, data, &lines);
        for (const Issue& issue : report.issues) index.issues.push_back({relative, issue});
        for (const RefUse& ref : report.refs) index.uses.push_back({relative, ref.path, ref.line, ref.catalog, ref.value, ref.key});
        for (const Provide& provide : schema->provides) {
            for (const Provided& provided : collectProvided(data, provide.at)) index.catalogs[provide.catalog].insert(provided.value);
        }
    }

    for (const CatalogUse& use : index.uses) {
        const auto catalog = index.catalogs.find(use.catalog);
        if (catalog == index.catalogs.end()) {
            index.brokenLinks.push_back({use.file, {false, use.path, use.line, std::format("names \"{}\" in the catalog \"{}\", which no schema provides", use.value, use.catalog)}});
        } else if (catalog->second.find(use.value) == catalog->second.end()) {
            index.brokenLinks.push_back({use.file, {false, use.path, use.line, std::format("\"{}\" is not in the catalog \"{}\"", use.value, use.catalog)}});
        }
    }
    return index;
}

std::string functionBody(const std::string& source, const std::string& name) {
    const std::string needle = name + "(";
    for (std::size_t at = source.find(needle); at != std::string::npos; at = source.find(needle, at + 1)) {
        if (at > 0 && (isIdentifierChar(source[at - 1]) || source[at - 1] == '.' || source[at - 1] == '>')) continue; // part of another name, or a call on an object
        // Skip the argument list, then whatever stands between ")" and "{": const, noexcept, a trailing return type.
        std::size_t i = at + needle.size();
        int depth = 1;
        while (i < source.size() && depth > 0) {
            if (source[i] == '(') ++depth;
            else if (source[i] == ')') --depth;
            ++i;
        }
        while (i < source.size() && source[i] != '{' && source[i] != ';' && source[i] != '}') ++i;
        if (i >= source.size() || source[i] != '{') continue; // a declaration or a call, not a definition
        const std::size_t begin = i;
        int braces = 0;
        bool inString = false;
        for (; i < source.size(); ++i) {
            const char c = source[i];
            if (inString) {
                if (c == '\\') ++i;
                else if (c == '"') inString = false;
            } else if (c == '"') {
                inString = true;
            } else if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') {
                while (i < source.size() && source[i] != '\n') ++i;
            } else if (c == '\'' && i + 2 < source.size()) {
                i += source[i + 1] == '\\' ? 3 : 2;
            } else if (c == '{') {
                ++braces;
            } else if (c == '}' && --braces == 0) {
                return source.substr(begin, i - begin + 1);
            }
        }
    }
    return {};
}

std::set<std::string> keysReadBy(const std::string& source) {
    std::set<std::string> keys;
    // Calls that take a member name first: the nlohmann ones, and the small readers loaders write for themselves (text, flag, whole, number, choice...).
    const std::set<std::string> callNames = {"contains", "at", "value", "find", "count", "get", "text", "flag", "whole", "number", "choice", "maybeNumber", "maybeEffect"};
    const auto startsHelper = [](const std::string& name) {
        for (const char* prefix : {"require", "read", "optional", "expect", "field"}) {
            if (name.starts_with(prefix)) return true;
        }
        return false;
    };
    std::size_t i = 0;
    while (i < source.size()) {
        const char c = source[i];
        if (c == '/' && i + 1 < source.size() && source[i + 1] == '/') { // a comment: its quotes are not code
            while (i < source.size() && source[i] != '\n') ++i;
            continue;
        }
        if (c == '\'' && i + 2 < source.size()) { // a character literal such as '"'
            i += source[i + 1] == '\\' ? 4 : 3;
            continue;
        }
        if (c != '"') {
            ++i;
            continue;
        }
        const std::size_t begin = i + 1;
        std::size_t end = begin;
        while (end < source.size() && source[end] != '"' && source[end] != '\n') end += source[end] == '\\' ? 2 : 1;
        const std::string literal = source.substr(begin, end - begin);
        i = end + 1;
        if (!isMemberName(literal)) continue;
        // What stands before the literal: "[", "(" after a call name, or "," inside a helper's argument list.
        std::size_t p = begin - 1;
        while (p > 0 && std::isspace(static_cast<unsigned char>(source[p - 1])) != 0) --p;
        if (p == 0) continue;
        const char before = source[p - 1];
        const auto nameBefore = [&](std::size_t paren) {
            std::size_t q = paren;
            while (q > 0 && std::isspace(static_cast<unsigned char>(source[q - 1])) != 0) --q;
            std::size_t start = q;
            while (start > 0 && isIdentifierChar(source[start - 1])) --start;
            return source.substr(start, q - start);
        };
        if (before == '[') {
            keys.insert(literal);
        } else if (before == '(') {
            if (callNames.count(nameBefore(p - 1)) != 0) keys.insert(literal);
        } else if (before == ',') {
            // Walk back to the unmatched "(" of this argument list (a short look: loaders write their helper calls on one line).
            int depth = 0;
            std::size_t q = p - 1;
            std::size_t limit = 0;
            while (q > 0 && limit++ < 240) {
                const char back = source[q - 1];
                if (back == ')' || back == ']') ++depth;
                else if (back == '(' || back == '[') {
                    if (depth == 0) break;
                    --depth;
                } else if (back == ';' || back == '{' || back == '}') {
                    q = 0;
                    break;
                }
                --q;
            }
            if (q > 0 && source[q - 1] == '(' && startsHelper(nameBefore(q - 1))) keys.insert(literal);
        }
    }
    return keys;
}

namespace {

void collectNames(const Node& node, std::set<std::string>& names) {
    for (const Property& property : node.properties) {
        names.insert(property.name);
        collectNames(*property.node, names);
    }
    if (node.items) collectNames(*node.items, names);
    if (node.additional) collectNames(*node.additional, names);
}

} // namespace

std::set<std::string> fieldNames(const Node& root) {
    std::set<std::string> names;
    collectNames(root, names);
    return names;
}

std::set<std::string> quotedWords(const std::string& source) {
    std::set<std::string> words;
    std::size_t i = 0;
    while (i < source.size()) {
        if (source[i] == '/' && i + 1 < source.size() && source[i + 1] == '/') {
            while (i < source.size() && source[i] != '\n') ++i;
        } else if (source[i] == '\'' && i + 2 < source.size()) {
            i += source[i + 1] == '\\' ? 4 : 3;
        } else if (source[i] == '"') {
            std::size_t end = i + 1;
            while (end < source.size() && source[end] != '"' && source[end] != '\n') end += source[end] == '\\' ? 2 : 1;
            const std::string literal = source.substr(i + 1, end - i - 1);
            if (isMemberName(literal)) words.insert(literal);
            i = end + 1;
        } else {
            ++i;
        }
    }
    return words;
}

namespace {

// What one loader source (or the functions of it a schema names) reads.
struct LoaderText {
    std::set<std::string> keys;  // member names read the way loaders read JSON (keysReadBy)
    std::set<std::string> words; // every name-like string literal: a field the loader writes out in a table or a loop still counts as read
};

LoaderText readLoader(const std::string& loader, const fs::path& repoRoot, const std::string& schemaName, std::vector<DriftProblem>& problems) {
    // "src/game/level.cpp#loadDefinitions,readTags": only those functions are read (a file may also hold other formats' loaders).
    const std::size_t hash = loader.find('#');
    const std::optional<std::string> text = core::readTextFile(repoRoot / loader.substr(0, hash));
    LoaderText result;
    if (!text) {
        problems.push_back({schemaName, std::format("lists the loader {} but it cannot be opened", loader)});
        return result;
    }
    std::string body;
    if (hash == std::string::npos) {
        body = *text;
    } else {
        std::size_t from = hash + 1;
        while (from <= loader.size()) {
            const std::size_t comma = loader.find(',', from);
            const std::string name = loader.substr(from, comma == std::string::npos ? std::string::npos : comma - from);
            const std::string part = functionBody(*text, name);
            if (part.empty()) problems.push_back({schemaName, std::format("lists the function {} of {} but no definition was found", name, loader.substr(0, hash))});
            body += part;
            if (comma == std::string::npos) break;
            from = comma + 1;
        }
    }
    result.keys = keysReadBy(body);
    result.words = quotedWords(body);
    return result;
}

} // namespace

std::vector<DriftProblem> checkDrift(const SchemaSet& set, const fs::path& repoRoot) {
    std::vector<DriftProblem> problems;
    std::map<std::string, LoaderText> loaders;                     // by the loader string of the schemas
    std::map<std::string, std::set<std::string>> fieldsForLoader;  // the names of every schema that lists the loader, plus its external fields
    for (const Schema& schema : set.schemas()) {
        const std::set<std::string> fields = fieldNames(*schema.root);
        for (const std::string& loader : schema.loaders) {
            if (loaders.find(loader) == loaders.end()) loaders[loader] = readLoader(loader, repoRoot, schema.name, problems);
            fieldsForLoader[loader].insert(fields.begin(), fields.end());
            fieldsForLoader[loader].insert(schema.externalFields.begin(), schema.externalFields.end());
        }
    }
    // A loader reads a member its schemas do not describe: the schema is behind the code.
    for (const auto& [loader, text] : loaders) {
        for (const std::string& key : text.keys) {
            if (fieldsForLoader[loader].count(key) == 0) problems.push_back({"", std::format("{} reads \"{}\" and no schema that lists it describes that field", loader, key)});
        }
    }
    // A schema describes a field no loader mentions at all: the schema is stale.
    for (const Schema& schema : set.schemas()) {
        if (schema.loaders.empty()) {
            problems.push_back({schema.name, "lists no loader (add \"loaders\": [\"src/...\"] so the drift test can compare its fields)"});
            continue;
        }
        const std::set<std::string> external(schema.externalFields.begin(), schema.externalFields.end());
        for (const std::string& name : fieldNames(*schema.root)) {
            bool mentioned = external.count(name) != 0;
            for (const std::string& loader : schema.loaders) mentioned = mentioned || loaders[loader].words.count(name) != 0;
            if (!mentioned) problems.push_back({schema.name, std::format("describes \"{}\" and no loader of this schema mentions it", name)});
        }
    }
    return problems;
}

} // namespace odysseus::sim::schema
