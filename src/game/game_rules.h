#pragma once

#include "boundary.h"

#include "sim/action_runner.h"
#include "sim/interaction.h"
#include "sim/rule_expr.h"

#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

class OdysseyGame;

// What the hero is acting on (US-151, US-152): one thing in the world as the rules see it. Things are named by an index into the
// game's own lists (a plant, a clan member, a rival camp), never by pointer, so a Subject stays valid while the menu is open.
struct Subject {
    enum class Kind { Plant, Person, KnappingStone, CampFire, SacredFire, RivalCamp };

    Kind kind = Kind::Plant;
    int index = -1;               // the plant, the clan member or the rival camp; -1 for the one-of-a-kind things
    std::string title;            // the menu's heading: "Tok", "The clan's fire"
    std::string name;             // what {target.name} says
    double x = 0.0;               // where the hero measures the distance to, world pixels
    double y = 0.0;
    sim::rules::ThingInfo info;   // kind and tags
};

// The thing under a world point, found in the order the old menu looked: a clan member, the knapping stone, the clan's fire, the
// sacred fire, a rival camp, a plant. Nothing there: empty.
std::optional<Subject> subjectAt(const OdysseyGame& game, double worldX, double worldY);
Subject plantSubject(const OdysseyGame& game, std::size_t plantIndex);

// A Subject as a stable name the runner can keep (a plant by its id, a clan member by their index) and back (US-153). Empty when the
// thing is gone (a plant not in the level any more, a rival camp that moved away).
sim::rules::ThingRef refOf(const OdysseyGame& game, const Subject& subject);
std::optional<Subject> subjectFor(const OdysseyGame& game, const sim::rules::ThingRef& ref);

// The tags the game gives its own things (people, fires, the stone, camps), for the unknown-tag check of the interaction files.
std::vector<std::string> builtInThingTags(const OdysseyGame& game);

// The real world as the rule language sees it (US-150, US-151): what `target.state`, `season`, `has(berries, 2)` mean in the game.
// It is made for one moment and one target, reads the game and changes nothing.
// Answered so far: actor.name, target.name, target.kind, target.state, target.inspect, season, time, distance, has(...), tag(...) and
// flag(sacred-fire). need, skill, trait, opinion, kin and other flags answer 0 until the stories that give them a meaning (US-154, US-164).
class GameRuleContext : public sim::rules::RuleContext {
public:
    GameRuleContext(const OdysseyGame& game, const Subject& subject);

    sim::rules::Value path(const std::string& dotted) const override;
    sim::rules::Value call(const std::string& name, const std::vector<sim::rules::Value>& args) const override;

private:
    const OdysseyGame& game_;
    Subject subject_;
};

// The part of the day as the files name it: morning (6-11), afternoon (12-17), evening (18-21), night (22-5).
std::string timeOfDayWord(int hour);

// The actions built into the game that interaction files may name with `do` (US-152).
const std::vector<std::string>& builtInActionNames();

// The hero starts an interaction on `subject` (US-153). An instant one does its effects now; a timed one runs for its duration (a ring fills
// over the target) and does its effects only if the hero is not interrupted. False when the interaction no longer exists.
bool startInteraction(OdysseyGame& game, const std::string& interactionId, const Subject& subject);

// One tick of the runner: effects waiting for their time happen, finished actions do their effects. Called by the game each play tick.
void tickInteractions(OdysseyGame& game);

} // namespace odysseus::game
