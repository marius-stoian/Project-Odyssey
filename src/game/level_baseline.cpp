#include "game/level_baseline.h"

#include <format>
#include <nlohmann/json.hpp>

namespace odysseus::game {

namespace {

constexpr const char* kNames[LevelBaseline::KindCount] = {"character", "pickup", "plant", "effect", "light", "building"};

// FNV-1a over the text of the entry: the same entry always has the same hash, on every machine.
std::uint64_t hashOf(const std::string& text) {
    std::uint64_t hash = 0xCBF29CE484222325ULL;
    for (const unsigned char c : text) {
        hash ^= c;
        hash *= 0x100000001B3ULL;
    }
    return hash;
}

std::string hex(std::uint64_t value) { return std::format("{:016x}", value); }

// The entries of one kind as they are in the level now: id -> hash.
template <typename Things>
std::map<int, std::uint64_t> hashesOf(const Things& things) {
    std::map<int, std::uint64_t> hashes;
    for (const auto& thing : things) hashes[thing.id] = hashOf(entryText(thing));
    return hashes;
}

} // namespace

LevelBaseline LevelBaseline::of(const Level& level) {
    LevelBaseline baseline;
    baseline.present_ = true;
    baseline.hashes_[Character] = hashesOf(level.characters);
    baseline.hashes_[Pickup] = hashesOf(level.pickups);
    baseline.hashes_[Plant] = hashesOf(level.plants);
    baseline.hashes_[Effect] = hashesOf(level.effects);
    baseline.hashes_[Light] = hashesOf(level.lights);
    baseline.hashes_[Building] = hashesOf(level.buildings);
    return baseline;
}

std::string LevelBaseline::toText() const {
    nlohmann::json root = nlohmann::json::object();
    for (int kind = 0; kind < KindCount; ++kind) {
        nlohmann::json entries = nlohmann::json::object();
        for (const auto& [id, hash] : hashes_[kind]) entries[std::to_string(id)] = hex(hash);
        root[kNames[kind]] = std::move(entries);
    }
    return root.dump();
}

LevelBaseline LevelBaseline::fromText(const std::string& text) {
    LevelBaseline baseline;
    const nlohmann::json root = nlohmann::json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object()) return baseline;
    LevelBaseline read;
    for (int kind = 0; kind < KindCount; ++kind) {
        const auto found = root.find(kNames[kind]);
        if (found == root.end() || !found->is_object()) return baseline; // not a baseline this game wrote: treated as none
        for (const auto& [key, value] : found->items()) {
            try {
                if (!value.is_string()) return baseline;
                read.hashes_[kind][std::stoi(key)] = std::stoull(value.get<std::string>(), nullptr, 16);
            } catch (const std::exception&) {
                return baseline;
            }
        }
    }
    read.present_ = true;
    return read;
}

int LevelChanges::count() const {
    int total = 0;
    for (const Ids& ids : kinds) total += static_cast<int>(ids.updated.size() + ids.removed.size());
    return total;
}

LevelChanges compareBaseline(const LevelBaseline& baseline, const Level& level) {
    LevelChanges changes;
    if (baseline.empty()) return changes;
    const LevelBaseline now = LevelBaseline::of(level);
    for (int kind = 0; kind < LevelBaseline::KindCount; ++kind) {
        const auto which = static_cast<LevelBaseline::Kind>(kind);
        const std::map<int, std::uint64_t>& before = baseline.hashes(which);
        const std::map<int, std::uint64_t>& after = now.hashes(which);
        for (const auto& [id, hash] : after) {
            const auto old = before.find(id);
            if (old == before.end() || old->second != hash) changes.kinds[kind].updated.insert(id); // new, or its entry changed
        }
        for (const auto& [id, hash] : before) {
            if (!after.contains(id)) changes.kinds[kind].removed.insert(id);
        }
    }
    return changes;
}

std::string levelChangesMessage(const LevelChanges& changes) {
    const int count = changes.count();
    if (count == 0) return {};
    return std::format("The level updated {} thing{}", count, count == 1 ? "" : "s");
}

} // namespace odysseus::game
