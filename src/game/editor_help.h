#pragma once

#include "boundary.h"

#include "luna/engine/ui.h"

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::game {

// What the Editor's fields say about themselves (US-300, D-58): purpose, range and example, read from assets/data/editor/help.json.
// A panel is given its help once it is built: every text and number field gets an id from its panel and its label ("npc.sword"),
// and the tooltip of that entry. A missing or broken file never stops the Editor: fields just show no tooltip.
class EditorHelp {
public:
    struct Entry {
        std::string purpose;
        std::string range;   // empty on a number field: made from the field's minimum and maximum
        std::string example;
        std::string suggest; // how the field offers values (US-302): number, none, values:a|b, files:folder/glob, catalog:name
        bool list = false;   // comma separated: the suggestion completes the item after the last comma
    };

    // Where the suggestions of a field come from (US-302). The game fills it in; every function reads the data in use at the moment it is called, so a class
    // saved a moment ago is offered at once.
    struct Sources {
        std::filesystem::path dataFolder;                                            // "files:<folder>/<glob>" is looked up under it
        std::function<std::vector<std::string>(const std::string& catalog)> catalog; // "catalog:<name>": the names of that catalog
        std::function<std::optional<int>(const std::string& fieldId)> numberDefault; // "number": the default of the kind or class being edited, if any
    };
    void setSources(Sources sources) { sources_ = std::move(sources); }
    // The values a field offers: what its entry's "suggest" names, read now. Empty for "none", for an id without an entry and for a source that is not set.
    // `minimum` and `maximum` are the limits of a number field (both 0: not a number field).
    std::vector<std::string> suggestionsFor(const std::string& id, int minimum = 0, int maximum = 0) const;

    // Reads the file. Every mistake becomes a line "file:line: message" in problems(); a clean file has none. A file that is
    // missing counts as a mistake too. After a mistake the help is empty: no field gets a tooltip.
    void load(const std::filesystem::path& file);
    const std::vector<std::string>& problems() const { return problems_; }
    // Reads the file again into a copy (US-303): only a clean copy replaces the entries in use. Returns the mistakes, none when it was taken.
    std::vector<std::string> reload(const std::filesystem::path& file);
    // The first mistake, for the Editor's status line; empty when the file is clean.
    std::string problem() const { return problems_.empty() ? std::string() : problems_.front(); }

    // Entries made from the schemas of the data files (US-191, K-M11: one help source): the Data tab's fields are named "data.<schema>.<path>" and their purpose and
    // example are the schema's description and example. They stay when help.json is read again.
    void setGenerated(std::map<std::string, Entry> entries) { generated_ = std::move(entries); }
    const std::map<std::string, Entry>& generated() const { return generated_; }
    // A field built by hand, not through apply(): it asks for its entry the same way, so the coverage test sees it.
    void ask(const std::string& id) { asked_.insert(id); }

    const Entry* find(const std::string& id) const;
    const std::map<std::string, Entry>& entries() const { return entries_; }

    // "npc" and "Sword: " give "npc.sword"; "who (elder or friend): " gives "event.who" (a hint in brackets is dropped) and an indented label gets "sub-". Lower case, letters and digits, other
    // runs become one '-'. Made from the label so the id cannot drift from the screen.
    static std::string fieldId(std::string_view panel, std::string_view label);

    // Gives every text and number field of the panel its help id and tooltip. Remembers the ids it was asked for (coverage test).
    void apply(luna::engine::Panel& panel, std::string_view prefix);

    // The ids of the fields apply() gave a list of suggestions to (a coverage test: every entry that is not "none" must reach a field).
    const std::set<std::string>& wired() const { return wired_; }

    // Every id apply() has met, and the ones of those without an entry.
    const std::set<std::string>& asked() const { return asked_; }
    std::vector<std::string> missing() const;

private:
    std::map<std::string, Entry> entries_;
    std::map<std::string, Entry> generated_; // from the schemas
    std::vector<std::string> problems_;
    std::set<std::string> asked_;
    std::set<std::string> wired_;
    Sources sources_;
    std::map<std::string, std::vector<int>> recent_; // per field id: the last numbers typed there this session, newest first
};

} // namespace odysseus::game
