#pragma once

#include "boundary.h"

#include "sim/schema.h"

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace odysseus::sim::refs {

// Keeping names consistent (US-193, EDT-02): an entry of a catalog is named in many places, and a rename must reach all of them. The places are found from the
// schemas (a field with `ref`, a map with `keyRef`, a text with a rule-language `format`), from the levels (which have a fixed shape), and from the dialogue files
// (the conditions and effects in them). Nothing here writes a file until `applyPlan` is called, so the owner sees the list first.

struct Roots {
    std::filesystem::path data;   // assets/data
    std::filesystem::path levels; // assets/levels
};

// One place a name is used.
struct Use {
    std::string file;  // below the data folder ("hero/recipes.json", "dialogue/elder-fire.dlg") or "levels/<name>.json"
    int line = 0;
    std::string where; // "recipes[2].output", "line 7"
    std::string kind;  // "entry" (the entry itself), "value", "key" or "text" (a name inside a rule or an effect)
};

// What a rename would do, made without writing anything.
struct Plan {
    std::vector<Use> uses;                                // every place that changes, in file order; the entry itself comes first
    std::map<std::filesystem::path, std::string> texts;   // the new text of each file that changes
    std::string problem;                                   // why the rename cannot be done (the new name is taken, ...); empty when it can
};

// Every place the entry `name` of the given catalogs is used, the entry itself left out: what a delete must tell the owner about.
std::vector<Use> usesOf(const Roots& roots, const schema::SchemaSet& set, const std::set<std::string>& catalogs, const std::string& name);

// The entry `from` of `catalogs` becomes `to`: in the file that holds the entry, in every field that links to it, in the keys of the maps that name it, in the
// conditions and effects of rule files and dialogues that write it as a word, and in the levels. `catalogs` are all the catalogs the entry is provided to
// (a plant is in "plants" and in "kinds"). Refused, with the reason in `problem`, when `to` is not a name or a catalog already has it.
Plan planRename(const Roots& roots, const schema::SchemaSet& set, const std::set<std::string>& catalogs, const std::string& from, const std::string& to);

// Writes every file of the plan, each through a temporary file with one backup. If one file cannot be written the ones already written are put back.
// Returns what went wrong, nothing when it worked.
std::optional<std::string> applyPlan(const Plan& plan);

// `from` replaced by `to` wherever it stands as a whole word (letters, digits, - and _ around it make it part of a longer word), outside of "quoted text".
std::string replaceWord(const std::string& text, const std::string& from, const std::string& to);

// A name an entry may have: lower case or capital letters, digits, spaces, - and _ (the catalogs hold names such as "iron sword"), 1 to 48 letters.
bool validEntryName(const std::string& name);

} // namespace odysseus::sim::refs
