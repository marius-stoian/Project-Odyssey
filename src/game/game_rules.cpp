#include "game/game_rules.h"

#include "game/odyssey_game.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace odysseus::game {

using sim::rules::Value;

std::string timeOfDayWord(int hour) {
    if (hour >= 22 || hour < 6) return "night";
    if (hour < 12) return "morning";
    if (hour < 18) return "afternoon";
    return "evening";
}

GameRuleContext::GameRuleContext(const OdysseyGame& game, int plantIndex) : game_(game), plantIndex_(plantIndex) {}

namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

} // namespace

Value GameRuleContext::path(const std::string& dotted) const {
    const WorldPlant* plant = (plantIndex_ >= 0 && static_cast<std::size_t>(plantIndex_) < game_.plants().size()) ? &game_.plants()[static_cast<std::size_t>(plantIndex_)] : nullptr;
    if (dotted == "actor" || dotted == "target" || dotted == "npc" || dotted == "hero") return Value::ofText(dotted); // a name for the call functions to look up
    if (dotted == "actor.name" || dotted == "hero.name") return Value::ofText(game_.life() != nullptr ? game_.life()->name() : std::string("Hero"));
    if (dotted == "actor.kind" || dotted == "hero.kind") return Value::ofText("hero");
    if (plant != nullptr) {
        if (dotted == "target.name" || dotted == "target.kind") return Value::ofText(plant->kind);
        if (dotted == "target.state") return Value::ofText(plant->state);
        if (dotted == "target.inspect") return Value::ofText(plant->def != nullptr ? plant->def->inspect : std::string());
    }
    if (dotted == "season") {
        return Value::ofText(game_.clan() != nullptr ? lower(sim::seasonName(game_.clan()->date().season)) : std::string("summer"));
    }
    if (dotted == "time") {
        int hour = 12;
        if (const sim::World* world = game_.clan()) {
            hour = static_cast<int>((world->ticks() % static_cast<std::uint64_t>(world->calendar().ticksPerDay())) / static_cast<std::uint64_t>(world->calendar().ticksPerHour()));
        }
        return Value::ofText(timeOfDayWord(hour));
    }
    if (dotted == "distance") {
        if (plant == nullptr) return Value::ofNumber(0);
        const double pixels = std::hypot(plant->feet.x - game_.hero().feetX(), plant->feet.y - game_.hero().feetY());
        return Value::ofNumber(static_cast<long long>(pixels / kTileSize)); // a tile is a metre
    }
    return Value::ofNumber(0);
}

Value GameRuleContext::call(const std::string& name, const std::vector<Value>& args) const {
    if (name == "has") {
        // has(item, n): the hero; has(who, item, n): `who` is the hero here (the only one with a bag so far).
        const std::size_t base = args.size() == 3 ? 1 : 0;
        if (game_.life() == nullptr || args.size() < base + 2 || !args[base].isText) return Value::ofNumber(0);
        if (base == 1 && args[0].isText && args[0].text != "hero" && args[0].text != "actor") return Value::ofNumber(0);
        return Value::ofNumber(game_.life()->count(args[base].text) >= args[base + 1].number ? 1 : 0);
    }
    if (name == "tag" && args.size() == 2 && args[0].isText && args[1].isText) {
        const WorldPlant* plant = (plantIndex_ >= 0 && static_cast<std::size_t>(plantIndex_) < game_.plants().size()) ? &game_.plants()[static_cast<std::size_t>(plantIndex_)] : nullptr;
        if (args[0].text == "target" && plant != nullptr && plant->def != nullptr) {
            return Value::ofNumber(std::find(plant->def->tags.begin(), plant->def->tags.end(), args[1].text) != plant->def->tags.end() ? 1 : 0);
        }
        if (args[0].text == "hero" || args[0].text == "actor") return Value::ofNumber(args[1].text == "hero" || args[1].text == "person" ? 1 : 0);
    }
    return Value::ofNumber(0);
}

} // namespace odysseus::game
