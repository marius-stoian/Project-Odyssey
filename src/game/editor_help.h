#pragma once

#include "boundary.h"

#include "luna/engine/ui.h"

#include <filesystem>
#include <map>
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

    // Reads the file. Every mistake becomes a line "file:line: message" in problems(); a clean file has none. A file that is
    // missing counts as a mistake too. After a mistake the help is empty: no field gets a tooltip.
    void load(const std::filesystem::path& file);
    const std::vector<std::string>& problems() const { return problems_; }
    // The first mistake, for the Editor's status line; empty when the file is clean.
    std::string problem() const { return problems_.empty() ? std::string() : problems_.front(); }

    const Entry* find(const std::string& id) const;
    const std::map<std::string, Entry>& entries() const { return entries_; }

    // "npc" and "Sword: " give "npc.sword"; "who (elder or friend): " gives "who-elder-or-friend". Lower case, letters and digits, other
    // runs become one '-'. Made from the label so the id cannot drift from the screen.
    static std::string fieldId(std::string_view panel, std::string_view label);

    // Gives every text and number field of the panel its help id and tooltip. Remembers the ids it was asked for (coverage test).
    void apply(luna::engine::Panel& panel, std::string_view prefix);

    // Every id apply() has met, and the ones of those without an entry.
    const std::set<std::string>& asked() const { return asked_; }
    std::vector<std::string> missing() const;
    void forgetAsked() { asked_.clear(); }

private:
    std::map<std::string, Entry> entries_;
    std::vector<std::string> problems_;
    std::set<std::string> asked_;
};

} // namespace odysseus::game
