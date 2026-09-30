#pragma once

#include "boundary.h"

#include "calendar.h"

#include <string>
#include <vector>

namespace odysseus::sim {

// How much an event matters to the clan's story, 0..100 (US-014). Printing a chronicle shows
// only entries at or above a threshold, so everyday gifts and thefts do not drown the births,
// deaths and feuds.
inline constexpr int kImportanceGift = 10;
inline constexpr int kImportanceTheft = 35;
inline constexpr int kImportanceStoreEmpty = 70; // hunger in the clan is a turn in its story
inline constexpr int kImportanceMammoth = 60;
inline constexpr int kImportancePeace = 60;
inline constexpr int kImportancePairing = 70;
inline constexpr int kImportanceFeud = 75;
inline constexpr int kImportanceFirstMammoth = 80;
inline constexpr int kImportanceBirth = 85;
inline constexpr int kImportanceDeath = 90;
inline constexpr int kDefaultChronicleThreshold = 50;

// One notable event, as a sentence (NA-02: the clan's tapestry).
struct ChronicleEntry {
    Date date;
    int importance = 0; // 0..100
    std::string text;   // "Ura was born to Tok and Maa."
};

class Chronicle {
public:
    void add(const Date& date, int importance, std::string text);

    const std::vector<ChronicleEntry>& entries() const { return entries_; }

    // The entries of one year (or of every year when year is 0) at or above the threshold,
    // in the order they happened.
    std::vector<ChronicleEntry> select(int year, int threshold) const;

private:
    std::vector<ChronicleEntry> entries_;
};

// "Spring, year 3: Ura was born to Tok and Maa."
std::string formatEntry(const ChronicleEntry& entry);

} // namespace odysseus::sim
