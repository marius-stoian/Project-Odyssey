#pragma once

#include "boundary.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace odysseus::sim {

// What a memory is about (US-013). More kinds join as the clan's life grows.
enum class MemoryKind { Gift, Theft, Death, Quarrel, Blame, Fight, Nursing, Sharing, Rescue, Rejection, Count };

const char* memoryKindName(MemoryKind kind);

// Something a person remembers: who did what, when, and how they feel about it (D-02).
// A copy heard through gossip is weaker (half the feeling) and marked second-hand.
struct Memory {
    int subject = 0;       // who did it (a PersonId)
    int object = 0;        // to whom (a PersonId)
    MemoryKind kind = MemoryKind::Gift;
    std::int64_t day = 0;  // when it happened (days since the world began)
    int feeling = 0;       // -100 (hateful) .. +100 (wonderful)
    bool major = false;    // major memories are kept for life; minor ones fade
    bool secondHand = false;
    int event = -1;        // the chronicle entry this memory is about, -1 = none (M2b): causes can be traced

    // The same event, however it was heard: gossip never copies an event twice.
    bool sameEvent(const Memory& other) const {
        return subject == other.subject && object == other.object && kind == other.kind && day == other.day;
    }
};

// From assets/data/sim/social.json.
struct SocialConfig {
    int giftFeeling = 40;
    int giftOpinion = 15;        // how much a gift raises the receiver's opinion of the giver
    int theftFeeling = -60;
    int theftOpinion = -25;
    int theftMeals = 2;          // meals a thief takes from the store
    int witnessPercent = 50;     // chance someone sees a theft
    int theftCooldownDays = 5;   // a thief waits this long before stealing again
    int talkOpinion = 2;         // a good talk warms two people to each other
    int gossipPercent = 50;      // chance a talk passes on one memory
    int talkativeGossipPercent = 80;
    int minorMemoryDays = 60;    // minor memories are forgotten after this many days
    int memoryLimit = 40;        // a person keeps at most this many memories
};

SocialConfig loadSocialConfig(const std::filesystem::path& file);

// Forgets minor memories older than the lifetime; major ones stay. Returns how many went.
int forgetOldMemories(std::vector<Memory>& memories, std::int64_t today, int minorMemoryDays);

// Adds a memory, making room by forgetting the oldest minor one (or else the oldest).
void remember(std::vector<Memory>& memories, const Memory& memory, int limit);

} // namespace odysseus::sim
