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

// The allow and deny lists a placed character resolves to (classes, kind, itself) go into what the rules see of it.
static void applyActionLists(const OdysseyGame& game, const PlacedCharacter& placed, sim::rules::ThingInfo& info) {
    const sim::rules::ResolvedNpc resolved = game.npcClasses().resolve(placed);
    for (const auto& [id, state] : resolved.actions) {
        if (state == sim::rules::ActionState::Allowed) info.allow.push_back(id);
        else if (state == sim::rules::ActionState::Denied) info.deny.push_back(id);
    }
}

std::optional<Subject> npcSubject(const OdysseyGame& game, int placedId) {
    const PlacedCharacter* placed = game.placedCharacter(placedId);
    if (placed == nullptr || game.npcPopulation().indexOf(placedId) < 0) return std::nullopt;
    if (game.npcDirector().mode(game.npcPopulation().indexOf(placedId)) == sim::NpcDirector::Mode::Dead) return std::nullopt; // the dead are not there to talk to
    const sim::rules::ResolvedNpc resolved = game.npcClasses().resolve(*placed);
    Subject subject;
    subject.kind = Subject::Kind::Npc;
    subject.index = placedId;
    subject.name = placed->name;
    subject.title = placed->name + " (" + game.attitudeWordOf(placedId) + ")"; // the attitude word shows in the menu title (US-264)
    const PixelPoint feet = game.npcPosition(placedId); // where its figure stands now: it walks where its schedule sends it
    subject.x = feet.x;
    subject.y = feet.y;
    subject.info.kind = placed->kind;
    subject.info.tags = resolved.tags;
    subject.info.tags.push_back("npc");
    if (game.npcDialogueFor(placedId) != nullptr) subject.info.tags.push_back("speaks"); // no dialogue for the player, no Talk (D-52 Q-11)
    if (game.tradeMarket().isTrader(placedId)) subject.info.tags.push_back("trades"); // it has a profile and a stock: the Trade action works (US-283)
    // A trader that keeps rare goods back (US-282): has-rare-goods, and rare-open when the hero stands high enough for every one it has in stock.
    if (const sim::TradeMarket::Trader* trader = game.tradeMarket().find(placedId); trader != nullptr && !trader->profile.rare.empty()) {
        subject.info.tags.push_back("has-rare-goods");
        const int opinion = game.npcPopulation().opinion(placedId, sim::NpcPopulation::kHero);
        if (game.tradeMarket().lockedGoods(placedId, opinion, game.npcOpinions()).empty()) subject.info.tags.push_back("rare-open");
    }
    applyActionLists(game, *placed, subject.info);
    return subject;
}

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

std::optional<Subject> buildingSubject(const OdysseyGame& game, int buildingId) {
    const sim::buildings::BuildingStore& store = game.buildings().store();
    const sim::buildings::PlacedBuilding* building = store.find(buildingId);
    if (building == nullptr) return std::nullopt;
    Subject subject;
    subject.kind = Subject::Kind::Building;
    subject.index = buildingId;
    subject.name = store.label(*building);
    const std::string state = store.ruleState(*building);
    std::string words = state == "waiting" ? "needs materials" : (state == "ready" ? "ready to build" : state);
    if (state == "damaged") words = std::format("damaged, {}%", store.condition(*building));
    if (state == "burning") words = "on fire!";
    subject.title = subject.name + ": " + words;
    const auto [cx, cy] = store.centre(*building);
    subject.x = cx * kTileSize + kTileSize / 2.0;
    subject.y = cy * kTileSize + kTileSize / 2.0;
    subject.info.kind = building->kind;
    subject.info.tags = store.tags(*building);
    return subject;
}

namespace {

Subject personSubject(const OdysseyGame& game, int person) {
    const sim::HeroLife* hero = game.life();
    const sim::HeroData* data = game.heroData();
    const Figure& figure = game.clanView().figures()[static_cast<std::size_t>(person)];
    Subject subject;
    subject.kind = Subject::Kind::Person;
    subject.index = person;
    subject.name = game.clan()->people()[static_cast<std::size_t>(person)].name;
    subject.title = subject.name;
    subject.x = figure.x;
    subject.y = figure.y;
    subject.info = {"person", {"person", "clan", "speaks"}};
    // The tags say what they can do for the hero now: `teaches-hunter` while they are the hero's master of that profession and have no
    // apprentice yet.
    if (hero != nullptr && data != nullptr) {
        for (int p = 0; p < static_cast<int>(sim::kProfessionCount); ++p) {
            if (hero->masterOf(p) == person && hero->apprenticeOf(p) < 0) subject.info.tags.push_back("teaches-" + data->professions[static_cast<std::size_t>(p)].id);
        }
    }
    return subject;
}

Subject stoneSubject(const OdysseyGame& game) {
    const PixelPoint stone = game.knappingStone();
    return Subject{Subject::Kind::KnappingStone, -1, "Knapping stone", "Knapping stone", static_cast<double>(stone.x), static_cast<double>(stone.y), {"knapping-stone", {"workstation", "knapping-stone"}}};
}

Subject campSubject(const OdysseyGame& game) {
    const PixelPoint camp = game.campPixels();
    return Subject{Subject::Kind::CampFire, -1, "The clan's fire", "The clan's fire", static_cast<double>(camp.x), static_cast<double>(camp.y), {"camp-fire", {"fire", "workstation", "camp-fire"}}};
}

std::optional<Subject> sacredSubject(const OdysseyGame& game) {
    const sim::HeroLife* hero = game.life();
    if (hero == nullptr || !hero->fire().founded) return std::nullopt;
    const double fx = hero->fire().tileX * kTileSize + 16;
    const double fy = hero->fire().tileY * kTileSize + 16;
    const std::string title = std::format("Sacred fire {}", hero->fire().name);
    return Subject{Subject::Kind::SacredFire, -1, title, title, fx, fy, {"sacred-fire", {"fire", "sacred-fire"}}};
}

std::optional<Subject> rivalSubject(const OdysseyGame& game, std::size_t i) {
    if (game.rivals() == nullptr || i >= game.rivals()->clans().size()) return std::nullopt;
    const sim::Tile tile = game.rivals()->clans()[i].camp;
    const std::string& name = game.rivals()->clans()[i].name;
    return Subject{Subject::Kind::RivalCamp, static_cast<int>(i), name, name, tile.x * kTileSize + 16.0, tile.y * kTileSize + 16.0, {"rival-camp", {"camp", "rival"}}};
}

} // namespace

std::optional<Subject> subjectAt(const OdysseyGame& game, double wx, double wy) {
    // A clan member.
    if (const int person = game.personAtWorld(wx, wy); person >= 0 && game.clan() != nullptr) return personSubject(game, person);
    // A placed person (US-265): found through the grid of the population, never by walking all of it. Its picture is 32 x 48 above its feet.
    {
        int best = -1;
        double bestDistance = 1e9;
        for (const PlacedCharacter& figure : game.bystanders()) {
            if (game.npcPopulation().indexOf(figure.id) < 0) continue; // only the placed people (their figures are where they walk to)
            const double px = figure.feet.x;
            const double py = figure.feet.y;
            if (std::abs(wx - px) > 14.0 || wy < py - 46.0 || wy > py + 6.0) continue;
            const double distance = std::hypot(wx - px, wy - py);
            if (distance < bestDistance) {
                bestDistance = distance;
                best = figure.id;
            }
        }
        if (best >= 0) {
            if (auto npc = npcSubject(game, best)) return npc;
        }
    }
    // A creature that is an NPC (an enemy or an animal with a kind file): it can be confronted (US-266).
    {
        int best = -1;
        double bestDistance = 1e9;
        for (std::size_t i = 0; i < game.enemies().size(); ++i) {
            const Enemy& enemy = game.enemies()[i];
            if (!enemy.isAlive() || std::abs(wx - enemy.feetX()) > 14.0 || wy < enemy.feetY() - 46.0 || wy > enemy.feetY() + 6.0) continue;
            if (game.npcClasses().kinds().find(enemy.kindName) == nullptr) continue;
            const double distance = std::hypot(wx - enemy.feetX(), wy - enemy.feetY());
            if (distance < bestDistance) {
                bestDistance = distance;
                best = static_cast<int>(i);
            }
        }
        if (best >= 0) return animalSubject(game, static_cast<std::size_t>(best));
    }
    // A workstation: the knapping stone, or the camp's fire.
    const PixelPoint stone = game.knappingStone();
    if (std::abs(wx - stone.x) < 20 && wy > stone.y - 26 && wy < stone.y + 6) return stoneSubject(game);
    const PixelPoint camp = game.campPixels();
    if (std::hypot(wx - camp.x, wy - (camp.y - 12)) < 22) return campSubject(game);
    // The sacred fire.
    if (const auto sacred = sacredSubject(game); sacred && std::hypot(wx - sacred->x, wy - sacred->y) < 24) return sacred;
    // A rival camp.
    if (game.rivals() != nullptr) {
        for (std::size_t i = 0; i < game.rivals()->clans().size(); ++i) {
            const auto rival = rivalSubject(game, i);
            if (rival && std::hypot(wx - rival->x, wy - (rival->y - 12)) < 26) return rival;
        }
    }
    // A building of the level: any of its pieces under the pointer (US-251).
    {
        const auto [cx, cy] = BuildingLayer::cellOf(wx, wy);
        if (const int id = game.buildings().store().buildingAt(cx, cy); id != 0) return buildingSubject(game, id);
    }
    // A plant: berries, flint, a tree.
    if (const int plantIndex = game.plantAtWorld(wx, wy); plantIndex >= 0) return plantSubject(game, static_cast<std::size_t>(plantIndex));
    return std::nullopt;
}

Subject animalSubject(const OdysseyGame& game, std::size_t enemyIndex) {
    const Enemy& enemy = game.enemies().at(enemyIndex);
    Subject subject;
    subject.kind = Subject::Kind::Animal;
    subject.index = static_cast<int>(enemyIndex);
    subject.title = enemy.name;
    // An NPC with a kind file shows its attitude to the hero in its menu title (US-264): "Grub (hostile)".
    if (game.npcClasses().kinds().find(enemy.kindName) != nullptr) subject.title = enemy.name + " (" + game.attitudeWordOf(enemy.id) + ")";
    subject.name = enemy.name;
    subject.x = enemy.feetX();
    subject.y = enemy.feetY();
    subject.info.kind = enemy.kindName;
    if (const CharacterKindDef* kind = game.definitions().character(enemy.kindName)) subject.info.tags = kind->tags;
    if (game.npcClasses().kinds().find(enemy.kindName) != nullptr) {
        subject.info.tags.push_back("npc"); // an NPC with a kind file can be confronted (US-266)
        if (const PlacedCharacter* placed = game.placedCharacter(enemy.id)) applyActionLists(game, *placed, subject.info);
    }
    return subject;
}

int placedIdOf(const OdysseyGame& game, const Subject& subject) {
    if (subject.kind == Subject::Kind::Npc) return subject.index;
    if (subject.kind == Subject::Kind::Animal && subject.index >= 0 && static_cast<std::size_t>(subject.index) < game.enemies().size()) return game.enemies()[static_cast<std::size_t>(subject.index)].id;
    return -1;
}

Subject heroSubject(const OdysseyGame& game) {
    Subject subject;
    subject.kind = Subject::Kind::Hero;
    subject.title = game.life() != nullptr ? game.life()->name() : std::string("Hero");
    subject.name = subject.title;
    subject.x = game.hero().feetX();
    subject.y = game.hero().feetY();
    subject.info = {"hero", {"hero", "person"}};
    if (game.heldWeapon() != nullptr) subject.info.tags.push_back("armed");
    if (game.hero().walking()) subject.info.tags.push_back("moving");
    return subject;
}

ActorRef actorOfRunnerId(int runnerId) {
    if (runnerId >= kAnimalActorBase) return {ActorRef::Kind::Animal, runnerId - kAnimalActorBase};
    if (runnerId >= kPersonActorBase) return {ActorRef::Kind::Person, runnerId - kPersonActorBase};
    return {ActorRef::Kind::Hero, -1};
}

int runnerIdOf(const ActorRef& actor) {
    switch (actor.kind) {
    case ActorRef::Kind::Hero: return kHeroActor;
    case ActorRef::Kind::Person: return kPersonActorBase + actor.index;
    case ActorRef::Kind::Animal: return kAnimalActorBase + actor.index;
    }
    return kHeroActor;
}

sim::rules::ThingInfo actorInfo(const OdysseyGame& game, const ActorRef& actor) {
    switch (actor.kind) {
    case ActorRef::Kind::Hero: return {"hero", {"hero", "person"}};
    case ActorRef::Kind::Person: return {"person", {"person", "clan"}};
    case ActorRef::Kind::Animal:
        for (const PlacedCharacter& placed : game.bystanders()) {
            if (placed.id == actor.index) {
                sim::rules::ThingInfo info{placed.kind, {}};
                if (const CharacterKindDef* kind = game.definitions().character(placed.kind)) info.tags = kind->tags;
                return info;
            }
        }
        for (const Enemy& enemy : game.enemies()) {
            if (enemy.id == actor.index) {
                sim::rules::ThingInfo info{enemy.kindName, {}};
                if (const CharacterKindDef* kind = game.definitions().character(enemy.kindName)) info.tags = kind->tags;
                return info;
            }
        }
        break;
    }
    return {"unknown", {}};
}

bool actorPosition(const OdysseyGame& game, const ActorRef& actor, double& x, double& y) {
    switch (actor.kind) {
    case ActorRef::Kind::Hero:
        x = game.hero().feetX();
        y = game.hero().feetY();
        return true;
    case ActorRef::Kind::Person:
        if (actor.index < 0 || static_cast<std::size_t>(actor.index) >= game.clanView().figures().size() || !game.clanView().figures()[static_cast<std::size_t>(actor.index)].present) return false;
        x = game.clanView().figures()[static_cast<std::size_t>(actor.index)].x;
        y = game.clanView().figures()[static_cast<std::size_t>(actor.index)].y;
        return true;
    case ActorRef::Kind::Animal:
        for (const PlacedCharacter& placed : game.bystanders()) {
            if (placed.id == actor.index) {
                x = placed.feet.x;
                y = placed.feet.y;
                return true;
            }
        }
        for (const Enemy& enemy : game.enemies()) {
            if (enemy.id == actor.index && enemy.isAlive()) {
                x = enemy.feetX();
                y = enemy.feetY();
                return true;
            }
        }
        return false;
    }
    return false;
}

sim::rules::ThingRef refOf(const OdysseyGame& game, const Subject& subject) {
    const int kind = static_cast<int>(subject.kind);
    if (subject.kind == Subject::Kind::Animal && subject.index >= 0 && static_cast<std::size_t>(subject.index) < game.enemies().size()) {
        return {kind, game.enemies()[static_cast<std::size_t>(subject.index)].id};
    }
    if (subject.kind == Subject::Kind::Plant && subject.index >= 0 && static_cast<std::size_t>(subject.index) < game.plants().size()) {
        return {kind, game.plants()[static_cast<std::size_t>(subject.index)].id};
    }
    return {kind, subject.index};
}

std::optional<Subject> subjectFor(const OdysseyGame& game, const sim::rules::ThingRef& ref) {
    switch (static_cast<Subject::Kind>(ref.kind)) {
    case Subject::Kind::Plant: {
        const int index = game.plantIndexById(ref.id);
        if (index < 0) return std::nullopt;
        return plantSubject(game, static_cast<std::size_t>(index));
    }
    case Subject::Kind::Person:
        if (game.clan() == nullptr || ref.id < 0 || static_cast<std::size_t>(ref.id) >= game.clanView().figures().size()) return std::nullopt;
        return personSubject(game, ref.id);
    case Subject::Kind::KnappingStone: return stoneSubject(game);
    case Subject::Kind::CampFire: return campSubject(game);
    case Subject::Kind::SacredFire: return sacredSubject(game);
    case Subject::Kind::RivalCamp: return rivalSubject(game, static_cast<std::size_t>(std::max(0, ref.id)));
    case Subject::Kind::Hero: return heroSubject(game);
    case Subject::Kind::Npc: return npcSubject(game, ref.id);
    case Subject::Kind::Building: return buildingSubject(game, ref.id);
    case Subject::Kind::Animal:
        for (std::size_t i = 0; i < game.enemies().size(); ++i) {
            if (game.enemies()[i].id == ref.id && game.enemies()[i].isAlive()) return animalSubject(game, i);
        }
        return std::nullopt;
    }
    return std::nullopt;
}
std::vector<std::string> builtInThingTags(const OdysseyGame& game) {
    std::vector<std::string> tags = {"building", "construction", "repairable", "burning", "enterable", "exit", "shelter", "sleep", "store", "storage", "work", "person", "clan", "npc", "speaks", "trader", "trades", "has-rare-goods", "rare-open", "place", "post", "event", "can-swap", "workstation", "knapping-stone", "camp-fire", "fire", "sacred-fire", "camp", "rival", "hero", "armed", "moving", "forage", "shelter", "water", "shrine", "market", "prey", "animal"};
    for (const PlacedPlace& place : game.level().places) tags.insert(tags.end(), place.tags.begin(), place.tags.end()); // the purposes the owner gave the places of this level
    if (const sim::HeroData* data = game.heroData()) {
        for (const auto& profession : data->professions) tags.push_back("teaches-" + profession.id);
    }
    for (const sim::buildings::KindDef& kind : game.buildings().data().kinds()) tags.push_back("blueprint-" + kind.id); // a place may teach a blueprint (US-251)
    return tags;
}

// ---- the rule context

int personNamed(const OdysseyGame& game, const Subject& subject, const std::string& word) {
    if (word == "npc" || word == "target") return subject.kind == Subject::Kind::Person ? subject.index : -1;
    if (word == "hero" || word == "actor") return game.life() != nullptr ? game.life()->personId() : -1;
    return -1;
}

GameRuleContext::GameRuleContext(const OdysseyGame& game, const Subject& subject, ActorRef actor) : game_(game), subject_(subject), actor_(actor) {}

Value GameRuleContext::path(const std::string& dotted) const {
    const WorldPlant* plant = (subject_.kind == Subject::Kind::Plant && subject_.index >= 0 && static_cast<std::size_t>(subject_.index) < game_.plants().size())
                                  ? &game_.plants()[static_cast<std::size_t>(subject_.index)]
                                  : nullptr;
    if (dotted == "actor" || dotted == "target" || dotted == "npc" || dotted == "hero") return Value::ofText(dotted); // a name for the call functions to look up
    if (dotted == "actor.name" && actor_.kind == ActorRef::Kind::Person && game_.clan() != nullptr && actor_.index >= 0 && static_cast<std::size_t>(actor_.index) < game_.clan()->people().size()) {
        return Value::ofText(game_.clan()->people()[static_cast<std::size_t>(actor_.index)].name);
    }
    if (dotted == "actor.name" && actor_.kind == ActorRef::Kind::Animal) return Value::ofText(actorInfo(game_, actor_).kind);
    if (dotted == "actor.kind") return Value::ofText(actorInfo(game_, actor_).kind);
    if (dotted == "actor.name" || dotted == "hero.name") return Value::ofText(game_.life() != nullptr ? game_.life()->name() : std::string("Hero"));
    if (dotted == "hero.kind") return Value::ofText("hero");
    if (dotted == "target.name" || dotted == "npc.name") return Value::ofText(subject_.name);
    if (dotted == "target.kind" || dotted == "npc.kind") return Value::ofText(subject_.info.kind);
    if (dotted == "target.state" && subject_.kind == Subject::Kind::Building) {
        const sim::buildings::PlacedBuilding* building = game_.buildings().store().find(subject_.index);
        return Value::ofText(building != nullptr ? game_.buildings().store().ruleState(*building) : std::string());
    }
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
        double ax = game_.hero().feetX();
        double ay = game_.hero().feetY();
        actorPosition(game_, actor_, ax, ay);
        const double pixels = std::hypot(subject_.x - ax, subject_.y - ay);
        return Value::ofNumber(static_cast<long long>(pixels / kTileSize)); // a tile is a metre
    }
    return Value::ofNumber(0);
}

Value GameRuleContext::call(const std::string& name, const std::vector<Value>& args) const {
    if (name == "need" && args.size() == 1 && args[0].isText) {
        // How much a need is missing, 0 (full) to 100 (desperate): the hungrier, the higher (the brief's `need(hunger) * 2`).
        if (actor_.kind != ActorRef::Kind::Person || game_.clan() == nullptr || actor_.index < 0 || static_cast<std::size_t>(actor_.index) >= game_.clan()->people().size()) return Value::ofNumber(0);
        const sim::Person& person = game_.clan()->people()[static_cast<std::size_t>(actor_.index)];
        for (std::size_t n = 0; n < sim::kNeedCount; ++n) {
            if (lower(sim::needName(static_cast<sim::Need>(n))) == lower(args[0].text)) return Value::ofNumber(100 - person.needs[static_cast<sim::Need>(n)]);
        }
        return Value::ofNumber(0);
    }
    if (name == "trait" && args.size() == 1 && args[0].isText) {
        if (actor_.kind != ActorRef::Kind::Person || game_.clan() == nullptr || actor_.index < 0 || static_cast<std::size_t>(actor_.index) >= game_.clan()->people().size()) return Value::ofNumber(0);
        const sim::Person& person = game_.clan()->people()[static_cast<std::size_t>(actor_.index)];
        for (std::size_t t = 0; t < sim::kTraitCount; ++t) {
            if (lower(sim::traitName(static_cast<sim::Trait>(t))) == lower(args[0].text)) return Value::ofNumber((person.traits >> t) & 1);
        }
        return Value::ofNumber(0);
    }
    if (name == "has") {
        // has(item, n): the hero; has(who, item, n): `who` is the hero here (the only one with a bag so far).
        const std::size_t base = args.size() == 3 ? 1 : 0;
        if (game_.life() == nullptr || args.size() < base + 2 || !args[base].isText) return Value::ofNumber(0);
        if (base == 1 && args[0].isText && args[0].text != "hero" && args[0].text != "actor") return Value::ofNumber(0);
        return Value::ofNumber(game_.life()->count(args[base].text) >= args[base + 1].number ? 1 : 0);
    }
    if (name == "tag" && args.size() == 2 && args[0].isText && args[1].isText) {
        if (args[0].text == "target" || args[0].text == "npc") return Value::ofNumber(hasTag(subject_.info.tags, args[1].text) ? 1 : 0);
        if (args[0].text == "actor" || args[0].text == "hero") return Value::ofNumber(hasTag(actorInfo(game_, actor_).tags, args[1].text) ? 1 : 0);
    }
    if (placedIdOf(game_, subject_) >= 0 && name == "opinion" && args.size() == 2 && args[0].isText && args[1].isText) {
        // What a placed person thinks of the hero (US-265): opinion(npc, hero). Anything else is 0 (the hero's own opinions are not kept).
        const bool npc = args[0].text == "npc" || args[0].text == "target";
        const bool hero = args[1].text == "hero" || args[1].text == "actor";
        return Value::ofNumber(npc && hero ? game_.npcPopulation().opinion(placedIdOf(game_, subject_), sim::NpcPopulation::kHero) : 0);
    }
    if (placedIdOf(game_, subject_) >= 0 && name == "mood" && args.size() == 1 && args[0].isText) {
        return Value::ofText(game_.attitudeWordOf(placedIdOf(game_, subject_))); // the attitude word, as in the title of the menu
    }
    if (name == "opinion" && args.size() == 2 && args[0].isText && args[1].isText && game_.clan() != nullptr) {
        const int who = personNamed(game_, subject_, args[0].text);
        const int about = personNamed(game_, subject_, args[1].text);
        const auto count = static_cast<int>(game_.clan()->people().size());
        if (who < 0 || about < 0 || who >= count || about >= count) return Value::ofNumber(0);
        return Value::ofNumber(game_.clan()->opinion(who, about));
    }
    if (name == "mood" && args.size() == 1 && args[0].isText && game_.clan() != nullptr && game_.life() != nullptr) {
        // How the person feels about the hero (D-38): the same word the conversation panel shows.
        const int who = personNamed(game_, subject_, args[0].text);
        if (who < 0 || static_cast<std::size_t>(who) >= game_.clan()->people().size()) return Value::ofText("neutral");
        return Value::ofText(sim::rules::moodWord(game_.clan()->opinion(who, game_.life()->personId()), game_.clan()->people()[static_cast<std::size_t>(who)].needs));
    }
    if (name == "flag" && args.size() == 1 && args[0].isText) {
        if (args[0].text == "sacred-fire") return Value::ofNumber(game_.life() != nullptr && game_.life()->fire().founded ? 1 : 0);
        return Value::ofNumber(game_.flags().get(args[0].text)); // set by `flag name` in a conversation or an interaction (US-164)
    }
    return Value::ofNumber(0);
}

} // namespace odysseus::game
