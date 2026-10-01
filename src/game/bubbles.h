#pragma once

#include "boundary.h"

#include <string>
#include <vector>

namespace odysseus::game {

class OdysseyGame;

// Speech bubbles with words over clan members' heads (US-162 greetings; US-165 uses them for NPCs talking to each other). One bubble per
// person at a time: a new one replaces the old. Bubbles only count down while the world runs, so an open panel freezes them.
struct Bubble {
    int person = -1;       // the clan member's id
    std::string text;
    int ticksLeft = 0;
};

class Bubbles {
public:
    void say(int person, std::string text, int ticks);
    void remove(int person) { std::erase_if(bubbles_, [person](const Bubble& b) { return b.person == person; }); }
    void tick(); // one play tick: every bubble has a tick less, and one that is out of time goes
    const Bubble* of(int person) const;
    const std::vector<Bubble>& all() const { return bubbles_; }
    void clear() { bubbles_.clear(); }

private:
    std::vector<Bubble> bubbles_; // in the order they were made
};

inline constexpr int kGreetingRangeMetres = 3;      // the hero passes within this of a friendly NPC (design section 5)
inline constexpr int kGreetingSeconds = 3;          // how long a greeting stays over the head
inline constexpr int kGreetingCooldownSeconds = 60; // at most one greeting a minute from the same NPC
inline constexpr int kGreetingBubblesAtOnce = 2;    // a whole camp saying hello at once is unreadable: the nearest speak first, the rest in turn

// Clan members talking to each other (US-165). When the social simulation makes two people near the hero talk, quarrel, court, share or give, the
// game shows it as a short exchange of bubbles: the first speaker's line, then the other's, each for 3 seconds and each gone when the next begins. What
// they say comes from a `@pair` script that fits them, else a short line from `social.<kind>` in smalltalk.json. The outcome (opinions, memories, the
// chronicle) is the simulation's own; the bubbles only show it. One exchange plays at a time; a few wait their turn.
inline constexpr int kExchangeRangeMetres = 12;  // both people must be this close to the hero
inline constexpr int kExchangeLineSeconds = 3;   // each line stays this long
inline constexpr std::size_t kExchangesWaiting = 3;

struct ExchangeLine {
    int person = -1;
    std::string text;
};

class Exchanges {
public:
    // A new run or a load: what has already happened (the chronicle so far) is not shown again, and nothing waits.
    void reset(std::size_t chronicleEntries);
    // Every play tick: new events are queued, the exchange under way moves on.
    void update(OdysseyGame& game);
    bool playing() const { return !current_.empty(); }
    const std::vector<ExchangeLine>& current() const { return current_; }
    std::size_t waiting() const { return waiting_.size(); }

private:
    void startLine(OdysseyGame& game);
    std::size_t seenEntries_ = 0;
    std::vector<std::vector<ExchangeLine>> waiting_;
    std::vector<ExchangeLine> current_;
    std::size_t index_ = 0;
    int ticksLeft_ = 0;
};

// Once per play tick: a clan member the hero passes within 3 m, who thinks well of them (opinion 0 or more) and has not greeted them in
// the last minute, says one of the greetings that fit them (the scripts marked `@bark`) in a bubble. Without a fitting script nothing happens.
// No more than two bubbles are over heads at once: when more people qualify the nearest greet first and the others when a bubble has gone
// (a person who has not spoken yet has not used their minute).
void updateGreetings(OdysseyGame& game);

} // namespace odysseus::game
