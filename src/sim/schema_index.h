#pragma once

#include "boundary.h"

#include "sim/schema.h"

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace odysseus::sim::schema {

// A mistake in one file of the data folder, with the file named.
struct FileIssue {
    std::string file;  // below assets/data, forward slashes: "sim/needs.json"
    Issue issue;
    std::string text() const { return issue.text(std::filesystem::path("assets/data") / file); }
};

// One value that names an entry of a catalog, and where it stands.
struct CatalogUse {
    std::string file;
    std::string path;
    int line = 0;
    std::string catalog;
    std::string value;
    bool key = false; // the value is the key of the member at `path`
};

// Everything the schemas can say about the whole data folder (US-190, US-193): the catalogs the files provide, every use of an entry (the links the
// forms turn into pickers and the rename walks), and every mistake found on the way.
struct DataIndex {
    std::map<std::string, std::set<std::string>> catalogs; // catalog name -> its entries
    std::vector<CatalogUse> uses;
    std::vector<FileIssue> issues;     // schema mistakes (errors and warnings), files without a schema, files that do not parse
    std::vector<FileIssue> brokenLinks; // a use whose entry no catalog has, listed with file and line
    std::vector<std::string> files;     // every data file seen, forward slashes

    bool clean() const;
    std::vector<std::string> usesOf(const std::string& catalog, const std::string& value) const; // "file:line: path", for the "used here" lists
};

// Reads the whole data folder with the set's schemas. `extra` adds catalogs the data files cannot provide (the atlas frames, for example); the
// dialogue files and the level files are found by the index itself.
DataIndex buildIndex(const std::filesystem::path& dataRoot, const SchemaSet& set, const std::map<std::string, std::set<std::string>>& extra = {});

// The drift test (ADR-020): the member names each loader reads, against the members its schema describes. A loader that reads a name the schema lacks,
// or a schema field that no loader reads, is listed. `repoRoot` is the folder the "loaders" paths of the schemas are relative to.
struct DriftProblem {
    std::string schema;
    std::string message;
};
std::vector<DriftProblem> checkDrift(const SchemaSet& set, const std::filesystem::path& repoRoot);

// The text of the body of a function (or a method): from the `{` after `name(...)` to its matching `}`. Empty when there is no definition.
std::string functionBody(const std::string& source, const std::string& name);

// The member names written in a piece of source code the way loaders read JSON: .contains("x"), .at("x"), .value("x", ...), ["x"], .find("x") and the
// text arguments of the require... and read... helpers.
std::set<std::string> keysReadBy(const std::string& sourceText);

// Every name-like string literal of a piece of source code (a loader that writes its fields in a table or a loop still mentions each one).
std::set<std::string> quotedWords(const std::string& sourceText);

// Every member name a schema describes, at any depth.
std::set<std::string> fieldNames(const Node& root);

} // namespace odysseus::sim::schema
