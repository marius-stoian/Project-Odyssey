#pragma once

#include "boundary.h"

#include "actions.h"
#include "ai.h"
#include "calendar.h"
#include "chronicle.h"
#include "clan.h"
#include "life.h"
#include "needs.h"
#include "npc_schedule.h"
#include "person.h"
#include "story.h"

#include "core/random.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
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
    // The routines of the professions (US-196), by profession id, given by the hero's data (`configForComfort`): a clan member's profession is worked out from their skills
    // (`professionOf`) and the active block of that routine weighs the actions they may choose. Empty: nobody has a routine, and nothing changes.
    std::map<std::string, rules::Schedule> routines;
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

    // The rules swapped while the clan lives (US-194): called between ticks, never inside one. A change the running clan cannot take (the length of a day or a
    // season, the top of the needs scale: everything already counted in them would change meaning) is described by `configProblem` ("" when the new rules fit)
    // and `replaceConfig` must not be called then. What only a new clan reads (its starting people and food) changes nothing for this one.
    std::string configProblem(const SimConfig& next) const;
    void replaceConfig(SimConfig next) { config_ = std::move(next); }

    std::uint64_t ticks() const { return ticks_; }
    Date date() const { return calendar_.dateAt(ticks_); }
    const Calendar& calendar() const { return calendar_; }
    std::uint64_t seed() const { return seed_; }

    // Today's temperature in whole degrees Celsius, rolled each morning (weather stream).
    int temperature() const { return temperature_; }

    // Outside help for a need (US-155: a fire pit warms people, a bed rests them): it rises by `amount`, capped at the maximum. The
    // dead and unknown ids are ignored. Whole numbers, so the world stays deterministic.
    void satisfyPersonNeed(int personId, Need need, int amount);
    // The opposite, for a hazard (a cold wind, a bad meal): the need falls by `amount`, never below 0.
    void drainPersonNeed(int personId, Need need, int amount);

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
    // Someone falls sick for `days` days, by the chronicle entry `cause`.
    void sicken(int person, int days, int cause);
    // `giver` shares part of their food with hungry `receiver`. Returns the event's id.
    int share(int giver, int receiver);
    // `adopter` takes in the orphan `child`. Returns the event's id, or -1 if the child is no orphan.
    int adopt(int adopter, int child);
    // A child whose known parents are all gone (dead or driven out).
    bool isOrphan(const Person& person) const;
    // A death from outside the daily rules (an accident in a test, a story event). Returns the event's id.
    int kill(int person, CauseOfDeath cause, int causeEvent = -1);
    // What `who` thinks of `about`, -100..100.

    int opinion(int who, int about) const;

    // The hero of a run (M5) is one of the clan. The run layer (hero_life.h) changes what is theirs through these, so the world
    // keeps its own rules: a person by id to change (age, traits), a change of opinion, the store, and a line in the chronicle.
    Person* personMutable(int id) { return id >= 0 && static_cast<std::size_t>(id) < people_.size() ? &people_[static_cast<std::size_t>(id)] : nullptr; }
    void adjustOpinion(int who, int about, int delta);
    // The owner's setup of the world (US-206): what `who` thinks of `about` is set to `value` as written (not added), and a grudge with its reason: the chronicle writes
    // the reason as an entry the grudge points to. Returns that entry's id, or -1 when either person does not exist.
    void setOpinion(int who, int about, int value);
    int addSetupGrudge(int who, int about, int weight, const std::string& reason);
    // What a conversation leaves in someone's mind (US-164): `holder` remembers that `other` did something, with a feeling (-100..100). It is an ordinary
    // memory (a Gift when the feeling is good, a Quarrel when it is bad, major from 60 either way), so forgetting, gossip at half strength and the chronicle
    // treat it like any other, and it also keeps `text` (what happened, a short clause) for small talk. Returns false for people who do not exist.
    bool rememberConversation(int holder, int other, const std::string& text, int feeling);

    // Who has just talked with whom (US-165): `talk` leaves a note here for a game that wants to show it (speech bubbles). It is a queue for the
    // screen, not part of the world: it is not saved, not hashed and nothing in the simulation reads it. The game takes it empty each tick; if nobody
    // does, only the newest 32 are kept.
    struct TalkEvent {
        int speaker = -1;
        int listener = -1;
    };
    std::vector<TalkEvent> takeTalks() { return std::exchange(talks_, {}); }
    void adjustFood(int meals) { food_ = food_ + meals < 0 ? 0 : food_ + meals; }
    // Buildings in the clan's life (US-257): who sleeps under a roof, and how many meals a storage pit keeps at half the spoilage. The game sets both
    // every hour from its buildings; they are not saved.
    void setHoused(std::vector<int> people) { housed_ = std::move(people); }
    void setStorageMeals(int meals) { storageMeals_ = meals < 0 ? 0 : meals; }
    // Writes an entry in the chronicle now; returns its id.
    int note(const std::string& text, int importance, EventKind kind = EventKind::Note, int who = -1, int other = -1);

    // Life events (US-014), called by the daily rules; tests may call them too.
    // Two adults become partners. Any courtship behind it is a cause of the Pairing event, and
    // every other suitor of either is passed over (US-113). Returns the event's id.
    int pair(int a, int b);
    // Partners go their separate ways. The event says why: the heaviest grudge between them, or
    // that their love faded. Returns the event's id.
    int part(int a, int b);
    // A hunter kills a mammoth: a feast, and the clan's first is remembered. `causes` are earlier
    // events behind it (a hunting party); returns the event's id.
    int bringDownMammoth(int hunter, std::vector<int> causes = {}, bool withParty = false);
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
    std::string professionOf(const Person& person) const; // "hunter", "gatherer", or "" for a child (US-196)
    void routineOf(const Person& person, Situation& situation) const;
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
    void assignCarers();    // the sick and hurt find someone to nurse them
    void shareFood();       // in a famine the better fed feed the hungriest
    void adoptOrphans();    // children whose parents are gone are taken in
    void releaseCare(Person& person); // a death or exile ends any nursing they gave or needed
    void hurtOnHunt(Person& hunter);  // a small-game hunt can end in a wound
    int exile(Person& person, int victim, std::vector<int> causes, const std::string& text);
    void teaching();        // masters take apprentices; the apprentices learn and graduate (US-114)
    void releaseTeaching(Person& person);
    void huntMammoth(Person& sighter); // a party of 3 to 5 hunters, or the sighter alone (US-114)
    void courtship();       // every morning: partings, then suitors court, are turned down or win (US-113)
    void stopCourting(Person& person);
    void dropSuitors(int beloved); // nobody courts someone who has died or been driven out
    void turnDown(Person& suitor, int beloved, const std::string& text);
    void makeJealous(Person& rival, int winner, int beloved, int pairing);
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
    std::vector<int> housed_;  // person ids
    int storageMeals_ = 0;
    Chronicle chronicle_;
    std::vector<TalkEvent> talks_; // see takeTalks()
};

} // namespace odysseus::sim
