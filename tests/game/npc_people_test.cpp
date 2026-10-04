// US-262 Placed NPCs are persons in the game: a wanderer is a person, a goblin of class monster and a deer stay creatures, persons age and are saved.
#include "camp.h"

#include "game/npc_class_book.h"
#include "sim/npc_population.h"

#include <chrono>
#include <functional>
#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

game::PlacedCharacter placedAt(int id, const std::string& kind, const std::string& name, game::PixelPoint at) {
    return {id, kind, at, game::Facing::South, name, 60, 4, {}};
}

struct Village {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    // The wanderer stands `away` pixels east of the hero (far enough not to meet them unless asked).
    explicit Village(const std::string& name, bool clan = false, int away = 600) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.clan = clan;
        const game::PixelPoint hero = level.heroStart;
        level.characters = {placedAt(level.nextId++, "wanderer", "Ossa", {hero.x + away, hero.y}), placedAt(level.nextId++, "goblin", "Grub", {hero.x + away, hero.y + 80}),
                            placedAt(level.nextId++, "deer", "Doe", {hero.x + away, hero.y + 160})};
        level.characters[2].classes = {};
        game::saveLevel(level, definitions, data / "village-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "village-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    void tick(int count) {
        for (int i = 0; i < count; ++i) odyssey->update({});
    }
};

} // namespace

TEST_CASE("US-262 Person and creatures: a wanderer is a person; a goblin of class monster and a deer are creatures") {
    Village village("npc-people-kinds");
    const sim::NpcPopulation& people = village.odyssey->npcPopulation();
    REQUIRE(people.size() == 1);
    const auto& placed = village.odyssey->level().characters;
    CHECK(people.indexOf(placed[0].id) >= 0);   // Ossa
    CHECK(people.indexOf(placed[1].id) == -1);  // the goblin
    CHECK(people.indexOf(placed[2].id) == -1);  // the deer
    CHECK(village.odyssey->isPersonKind(placed[0]));
    CHECK_FALSE(village.odyssey->isPersonKind(placed[1]));
    CHECK_FALSE(village.odyssey->isPersonKind(placed[2]));
    CHECK(people.kind(0) == "wanderer");
    CHECK(people.x(0) == village.odyssey->level().heroStart.x + 600);
    CHECK(people.ageDays(0) >= 20 * 28);
    // The goblin still fights as before: it is an enemy of the game, standing where it was placed.
    CHECK(village.odyssey->enemies().size() >= 1);
}

TEST_CASE("US-262 Person in play: a day passes, the person ages, their needs change and they remember the day; meeting the hero is remembered") {
    Village village("npc-people-day");
    const sim::NpcPopulation& people = village.odyssey->npcPopulation();
    const int age = people.ageDays(0);
    const int hunger = people.need(0, sim::Need::Hunger);
    village.tick(2400); // one game day at 20 ticks a second, 2 minutes
    CHECK(people.day() == 1);
    CHECK(people.ageDays(0) == age + 1);
    CHECK(people.need(0, sim::Need::Hunger) < hunger);
    REQUIRE(people.noteCount(0) >= 1);

    CHECK(people.noteCount(0) == 1); // the day, nobody met
}

TEST_CASE("US-262 Meeting: a person the hero stands next to remembers meeting them, once a day") {
    Village village("npc-people-meet", false, 20);
    const sim::NpcPopulation& people = village.odyssey->npcPopulation();
    village.tick(40);
    REQUIRE(people.noteCount(0) == 1);
    CHECK(people.note(0, 0).text == "met the hero");
    village.tick(100); // the same day: no second memory
    CHECK(people.noteCount(0) == 1);
    village.tick(2400); // the next day: the day itself, and a new meeting
    CHECK(people.noteCount(0) == 3);
}

TEST_CASE("US-262 Saved in the game: the persons are written with the autosave and read back") {
    Village village("npc-people-save", true);
    const sim::NpcPopulation& people = village.odyssey->npcPopulation();
    village.tick(2400 + 200);
    REQUIRE(people.size() == 1);
    const int age = people.ageDays(0);
    const std::uint64_t hash = people.hash();
    REQUIRE(village.odyssey->autosave());
    CHECK(fs::exists(village.odyssey->saveDirectory() / "npcs.json"));
    // Change the live state, then load: the saved state comes back.
    village.odyssey->npcPopulationMutable().runTicks(2400 * 3);
    CHECK(people.ageDays(0) == age + 3);
    REQUIRE(village.odyssey->loadAutosave());
    CHECK(people.ageDays(0) == age);
    CHECK(people.hash() == hash);
}

TEST_CASE("US-263 Frame: the game with 100,000 far persons loaded keeps its frame time") {
    Village village("npc-people-crowd");
    sim::NpcPopulation& people = village.odyssey->npcPopulationMutable();
    for (int i = 0; i < 100000; ++i) {
        people.add(1000 + i, "wanderer", 20 * 28, i % 40, 4000 + (i * 7919) % 12000, 4000 + (i * 104729) % 12000); // all far from the hero
    }
    REQUIRE(people.size() == 100001);
    double worstMs = 0.0;
    for (int i = 0; i < 1200; ++i) { // a minute of play at 20 ticks a second
        const auto started = std::chrono::steady_clock::now();
        village.odyssey->update({});
        worstMs = std::max(worstMs, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
    }
    MESSAGE("a minute of play with 100,000 far persons: worst update ", worstMs, " ms");
    CHECK(people.nearCount() < 50);
#ifdef NDEBUG
    CHECK(worstMs < 16.0); // the D-06 target of 60 FPS, for the simulation part of a frame
#endif
}
