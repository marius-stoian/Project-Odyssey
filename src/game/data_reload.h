#pragma once

#include "boundary.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace odysseus::game {

// What one reload of a data set did (US-303).
struct ReloadResult {
    bool ok = true;              // false: the new files had mistakes and the last good data stays in use
    bool atNextStart = false;    // the set cannot be swapped while the game runs: nothing was changed, the new files apply at the next start
    std::vector<std::string> errors;   // "file:line: message"
    std::vector<std::string> warnings; // things that did not stop the reload (a placed thing whose kind is gone)
    double milliseconds = 0.0;   // how long it took, measured by the registry (NFR-09: under 100 ms)
};

// One group of files that are reloaded together: the interaction files with the dialogues and quests, lights.json, the plant and object catalogs.
// `reload` reads the files into a copy on the side and replaces the data in use only when the copy is clean, so a typo never leaves the game
// half-reloaded; what must follow (placed things, palettes, help) is its job too.
struct DataSet {
    std::string name;
    std::vector<std::filesystem::path> watch; // files, or folders (everything under a folder belongs to the set)
    std::function<ReloadResult()> reload;
    bool restartOnly = false; // the files cannot be swapped while the game runs: a change is only reported ("applies at the next start"); F5 leaves it alone
};

struct ReloadOutcome {
    std::string set;
    std::filesystem::path file; // the file that was changed, when a file named the set (`changed`); empty for F5 and for a set reloaded by name
    ReloadResult result;
};

// The registry of reloadable data sets (US-303). The game registers its sets once; an Editor save names the file it wrote (`changed`), the
// watcher of US-304 names files changed outside, and F5 reloads everything (`reloadAll`).
class DataReload {
public:
    void add(DataSet set);
    const std::vector<DataSet>& sets() const { return sets_; }

    // The names of the sets that watch this file, in registration order.
    std::vector<std::string> setsFor(const std::filesystem::path& file) const;
    // Reloads each set that watches the file, once.
    std::vector<ReloadOutcome> changed(const std::filesystem::path& file);
    // Reloads the named set; nothing when there is no such set.
    std::vector<ReloadOutcome> reload(const std::string& name);
    std::vector<ReloadOutcome> reloadAll();

    // The latest outcome of a set, or nullptr when it never ran.
    const ReloadOutcome* last(const std::string& name) const;

private:
    ReloadOutcome run(const DataSet& set);

    std::vector<DataSet> sets_;
    std::vector<ReloadOutcome> last_;
};

} // namespace odysseus::game
