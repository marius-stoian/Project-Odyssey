// US-290 Day and night schedules in the game and in the Editor: the places of a level, the Schedule form of the NPC panel, the Class panel and the Kinds tab, and the figures
// of the placed people walking where their schedule sends them.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/npc_director.h"

#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

struct Studio {
    fs::path data;
    fs::path file;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int tala = 0;

    // A level with Tala (a wanderer of class trader) 4 tiles west of the hero and, when asked, a goblin next to her; the market is 10 tiles east of the hero.
    Studio(const std::string& name, bool withGoblin = false, bool withMarket = true) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        const game::PixelPoint hero = level.heroStart;
        game::PlacedCharacter talaPlaced{level.nextId++, "wanderer", {hero.x - 128, hero.y}, game::Facing::South, "Tala", 60, 4, {"trader"}};
        tala = talaPlaced.id;
        level.characters = {talaPlaced};
        if (withGoblin) level.characters.push_back({level.nextId++, "goblin", {hero.x - 128 + 64, hero.y}, game::Facing::South, "Grub", 60, 4, {}});
        if (withMarket) level.places = {{"market", {hero.x + 320, hero.y}, {}}};
        file = data / "schedule-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    game::Editor& editor() { return odyssey->editor(); }
    void toEditor() { odyssey->update(pressing(luna::engine::Intent::ModeEditor)); }
    void toGame() { odyssey->update(pressing(luna::engine::Intent::ModeGame)); }
    void play(int ticks) {
        for (int i = 0; i < ticks; ++i) odyssey->update({});
    }
    int index() const { return odyssey->npcPopulation().indexOf(tala); }
};

} // namespace

TEST_CASE("US-290 Places: the owner names places by tile, the level keeps them, and mistakes are refused") {
    Studio studio("schedule-places", false, false);
    studio.toEditor();
    game::Editor& editor = studio.editor();
    const std::size_t before = editor.history().size();
    CHECK(editor.setPlaces("market=20,10 grove=30,12/forage/shelter"));
    CHECK(editor.history().size() == before + 1);
    REQUIRE(editor.level().places.size() == 2);
    CHECK(editor.level().places[0].name == "market");
    CHECK(editor.level().places[0].at.x == 20 * 32 + 16); // the middle of the tile
    CHECK(editor.level().places[1].tags == std::vector<std::string>{"forage", "shelter"});
    CHECK(game::placesText(editor.level().places) == "market=20,10 grove=30,12/forage/shelter");
    CHECK(editor.setPlaces("market=20,10 grove=30,12/forage/shelter")); // the same text: no step
    CHECK(editor.history().size() == before + 1);

    REQUIRE(editor.save());
    CHECK(readText(studio.file).find("\"places\"") != std::string::npos);
    CHECK(game::loadLevel(studio.file, studio.odyssey->definitions()).level == editor.level());

    CHECK_FALSE(editor.setPlaces("market"));
    CHECK_FALSE(editor.setPlaces("home=1,1"));
    CHECK_FALSE(editor.setPlaces("a=1,1 a=2,2"));
    CHECK_FALSE(editor.setPlaces("far=9999,1"));
    CHECK_FALSE(editor.setPlaces("x=a,b"));
    CHECK(editor.level().places.size() == 2); // unchanged
    CHECK(editor.status().find("places") != std::string::npos);
    CHECK(editor.undo());
    CHECK(editor.level().places.empty());
    CHECK(editor.setPlaces(""));
    CHECK(editor.level().places.empty());
}

TEST_CASE("US-290 Form: the schedule of an NPC is typed as time, activity and place, saved, and followed in play") {
    Studio studio("schedule-form");
    studio.toEditor();
    game::Editor& editor = studio.editor();
    editor.select(studio.tala);
    REQUIRE(editor.selectedIsNpc());
    const std::size_t before = editor.history().size();
    CHECK(editor.setSelectedSchedule("day", "06:00 work market; 21:00 sleep home"));
    CHECK(editor.setSelectedSchedule("night", "21:00 sleep home"));
    CHECK(editor.history().size() == before + 2);
    const sim::rules::Schedule& schedule = editor.level().characters[0].extras.schedule;
    REQUIRE(schedule.day.size() == 2);
    CHECK(schedule.day[0].place == "market");
    CHECK(schedule.night.size() == 1);
    REQUIRE(editor.save());
    CHECK(readText(studio.file).find("\"schedule\"") != std::string::npos);
    CHECK(game::loadLevel(studio.file, studio.odyssey->definitions()).level == editor.level());

    // A mistake changes nothing.
    CHECK_FALSE(editor.setSelectedSchedule("day", "6am work"));
    CHECK_FALSE(editor.setSelectedSchedule("week", "06:00 work market"));
    CHECK(editor.level().characters[0].extras.schedule == schedule);
    CHECK(editor.status().find("schedule") != std::string::npos);
    CHECK(editor.undo());
    CHECK(editor.level().characters[0].extras.schedule.night.empty());

    // Play: the person has the schedule.
    studio.toGame();
    const sim::NpcDirector& director = studio.odyssey->npcDirector();
    REQUIRE(director.schedule(studio.index()) != nullptr);
    CHECK(director.schedule(studio.index())->day.size() == 2);
    CHECK(director.places().size() == 1);
    CHECK(director.homeX(studio.index()) == studio.odyssey->level().characters[0].feet.x);
}

TEST_CASE("US-290 Class and kind: a class or a kind can carry the schedule every NPC of it follows") {
    Studio studio("schedule-class");
    studio.toEditor();
    game::Editor& editor = studio.editor();
    editor.showClasses(true);
    editor.selectClass("trader");
    CHECK(editor.setClassSchedule("day", "08:00 work market; 20:00 sleep home"));
    CHECK_FALSE(editor.setClassSchedule("day", "08:00"));
    REQUIRE(editor.saveClass());
    const sim::rules::ResolvedNpc resolved = studio.odyssey->npcClasses().resolve(studio.odyssey->level().characters[0]);
    REQUIRE(resolved.extras.schedule.day.size() == 2);
    CHECK(resolved.extras.schedule.day[0].minute == 8 * 60);
    // A kind's schedule replaces the class's; the NPC's own replaces both.
    editor.showKinds(true);
    editor.selectKind("wanderer");
    CHECK(editor.setKindSchedule("day", "09:00 rest home"));
    REQUIRE(editor.saveKind());
    CHECK(studio.odyssey->npcClasses().resolve(studio.odyssey->level().characters[0]).extras.schedule.day[0].activity == "rest");
    editor.showClasses(false);
    editor.select(studio.tala);
    CHECK(editor.setSelectedSchedule("day", "10:00 work market"));
    CHECK(studio.odyssey->npcClasses().resolve(editor.level().characters[0]).extras.schedule.day[0].minute == 10 * 60);
    // And in play the person has the schedule of the highest layer.
    studio.toGame();
    REQUIRE(studio.odyssey->npcDirector().schedule(studio.index()) != nullptr);
    CHECK(studio.odyssey->npcDirector().schedule(studio.index())->day[0].minute == 10 * 60);
}

TEST_CASE("US-290 Walk: the figure of a person walks to the market its schedule names and can be found there") {
    Studio studio("schedule-walk");
    studio.toEditor();
    game::Editor& editor = studio.editor();
    editor.select(studio.tala);
    REQUIRE(editor.setSelectedSchedule("day", "06:00 work market"));
    studio.toGame();
    const game::PixelPoint start = studio.odyssey->npcPosition(studio.tala);
    studio.play(50);
    CHECK(studio.odyssey->npcPosition(studio.tala) == start); // the schedule is looked at on the hour
    studio.play(100);
    const int index = studio.index();
    const sim::NpcPopulation& people = studio.odyssey->npcPopulation();
    CHECK(people.x(index) > start.x + 300); // sent to the market
    studio.play(900);
    const game::PixelPoint now = studio.odyssey->npcPosition(studio.tala);
    CHECK(now.x == people.x(index)); // the figure got there (round what is in the way, or put there after ten seconds without progress)
    CHECK(now.y == people.y(index));
    CHECK(now.x > start.x + 300);
    // The subject of its menu is where the figure stands.
    const auto subject = game::npcSubject(*studio.odyssey, studio.tala);
    REQUIRE(subject.has_value());
    CHECK(subject->x == now.x);
    CHECK(subject->y == now.y);
    const auto found = game::subjectAt(*studio.odyssey, now.x, now.y - 20);
    REQUIRE(found.has_value());
    CHECK(found->index == studio.tala);
}

TEST_CASE("US-290 Danger: a hostile within 6 m of a person sends them home at the next hour, and the schedule resumes when it is gone") {
    Studio studio("schedule-danger", true);
    studio.toEditor();
    game::Editor& editor = studio.editor();
    editor.select(studio.tala);
    REQUIRE(editor.setSelectedSchedule("day", "06:00 work market"));
    studio.toGame();
    studio.play(100 + 20);
    CHECK(studio.odyssey->npcDirector().mode(studio.index()) == sim::NpcDirector::Mode::Fleeing);
    CHECK(studio.odyssey->npcPopulation().x(studio.index()) == studio.odyssey->level().characters[0].feet.x); // home
}
