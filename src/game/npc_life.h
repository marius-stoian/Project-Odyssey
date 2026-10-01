#pragma once

#include "boundary.h"

#include "core/random.h"
#include "sim/action_runner.h"
#include "sim/npc_chooser.h"

#include <cstdint>
#include <map>
#include <string>
#include <utility>

namespace odysseus::game {

class OdysseyGame;
struct Subject;

// What clan members and animals do on their own (US-154, INT-05). They use the very same interactions as the hero and the very same
// action runner: an idle one looks at the things near them, scores every interaction it may do with the `npc.score` of its file,
// picks the best, walks there and does it. So a hungry person walks to a ripe bush and gathers it, a deer grazes on grass and runs
// from a wolf, and the player sees it all happen. Everything is whole numbers and ordered containers, and the only randomness is one
// seeded stream used to break ties, so the same world always lives the same way.
//
// D-36: an action under way is dropped for danger (a hostile within 6 m) and otherwise finished.
class NpcLife {
public:
    enum class State { Idle, Walking, Working, Fleeing };

    struct Mind {
        State state = State::Idle;
        std::string interaction;           // what they are walking to do, or doing
        sim::rules::ThingRef target;       // and to what
        double goalX = 0.0;                // where they are walking to (world pixels)
        double goalY = 0.0;
        std::int64_t thinkAt = 0;          // the next tick they look around
    };

    static constexpr int kThinkTicks = 20;        // an idle actor looks around once a second
    static constexpr int kMinScore = 30;          // a score below this is not worth getting up for
    static constexpr double kLookMetres = 12.0;   // how far they look
    static constexpr double kDangerMetres = 6.0;  // a hostile this close drops what they are doing
    static constexpr double kWalkPixelsPerTick = 2.0;
    static constexpr double kFleePixelsPerTick = 4.0;
    static constexpr double kFleeMetres = 8.0;    // how far a frightened animal runs

    NpcLife() : random_(0x4E50ULL, 7ULL) {}

    void reset();
    // One play tick, after the clan's own tick and the runner's.
    void tick(OdysseyGame& game);
    // `do flee`: the actor runs from the threat (a place about 8 m away from it, inside the map).
    void fleeFrom(OdysseyGame& game, int runnerId, const Subject& threat);

    const Mind* mind(int runnerId) const;
    const std::map<int, Mind>& minds() const { return minds_; }
    std::uint64_t hash() const;

private:
    void actPerson(OdysseyGame& game, int person);
    void actAnimal(OdysseyGame& game, std::size_t bystanderIndex);
    // Shared by both: the state machine of one actor at (x, y). `moveToward` is how this kind of actor walks.
    void runMind(OdysseyGame& game, int runnerId, double x, double y);
    void think(OdysseyGame& game, int runnerId, double x, double y);
    bool dangerNear(const OdysseyGame& game, int runnerId, double x, double y) const;
    void startWork(OdysseyGame& game, int runnerId, Mind& mind);

    std::map<int, Mind> minds_; // by the actor's number in the runner
    std::map<int, std::pair<double, double>> animalFeet_; // exact feet of the walking animals, by id in the level
    sim::rules::CooldownTable cooldowns_;
    core::Pcg32 random_;
};

} // namespace odysseus::game
