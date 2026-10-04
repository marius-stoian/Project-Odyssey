// The actions built into the game that interaction files name with `do` (US-152). Each one is what the old context menu ran
// for that item, moved here unchanged, so a menu made from data behaves exactly like the menu made from code.
#include "game/game_rules.h"
#include "game/odyssey_game.h"

#include "core/log.h"
#include "sim/dialogue_select.h"

#include <algorithm>
#include <format>

namespace odysseus::game {

namespace {

// The profession of heroData with this id ("hunter", "flint-knapper"), or -1.
int professionIndex(const OdysseyGame& game, const std::string& id) {
    if (const sim::HeroData* data = game.heroData()) {
        for (std::size_t p = 0; p < data->professions.size(); ++p) {
            if (data->professions[p].id == id) return static_cast<int>(p);
        }
    }
    return -1;
}

// "energy" to the need, case does not matter.
std::optional<sim::Need> needByName(std::string name) {
    for (char& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    for (std::size_t n = 0; n < sim::kNeedCount; ++n) {
        std::string candidate = sim::needName(static_cast<sim::Need>(n));
        for (char& c : candidate) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (candidate == name) return static_cast<sim::Need>(n);
    }
    return std::nullopt;
}

// Runs the built-in action `name` on `subject`. False when the game has no action of that name.
bool runBuiltin(OdysseyGame& game, const std::string& name, const std::vector<std::string>& args, const Subject& subject) {
    // Talking to a placed person needs no run of the hero (US-265): it opens the dialogue the person has for the player.
    if (name == "talk" && subject.kind == Subject::Kind::Npc) {
        if (!openConversation(game, subject)) game.run().setMessage(subject.name + " has nothing to say.");
        return true;
    }
    // A placed NPC with goods to trade opens the trade screen (US-283); the rival camps keep their own barter (`open-barter`).
    if (name == "open-trade" && subject.kind == Subject::Kind::Npc) {
        if (game.life() == nullptr) {
            game.run().setMessage("Start a run to trade.");
        } else if (!game.tradeMarket().isTrader(subject.index)) {
            game.run().setMessage(subject.name + " has nothing to trade.");
        } else {
            game.run().openBarter(NpcTrader{subject.index});
        }
        return true;
    }
    // A trader shows the rare goods it keeps for people it likes (US-282).
    if (name == "rare-goods" && subject.kind == Subject::Kind::Npc) {
        const sim::TradeMarket& market = game.tradeMarket();
        std::string shown;
        for (const std::string& item : market.offeredGoods(subject.index, game.npcPopulation().opinion(subject.index, sim::NpcPopulation::kHero), game.npcOpinions())) {
            if (!market.isRare(subject.index, item)) continue;
            const sim::Item* known = game.heroData() != nullptr ? game.heroData()->item(item) : nullptr;
            shown += (shown.empty() ? "" : ", ") + (known != nullptr ? known->name : item);
        }
        game.run().setMessage(shown.empty() ? subject.name + " has no rare goods now." : subject.name + " shows you rare goods: " + shown + ".");
        return true;
    }
    // The confrontations of NPCs (US-266) need no run either.
    if (const int placedId = placedIdOf(game, subject); placedId >= 0) {
        sim::NpcPopulation& people = game.npcPopulationMutable();
        if (name == "confront") {
            game.run().openConfront(game, subject);
            return true;
        }
        if (name == "actions") {
            game.run().openActions(game, subject);
            return true;
        }
        if (name == "spread-opinion" && args.size() == 1) {
            // Everyone who can hear it and knows the target now thinks of the hero too: persons within the hearing range of the data.
            const int amount = std::clamp(std::atoi(args[0].c_str()), -200, 200);
            const int range = game.npcOpinions().hearingTiles * kTileSize;
            int heard = 0;
            for (const int index : people.near(static_cast<int>(subject.x), static_cast<int>(subject.y), range)) {
                const int id = people.id(index);
                if (id == placedId || !people.knows(id, placedId)) continue;
                people.adjust(id, sim::NpcPopulation::kHero, amount);
                ++heard;
            }
            core::logInfo(std::format("{} heard it and think of the hero {:+}", heard, amount));
            return true;
        }
        if (name == "calm" && args.size() == 2) {
            // calm 15 70: a 70 in 100 chance (a roll of the dialogue stream) that it stops attacking and thinks 15 better of the hero.
            const int gain = std::clamp(std::atoi(args[0].c_str()), -200, 200);
            const int percent = std::clamp(std::atoi(args[1].c_str()), 0, 100);
            if (static_cast<int>(game.dialogueRandom().below(100)) < percent) {
                game.calmFight(placedId);
                people.adjust(placedId, sim::NpcPopulation::kHero, gain);
                game.run().setMessage(subject.name + " calms down.");
            } else {
                game.run().setMessage(subject.name + " will not listen.");
            }
            return true;
        }
        if (name == "provoke") {
            game.startFight(placedId);
            game.run().setMessage(subject.name + " attacks!");
            return true;
        }
    }
    sim::HeroLife* hero = game.life();
    if (hero == nullptr) return true; // no run is going on: nothing to act with
    const auto plant = static_cast<std::size_t>(subject.index);
    if (name == "gather-berries") {
        // The berries, the skill and the message; what happens to the plant is written in gather.json (it is picked, then ripens again).
        game.run().setMessage(hero->gatherBerries().message);
        game.tutorial().notify("gather");
    } else if (name == "knap") {
        game.run().setMessage(hero->knapFlint().message);
        game.harvestPlant(plant);
    } else if (name == "pick-flint") {
        game.run().setMessage(hero->pickFlint().message);
        game.harvestPlant(plant);
    } else if (name == "chop") {
        game.run().setMessage(hero->chopWood().message);
        game.harvestPlant(plant);
    } else if (name == "inspect") {
        game.run().setMessage(plant < game.plants().size() && game.plants()[plant].def != nullptr ? game.plants()[plant].def->inspect : std::string());
    } else if (name == "talk") {
        // A script that speaks for them opens the conversation panel (US-161); without one it is the plain talk of M5.
        if (!openConversation(game, subject)) game.run().setMessage(hero->talkTo(subject.index).message);
    } else if (name == "give-berries") {
        game.run().setMessage(hero->giveBerriesTo(subject.index).message);
    } else if (name == "ask-to-teach") {
        const int profession = args.empty() ? -1 : professionIndex(game, args[0]);
        if (profession < 0) return false;
        game.run().setMessage(hero->askToApprentice(profession).message);
    } else if (name == "open-craft") {
        if (args.empty()) return false;
        game.run().openCraft(args[0]);
    } else if (name == "eat-berries") {
        game.run().setMessage(hero->eatBerries().message);
        game.tutorial().notify("eat");
    } else if (name == "tend-camp-fire") {
        game.run().setMessage(hero->tendCampFire().message);
        game.tutorial().notify("tend");
    } else if (name == "tend-sacred-fire") {
        game.run().setMessage(hero->tendFire().message);
    } else if (name == "hold-ritual") {
        game.run().setMessage(hero->holdRitual(game.attendeesAt(subject.x, subject.y, game.heroData()->config.fire.ritualRadiusTiles)).message);
    } else if (name == "open-barter") {
        game.run().openBarter(subject.index);
    } else if (name == "graze" || name == "flee") {
        return false; // for animals and clan members, never the hero
    } else if (name == "restore") {
        // restore energy 30: the hero's need rises (a bed, a shelter).
        const std::optional<sim::Need> need = args.size() == 2 ? needByName(args[0]) : std::nullopt;
        if (!need) return false;
        game.helpPerson(hero->personId(), *need, std::atoi(args[1].c_str()));
        game.run().setMessage(std::format("You feel better: {} +{}.", args[0], args[1]));
    } else if (name == "warm-nearby") {
        // warm-nearby 6 25: everyone within 6 tiles of the thing, the hero too, gets 25 Warmth (a fire pit).
        if (args.size() != 2) return false;
        const double reach = std::atoi(args[0].c_str()) * static_cast<double>(kTileSize);
        const int amount = std::atoi(args[1].c_str());
        int warmed = 0;
        const auto& figures = game.clanView().figures();
        for (std::size_t i = 0; i < figures.size(); ++i) {
            if (figures[i].present && std::hypot(figures[i].x - subject.x, figures[i].y - subject.y) <= reach && game.helpPerson(static_cast<int>(i), sim::Need::Warmth, amount)) ++warmed;
        }
        if (std::hypot(game.hero().feetX() - subject.x, game.hero().feetY() - subject.y) <= reach && game.helpPerson(hero->personId(), sim::Need::Warmth, amount)) ++warmed;
        core::logInfo(std::format("The fire warms {} people", warmed));
        game.run().setMessage(std::format("The fire warms {} people.", warmed));
    } else {
        return false;
    }
    return true;
}

// The word an effect argument names: `flag met-elder` is the word met-elder, however the expression reader took it apart.
std::string effectWord(const sim::rules::Effect& effect, std::size_t index) {
    const sim::rules::Expr& e = *effect.args[index];
    return e.kind == sim::rules::Expr::Kind::Text || e.kind == sim::rules::Expr::Kind::Path ? e.text : effect.argSources[index];
}

// The game's side of the action runner (US-153): what `set target.state`, `do`, `say` and the rest mean in the world.
class GameEffectHost final : public sim::rules::EffectHost {
public:
    explicit GameEffectHost(OdysseyGame& game) : game_(game) {}

    void setState(const sim::rules::ThingRef& target, const std::string& state) override {
        if (static_cast<Subject::Kind>(target.kind) != Subject::Kind::Plant) return; // only plants have states so far
        const int index = game_.plantIndexById(target.id);
        if (index >= 0) game_.setPlantState(static_cast<std::size_t>(index), state);
    }

    void apply(const sim::rules::Effect& effect, int actor, const sim::rules::ThingRef& target) override {
        const std::optional<Subject> subject = subjectFor(game_, target);
        if (!subject) return; // the thing is gone: nothing to act on
        if (actor != kHeroActor) {
            applyForNpc(effect, actor, *subject);
            return;
        }
        if (effect.verb == "do") {
            std::vector<std::string> args;
            for (std::size_t i = 1; i < effect.args.size(); ++i) {
                args.push_back(effect.args[i]->kind == sim::rules::Expr::Kind::Text || effect.args[i]->kind == sim::rules::Expr::Kind::Path ? effect.args[i]->text : effect.argSources[i]);
            }
            const std::string name = effect.args.empty() ? std::string() : effect.args[0]->text;
            if (!runBuiltin(game_, name, args, *subject)) core::logWarning(std::format("Interactions: the built-in action \"{}\" cannot be carried out here", name));
        } else if (effect.verb == "say") {
            const GameRuleContext context(game_, *subject);
            game_.run().setMessage(sim::rules::fillTokens(effect.args.empty() ? std::string() : effect.args[0]->text, context));
        } else if (effect.verb == "fx") {
            // fx flame: a visual effect of effects.json, played once over the thing.
            if (!effect.args.empty()) game_.playEffect(effect.args[0]->text, subject->x, subject->y - 16.0, 48);
        } else if (effect.verb == "opinion" && effect.args.size() == 3 && placedIdOf(game_, *subject) >= 0) {
            // The same for a placed person or creature (US-265, US-266): only what they think of the hero is kept.
            const GameRuleContext context(game_, *subject);
            const bool npc = effect.args[0]->text == "npc" || effect.args[0]->text == "target";
            const bool hero = effect.args[1]->text == "hero" || effect.args[1]->text == "actor";
            const long long delta = sim::rules::evaluate(*effect.args[2], context).number;
            if (npc && hero) game_.npcPopulationMutable().adjust(placedIdOf(game_, *subject), sim::NpcPopulation::kHero, static_cast<int>(std::clamp(delta, -200LL, 200LL)));
        } else if (effect.verb == "remember" && effect.args.size() >= 2 && subject->kind == Subject::Kind::Npc) {
            // remember npc "{hero} brought flint" 20: the placed person keeps the memory (US-265).
            const GameRuleContext context(game_, *subject);
            const long long feeling = effect.args.size() == 3 ? sim::rules::evaluate(*effect.args[2], context).number : 10;
            const int index = game_.npcPopulation().indexOf(subject->index);
            if (index >= 0 && (effect.args[0]->text == "npc" || effect.args[0]->text == "target")) {
                game_.npcPopulationMutable().remember(index, sim::rules::fillDialogueTokens(effect.args[1]->text, context), static_cast<int>(std::clamp(feeling, -100LL, 100LL)));
            }
        } else if (effect.verb == "opinion" && effect.args.size() == 3) {
            // opinion npc hero 5: what the first thinks of the second changes (a conversation's choice, US-161).
            const GameRuleContext context(game_, *subject);
            const int who = personNamed(game_, *subject, effect.args[0]->text);
            const int about = personNamed(game_, *subject, effect.args[1]->text);
            const long long delta = sim::rules::evaluate(*effect.args[2], context).number;
            game_.changeOpinion(who, about, static_cast<int>(std::clamp(delta, -200LL, 200LL)));
        } else if (effect.verb == "flag" && !effect.args.empty()) {
            // flag met-elder, flag trust 3: a story note (US-164). 1 unless a value is given.
            const GameRuleContext context(game_, *subject);
            const std::string name = effectWord(effect, 0);
            const long long value = effect.args.size() == 2 ? sim::rules::evaluate(*effect.args[1], context).number : 1;
            game_.flags().set(name, static_cast<int>(std::clamp(value, -1000000LL, 1000000LL)));
        } else if (effect.verb == "remember" && effect.args.size() >= 2) {
            // remember npc "{hero} shared berries" 20: what happened, as a short clause, and how it feels (default 10, -100..100) (US-164).
            const GameRuleContext context(game_, *subject);
            const int holder = personNamed(game_, *subject, effect.args[0]->text);
            const int other = personNamed(game_, *subject, holder == personNamed(game_, *subject, "hero") ? "npc" : "hero");
            const long long feeling = effect.args.size() == 3 ? sim::rules::evaluate(*effect.args[2], context).number : 10;
            game_.rememberConversation(holder, other, sim::rules::fillDialogueTokens(effect.args[1]->text, context), static_cast<int>(std::clamp(feeling, -100LL, 100LL)));
        } else if (effect.verb == "chronicle" && !effect.args.empty()) {
            // chronicle "{hero} promised {npc} a hunt": a line in the clan's chronicle (US-164).
            const GameRuleContext context(game_, *subject);
            game_.chronicleLine(sim::rules::fillDialogueTokens(effect.args[0]->text, context), personNamed(game_, *subject, "hero"), personNamed(game_, *subject, "npc"));
        } else if (effect.verb == "give" || effect.verb == "take") {
            // give actor berries 2: the hero's bag, the only one with items so far.
            if (effect.args.size() == 3 && game_.life() != nullptr && effect.args[0]->text != "npc" && effect.args[0]->text != "target") {
                const int n = effect.args[2]->kind == sim::rules::Expr::Kind::Number ? static_cast<int>(effect.args[2]->number) : 1;
                if (effect.verb == "give") game_.life()->give(effect.args[1]->text, n);
                else game_.life()->take(effect.args[1]->text, n);
            }
        } else {
            core::logInfo(std::format("Interactions: \"{}\" is not carried out yet", effect.source));
        }
    }

    // What an effect means when a clan member or an animal does it (US-154): the few built-ins they use, and a visual effect.
    void applyForNpc(const sim::rules::Effect& effect, int actor, const Subject& subject) {
        const ActorRef who = actorOfRunnerId(actor);
        if (effect.verb == "fx") {
            if (!effect.args.empty()) game_.playEffect(effect.args[0]->text, subject.x, subject.y - 16.0, 48);
            return;
        }
        if (effect.verb != "do" || effect.args.empty()) return;
        const std::string& name = effect.args[0]->text;
        if (name == "gather-berries" && who.kind == ActorRef::Kind::Person) {
            game_.helpPerson(who.index, sim::Need::Hunger, 30); // they eat a little of what they pick
        } else if (name == "flee") {
            game_.npcs().fleeFrom(game_, actor, subject);
        }
        // graze: the animal stays at the grass for the length of the action; nothing else happens
    }

    int ticksPerDay() const override { return game_.clan() != nullptr ? game_.clan()->calendar().ticksPerDay() : 24 * 20 * 60; }

private:
    OdysseyGame& game_;
};

} // namespace

const std::vector<std::string>& builtInActionNames() {
    static const std::vector<std::string> names = {"gather-berries", "knap", "pick-flint", "chop", "inspect", "talk", "confront", "actions", "spread-opinion", "calm", "provoke", "give-berries", "ask-to-teach",
                                                   "open-craft", "eat-berries", "tend-camp-fire", "tend-sacred-fire", "hold-ritual", "open-barter", "open-trade", "rare-goods", "walk-to", "restore", "warm-nearby", "graze", "flee"};
    return names;
}

bool startInteraction(OdysseyGame& game, const std::string& interactionId, const Subject& subject) {
    const sim::rules::Interaction* interaction = game.interactions().find(interactionId);
    if (interaction == nullptr) return false;
    GameEffectHost host(game);
    game.actions().cancel(0); // a new action replaces the one the hero was doing
    game.actions().start(*interaction, 0, refOf(game, subject), game.actionClock(), host);
    return true;
}

bool startInteractionFor(OdysseyGame& game, int actor, const std::string& interactionId, const sim::rules::ThingRef& target) {
    const sim::rules::Interaction* interaction = game.interactions().find(interactionId);
    if (interaction == nullptr) return false;
    GameEffectHost host(game);
    return game.actions().start(*interaction, actor, target, game.actionClock(), host);
}

void tickInteractions(OdysseyGame& game) {
    GameEffectHost host(game);
    game.actions().tick(game.actionClock(), game.interactions(), host);
}

bool openConversation(OdysseyGame& game, const Subject& subject) {
    if (subject.kind == Subject::Kind::Npc) {
        const sim::rules::DlgScript* script = game.npcDialogueFor(subject.index);
        if (script == nullptr) return false;
        game.run().openTalk(sim::rules::Conversation(*script, kHeroActor, refOf(game, subject)), subject);
        game.run().setMessage({});
        return true;
    }
    if (subject.kind != Subject::Kind::Person || game.clan() == nullptr) return false;
    const GameRuleContext context(game, subject);
    sim::rules::WhoFacts who;
    who.name = subject.name;
    who.roles = sim::rules::rolesOf(*game.clan(), subject.index);
    const sim::rules::DlgScript* script = sim::rules::selectScript(game.dialogues(), who, context, game.dialogueRandom());
    const int person = subject.index;
    const int hero = game.life() != nullptr ? game.life()->personId() : -1;
    // `{smalltalk.topic}` in a script is a line made up from what this person remembers, heard and needs.
    const auto smalltalkSource = [&game, person, hero](const std::string& topic) {
        const auto said = game.smalltalk().say(*game.clan(), person, hero, game.dialogueRandom(), topic);
        return said ? said->text : std::string();
    };
    if (script != nullptr) {
        sim::rules::Conversation conversation(*script, kHeroActor, refOf(game, subject));
        conversation.setSmalltalk(smalltalkSource);
        game.run().openTalk(std::move(conversation), subject);
        return true;
    }
    // No script fits them: small talk, their line and a friendly answer and a rude one. The old talk still warms the two to each other.
    if (!game.smalltalk().ready() || hero < 0) return false;
    const auto said = game.smalltalk().say(*game.clan(), person, hero, game.dialogueRandom());
    if (!said) return false;
    if (game.life() != nullptr) game.life()->talkTo(person);
    game.run().openTalk(sim::rules::Conversation(sim::rules::smalltalkScript(subject.name, said->text), kHeroActor, refOf(game, subject)), subject);
    game.run().setMessage({});
    return true;
}

bool chooseConversationOption(OdysseyGame& game, sim::rules::Conversation& conversation, const Subject& subject, int index) {
    GameEffectHost host(game);
    const GameRuleContext context(game, subject);
    return conversation.choose(index, context, game.actions(), game.actionClock(), host);
}

} // namespace odysseus::game
