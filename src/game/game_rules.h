#pragma once

#include "boundary.h"

#include "sim/action_runner.h"
#include "sim/conversation.h"
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
    enum class Kind { Plant, Person, KnappingStone, CampFire, SacredFire, RivalCamp, Animal, Hero };

    Kind kind = Kind::Plant;
    int index = -1;               // the plant, the clan member, the rival camp or the animal; -1 for the one-of-a-kind things
    std::string title;            // the menu's heading: "Tok", "The clan's fire"
    std::string name;             // what {target.name} says
    double x = 0.0;               // where the hero measures the distance to, world pixels
    double y = 0.0;
    sim::rules::ThingInfo info;   // kind and tags
};

// Who acts (US-154): the hero, a clan member, or an animal. The action runner knows them by one number: 0 for the hero, 1000 + the
// person's id for a clan member, 100000 + the character's id for an animal.
inline constexpr int kHeroActor = 0;
inline constexpr int kPersonActorBase = 1000;
inline constexpr int kAnimalActorBase = 100000;

struct ActorRef {
    enum class Kind { Hero, Person, Animal };
    Kind kind = Kind::Hero;
    int index = -1; // the person id, or the animal's id in the level
};
ActorRef actorOfRunnerId(int runnerId);
int runnerIdOf(const ActorRef& actor);
sim::rules::ThingInfo actorInfo(const OdysseyGame& game, const ActorRef& actor);
// Where the actor's feet are, world pixels; false when they are gone.
bool actorPosition(const OdysseyGame& game, const ActorRef& actor, double& x, double& y);

// The thing under a world point, found in the order the old menu looked: a clan member, the knapping stone, the clan's fire, the
// sacred fire, a rival camp, a plant. Nothing there: empty.
std::optional<Subject> subjectAt(const OdysseyGame& game, double worldX, double worldY);
Subject plantSubject(const OdysseyGame& game, std::size_t plantIndex);
Subject animalSubject(const OdysseyGame& game, std::size_t enemyIndex); // a placed animal or character (tags: animal, prey or hostile)
Subject heroSubject(const OdysseyGame& game);                           // tags: hero, person, and armed and/or moving (D-36: prey flee those)

// A Subject as a stable name the runner can keep (a plant by its id, a clan member by their index) and back (US-153). Empty when the
// thing is gone (a plant not in the level any more, a rival camp that moved away).
sim::rules::ThingRef refOf(const OdysseyGame& game, const Subject& subject);
std::optional<Subject> subjectFor(const OdysseyGame& game, const sim::rules::ThingRef& ref);

// The tags the game gives its own things (people, fires, the stone, camps), for the unknown-tag check of the interaction files.
std::vector<std::string> builtInThingTags(const OdysseyGame& game);

// The real world as the rule language sees it (US-150, US-151): what `target.state`, `season`, `has(berries, 2)` mean in the game.
// It is made for one moment and one target, reads the game and changes nothing.
// Answered so far: actor.name, target.name, target.kind, target.state, target.inspect, season, time, distance, has(...), tag(...) and
// flag(sacred-fire), and opinion(a, b) and mood(who) between the hero and a clan member (US-161). skill, kin and other flags answer 0 until the stories that
// give them a meaning (US-164).
class GameRuleContext : public sim::rules::RuleContext {
public:
    // `actor` is who acts (the hero unless said): it decides what actor.name, distance, need(...) and trait(...) mean.
    GameRuleContext(const OdysseyGame& game, const Subject& subject, ActorRef actor = {});

    sim::rules::Value path(const std::string& dotted) const override;
    sim::rules::Value call(const std::string& name, const std::vector<sim::rules::Value>& args) const override;

private:
    const OdysseyGame& game_;
    Subject subject_;
    ActorRef actor_;
};

// Which clan member a word of the rule language means in a conversation: npc or target is the person talked to, hero or actor is the hero.
// -1 for anything else (or when the thing is not a person).
int personNamed(const OdysseyGame& game, const Subject& subject, const std::string& word);

// The part of the day as the files name it: morning (6-11), afternoon (12-17), evening (18-21), night (22-5).
std::string timeOfDayWord(int hour);

// The actions built into the game that interaction files may name with `do` (US-152).
const std::vector<std::string>& builtInActionNames();

// The hero starts an interaction on `subject` (US-153). An instant one does its effects now; a timed one runs for its duration (a ring fills
// over the target) and does its effects only if the hero is not interrupted. False when the interaction no longer exists.
bool startInteraction(OdysseyGame& game, const std::string& interactionId, const Subject& subject);
// The same for any actor (a clan member, an animal) on a thing named by a ref, without stopping what they were doing (US-154).
bool startInteractionFor(OdysseyGame& game, int actor, const std::string& interactionId, const sim::rules::ThingRef& target);

// One tick of the runner: effects waiting for their time happen, finished actions do their effects. Called by the game each play tick.
void tickInteractions(OdysseyGame& game);

// Talk (US-161): opens the conversation panel with the script that speaks for this clan member and pauses the world. False when no
// script fits them (then the caller keeps to the plain talk).
bool openConversation(OdysseyGame& game, const Subject& subject);
// The player picks choice `index` (0-based, as the panel lists them): its effects happen in the world, then the talk moves on or ends.
bool chooseConversationOption(OdysseyGame& game, sim::rules::Conversation& conversation, const Subject& subject, int index);

} // namespace odysseus::game
