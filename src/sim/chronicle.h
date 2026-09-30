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
inline constexpr int kImportanceQuarrel = 30;
inline constexpr int kImportanceSharing = 30;
inline constexpr int kImportanceSickness = 35;
inline constexpr int kImportanceNursing = 35;
inline constexpr int kImportanceAdoption = 55;
inline constexpr int kImportanceRecovery = 25;
inline constexpr int kImportanceBlame = 60;
inline constexpr int kImportanceRevenge = 80;
inline constexpr int kImportanceExile = 80;
inline constexpr int kImportanceCourtship = 35;
inline constexpr int kImportanceRejection = 40;
inline constexpr int kImportanceJealousy = 55;
inline constexpr int kImportanceParting = 65;
inline constexpr int kImportanceApprentice = 45;
inline constexpr int kImportanceGraduation = 50;
inline constexpr int kImportanceHuntParty = 60;
inline constexpr int kImportanceHero = 65;
inline constexpr int kImportanceCoward = 55;
inline constexpr int kImportanceRescue = 65;
inline constexpr int kImportanceTheft = 35;
inline constexpr int kImportanceLean = 55;       // a failed harvest: the root of many hard winters
inline constexpr int kImportanceStoreEmpty = 70; // hunger in the clan is a turn in its story
inline constexpr int kImportanceMammoth = 60;
inline constexpr int kImportancePeace = 60;
inline constexpr int kImportancePairing = 70;
inline constexpr int kImportanceFeud = 75;
inline constexpr int kImportanceFirstMammoth = 80;
inline constexpr int kImportanceBirth = 85;
inline constexpr int kImportanceDeath = 90;
inline constexpr int kDefaultChronicleThreshold = 50;

// What kind of event an entry is (M2b story engine). The number is saved in save files, so new
// kinds only ever join at the end. Who / other / aux mean, by kind:
//   Birth: who = the child, other = the mother, aux = the father
//   Death: who = the one who died, other = a person behind the cause (a thief, a hunt's leader)
//   Pairing, Parting, Feud, Peace, Quarrel, Revenge: who and other are the two people
//   Theft: who = the thief, other = a witness or -1
//   Blame: who = the one who blames, other = the one blamed, aux = the dead person
//   Gift, Courtship, Rescue and the like: who does it to other
// Kinds that later stories use are listed now so the numbers never change.
enum class EventKind {
    Note, Birth, Death, Pairing, Parting, Feud, Peace, Theft, StoreEmpty, Lean, Mammoth, Gift,
    Quarrel, Blame, Revenge, Exile, Sickness, Injury, Recovery, Nursing, Sharing, Adoption,
    Courtship, Jealousy, Rejection, Apprentice, Graduation, HuntParty, Rescue, Hero, Coward,
    Count
};

const char* eventKindName(EventKind kind);

// "Tok", "Tok and Brak", "Tok, Brak and Ura".
std::string joinNames(const std::vector<std::string>& names);

// One notable event, as a sentence (NA-02: the clan's tapestry), with the ids of the earlier
// events that caused it, so the events link into a story (STO-03).
struct ChronicleEntry {
    Date date;
    int importance = 0; // 0..100
    std::string text;   // "Ura was born to Tok and Maa."
    int id = 0;         // its place in the log; never reused
    EventKind kind = EventKind::Note;
    int who = -1;       // PersonIds, -1 = none (see EventKind for their meaning)
    int other = -1;
    int aux = -1;
    std::vector<int> causes; // ids of earlier events
};

class Chronicle {
public:
    // Free text with no links (kind Note). Returns the new entry's id.
    int add(const Date& date, int importance, std::string text);
    // The full form: kind, people and causes. The causes must be earlier entries.
    int record(const Date& date, int importance, EventKind kind, int who, int other, int aux, std::vector<int> causes,
               std::string text);

    const std::vector<ChronicleEntry>& entries() const { return entries_; }
    // The entry with this id, or nullptr.
    const ChronicleEntry* find(int id) const;

    // The entries of one year (or of every year when year is 0) at or above the threshold,
    // in the order they happened.
    std::vector<ChronicleEntry> select(int year, int threshold) const;

private:
    std::vector<ChronicleEntry> entries_;
};

// "Spring, year 3: Ura was born to Tok and Maa."
std::string formatEntry(const ChronicleEntry& entry);

// An event and, indented below it, the earlier events that caused it, and theirs in turn
// ("Traceable", US-110). Each event is listed once; the depth is capped for readability.
std::vector<std::string> explainEvent(const Chronicle& chronicle, int id);

} // namespace odysseus::sim
