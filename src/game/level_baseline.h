#pragma once

#include "boundary.h"

#include "game/level.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>

namespace odysseus::game {

// What the level looked like when a run was saved (US-305, EDT-11): for every placed thing its id and a hash of its entry in the level file. When the run is
// loaded the baseline is compared with the level as it is now; the things whose hash is the same keep the state of the run, the others come fresh from the
// level. The hash is of the entry as the level file writes it (`entryText`), so every field the file holds counts and no field is forgotten.
class LevelBaseline {
public:
    // The kinds of things a level places, as written in the save.
    enum Kind { Character, Pickup, Plant, Effect, Light, Building, KindCount };

    static LevelBaseline of(const Level& level);

    bool empty() const { return !present_; } // a save from before M10b has none
    const std::map<int, std::uint64_t>& hashes(Kind kind) const { return hashes_[kind]; }

    std::string toText() const;                       // JSON: {"character": {"3": "hex"}, ...}
    static LevelBaseline fromText(const std::string& text); // a damaged text is an empty baseline: the save loads as one from before M10b

    friend bool operator==(const LevelBaseline&, const LevelBaseline&) = default;

private:
    std::map<int, std::uint64_t> hashes_[KindCount];
    bool present_ = false;
};

// What the level changed since the baseline, per kind: `updated` holds ids that are new or whose entry changed (they come fresh from the level), `removed` ids that
// are no longer in the level (they leave the run).
struct LevelChanges {
    struct Ids {
        std::set<int> updated;
        std::set<int> removed;
        bool touches(int id) const { return updated.contains(id) || removed.contains(id); }
    };
    Ids kinds[LevelBaseline::KindCount];

    const Ids& of(LevelBaseline::Kind kind) const { return kinds[kind]; }
    // How many things the level updated, for the status line: new, changed and removed ones together.
    int count() const;
    bool any() const { return count() > 0; }
};

// Compares a baseline with the level now. An empty baseline changes nothing: a save without one loads as before, and is written with one.
LevelChanges compareBaseline(const LevelBaseline& baseline, const Level& level);

// The text of the status line: "The level updated 3 things", or empty when it updated none.
std::string levelChangesMessage(const LevelChanges& changes);

} // namespace odysseus::game
