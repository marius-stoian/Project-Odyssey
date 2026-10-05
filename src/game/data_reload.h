#pragma once

#include "boundary.h"

#include <filesystem>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
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
    // Several files at once (the watcher): each set that watches any of them reloads once.
    std::vector<ReloadOutcome> changed(const std::vector<std::filesystem::path>& files);
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

// Notices files that changed outside the game (US-304): the owner saves a file in a text editor and the game reads it again by itself. It polls the
// time and size of every file under the watched roots (std::filesystem, no thread, no library): the caller gives it the time, so a test drives it with
// a clock of its own. A change is reported once it has been quiet for kDebounceSeconds (an editor often writes twice), so a saved change is live within
// about 0.25 + 0.3 s plus the reload, inside the 1 s of D-58 Q8.
class FileWatcher {
public:
    static constexpr double kPollSeconds = 0.25;  // a round of looks at every file starts at most this often
    static constexpr int kFilesPerTick = 20;      // files looked at in one call: a tick stays cheap, a round of 200 files takes ten ticks
    static constexpr double kDebounceSeconds = 0.3;

    // A file, or a folder (everything under it counts).
    void watch(const std::filesystem::path& root);
    // Takes the times of every watched file as they are now: what is already there is not a change.
    void snapshot();
    // Call every tick with the time in seconds. The files that changed, appeared or went and have been quiet for the debounce time (each once).
    std::vector<std::filesystem::path> poll(double nowSeconds);

    // The game wrote this file itself (an Editor save): the change it made is ignored once.
    void noteOwnWrite(const std::filesystem::path& file);
    // The game read a root again after its own write (a reload it made): the times under it are taken as they are, and what waited there is dropped.
    void resync(const std::filesystem::path& root);

    std::size_t files() const { return known_.size(); }

private:
    struct Stamp {
        std::filesystem::file_time_type time{};
        std::uintmax_t size = 0;
        friend bool operator==(const Stamp&, const Stamp&) = default;
    };
    struct Seen {
        std::filesystem::path path;
        Stamp stamp;
    };
    struct Waiting {
        std::filesystem::path path;
        double since = 0.0;
    };
    std::map<std::string, Seen> scan(const std::filesystem::path& root) const;
    void look(double nowSeconds, int& budget);
    void compare(const std::filesystem::path& root, const std::map<std::string, Seen>& current, double nowSeconds);

    std::vector<std::filesystem::path> roots_;
    std::map<std::string, Stamp> known_;
    std::map<std::string, Stamp> own_;   // the stamp an own write left behind: a change to exactly this is not reported
    std::map<std::string, Waiting> waiting_;
    double roundStart_ = -1.0; // when the round of looks began
    bool inRound_ = false;
    std::size_t rootIndex_ = 0; // the root being looked at in this round
    std::optional<std::filesystem::recursive_directory_iterator> dir_; // where the look at a folder stopped
    std::map<std::string, Seen> partial_;                              // the files seen so far under that root
};

} // namespace odysseus::game
