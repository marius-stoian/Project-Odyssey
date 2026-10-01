#pragma once

#include "boundary.h"

#include "actions.h"
#include "memory.h"
#include "needs.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace odysseus::sim {

using PersonId = int; // index into World::people(), never reused: the dead keep their place

enum class Sex { Female, Male };

enum class CauseOfDeath { None, Starvation, Cold, OldAge, Hunting, Childbirth, Fight, Wound, Illness, Count };

// How a person is: well, sick or injured (M2b). The unwell cannot work and may die of it.
enum class Health { Well, Sick, Injured };

// "starvation", "the cold", ... as used in the chronicle: "Tok died of starvation."
const char* causeName(CauseOfDeath cause);

// The six traits of D-02. A person has one or two; Brave and Timid never together.
enum class Trait { Brave, Timid, Kind, Greedy, Talkative, Diligent, Count };

inline constexpr std::size_t kTraitCount = static_cast<std::size_t>(Trait::Count);

const char* traitName(Trait trait);

// What the AI decided last hour, with every action's score (0 = not possible), so the
// headless runner can show why someone did what they did (US-012 "Inspectable").
struct Decision {
    std::array<int, kActionCount> scores{};
    Action chosen = Action::Rest;
};

// A reason to think badly of someone: the event that turned `owner` against `about` (M2b).
// Feuds, blame and revenge look through these to say *why* (STO-03).
struct Grudge {
    int about = -1;   // PersonId
    int event = -1;   // chronicle entry that caused it
    int weight = 0;   // how much it counted (opinion lost)
};

struct Person;

// The heaviest grudge `holder` has about `other`, or `best` when it is heavier, or nullptr.
// Equal weights: the earlier event (a total order, so no ties remain).
const Grudge* heaviestGrudge(const Person& holder, int other, const Grudge* best = nullptr);

// One member of the clan. Plain data: the systems (needs, AI, memory) are functions that
// read and change it. This "struct as component" style is what an ECS formalises later.
struct Person {
    PersonId id = 0;
    std::string name;
    Sex sex = Sex::Female;
    int ageDays = 0;
    bool alive = true;
    CauseOfDeath causeOfDeath = CauseOfDeath::None;
    bool exiled = false;      // driven out by the clan: no longer alive in the clan, but not dead (M2b)
    Health health = Health::Well;
    int healthDays = 0;       // days left until they recover
    int healthEvent = -1;     // the chronicle entry that made them unwell
    int carer = -1;           // who nurses them now (a PersonId), -1 = nobody
    int nursing = -1;         // whom they nurse now, -1 = nobody
    int guardian = -1;        // who took them in as an orphan, -1 = nobody
    Needs needs;
    int daysAtZeroHunger = 0; // consecutive day starts with Hunger at 0
    int daysAtZeroWarmth = 0; // the same for Warmth (only winter cold kills)

    std::uint8_t traits = 0; // one bit per Trait
    int gatherSkill = 10;    // 0..100, grows with practice
    int huntSkill = 10;
    int gatherPractice = 0;  // hours practised towards the next skill point
    int huntPractice = 0;
    Action action = Action::Rest; // what they are doing this hour
    Decision lastDecision;
    std::vector<Memory> memories;   // oldest first (US-013)
    std::vector<MemoryNote> notes;  // what they remember that is not about two people, oldest first (US-163); saved, never read by the simulation
    std::vector<int> opinions;      // opinion of every person, by id: -100..100
    std::vector<Grudge> grudges;    // why they think badly of others (M2b), oldest first
    std::int64_t lastGiftDay = -1;  // one gift a day at most
    std::int64_t lastTheftDay = -1; // thieves wait a few days between thefts
    int mother = -1;       // PersonIds, -1 = unknown (the founders) or none
    int father = -1;
    int partner = -1;
    int courting = -1;        // the unpaired adult they court now, -1 = nobody (M2b)
    int courtDays = 0;        // days spent courting them
    int courtEvent = -1;      // the chronicle entry that began the courtship
    std::int64_t courtPauseDay = -1; // no courting before this day: a broken heart mends
    int master = -1;          // who teaches them now (a youth's teacher), -1 = nobody (M2b)
    int apprentice = -1;      // whom they teach now, -1 = nobody
    bool teachHunt = false;   // the lessons are in hunting (else gathering)
    int teachEvent = -1;      // the chronicle entry that began the apprenticeship
    int pregnantDays = 0;  // 0 = not expecting
    int childFather = -1;  // the father of the child she is expecting
    std::int64_t lastBirthDay = -1;

    int ageYears(int daysPerYear) const { return ageDays / daysPerYear; }
    bool has(Trait trait) const { return (traits & (1U << static_cast<unsigned>(trait))) != 0; }
    void give(Trait trait) { traits = static_cast<std::uint8_t>(traits | (1U << static_cast<unsigned>(trait))); }
};

} // namespace odysseus::sim
