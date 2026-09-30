#pragma once

#include "boundary.h"

#include "actions.h"
#include "ai.h"
#include "calendar.h"
#include "chronicle.h"
#include "clan.h"
#include "life.h"
#include "needs.h"
#include "person.h"

#include "core/random.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace odysseus::sim {

// Every tunable number of the simulation, loaded from assets/data/sim/ (Charter rule 7).
struct SimConfig {
    CalendarConfig calendar;
    NeedsConfig needs;
    ActionConfig actions;
    ClanConfig clan;
    NameList names;
    SocialConfig social;
    LifeConfig life;
};

SimConfig loadSimConfig(const std::filesystem::path& dataDirectory);

// Separate random streams per system (Charter rule 6): adding a random call in one system
// never changes the numbers another system sees.
enum class Stream : std::uint64_t { Weather = 1, People = 2, Decisions = 3, Hunting = 4, Social = 5, Life = 6 };

// The whole simulated world. No graphics, no operating system: it runs the same in the
// game, in the headless runner and in tests, and the same seed gives the same history.
class World {
public:
    World(std::uint64_t seed, SimConfig config);

    // One fixed step: 1/20 of a game second (ADR-006).
    void tick();
    void runTicks(std::uint64_t count);

    std::uint64_t ticks() const { return ticks_; }
    Date date() const { return calendar_.dateAt(ticks_); }
    const Calendar& calendar() const { return calendar_; }
    std::uint64_t seed() const { return seed_; }

    // Today's temperature in whole degrees Celsius, rolled each morning (weather stream).
    int temperature() const { return temperature_; }

    // Everyone who ever lived, in id order (the dead keep their place).
    const std::vector<Person>& people() const { return people_; }
    int population() const;
    int food() const { return food_; }
    const Chronicle& chronicle() const { return chronicle_; }
    const SimConfig& config() const { return config_; }

    // The clan's shared food store, in meals, and how badly it needs filling (0..100).
    int storePressure() const;
    // What the AI sees for this person now (US-012).
    Situation situationOf(const Person& person) const;
    const Person* findPerson(const std::string& nameOrId) const;

    // With daily life off, nobody acts or eats: needs only decay. Tests of the needs rules
    // alone use it (US-011 "without eating, sleeping, warmth or company").
    void setDailyLife(bool enabled) { dailyLife_ = enabled; }

    // Social events (US-013). The AI's actions call these; tests may too.
    // A gift: the receiver remembers who gave it and likes the giver more.
    void giveGift(int giver, int receiver);
    // A theft seen by a witness: a major, bitter memory about the thief.
    void recordTheft(int thief, int witness);
    // Two people talk: both feel better and like each other a little more, and the speaker
    // may pass on one memory the listener lacks, as a weaker copy (gossip). Returns true
    // if a memory was passed on.
    bool talk(int speaker, int listener);
    // What `who` thinks of `about`, -100..100.
    int opinion(int who, int about) const;

    // Life events (US-014), called by the daily rules; tests may call them too.
    // Two adults become partners.
    void pair(int a, int b);
    // A hunter kills a mammoth: a feast, and the clan's first is remembered.
    void bringDownMammoth(int hunter);
    // Pairs of people who feud now (smaller id first).
    const std::vector<std::pair<int, int>>& feuds() const { return feuds_; }
    int mammothsKilled() const { return mammoths_; }

    // One number summarising the entire state; equal worlds have equal hashes (ADR-011).
    std::uint64_t hash() const;

private:
    void startDay();
    void passHour(int hour);
    void doAction(Person& person);
    void eatTogether();
    void decideAll(int nextHour);
    void practise(int& practice, int& skill);
    void changeOpinion(Person& who, int about, int change);
    Person* favouriteAwake(const Person& person, bool courting = false);
    bool courtable(const Person& a, const Person& b) const;
    std::int64_t today() const { return date().day; }
    void checkSurvival(Person& person, bool winter);
    void die(Person& person, CauseOfDeath cause, int other = -1);
    void lifeEvents();
    void pairUp();
    void updateFeuds();
    void giveBirth(Person& mother, std::vector<Person>& newborns);
    bool closeKin(const Person& a, const Person& b) const;

    std::uint64_t seed_;
    SimConfig config_;
    Calendar calendar_;
    std::uint64_t ticks_ = 0;
    core::Pcg32 weather_;
    core::Pcg32 peopleRandom_;
    core::Pcg32 decisionRandom_;
    core::Pcg32 huntRandom_;
    core::Pcg32 socialRandom_;
    core::Pcg32 lifeRandom_;
    std::vector<std::pair<int, int>> feuds_;
    int mammoths_ = 0;
    bool storeRanOut_ = false; // while true, another empty evening is not news
    int forageLeft_ = 0;       // what the land still offers today (carrying capacity)
    int gameLeft_ = 0;
    int hour_ = 1; // the hour of the day now starting (1..24)
    bool dailyLife_ = true;
    int temperature_ = 0;
    std::vector<Person> people_;
    int food_ = 0;
    Chronicle chronicle_;
};

} // namespace odysseus::sim
