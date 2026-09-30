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
#include "story.h"

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
    StoryConfig story;
};

SimConfig loadSimConfig(const std::filesystem::path& dataDirectory);

// Separate random streams per system (Charter rule 6): adding a random call in one system
// never changes the numbers another system sees.
enum class Stream : std::uint64_t { Weather = 1, People = 2, Decisions = 3, Hunting = 4, Social = 5, Life = 6, Story = 7 };

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
    // A theft, seen by `witness` (or by nobody: -1). It is always an event in the chronicle;
    // a witness also keeps a major, bitter memory of the thief. Returns the event's id.
    int recordTheft(int thief, int witness);
    // Two people talk: both feel better and like each other a little more, and the speaker
    // may pass on one memory the listener lacks, as a weaker copy (gossip). Returns true
    // if a memory was passed on.
    bool talk(int speaker, int listener);

    // Story interactions (US-111). The daily rules call these; tests may too.
    // Two people quarrel: both remember it and think less of each other. Returns the event's id.
    int quarrel(int a, int b);
    // `griever` blames `blamed` for the death of `dead` (whose death was event `deathEvent`), for
    // the reason `why` ("because Brak had stolen from the store"). Returns the event's id.
    int blame(int griever, int blamed, int dead, int deathEvent, const std::string& why);
    // The aggressor of a feud takes revenge on the victim: a fight, or the clan drives the
    // aggressor out. Returns the event's id.
    int takeRevenge(int aggressor, int victim);
    // Someone is hurt for `days` days, by the chronicle entry `cause`.
    void injure(int person, int days, int cause);
    // What `who` thinks of `about`, -100..100.
    int opinion(int who, int about) const;

    // Life events (US-014), called by the daily rules; tests may call them too.
    // Two adults become partners.
    void pair(int a, int b);
    // A hunter kills a mammoth: a feast, and the clan's first is remembered.
    void bringDownMammoth(int hunter);
    // Pairs of people who feud now (smaller id first).
    std::vector<std::pair<int, int>> feuds() const;
    int mammothsKilled() const { return mammoths_; }

    // One number summarising the entire state; equal worlds have equal hashes (ADR-011).
    std::uint64_t hash() const;

private:
    // Saving and loading (US-016) must see every private member: only the save format may.
    friend struct WorldArchive;

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
    // Records the death as an event with its reason and returns the event's id. `causeEvent`
    // is the event behind the death when the caller knows it (a birth, a fight...).
    int die(Person& person, CauseOfDeath cause, int causeEvent = -1);
    // The story engine's helpers (US-110): who thinks badly of whom and why, and how the
    // reasons are put into words.
    void addGrudge(Person& owner, int about, int event, int weight);
    std::string reasonPhrase(const ChronicleEntry& event) const;
    // The most recent entries of a kind within `days` (newest first, at most maxCauses).
    std::vector<int> recentEvents(EventKind kind, int days) const;
    const std::string& nameOf(int id) const { return people_[static_cast<std::size_t>(id)].name; }
    std::string seasonPhrase() const;
    void lifeEvents();
    void encounters();      // every morning each person meets someone; quarrels may follow
    void considerRevenge(); // feuds that keep worsening end in a fight or an exile
    void updateHealth();    // the hurt heal, or die of their wounds
    int exile(Person& person, int victim, std::vector<int> causes, const std::string& text);
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
    // A running feud: the two people (smaller id first), the chronicle entry that started it and
    // when, and the day revenge was last taken (-1 = never).
    struct FeudRecord {
        int a = 0;
        int b = 0;
        int event = -1;
        std::int64_t sinceDay = 0;
        std::int64_t lastRevengeDay = -1;
    };
    std::vector<FeudRecord> feuds_;
    int mammoths_ = 0;
    bool storeRanOut_ = false; // while true, another empty evening is not news
    core::Pcg32 storyRandom_;
    int leanEvent_ = -1;       // this year's failed harvest (a chronicle id) until the next spring
    int forageLeft_ = 0;       // what the land still offers today (carrying capacity)
    int lastMammothYear_ = 0;  // the herd passes the valley once a year: one mammoth at most
    int gameLeft_ = 0;
    int hour_ = 1; // the hour of the day now starting (1..24)
    bool dailyLife_ = true;
    int temperature_ = 0;
    std::vector<Person> people_;
    int food_ = 0;
    Chronicle chronicle_;
};

} // namespace odysseus::sim
