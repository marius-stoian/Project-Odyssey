#include "sim/npc_population.h"

#include "sim/data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <charconv>
#include <format>

namespace odysseus::sim {

namespace {

using nlohmann::json;

static_assert(kHoursPerDay == 24);

void mix(std::uint64_t& hash, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        hash ^= (value >> (i * 8)) & 0xFFU;
        hash *= 0x100000001B3ULL; // FNV-1a
    }
}

void append(std::string& out, std::int64_t value) {
    char buffer[24];
    const auto [end, error] = std::to_chars(buffer, buffer + sizeof(buffer), value);
    (void)error;
    out.append(buffer, end);
}

} // namespace

NpcPopulation::NpcPopulation(CalendarConfig calendar, NeedsConfig needs, DailyConfig daily)
    : calendar_(calendar), needsConfig_(needs), daily_(daily) {}

int NpcPopulation::intern(std::vector<std::string>& names, std::unordered_map<std::string, int>& lookup, std::string_view text) {
    const std::string key(text);
    if (const auto found = lookup.find(key); found != lookup.end()) return found->second;
    const int index = static_cast<int>(names.size());
    names.push_back(key);
    lookup.emplace(key, index);
    return index;
}

std::int64_t NpcPopulation::cellKey(int x, int y) {
    static_assert(kCellSize == 256, "the grid shifts by 8 bits");
    return (static_cast<std::int64_t>(x >> 8) << 32) ^ static_cast<std::int64_t>(static_cast<std::uint32_t>(y >> 8));
}

void NpcPopulation::gridAdd(int index) { grid_[cellKey(xs_[at(index)], ys_[at(index)])].push_back(index); }

void NpcPopulation::gridRemove(int index) {
    auto found = grid_.find(cellKey(xs_[at(index)], ys_[at(index)]));
    if (found == grid_.end()) return;
    auto& cell = found->second;
    cell.erase(std::remove(cell.begin(), cell.end(), index), cell.end());
    if (cell.empty()) grid_.erase(found);
}

int NpcPopulation::add(int id, std::string_view kind, int ageDays, int family, int x, int y) {
    if (const int existing = indexOf(id); existing >= 0) return existing;
    const int index = static_cast<int>(ids_.size());
    ids_.push_back(id);
    kinds_.push_back(static_cast<std::uint16_t>(intern(kindNames_, kindLookup_, kind)));
    ages_.push_back(ageDays);
    families_.push_back(family);
    xs_.push_back(x);
    ys_.push_back(y);
    for (std::size_t n = 0; n < kNeedCount; ++n) needs_.push_back(static_cast<std::int16_t>(needsConfig_.maximum));
    hours_.push_back(0);
    noteCounts_.push_back(0);
    noteHeads_.push_back(0);
    notes_.resize(notes_.size() + kNotesPerPerson);
    indexById_.emplace(id, index);
    gridAdd(index);
    return index;
}

void NpcPopulation::move(int index, int x, int y) {
    gridRemove(index);
    xs_[at(index)] = x;
    ys_[at(index)] = y;
    gridAdd(index);
}

int NpcPopulation::indexOf(int id) const {
    const auto found = indexById_.find(id);
    return found == indexById_.end() ? -1 : found->second;
}

bool NpcPopulation::isNear(int index) const {
    if (!focusSet_) return false;
    const std::int64_t dx = xs_[at(index)] - focusX_;
    const std::int64_t dy = ys_[at(index)] - focusY_;
    return dx * dx + dy * dy <= static_cast<std::int64_t>(daily_.nearRadius) * daily_.nearRadius;
}

std::vector<int> NpcPopulation::near(int x, int y, int radius) const {
    std::vector<int> found;
    const std::int64_t limit = static_cast<std::int64_t>(radius) * radius;
    for (int cy = (y - radius) >> 8; cy <= (y + radius) >> 8; ++cy) {
        for (int cx = (x - radius) >> 8; cx <= (x + radius) >> 8; ++cx) {
            const auto cell = grid_.find((static_cast<std::int64_t>(cx) << 32) ^ static_cast<std::int64_t>(static_cast<std::uint32_t>(cy)));
            if (cell == grid_.end()) continue;
            for (const std::int32_t index : cell->second) {
                const std::int64_t dx = xs_[at(index)] - x;
                const std::int64_t dy = ys_[at(index)] - y;
                if (dx * dx + dy * dy <= limit) found.push_back(index);
            }
        }
    }
    std::sort(found.begin(), found.end()); // the same order whatever the grid held
    return found;
}

void NpcPopulation::tick() {
    ++ticks_;
    const std::uint64_t perHour = static_cast<std::uint64_t>(calendar_.ticksPerHour());
    if (ticks_ % perHour != 0) return;
    const int hour = static_cast<int>((ticks_ / perHour) % kHoursPerDay);
    if (hour == 0) dailyUpdate();
    else hourMark(hour);
}

// The needs of a person fall through the hours; whoever was not looked at in between gets the hours they missed all at once. The fall through hour h is the
// daily rate times h over 24, so the total of any split of the day is the same.
void NpcPopulation::catchUp(std::size_t person, int hour, bool winter) {
    const int applied = hours_[person];
    if (hour <= applied) return;
    for (std::size_t n = 0; n < kNeedCount; ++n) {
        const int rate = dailyRate(needsConfig_, static_cast<Need>(n), winter);
        int value = needs_[person * kNeedCount + n] - (fallThrough(rate, hour) - fallThrough(rate, applied));
        if (value < 0) value = 0;
        needs_[person * kNeedCount + n] = static_cast<std::int16_t>(value);
    }
    hours_[person] = static_cast<std::uint8_t>(hour);
}

// An hour of the day is over: the persons near the focus have it simulated now (the others get it at the end of the day).
void NpcPopulation::hourMark(int hour) {
    nearNow_ = 0;
    if (!focusSet_) return;
    const bool winter = calendar_.dateAt(ticks_ - 1).season == Season::Winter;
    const std::vector<int> nearby = near(focusX_, focusY_, daily_.nearRadius);
    nearNow_ = nearby.size();
    for (const int index : nearby) catchUp(at(index), hour, winter);
}

// A day ends: everyone gets what is left of the day's fall, ages a day and, when a need ended the day low, is restored by what the land, the fire and the
// neighbours give; and everyone remembers how the day went.
void NpcPopulation::dailyUpdate() {
    const bool winter = calendar_.dateAt(ticks_ - 1).season == Season::Winter;
    const int ordinary = intern(texts_, textLookup_, "an ordinary day");
    const int hard = intern(texts_, textLookup_, "a hard day");
    const int good = intern(texts_, textLookup_, "a good day");
    for (std::size_t person = 0; person < ids_.size(); ++person) {
        catchUp(person, kHoursPerDay, winter);
        ++ages_[person];
        int total = 0;
        for (std::size_t n = 0; n < kNeedCount; ++n) {
            int value = needs_[person * kNeedCount + n];
            if (value < daily_.restoreBelow) value = std::min(needsConfig_.maximum, value + daily_.restore[n]);
            needs_[person * kNeedCount + n] = static_cast<std::int16_t>(value);
            total += value;
        }
        hours_[person] = 0;
        const int average = total / static_cast<int>(kNeedCount);
        const int text = average >= 70 ? good : (average < 40 ? hard : ordinary);
        rememberStored(person, text, average - 50);
    }
}

void NpcPopulation::rememberStored(std::size_t person, int text, int feeling) {
    const std::size_t base = person * kNotesPerPerson;
    StoredNote& slot = notes_[base + noteHeads_[person]];
    slot.day = static_cast<std::int32_t>(day());
    slot.text = static_cast<std::int16_t>(text);
    slot.feeling = static_cast<std::int16_t>(std::clamp(feeling, -100, 100));
    noteHeads_[person] = static_cast<std::uint8_t>((noteHeads_[person] + 1) % kNotesPerPerson);
    if (noteCounts_[person] < kNotesPerPerson) ++noteCounts_[person];
}

void NpcPopulation::remember(int index, std::string_view text, int feeling) {
    rememberStored(at(index), intern(texts_, textLookup_, text), feeling);
}

NpcPopulation::Note NpcPopulation::note(int index, int which) const {
    const std::size_t person = at(index);
    const int count = noteCounts_[person];
    // The oldest kept note sits `count` places before the head.
    const int first = (noteHeads_[person] - count + 2 * kNotesPerPerson) % kNotesPerPerson;
    const StoredNote& stored = notes_[person * kNotesPerPerson + static_cast<std::size_t>((first + which) % kNotesPerPerson)];
    return {stored.day, texts_[static_cast<std::size_t>(stored.text)], stored.feeling};
}

std::uint64_t NpcPopulation::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    mix(h, ticks_);
    mix(h, ids_.size());
    for (std::size_t person = 0; person < ids_.size(); ++person) {
        mix(h, static_cast<std::uint64_t>(ids_[person]));
        mix(h, kinds_[person]);
        mix(h, static_cast<std::uint64_t>(ages_[person]));
        mix(h, static_cast<std::uint64_t>(families_[person]));
        mix(h, static_cast<std::uint64_t>(xs_[person]));
        mix(h, static_cast<std::uint64_t>(ys_[person]));
        mix(h, hours_[person]);
        for (std::size_t n = 0; n < kNeedCount; ++n) mix(h, static_cast<std::uint64_t>(needs_[person * kNeedCount + n]));
        mix(h, noteCounts_[person]);
        for (int k = 0; k < noteCounts_[person]; ++k) {
            const Note n = note(static_cast<int>(person), k);
            mix(h, static_cast<std::uint64_t>(n.day));
            mix(h, static_cast<std::uint64_t>(n.feeling));
            for (const char c : n.text) mix(h, static_cast<unsigned char>(c));
        }
    }
    for (const std::string& kind : kindNames_) {
        for (const char c : kind) mix(h, static_cast<unsigned char>(c));
    }
    return h;
}

// One array per person: [id, kind, age, family, x, y, hours, hunger, energy, warmth, social, notes, then day, text, feeling for every note, oldest first].
std::string NpcPopulation::toText() const {
    std::string out = "{\"version\":2,\"ticks\":";
    append(out, static_cast<std::int64_t>(ticks_));
    out += ",\"kinds\":" + json(kindNames_).dump() + ",\"texts\":" + json(texts_).dump() + ",\"people\":[";
    for (std::size_t person = 0; person < ids_.size(); ++person) {
        out += person == 0 ? "[" : ",[";
        for (const std::int64_t value : {static_cast<std::int64_t>(ids_[person]), static_cast<std::int64_t>(kinds_[person]), static_cast<std::int64_t>(ages_[person]),
                                        static_cast<std::int64_t>(families_[person]), static_cast<std::int64_t>(xs_[person]), static_cast<std::int64_t>(ys_[person]),
                                        static_cast<std::int64_t>(hours_[person])}) {
            append(out, value);
            out += ',';
        }
        for (std::size_t n = 0; n < kNeedCount; ++n) {
            append(out, needs_[person * kNeedCount + n]);
            out += ',';
        }
        append(out, noteCounts_[person]);
        const int count = noteCounts_[person];
        const int first = (noteHeads_[person] - count + 2 * kNotesPerPerson) % kNotesPerPerson;
        for (int k = 0; k < count; ++k) {
            const StoredNote& stored = notes_[person * kNotesPerPerson + static_cast<std::size_t>((first + k) % kNotesPerPerson)];
            out += ',';
            append(out, stored.day);
            out += ',';
            append(out, stored.text);
            out += ',';
            append(out, stored.feeling);
        }
        out += ']';
    }
    out += "]}\n";
    return out;
}

NpcPopulation NpcPopulation::fromText(std::string_view text, CalendarConfig calendar, NeedsConfig needs, DailyConfig daily) {
    const std::filesystem::path file = "npcs.json";
    json data;
    try {
        data = json::parse(text);
    } catch (const json::exception& error) {
        throw DataError(file, "(syntax)", error.what());
    }
    try {
        if (data.at("version").get<int>() != kSaveVersion) {
            throw DataError(file, "version", std::format("is {}: this game reads version {}", data.at("version").get<int>(), kSaveVersion));
        }
        NpcPopulation population(calendar, needs, daily);
        population.ticks_ = data.at("ticks").get<std::uint64_t>();
        population.kindNames_ = data.at("kinds").get<std::vector<std::string>>();
        for (std::size_t k = 0; k < population.kindNames_.size(); ++k) population.kindLookup_.emplace(population.kindNames_[k], static_cast<int>(k));
        population.texts_ = data.at("texts").get<std::vector<std::string>>();
        for (std::size_t k = 0; k < population.texts_.size(); ++k) population.textLookup_.emplace(population.texts_[k], static_cast<int>(k));
        for (const json& entry : data.at("people")) {
            if (entry.size() < 12) throw DataError(file, "people", "a person needs at least 12 numbers");
            const int kind = entry.at(1).get<int>();
            if (kind < 0 || static_cast<std::size_t>(kind) >= population.kindNames_.size()) throw DataError(file, "people", "a person has an unknown kind");
            const int index = population.add(entry.at(0).get<int>(), population.kindNames_[static_cast<std::size_t>(kind)], entry.at(2).get<int>(), entry.at(3).get<int>(),
                                             entry.at(4).get<int>(), entry.at(5).get<int>());
            const std::size_t person = population.at(index);
            population.hours_[person] = static_cast<std::uint8_t>(entry.at(6).get<int>());
            for (std::size_t n = 0; n < kNeedCount; ++n) population.needs_[person * kNeedCount + n] = static_cast<std::int16_t>(entry.at(7 + n).get<int>());
            const int count = entry.at(11).get<int>();
            if (count < 0 || count > kNotesPerPerson || entry.size() != static_cast<std::size_t>(12 + 3 * count)) throw DataError(file, "people", "a person has a wrong number of memories");
            for (int k = 0; k < count; ++k) {
                const int textIndex = entry.at(static_cast<std::size_t>(13 + 3 * k)).get<int>();
                if (textIndex < 0 || static_cast<std::size_t>(textIndex) >= population.texts_.size()) throw DataError(file, "people", "a memory has an unknown text");
                StoredNote& slot = population.notes_[person * kNotesPerPerson + static_cast<std::size_t>(k)];
                slot.day = static_cast<std::int32_t>(entry.at(static_cast<std::size_t>(12 + 3 * k)).get<std::int64_t>());
                slot.text = static_cast<std::int16_t>(textIndex);
                slot.feeling = static_cast<std::int16_t>(entry.at(static_cast<std::size_t>(14 + 3 * k)).get<int>());
            }
            population.noteCounts_[person] = static_cast<std::uint8_t>(count);
            population.noteHeads_[person] = static_cast<std::uint8_t>(count % kNotesPerPerson);
        }
        return population;
    } catch (const json::exception& error) {
        throw DataError(file, "(content)", error.what());
    }
}

} // namespace odysseus::sim
