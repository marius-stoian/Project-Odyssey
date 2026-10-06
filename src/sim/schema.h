#pragma once

#include "boundary.h"

#include "sim/json_text.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::sim::schema {

// Data files described by data (US-190, ADR-020). A schema is a small subset of JSON Schema: it says what each field is (type, range, choices, a link
// to another catalog, help text) so that three readers can share it: the loaders (a value out of range is refused with file, line and field), the CI
// test (every shipped file passes, every link points somewhere) and the Editor's Data tab (the form is built from it).
//
// Keys a schema node may use: type (object, array, string, number, integer, boolean), properties, additionalProperties (true, or a schema for every
// member that is not named: a map keyed by id), items, required, minimum, maximum, minItems, maxItems, maxLength, enum, default, description,
// example, ref ("catalog:items": the value is a name in that catalog), format (rule, effect, colour, frame, time). At the root also title, provides,
// loaders and externalFields. A key starting with "_" or "//", and the keys "note" and "comment" of a data file, are always allowed (note fields).

enum class Type { Any, Object, Array, String, Number, Integer, Boolean };

struct Node;
using NodePtr = std::shared_ptr<const Node>;

struct Property {
    std::string name;
    NodePtr node;
};

struct Node {
    Type type = Type::Any;
    std::string description; // the help text of the field: what it is for
    std::string example;     // one value written the way it would be typed
    std::string ref;         // "catalog:<name>": the value must be a name of that catalog
    std::string format;      // rule, effect, colour, frame or time: how the text is read (the forms pick a widget by it)
    std::string defaultText; // the default as JSON text, empty when there is none
    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<int> minItems;
    std::optional<int> maxItems;
    std::optional<int> maxLength;
    std::vector<nlohmann::json> choices; // "enum"
    std::vector<Property> properties;    // in the order the schema lists them, which is the order the form shows them
    std::vector<std::string> required;
    NodePtr additional;                  // the schema of every member that is not in `properties` (a map keyed by id)
    bool anyMember = false;              // "additionalProperties": true: other members are accepted and not looked at
    NodePtr items;                       // the schema of every element of an array

    const Node* property(const std::string& name) const;
    bool isRequired(const std::string& name) const;
};

// The values of a file that make up a catalog: "at" walks the document ("items[].id": the id of every element of items; "materials.*": the member names).
struct Provide {
    std::string catalog;
    std::string at;
};

struct Schema {
    std::string name;
    std::filesystem::path file;
    std::string title;
    NodePtr root;
    std::vector<Provide> provides;
    std::vector<std::string> loaders;        // the source files that read these data files, for the drift test (member names the loader reads)
    std::vector<std::string> externalFields; // members a loader reads in a way the drift test cannot see (kept short)
};

// One thing wrong, or odd, in a file.
struct Issue {
    bool warning = false; // a warning does not stop a load; CI and the Editor show both
    std::string path;     // "weapons[2].damage"
    int line = 0;         // 0 when unknown
    std::string message;
    // "assets/data/weapons.json:23: weapons[2].damage: must be between 1 and 999 (is 0)"
    std::string text(const std::filesystem::path& file) const;
};

// A value that names an entry of a catalog (a field with "ref").
struct RefUse {
    std::string catalog;
    std::string value;
    std::string path;
    int line = 0;
};

struct Report {
    std::vector<Issue> issues;
    std::vector<RefUse> refs;
    bool hasErrors() const;
    std::vector<const Issue*> errors() const;
};

// Checks `data` against `root`. `lines` (made from the same text) gives the issues their line numbers.
Report check(const Node& root, const nlohmann::json& data, const JsonLines* lines = nullptr);

// The values a "provides" path names in a document, with the path of each ("items[2].id").
struct Provided {
    std::string value;
    std::string path;
};
std::vector<Provided> collectProvided(const nlohmann::json& data, std::string_view at);

// Which schema covers which file: schemas/index.json.
struct IndexEntry {
    std::string match;  // a path below assets/data/ where * stands for any run of letters but a slash: "sim/needs.json", "interactions/*.json"
    std::string schema; // the schema's name; empty for a text file
    bool text = false;  // a text file (.dlg) read by its own parser: it needs no schema
};

class SchemaSet {
public:
    // Reads schemas/index.json and every schema it names; a mistake in any of them is a DataError naming the file and the key.
    static SchemaSet load(const std::filesystem::path& folder);

    const std::vector<Schema>& schemas() const { return schemas_; }
    const std::vector<IndexEntry>& index() const { return index_; }
    const std::filesystem::path& folder() const { return folder_; }
    const Schema* schema(const std::string& name) const;
    const IndexEntry* entryFor(const std::string& relativePath) const;
    const Schema* schemaFor(const std::string& relativePath) const;

private:
    std::filesystem::path folder_;
    std::vector<Schema> schemas_;
    std::vector<IndexEntry> index_;
};

// "interactions/*.json" matches "interactions/gather.json" (a * never crosses a slash).
bool globMatches(std::string_view pattern, std::string_view path);

// The set every loader checks against. The game and the headless program install it once at start; with none installed (a unit test reading a
// temporary file) nothing is checked. `dataRoot` is assets/data/: only files below it are looked up.
void install(std::shared_ptr<const SchemaSet> set, std::filesystem::path dataRoot);
void uninstall();
const SchemaSet* installed();

// The path of `file` below the installed data folder with forward slashes ("sim/needs.json"); empty when there is none or the file lies elsewhere.
std::string relativeDataPath(const std::filesystem::path& file);

// Checks one loaded file. Nothing installed, or no schema for the file: an empty report. `text` (the file's own text) gives the issues their lines.
Report checkFile(const std::filesystem::path& file, const nlohmann::json& data, std::string_view text = {});

// The same for a loader that has only the file's display name below assets/data ("interactions/gather.json") and its text: the issues of the file, none when
// nothing is installed, the file has no schema or the text is not JSON (the loader's own parser says so).
std::vector<Issue> issuesForText(const std::string& relativeName, std::string_view text);

// What a loader calls after parsing a file: the first mistake of the report (a type, range, choice or missing field) is thrown as a DataError that
// names file, line, field and the allowed range; warnings (an unknown field) go to the log once per file.
void checkLoaded(const std::filesystem::path& file, const nlohmann::json& data, std::string_view text = {});

} // namespace odysseus::sim::schema
