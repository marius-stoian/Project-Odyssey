// US-292 NPCs act on each other in the game: two traders swap goods, hostile persons fight with the numbers of the hero's own fights, the dead leave the world, and the persons talk.
#include "camp.h"

#include "game/game_rules.h"
#include "sim/npc_director.h"

#include <functional>
#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

struct Street {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int ossa = 0;
    int brek = 0;
    int vell = 0;

    // Ossa and Brek stand next to each other 4 tiles west of the hero; Vell is a few tiles off.
    explicit Street(const std::string& name, const std::function<void(game::PlacedCharacter&, game::PlacedCharacter&)>& dress = {}) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        const game::PixelPoint hero = level.heroStart;
        game::PlacedCharacter first{level.nextId++, "wanderer", {hero.x - 128, hero.y}, game::Facing::South, "Ossa", 100, 40, {}};
        game::PlacedCharacter second{level.nextId++, "wanderer", {hero.x - 128 + 30, hero.y}, game::Facing::South, "Brek", 60, 2, {}};
        game::PlacedCharacter third{level.nextId++, "wanderer", {hero.x - 128 + 220, hero.y + 100}, game::Facing::South, "Vell", 60, 4, {}};
        ossa = first.id;
        brek = second.id;
        vell = third.id;
        if (dress) dress(first, second);
        level.characters = {first, second, third};
        game::saveLevel(level, definitions, data / "street-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "street-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    void play(int ticks) {
        for (int i = 0; i < ticks; ++i) odyssey->update({});
    }
    int index(int id) const { return odyssey->npcPopulation().indexOf(id); }
    bool figure(int id) const {
        for (const game::PlacedCharacter& placed : odyssey->bystanders()) {
            if (placed.id == id) return true;
        }
        return false;
    }
};

} // namespace

TEST_CASE("US-292 Trade: two traders near each other, each wanting what the other has, swap goods when they are idle") {
    Street street("dealings-trade", [](game::PlacedCharacter& first, game::PlacedCharacter& second) {
        first.extras.trade.stock = {{"flint", 3}};
        first.extras.trade.wants = {"fur"};
        second.extras.trade.stock = {{"fur", 2}};
        second.extras.trade.wants = {"flint"};
    });
    const sim::TradeMarket& market = street.odyssey->tradeMarket();
    REQUIRE(market.isTrader(street.ossa));
    REQUIRE(market.isTrader(street.brek));
    CHECK(market.stock(street.ossa, "flint") == 3);
    street.play(110); // the first hour mark
    CHECK(market.stock(street.ossa, "flint") < 3);
    CHECK(market.stock(street.ossa, "fur") > 0);
    CHECK(market.stock(street.brek, "fur") < 2);
    CHECK(market.stock(street.brek, "flint") > 0);
    CHECK(market.stock(street.ossa, "flint") + market.stock(street.brek, "flint") == 3); // nothing made, nothing lost
    CHECK(street.odyssey->npcDirector().lastAction(street.index(street.ossa)) == "npc-swap");
    CHECK(street.odyssey->npcPopulation().opinion(street.ossa, street.brek) > 0);
}

TEST_CASE("US-292 Talk: persons next to each other talk and have met afterwards; the persons are not the hero's clan") {
    Street street("dealings-talk");
    street.play(110);
    CHECK(street.odyssey->npcPopulation().met(street.ossa, street.brek));
    CHECK(street.odyssey->npcPopulation().met(street.brek, street.ossa));
    const std::string last = street.odyssey->npcDirector().lastAction(street.index(street.ossa));
    CHECK(last == "npc-chat");
}

TEST_CASE("US-292 Fight: two NPCs who are hostile to each other fight when they meet, the weaker dies and its figure leaves the world, and nobody can talk to the dead") {
    Street street("dealings-fight");
    sim::NpcPopulation& people = street.odyssey->npcPopulationMutable();
    people.adjust(street.ossa, street.brek, -80); // Ossa has had enough of Brek
    REQUIRE(people.attitude(street.ossa, street.brek) == sim::Attitude::Hostile);
    REQUIRE(street.figure(street.brek));
    street.play(110);
    const sim::NpcDirector& director = street.odyssey->npcDirector();
    CHECK(director.mode(street.index(street.brek)) == sim::NpcDirector::Mode::Dead); // Brek has 60 hit points against 40 damage a hit
    CHECK(director.mode(street.index(street.ossa)) != sim::NpcDirector::Mode::Dead);
    CHECK(director.hp(street.index(street.ossa)) < 100); // Brek struck back
    CHECK_FALSE(street.figure(street.brek));
    CHECK(street.figure(street.ossa));
    CHECK_FALSE(game::npcSubject(*street.odyssey, street.brek).has_value());
    CHECK(game::npcSubject(*street.odyssey, street.ossa).has_value());
    // Vell, a few tiles off, did not know Brek: nothing changed for her.
    CHECK(street.figure(street.vell));
}

TEST_CASE("US-292 Same rules: the combat numbers of a person are the hit points and the sword damage of its placed character") {
    Street street("dealings-numbers");
    const sim::NpcDirector& director = street.odyssey->npcDirector();
    CHECK(director.hp(street.index(street.ossa)) == 100);
    CHECK(director.hp(street.index(street.brek)) == 60);
    CHECK(director.hp(street.index(street.vell)) == 60);
}
