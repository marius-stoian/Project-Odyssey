// The actions built into the game that interaction files name with `do` (US-152). Each one is what the old context menu ran
// for that item, moved here unchanged, so a menu made from data behaves exactly like the menu made from code.
#include "game/game_rules.h"
#include "game/odyssey_game.h"

#include "core/log.h"

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
        game.run().setMessage(hero->talkTo(subject.index).message);
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

// The game's side of the action runner (US-153): what `set target.state`, `do`, `say` and the rest mean in the world.
class GameEffectHost final : public sim::rules::EffectHost {
public:
    explicit GameEffectHost(OdysseyGame& game) : game_(game) {}

    void setState(const sim::rules::ThingRef& target, const std::string& state) override {
        if (static_cast<Subject::Kind>(target.kind) != Subject::Kind::Plant) return; // only plants have states so far
        const int index = game_.plantIndexById(target.id);
        if (index >= 0) game_.setPlantState(static_cast<std::size_t>(index), state);
    }

    void apply(const sim::rules::Effect& effect, int /*actor*/, const sim::rules::ThingRef& target) override {
        const std::optional<Subject> subject = subjectFor(game_, target);
        if (!subject) return; // the thing is gone: nothing to act on
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

    int ticksPerDay() const override { return game_.clan() != nullptr ? game_.clan()->calendar().ticksPerDay() : 24 * 20 * 60; }

private:
    OdysseyGame& game_;
};

} // namespace

const std::vector<std::string>& builtInActionNames() {
    static const std::vector<std::string> names = {"gather-berries", "knap", "pick-flint", "chop", "inspect", "talk", "give-berries", "ask-to-teach",
                                                   "open-craft", "eat-berries", "tend-camp-fire", "tend-sacred-fire", "hold-ritual", "open-barter", "restore", "warm-nearby"};
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

void tickInteractions(OdysseyGame& game) {
    GameEffectHost host(game);
    game.actions().tick(game.actionClock(), game.interactions(), host);
}

} // namespace odysseus::game
