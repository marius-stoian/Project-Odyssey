#pragma once

#include "boundary.h"

#include "sim/calendar.h"
#include "sim/needs.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace odysseus::sim {

// The persons of a level that are not part of the clan (US-262, D-52): the people placed in a level, and later the people of a region. Each one has
// needs, ages day by day, belongs to a family and keeps a few memories, like a clan member, but they are kept in a compact store (struct of arrays,
// no object and no heap block per person, ADR-022 in US-263) so that tens of thousands fit. Deterministic: whole numbers only, no random numbers,
// the same ticks give the same persons. The simulation of the clan (World) is not touched.
class NpcPopulation {
public:
    static constexpr int kNotesPerPerson = 6; // each person remembers their last six days or meetings

    // How the daily rules restore a need that has fallen low (the land, the fire and the neighbours provide, since the placed people have no store).
    struct DailyConfig {
        std::array<int, kNeedCount> restore{40, 40, 30, 20};
        int restoreBelow = 50; // a need that ends the day under this is restored
    };

    NpcPopulation() : calendar_(CalendarConfig{}) {}
    NpcPopulation(CalendarConfig calendar, NeedsConfig needs, DailyConfig daily = {});

    // Adds a person and returns their index. `id` is the id of the placed character in the level and the same in the save: one identity from the level
    // file to the save. Adding an id twice returns the index of the first.
    int add(int id, std::string_view kind, int ageDays, int family, int x, int y);

    // One fixed step (1/20 of a game second). When a game day ends, every person ages one day, their needs change and they remember the day.
    void tick();
    void runTicks(std::uint64_t count) {
        for (std::uint64_t i = 0; i < count; ++i) tick();
    }

    std::size_t size() const { return ids_.size(); }
    // The index of the person with this id, or -1.
    int indexOf(int id) const;
    std::uint64_t ticks() const { return ticks_; }
    std::int64_t day() const { return static_cast<std::int64_t>(ticks_ / static_cast<std::uint64_t>(calendar_.ticksPerDay())); }

    int id(int index) const { return ids_[at(index)]; }
    const std::string& kind(int index) const { return kindNames_[kinds_[at(index)]]; }
    int ageDays(int index) const { return ages_[at(index)]; }
    int family(int index) const { return families_[at(index)]; }
    void setFamily(int index, int family) { families_[at(index)] = family; }
    int x(int index) const { return xs_[at(index)]; }
    int y(int index) const { return ys_[at(index)]; }
    int need(int index, Need which) const { return needs_[at(index) * kNeedCount + static_cast<std::size_t>(which)]; }

    // What a person remembers, oldest first.
    struct Note {
        std::int64_t day = 0;
        std::string text;
        int feeling = 0; // -100..100
    };
    int noteCount(int index) const { return noteCounts_[at(index)]; }
    Note note(int index, int which) const; // which: 0 = the oldest kept
    // Adds a memory dated today; the oldest of the six goes when the person remembers more.
    void remember(int index, std::string_view text, int feeling);
    // The hero was near: the person remembers meeting them (once is enough for a day; the caller decides when).
    void meetHero(int index) { remember(index, "met the hero", 10); }

    // One number summarising every person; equal populations have equal hashes (ADR-011).
    std::uint64_t hash() const;

    // The save text (versioned JSON) and its reader. The text of a loaded population is the text it was saved from; a damaged text is a DataError.
    std::string toText() const;
    static NpcPopulation fromText(std::string_view text, CalendarConfig calendar, NeedsConfig needs, DailyConfig daily = {});

    static constexpr int kSaveVersion = 1;

private:
    struct StoredNote {
        std::int32_t day = 0;
        std::int16_t text = 0; // an index into texts_
        std::int16_t feeling = 0;
    };

    std::size_t at(int index) const { return static_cast<std::size_t>(index); }
    int intern(std::vector<std::string>& names, std::unordered_map<std::string, int>& lookup, std::string_view text);
    void dailyUpdate();
    void rememberStored(std::size_t person, int text, int feeling);

    Calendar calendar_;
    NeedsConfig needsConfig_;
    DailyConfig daily_;
    std::uint64_t ticks_ = 0;

    // One entry per person, same index in every array.
    std::vector<std::int32_t> ids_;
    std::vector<std::uint16_t> kinds_;
    std::vector<std::int32_t> ages_;
    std::vector<std::int32_t> families_;
    std::vector<std::int32_t> xs_;
    std::vector<std::int32_t> ys_;
    std::vector<std::int16_t> needs_;       // kNeedCount per person
    std::vector<std::uint8_t> noteCounts_;
    std::vector<std::uint8_t> noteHeads_;   // where the next note is written
    std::vector<StoredNote> notes_;         // kNotesPerPerson per person, a ring
    std::unordered_map<int, int> indexById_; // only looked up, never walked: the order of the arrays is the order of adding

    std::vector<std::string> kindNames_;
    std::unordered_map<std::string, int> kindLookup_;
    std::vector<std::string> texts_;
    std::unordered_map<std::string, int> textLookup_;
};

} // namespace odysseus::sim
