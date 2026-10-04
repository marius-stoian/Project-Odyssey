// US-291 Action sources in the game: the Does line of the Editor, the class action of the shipped guard class in play, and a fire that sends the people near it to help.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/npc_director.h"

#include <cstdlib>
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
    game::PixelPoint gate;
    game::PixelPoint firePit;

    // Tala stands 4 tiles west of the hero with the given classes; a gate (a place tagged post) is 10 tiles east of the hero; a fire pit lies 5 tiles east of Tala.
    Studio(const std::string& name, std::vector<std::string> classes) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        const game::PixelPoint hero = level.heroStart;
        game::PlacedCharacter placed{level.nextId++, "wanderer", {hero.x - 128, hero.y}, game::Facing::South, "Tala", 60, 4, std::move(classes)};
        tala = placed.id;
        level.characters = {placed};
        gate = {hero.x + 320, hero.y};
        firePit = {hero.x - 128 + 160, hero.y};
        level.places = {{"gate", gate, {"post"}}};
        level.plants.push_back({level.nextId++, "fire pit", firePit});
        file = data / "life-level.json";
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
    std::size_t firePitIndex() const {
        for (std::size_t i = 0; i < odyssey->plants().size(); ++i) {
            if (odyssey->plants()[i].kind == "fire pit") return i;
        }
        FAIL("the level has no fire pit");
        return 0;
    }
};

} // namespace

TEST_CASE("US-291 Does: the Does line sets the custom actions of an NPC, the actions of a class and of a kind; each NPC change is one step of Undo") {
    Studio studio("life-does", {"talker"});
    studio.toEditor();
    game::Editor& editor = studio.editor();
    editor.select(studio.tala);
    REQUIRE(editor.selectedIsNpc());
    const std::size_t before = editor.history().size();
    CHECK(editor.setSelectedDoes("patrol, sing"));
    CHECK(editor.history().size() == before + 1);
    CHECK(editor.level().characters[0].extras.does == std::vector<std::string>{"patrol", "sing"});
    CHECK(editor.setSelectedDoes("patrol sing")); // the same list: no step
    CHECK(editor.history().size() == before + 1);
    CHECK_FALSE(editor.setSelectedDoes("Patrol"));
    CHECK(editor.status().find("does") != std::string::npos);
    REQUIRE(editor.save());
    CHECK(readText(studio.file).find("\"does\"") != std::string::npos);
    CHECK(game::loadLevel(studio.file, studio.odyssey->definitions()).level == editor.level());
    CHECK(editor.undo());
    CHECK(editor.level().characters[0].extras.does.empty());

    editor.showClasses(true);
    editor.selectClass("talker");
    CHECK(editor.setClassDoes("chat"));
    REQUIRE(editor.saveClass());
    CHECK(studio.odyssey->npcClasses().resolve(editor.level().characters[0]).classActions == std::vector<std::string>{"chat"});
    editor.showKinds(true);
    editor.selectKind("wanderer");
    CHECK(editor.setKindDoes("patrol"));
    REQUIRE(editor.saveKind());
    const sim::rules::ResolvedNpc resolved = studio.odyssey->npcClasses().resolve(editor.level().characters[0]);
    CHECK(resolved.classActions == std::vector<std::string>{"chat"});    // from the class
    CHECK(resolved.customActions == std::vector<std::string>{"patrol"}); // from the kind
}

TEST_CASE("US-291 Class: an NPC of the shipped guard class patrols in play, between the posts of the level") {
    Studio studio("life-patrol", {"guard"});
    studio.toGame();
    REQUIRE(studio.index() >= 0);
    const sim::NpcProfile* profile = studio.odyssey->npcDirector().profile(studio.index());
    REQUIRE(profile != nullptr);
    CHECK(profile->classActions == std::vector<std::string>{"patrol"});
    studio.play(110); // the first hour mark
    CHECK(studio.odyssey->npcDirector().lastAction(studio.index()) == "patrol");
    const sim::NpcPopulation& people = studio.odyssey->npcPopulation();
    CHECK(std::abs(people.x(studio.index()) - studio.gate.x) <= 48); // at the gate, give or take the scatter of persons at one place
    CHECK(std::abs(people.y(studio.index()) - studio.gate.y) <= 48);
    studio.play(1000);
    const game::PixelPoint now = studio.odyssey->npcPosition(studio.tala);
    CHECK(now.x == people.x(studio.index())); // the figure walked there
}

TEST_CASE("US-291 Event: a fire lit within 12 m of a talker sends her to help; a guard of no such class does not go") {
    Studio studio("life-fire", {"talker"});
    studio.toGame();
    studio.play(5);
    CHECK(studio.odyssey->npcDirector().lastAction(studio.index()).empty());
    const game::PixelPoint start = studio.odyssey->npcPosition(studio.tala);
    studio.odyssey->setPlantState(studio.firePitIndex(), "burning"); // the fire starts
    CHECK(studio.odyssey->npcDirector().lastAction(studio.index()) == "help-with-fire");
    const sim::NpcPopulation& people = studio.odyssey->npcPopulation();
    CHECK(std::abs(people.x(studio.index()) - studio.firePit.x) <= 48);
    studio.play(400);
    const game::PixelPoint now = studio.odyssey->npcPosition(studio.tala);
    CHECK(now.x > start.x + 100); // her figure walked to the fire, 5 m east
    CHECK(now.x == people.x(studio.index()));
}

TEST_CASE("US-291 No quests: the actions of an NPC come from its class, custom and event sources only") {
    const sim::ActionSources sources = sim::ActionSources::standard();
    CHECK(sources.count() == 4); // class, custom, event and default: no quest source
    Studio studio("life-sources", {"guard"});
    const sim::rules::ResolvedNpc resolved = studio.odyssey->npcClasses().resolve(studio.odyssey->level().characters[0]);
    CHECK(resolved.classActions == std::vector<std::string>{"patrol"});
    CHECK(resolved.customActions.empty());
}
