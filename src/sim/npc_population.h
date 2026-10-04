#pragma once

#include "boundary.h"

#include "sim/calendar.h"
#include "sim/needs.h"
#include "sim/opinion.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace odysseus::sim {

// The persons of a level or a region that are not part of the clan (US-262, US-263, D-52, ADR-022): the people placed in a level, and the crowds of an
// end-game region. Each one has needs, ages day by day, belongs to a family and keeps a few memories, like a clan member, but they are kept in a compact
// store (struct of arrays: one array per field, no object and no heap block per person) so that 100,000 fit. Deterministic: whole numbers only, no
// random numbers, the same ticks and the same focus give the same persons. The simulation of the clan (World) is not touched.
//
// Detail by distance (D-52 S-02): the hours of a person's day are simulated only for persons within `nearRadius` of the focus (the hero), looked up
// through a spatial grid, never by walking the whole store. Every person gets the rest of the day's changes when the day ends, so a person is the same at
// the end of a day whether they were near or far all day, or walked in and out of the radius: no jump and no loss.
class NpcPopulation {
public:
    static constexpr int kNotesPerPerson = 6; // each person remembers their last six days or meetings

    // How the daily rules restore a need that has fallen low (the land, the fire and the neighbours provide, since the placed people have no store).
    struct DailyConfig {
        std::array<int, kNeedCount> restore{40, 40, 30, 20};
        int restoreBelow = 50;     // a need that ends the day under this is restored
        int nearRadius = 800;      // pixels (25 tiles): persons this close to the focus get their hours simulated
    };

    NpcPopulation() : calendar_(CalendarConfig{}) {}
    NpcPopulation(CalendarConfig calendar, NeedsConfig needs, DailyConfig daily = {});

    // Adds a person and returns their index. `id` is the id of the placed character in the level and the same in the save: one identity from the level
    // file to the save. Adding an id twice returns the index of the first.
    // `attitude` is the starting attitude of the person to the hero (US-264).
    int add(int id, std::string_view kind, int ageDays, int family, int x, int y, Attitude attitude = Attitude::Neutral);
    // A person moves (schedules, M9c): the grid follows (it is only touched when the person changes cell).
    void move(int index, int x, int y);
    // A person close to this one, picked by `salt` among those in the same grid cell of the grid, within `radius` pixels: -1 when there is nobody. It looks at a few
    // places of the cell and never walks a crowded one, so it costs the same in a crowd of 100,000 (US-292). The answer depends only on who stands where.
    int neighbour(int index, int salt, int radius) const;

    // Where "near" is measured from: the hero. Until it is set every person is far.
    void setFocus(int x, int y) {
        focusX_ = x;
        focusY_ = y;
        focusSet_ = true;
    }
    // Whether a person is within the near radius of the focus now.
    bool isNear(int index) const;
    // The persons within the near radius of the focus, in ascending order (empty until the focus is set).
    std::vector<int> nearFocus() const;
    bool hasFocus() const { return focusSet_; }
    // The indices of every person within `radius` pixels of a point, in ascending order. Costs the cells it touches, not the size of the store.
    std::vector<int> near(int x, int y, int radius) const;
    std::size_t nearCount() const { return nearNow_; } // how many persons had their hour simulated at the last hour mark

    // One fixed step (1/20 of a game second). Every game hour the near persons are brought up to that hour; when a game day ends, every person ages one day, their
    // needs change and they remember the day.
    void tick();
    void runTicks(std::uint64_t count) {
        for (std::uint64_t i = 0; i < count; ++i) tick();
    }

    std::size_t size() const { return ids_.size(); }
    // The index of the person with this id, or -1.
    int indexOf(int id) const;
    std::uint64_t ticks() const { return ticks_; }
    int ticksPerHour() const { return calendar_.ticksPerHour(); }
    int ticksPerDay() const { return calendar_.ticksPerDay(); }
    // The hour of the day (0 to 23) at the tick that has just run.
    int hourOfDay() const { return static_cast<int>((ticks_ / static_cast<std::uint64_t>(calendar_.ticksPerHour())) % kHoursPerDay); }
    const NeedsConfig& needsConfig() const { return needsConfig_; }
    std::int64_t day() const { return static_cast<std::int64_t>(ticks_ / static_cast<std::uint64_t>(calendar_.ticksPerDay())); }
    const DailyConfig& daily() const { return daily_; }

    int id(int index) const { return ids_[at(index)]; }
    const std::string& kind(int index) const { return kindNames_[kinds_[at(index)]]; }
    int ageDays(int index) const { return ages_[at(index)]; }
    int family(int index) const { return families_[at(index)]; }
    void setFamily(int index, int family) { families_[at(index)] = family; }
    int x(int index) const { return xs_[at(index)]; }
    int y(int index) const { return ys_[at(index)]; }
    int need(int index, Need which) const { return needs_[at(index) * kNeedCount + static_cast<std::size_t>(which)]; }
    // A need changes (eating, sleeping, and the tests of schedules): clamped from 0 to the maximum of the needs.
    void setNeed(int index, Need which, int value) { needs_[at(index) * kNeedCount + static_cast<std::size_t>(which)] = static_cast<std::int16_t>(std::clamp(value, 0, needsConfig_.maximum)); }
    // How many hours of today are already in this person's needs (0 to 24).
    int hoursApplied(int index) const { return hours_[at(index)]; }

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

    // Opinions (US-264, D-52 Q-04, Q-05): every person has an opinion, a whole number from -100 to 100, of the hero and of every person they have met, and
    // the attitude word follows from it. Only pairs that have met are stored (an entry appears the first time something happens between two); until then
    // the opinion is what it would be by itself: the starting attitude for the hero, the family bonus for a person of the same family, else 0. `target` is a
    // person id, or kHero.
    static constexpr int kHero = 0;
    void setOpinionConfig(const OpinionConfig& config) { opinionConfig_ = config; }
    const OpinionConfig& opinionConfig() const { return opinionConfig_; }
    // A creature (an animal or a monster, US-262) is not a person but has an attitude to the hero like one: it is registered by the id of its placed
    // character with its starting attitude, and then holds opinions of the hero like a person does (US-266). It has no needs, memories or age.
    void addCreature(int id, Attitude attitude) { creatures_[id] = static_cast<std::uint8_t>(attitude); }
    std::size_t creatureCount() const { return creatures_.size(); }
    // Whether the id is a person or a creature, so it has an opinion of the hero.
    bool hasOpinions(int holderId) const { return indexOf(holderId) >= 0 || creatures_.count(holderId) != 0; }
    // Whether the holder knows the target: they have met, or they are of the same family.
    bool knows(int holderId, int targetId) const;
    Attitude startAttitude(int holderId) const; // the attitude the person was given at the start
    int opinion(int holderId, int targetId) const;
    Mood mood(int holderId, int targetId) const;
    Attitude attitude(int holderId, int targetId) const; // the word: fear and envy, else the band of the opinion
    bool met(int holderId, int targetId) const;          // an entry exists
    int adjust(int holderId, int targetId, int delta);  // the opinion changes (clamped); the pair now counts as met; returns the new opinion
    void setMood(int holderId, int targetId, Mood mood);
    // An event of opinions.json by name ("gift", "trade", "help", "marriage-in-family"...): the opinion changes by its amount. False for an unknown name.
    bool event(int holderId, int targetId, std::string_view name);
    // A conversation of quality -2 (awful) to 2 (great): its amount, plus the bonus when the pair talked within the last days.
    void talked(int holderId, int targetId, int quality);
    std::size_t opinionEntries() const { return pairs_.size(); }

    // One number summarising every person; equal populations have equal hashes (ADR-011).
    std::uint64_t hash() const;

    // The save text (versioned JSON, one array per person to keep 100,000 of them small) and its reader. A damaged text is a DataError.
    std::string toText() const;
    static NpcPopulation fromText(std::string_view text, CalendarConfig calendar, NeedsConfig needs, DailyConfig daily = {});

    static constexpr int kSaveVersion = 3;

private:
    struct StoredNote {
        std::int32_t day = 0;
        std::int16_t text = 0; // an index into texts_
        std::int16_t feeling = 0;
    };

    std::size_t at(int index) const { return static_cast<std::size_t>(index); }
    int intern(std::vector<std::string>& names, std::unordered_map<std::string, int>& lookup, std::string_view text);
    // The cumulative fall of a need through hour `hour` (0..24) of a day with this rate; the 24 hourly shares add up to the daily rate exactly.
    static int fallThrough(int rate, int hour) { return rate * hour / kHoursPerDay; }
    void catchUp(std::size_t person, int hour, bool winter); // brings the needs of a person up to the end of hour `hour` of today
    void hourMark(int hour);
    void dailyUpdate();
    void rememberStored(std::size_t person, int text, int feeling);
    static std::int64_t cellKey(int x, int y);
    void gridAdd(int index);
    void gridRemove(int index);

    Calendar calendar_;
    NeedsConfig needsConfig_;
    DailyConfig daily_;
    std::uint64_t ticks_ = 0;
    int focusX_ = 0;
    int focusY_ = 0;
    bool focusSet_ = false;
    std::size_t nearNow_ = 0;

    // One entry per person, same index in every array.
    std::vector<std::int32_t> ids_;
    std::vector<std::uint16_t> kinds_;
    std::vector<std::int32_t> ages_;
    std::vector<std::int32_t> families_;
    std::vector<std::int32_t> xs_;
    std::vector<std::int32_t> ys_;
    std::vector<std::int16_t> needs_;       // kNeedCount per person
    std::vector<std::uint8_t> attitudes_;   // the starting attitude of each person to the hero (an Attitude)
    std::vector<std::uint8_t> hours_;       // hours of today already applied to the needs
    std::vector<std::uint8_t> noteCounts_;
    std::vector<std::uint8_t> noteHeads_;   // where the next note is written
    std::vector<StoredNote> notes_;         // kNotesPerPerson per person, a ring
    std::unordered_map<int, int> indexById_; // only looked up, never walked: the order of the arrays is the order of adding

    // The opinions of the pairs that met (US-264), keyed by holder id and target id.
    struct Pair {
        std::int16_t opinion = 0;
        std::uint8_t mood = 0; // a Mood
        std::int32_t lastTalkDay = -1000000;
    };
    static std::uint64_t pairKey(int holderId, int targetId) { return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(holderId)) << 32) | static_cast<std::uint32_t>(targetId); }
    bool holderInfo(int holderId, Attitude& attitude, int& family) const; // a person or a creature; false for anyone else
    Pair defaultPair(int holderId, int targetId) const;
    Pair* pairFor(int holderId, int targetId); // creates the entry from the default when the pair has not met; nullptr when the holder is nobody
    std::unordered_map<int, std::uint8_t> creatures_; // placed id -> starting Attitude
    OpinionConfig opinionConfig_;
    std::unordered_map<std::uint64_t, Pair> pairs_;

    // The spatial grid: cells of kCellSize pixels, each the indices of the persons in it, in the order they came.
    static constexpr int kCellSize = 256;
    std::unordered_map<std::int64_t, std::vector<std::int32_t>> grid_;

    std::vector<std::string> kindNames_;
    std::unordered_map<std::string, int> kindLookup_;
    std::vector<std::string> texts_;
    std::unordered_map<std::string, int> textLookup_;
};

} // namespace odysseus::sim
