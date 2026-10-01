#include "sim/npc_chooser.h"

#include <algorithm>

namespace odysseus::sim::rules {

std::optional<std::size_t> pickBest(const std::vector<int>& scores, int minimum, odysseus::core::Pcg32& random) {
    int best = minimum - 1;
    for (const int score : scores) best = std::max(best, score);
    const std::uint32_t draw = random.next(); // one draw every time, tie or not
    if (best < minimum) return std::nullopt;
    std::vector<std::size_t> top;
    for (std::size_t i = 0; i < scores.size(); ++i) {
        if (scores[i] == best) top.push_back(i);
    }
    return top[draw % top.size()];
}

bool CooldownTable::ready(int actor, const std::string& interaction, std::int64_t now) const {
    const auto it = until_.find({actor, interaction});
    return it == until_.end() || now >= it->second;
}

void CooldownTable::start(int actor, const std::string& interaction, std::int64_t until) {
    until_[{actor, interaction}] = until;
}

void CooldownTable::forget(int actor) {
    std::erase_if(until_, [actor](const auto& entry) { return entry.first.first == actor; });
}

std::uint64_t CooldownTable::hash() const {
    std::uint64_t h = 0xCBF29CE484222325ULL;
    const auto mix = [&h](std::uint64_t value) { h ^= value + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2); };
    for (const auto& [key, until] : until_) {
        mix(static_cast<std::uint64_t>(key.first));
        for (const char c : key.second) mix(static_cast<unsigned char>(c));
        mix(static_cast<std::uint64_t>(until));
    }
    return h;
}

} // namespace odysseus::sim::rules
