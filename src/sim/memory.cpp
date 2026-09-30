#include "sim/memory.h"

#include "sim/json_data.h"

#include <algorithm>

namespace odysseus::sim {

const char* memoryKindName(MemoryKind kind) {
    switch (kind) {
    case MemoryKind::Gift: return "gift";
    case MemoryKind::Theft: return "theft";
    case MemoryKind::Death: return "death";
    case MemoryKind::Quarrel: return "quarrel";
    case MemoryKind::Blame: return "blame";
    case MemoryKind::Fight: return "fight";
    case MemoryKind::Nursing: return "nursing";
    case MemoryKind::Sharing: return "sharing";
    case MemoryKind::Rescue: return "rescue";
    case MemoryKind::Rejection: return "rejection";
    default: return "?";
    }
}

SocialConfig loadSocialConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    SocialConfig config;
    config.giftFeeling = requireInt(json, file, "giftFeeling", 0, 100);
    config.giftOpinion = requireInt(json, file, "giftOpinion", 0, 100);
    config.theftFeeling = requireInt(json, file, "theftFeeling", -100, 0);
    config.theftOpinion = requireInt(json, file, "theftOpinion", -100, 0);
    config.theftMeals = requireInt(json, file, "theftMeals", 0, 100);
    config.witnessPercent = requireInt(json, file, "witnessPercent", 0, 100);
    config.theftCooldownDays = requireInt(json, file, "theftCooldownDays", 1, 365);
    config.talkOpinion = requireInt(json, file, "talkOpinion", 0, 100);
    config.gossipPercent = requireInt(json, file, "gossipPercent", 0, 100);
    config.talkativeGossipPercent = requireInt(json, file, "talkativeGossipPercent", 0, 100);
    config.minorMemoryDays = requireInt(json, file, "minorMemoryDays", 1, 10'000);
    config.memoryLimit = requireInt(json, file, "memoryLimit", 1, 1000);
    return config;
}

int forgetOldMemories(std::vector<Memory>& memories, std::int64_t today, int minorMemoryDays) {
    const auto before = memories.size();
    // erase + remove_if: move the survivors to the front, then cut off the rest.
    memories.erase(std::remove_if(memories.begin(), memories.end(),
                                  [today, minorMemoryDays](const Memory& memory) {
                                      return !memory.major && today - memory.day > minorMemoryDays;
                                  }),
                   memories.end());
    return static_cast<int>(before - memories.size());
}

void remember(std::vector<Memory>& memories, const Memory& memory, int limit) {
    if (static_cast<int>(memories.size()) >= limit) {
        // Memories are kept oldest first, so the first minor one is the oldest minor one.
        auto oldest = std::find_if(memories.begin(), memories.end(), [](const Memory& m) { return !m.major; });
        memories.erase(oldest != memories.end() ? oldest : memories.begin());
    }
    memories.push_back(memory);
}

} // namespace odysseus::sim
