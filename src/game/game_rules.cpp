#include "game/game_rules.h"

#include "game/odyssey_game.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>

namespace odysseus::game {

using sim::rules::Value;

std::string timeOfDayWord(int hour) {
    if (hour >= 22 || hour < 6) return "night";
    if (hour < 12) return "morning";
    if (hour < 18) return "afternoon";
    return "evening";
}

namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

bool hasTag(const std::vector<std::string>& tags, const std::string& tag) { return std::find(tags.begin(), tags.end(), tag) != tags.end(); }

} // namespace

// ---- what the hero can act on

Subject plantSubject(const OdysseyGame& game, std::size_t plantIndex) {
    const WorldPlant& plant = game.plants().at(plantIndex);
    Subject subject;
    subject.kind = Subject::Kind::Plant;
    subject.index = static_cast<int>(plantIndex);
    subject.title = plant.kind;
    subject.name = plant.kind;
    subject.x = plant.feet.x;
    subject.y = plant.feet.y;
    subject.info = {plant.kind, plant.def != nullptr ? plant.def->tags : std::vector<std::string>{}};
    return subject;
}

std::optional<Subject> subjectAt(const OdysseyGame& game, double wx, double wy) {
    const sim::HeroLife* hero = game.life();
    const sim::HeroData* data = game.heroData();
    // A clan member. The tags say what they can do for the hero now: `teaches-hunter` while they are the hero's master of that
    // profession and have no apprentice yet.
    if (const int person = game.personAtWorld(wx, wy); person >= 0 && game.clan() != nullptr) {
        const Figure& figure = game.clanView().figures()[static_cast<std::size_t>(person)];
        Subject subject;
        subject.kind = Subject::Kind::Person;
        subject.index = person;
        subject.name = game.clan()->people()[static_cast<std::size_t>(person)].name;
        subject.title = subject.name;
        subject.x = figure.x;
        subject.y = figure.y;
        subject.info = {"person", {"person", "clan"}};
        if (hero != nullptr && data != nullptr) {
            for (int p = 0; p < static_cast<int>(sim::kProfessionCount); ++p) {
                if (hero->masterOf(p) == person && hero->apprenticeOf(p) < 0) subject.info.tags.push_back("teaches-" + data->professions[static_cast<std::size_t>(p)].id);
            }
        }
        return subject;
    }
    // A workstation: the knapping stone, or the camp's fire.
    const PixelPoint stone = game.knappingStone();
    if (std::abs(wx - stone.x) < 20 && wy > stone.y - 26 && wy < stone.y + 6) {
        return Subject{Subject::Kind::KnappingStone, -1, "Knapping stone", "Knapping stone", static_cast<double>(stone.x), static_cast<double>(stone.y), {"knapping-stone", {"workstation", "knapping-stone"}}};
    }
    const PixelPoint camp = game.campPixels();
    if (std::hypot(wx - camp.x, wy - (camp.y - 12)) < 22) {
        return Subject{Subject::Kind::CampFire, -1, "The clan's fire", "The clan's fire", static_cast<double>(camp.x), static_cast<double>(camp.y), {"camp-fire", {"fire", "workstation", "camp-fire"}}};
    }
    // The sacred fire.
    if (hero != nullptr && hero->fire().founded) {
        const double fx = hero->fire().tileX * kTileSize + 16;
        const double fy = hero->fire().tileY * kTileSize + 16;
        if (std::hypot(wx - fx, wy - fy) < 24) {
            const std::string title = std::format("Sacred fire {}", hero->fire().name);
            return Subject{Subject::Kind::SacredFire, -1, title, title, fx, fy, {"sacred-fire", {"fire", "sacred-fire"}}};
        }
    }
    // A rival camp.
    if (game.rivals() != nullptr) {
        for (std::size_t i = 0; i < game.rivals()->clans().size(); ++i) {
            const sim::Tile tile = game.rivals()->clans()[i].camp;
            const double cx = tile.x * kTileSize + 16;
            const double cy = tile.y * kTileSize + 16;
            if (std::hypot(wx - cx, wy - (cy - 12)) < 26) {
                const std::string& name = game.rivals()->clans()[i].name;
                return Subject{Subject::Kind::RivalCamp, static_cast<int>(i), name, name, cx, cy, {"rival-camp", {"camp", "rival"}}};
            }
        }
    }
    // A plant: berries, flint, a tree.
    if (const int plantIndex = game.plantAtWorld(wx, wy); plantIndex >= 0) return plantSubject(game, static_cast<std::size_t>(plantIndex));
    return std::nullopt;
}

std::vector<std::string> builtInThingTags(const OdysseyGame& game) {
    std::vector<std::string> tags = {"person", "clan", "workstation", "knapping-stone", "camp-fire", "fire", "sacred-fire", "camp", "rival", "hero"};
    if (const sim::HeroData* data = game.heroData()) {
        for (const auto& profession : data->professions) tags.push_back("teaches-" + profession.id);
    }
    return tags;
}

// ---- the rule context

GameRuleContext::GameRuleContext(const OdysseyGame& game, const Subject& subject) : game_(game), subject_(subject) {}

Value GameRuleContext::path(const std::string& dotted) const {
    const WorldPlant* plant = (subject_.kind == Subject::Kind::Plant && subject_.index >= 0 && static_cast<std::size_t>(subject_.index) < game_.plants().size())
                                  ? &game_.plants()[static_cast<std::size_t>(subject_.index)]
                                  : nullptr;
    if (dotted == "actor" || dotted == "target" || dotted == "npc" || dotted == "hero") return Value::ofText(dotted); // a name for the call functions to look up
    if (dotted == "actor.name" || dotted == "hero.name") return Value::ofText(game_.life() != nullptr ? game_.life()->name() : std::string("Hero"));
    if (dotted == "actor.kind" || dotted == "hero.kind") return Value::ofText("hero");
    if (dotted == "target.name" || dotted == "npc.name") return Value::ofText(subject_.name);
    if (dotted == "target.kind" || dotted == "npc.kind") return Value::ofText(subject_.info.kind);
    if (dotted == "target.state") return Value::ofText(plant != nullptr ? plant->state : std::string());
    if (dotted == "target.inspect") return Value::ofText(plant != nullptr && plant->def != nullptr ? plant->def->inspect : std::string());
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
        const double pixels = std::hypot(subject_.x - game_.hero().feetX(), subject_.y - game_.hero().feetY());
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
        if (args[0].text == "target" || args[0].text == "npc") return Value::ofNumber(hasTag(subject_.info.tags, args[1].text) ? 1 : 0);
        if (args[0].text == "hero" || args[0].text == "actor") return Value::ofNumber(args[1].text == "hero" || args[1].text == "person" ? 1 : 0);
    }
    if (name == "flag" && args.size() == 1 && args[0].isText) {
        if (args[0].text == "sacred-fire") return Value::ofNumber(game_.life() != nullptr && game_.life()->fire().founded ? 1 : 0);
    }
    return Value::ofNumber(0);
}

} // namespace odysseus::game
