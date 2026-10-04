#pragma once

#include "boundary.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace odysseus::sim {

// The nine attitude words (D-52 Q-04). Each NPC has its own attitude to every entity it knows; there are no factions.
enum class Attitude : std::uint8_t { Friendly, Neutral, Wary, Hostile, Scared, Suspicious, Enchanted, Lovingly, Enviously, Count };

inline constexpr std::size_t kAttitudeCount = static_cast<std::size_t>(Attitude::Count);

const char* attitudeName(Attitude attitude);
std::optional<Attitude> attitudeFromName(std::string_view name);

// A feeling that is not a point on the scale of liking: fear and envy colour the word whatever the opinion number says, until something clears it.
enum class Mood : std::uint8_t { None, Scared, Enviously };

// From assets/data/sim/opinions.json (US-264): where the words begin, what each kind of event is worth, what a new NPC thinks of the hero before they have
// met, and how dialogue counts. Every number is data; whole numbers only.
struct OpinionConfig {
    static constexpr int kMin = -100;
    static constexpr int kMax = 100;

    // The lowest opinion each band of the scale begins at (a word is the highest band whose start is not above the opinion).
    // Hostile, wary, suspicious, neutral, friendly, enchanted, lovingly.
    struct Band {
        Attitude word;
        int from;
    };
    std::array<Band, 7> bands{{{Attitude::Hostile, -100}, {Attitude::Wary, -59}, {Attitude::Suspicious, -29}, {Attitude::Neutral, -9},
                               {Attitude::Friendly, 10}, {Attitude::Enchanted, 40}, {Attitude::Lovingly, 70}}};
    // The opinion of the hero an NPC starts with, by the attitude it is given in its kind file or the level.
    std::array<int, kAttitudeCount> start{25, 0, -45, -80, -10, -20, 55, 80, -15};
    // What happens to an opinion: "gift", "trade", "help", "marriage-in-family"... (any name; the games asks for them by name).
    std::map<std::string, int> events{{"gift", 15}, {"trade", 5}, {"help", 20}, {"marriage-in-family", 10}, {"insult", -20}, {"theft", -30}};
    int sameFamily = 20;       // two persons of the same family start with this opinion of each other
    // Dialogue: the quality of a conversation, -2 (awful) .. 2 (great), and the bonus for talking often.
    std::array<int, 5> talk{-8, -4, 0, 3, 6};
    int talkFrequencyBonus = 1; // added when the pair talked within the last `talkFrequencyDays` days
    int talkFrequencyDays = 3;
};

OpinionConfig loadOpinionConfig(const std::filesystem::path& file);

// The word for an opinion and a mood: fear and envy win; otherwise the band the number falls in.
Attitude attitudeFor(const OpinionConfig& config, int opinion, Mood mood);
// The opinion of the hero (and the mood) a given starting attitude means.
int startOpinion(const OpinionConfig& config, Attitude attitude);
Mood startMood(Attitude attitude);

} // namespace odysseus::sim
