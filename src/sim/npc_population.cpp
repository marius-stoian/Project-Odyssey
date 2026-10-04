#include "sim/npc_population.h"

#include "sim/data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>

namespace odysseus::sim {

namespace {

using nlohmann::json;

void mix(std::uint64_t& hash, std::uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        hash ^= (value >> (i * 8)) & 0xFFU;
        hash *= 0x100000001B3ULL; // FNV-1a
    }
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
    noteCounts_.push_back(0);
    noteHeads_.push_back(0);
    notes_.resize(notes_.size() + kNotesPerPerson);
    indexById_.emplace(id, index);
    return index;
}

int NpcPopulation::indexOf(int id) const {
    const auto found = indexById_.find(id);
    return found == indexById_.end() ? -1 : found->second;
}

void NpcPopulation::tick() {
    ++ticks_;
    if (ticks_ % static_cast<std::uint64_t>(calendar_.ticksPerDay()) == 0) dailyUpdate();
}

// A day ends: everyone ages a day, each need falls by its daily rate (more for warmth in winter) and, when it ends the day low, is restored by what the
// land, the fire and the neighbours give; and everyone remembers how the day went.
void NpcPopulation::dailyUpdate() {
    const bool winter = calendar_.dateAt(ticks_).season == Season::Winter;
    const int ordinary = intern(texts_, textLookup_, "an ordinary day");
    const int hard = intern(texts_, textLookup_, "a hard day");
    const int good = intern(texts_, textLookup_, "a good day");
    for (std::size_t person = 0; person < ids_.size(); ++person) {
        ++ages_[person];
        int total = 0;
        for (std::size_t n = 0; n < kNeedCount; ++n) {
            const Need which = static_cast<Need>(n);
            int value = needs_[person * kNeedCount + n] - dailyRate(needsConfig_, which, winter);
            if (value < 0) value = 0;
            if (value < daily_.restoreBelow) value = std::min(needsConfig_.maximum, value + daily_.restore[n]);
            needs_[person * kNeedCount + n] = static_cast<std::int16_t>(value);
            total += value;
        }
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
        for (const char c : kindNames_[kinds_[person]]) mix(h, static_cast<unsigned char>(c));
        mix(h, static_cast<std::uint64_t>(ages_[person]));
        mix(h, static_cast<std::uint64_t>(families_[person]));
        mix(h, static_cast<std::uint64_t>(xs_[person]));
        mix(h, static_cast<std::uint64_t>(ys_[person]));
        for (std::size_t n = 0; n < kNeedCount; ++n) mix(h, static_cast<std::uint64_t>(needs_[person * kNeedCount + n]));
        mix(h, noteCounts_[person]);
        for (int k = 0; k < noteCounts_[person]; ++k) {
            const Note n = note(static_cast<int>(person), k);
            mix(h, static_cast<std::uint64_t>(n.day));
            mix(h, static_cast<std::uint64_t>(n.feeling));
            for (const char c : n.text) mix(h, static_cast<unsigned char>(c));
        }
    }
    return h;
}

std::string NpcPopulation::toText() const {
    json people = json::array();
    for (std::size_t person = 0; person < ids_.size(); ++person) {
        json notes = json::array();
        for (int k = 0; k < noteCounts_[person]; ++k) {
            const Note n = note(static_cast<int>(person), k);
            notes.push_back({{"day", n.day}, {"text", n.text}, {"feeling", n.feeling}});
        }
        json needs = json::array();
        for (std::size_t n = 0; n < kNeedCount; ++n) needs.push_back(needs_[person * kNeedCount + n]);
        people.push_back({{"id", ids_[person]}, {"kind", kindNames_[kinds_[person]]}, {"ageDays", ages_[person]}, {"family", families_[person]},
                          {"x", xs_[person]}, {"y", ys_[person]}, {"needs", needs}, {"notes", notes}});
    }
    return json{{"version", kSaveVersion}, {"ticks", ticks_}, {"people", people}}.dump(1) + "\n";
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
        if (data.at("version").get<int>() > kSaveVersion) {
            throw DataError(file, "version", "was saved by a newer version of the game");
        }
        NpcPopulation population(calendar, needs, daily);
        population.ticks_ = data.at("ticks").get<std::uint64_t>();
        for (const json& entry : data.at("people")) {
            const int index = population.add(entry.at("id").get<int>(), entry.at("kind").get<std::string>(), entry.at("ageDays").get<int>(),
                                             entry.at("family").get<int>(), entry.at("x").get<int>(), entry.at("y").get<int>());
            const json& values = entry.at("needs");
            if (values.size() != kNeedCount) throw DataError(file, "needs", "must hold one number per need");
            for (std::size_t n = 0; n < kNeedCount; ++n) population.needs_[population.at(index) * kNeedCount + n] = static_cast<std::int16_t>(values.at(n).get<int>());
            for (const json& note : entry.at("notes")) population.remember(index, note.at("text").get<std::string>(), note.at("feeling").get<int>());
            // remember() dated the note today; the saved day is the true one.
            const std::size_t person = population.at(index);
            const int count = population.noteCounts_[person];
            for (int k = 0; k < count; ++k) {
                const int first = (population.noteHeads_[person] - count + 2 * kNotesPerPerson) % kNotesPerPerson;
                population.notes_[person * kNotesPerPerson + static_cast<std::size_t>((first + k) % kNotesPerPerson)].day =
                    static_cast<std::int32_t>(entry.at("notes").at(static_cast<std::size_t>(k)).at("day").get<std::int64_t>());
            }
        }
        return population;
    } catch (const json::exception& error) {
        throw DataError(file, "(content)", error.what());
    }
}

} // namespace odysseus::sim
