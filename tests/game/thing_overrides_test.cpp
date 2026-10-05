// US-173 Attach to placed things: a conversation for a placed character, own interaction values for one placed plant, saved with the level.
#include "camp.h"

#include "game/editor.h"

#include <memory>
#include <string>

using namespace camp_support;

TEST_CASE("US-173 Override: only the plant with its own regrow time waits 60 s") {
    Spec spec;
    spec.plants.push_back({"wheat", 2, 0});
    spec.wheatOverrides = {{"gather", "delay", 60000}};
    Camp camp("us173-override", {}, true, {}, spec);
    REQUIRE(camp.odyssey.plants().size() == 2);
    REQUIRE(camp.odyssey.plants()[0].overrides.size() == 1);
    REQUIRE(camp.odyssey.plants()[1].overrides.empty());

    // The plant with its own value: gather takes 3 s as before, then it regrows after 60 s, not 15.
    REQUIRE(game::startInteraction(camp.odyssey, "gather", game::plantSubject(camp.odyssey, 0)));
    camp.play(60);
    CHECK(camp.odyssey.plants()[0].state == "picked");
    camp.play(300); // 15 s: the ordinary wait is over
    CHECK(camp.odyssey.plants()[0].state == "picked");
    camp.play(899);
    CHECK(camp.odyssey.plants()[0].state == "picked");
    camp.play(1); // 60 s after the pick
    CHECK(camp.odyssey.plants()[0].state == "ripe");

    // The other plant of the same kind is not changed.
    REQUIRE(game::startInteraction(camp.odyssey, "gather", game::plantSubject(camp.odyssey, 1)));
    camp.play(60);
    CHECK(camp.odyssey.plants()[1].state == "picked");
    camp.play(300);
    CHECK(camp.odyssey.plants()[1].state == "ripe");
}

TEST_CASE("US-173 Override: a duration of its own changes how long the job takes, for that plant only") {
    Spec spec;
    spec.plants.push_back({"wheat", 2, 0});
    spec.wheatOverrides = {{"gather", "duration", 1000}};
    Camp camp("us173-duration", {}, true, {}, spec);
    REQUIRE(game::startInteraction(camp.odyssey, "gather", game::plantSubject(camp.odyssey, 0)));
    camp.play(20); // 1 s
    CHECK(camp.odyssey.plants()[0].state == "picked");
    REQUIRE(game::startInteraction(camp.odyssey, "gather", game::plantSubject(camp.odyssey, 1)));
    camp.play(20);
    CHECK(camp.odyssey.plants()[1].state == "ripe"); // still working: 3 s
    camp.play(40);
    CHECK(camp.odyssey.plants()[1].state == "picked");
}

TEST_CASE("US-173 Saved: own values are saved with the level and loaded again; a level without them is saved as it was") {
    const fs::path data = dataCopy("us173-saved");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.plants.clear();
    level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 32, level.heroStart.y}, {{"gather", "delay", 60000}, {"gather", "duration", 2500}}});
    level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 64, level.heroStart.y}, {}});
    const fs::path file = data.parent_path() / "overrides-level.json";
    game::saveLevel(level, definitions, file);
    const std::string text = readText(file);
    CHECK(text.find("\"delay\"") != std::string::npos);
    CHECK(text.find("\"value\": 60") != std::string::npos);
    CHECK(text.find("\"value\": 2.5") != std::string::npos);
    const game::Level again = game::loadLevel(file, definitions).level;
    REQUIRE(again.plants.size() == 2);
    CHECK(again.plants[0].overrides == level.plants[0].overrides);
    CHECK(again.plants[1].overrides.empty());
    // A plant with none writes no `overrides` at all.
    level.plants.erase(level.plants.begin());
    game::saveLevel(level, definitions, file);
    CHECK(readText(file).find("overrides") == std::string::npos);
}

TEST_CASE("US-173 Saved: a mistake in an override names the file and the field") {
    const fs::path data = dataCopy("us173-bad");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.plants.clear();
    level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 32, level.heroStart.y}, {{"gather", "delay", 60000}}});
    const fs::path file = data.parent_path() / "bad-overrides.json";
    game::saveLevel(level, definitions, file);
    std::string text = readText(file);
    const std::size_t at = text.find("\"delay\"");
    REQUIRE(at != std::string::npos);
    text.replace(at, 7, "\"speed\"");
    writeText(file, text);
    try {
        game::loadLevel(file, definitions);
        FAIL("the level should not load");
    } catch (const std::exception& e) {
        const std::string message = e.what();
        CHECK(message.find("plants[0].overrides[0].field") != std::string::npos);
    }
}

TEST_CASE("US-173 Attach: the Editor sets an own value as one step of Undo, and says what is wrong with a mistake") {
    const fs::path data = dataCopy("us173-editor");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.plants.clear();
    level.plants.push_back({level.nextId++, "wheat", {level.heroStart.x + 32, level.heroStart.y}, {}});
    const int id = level.plants[0].id;
    game::Editor editor(level, definitions, data.parent_path() / "editor-level.json", 480, 270);
    editor.setActionIds({"gather", "inspect"});
    editor.select(id);
    CHECK(editor.setSelectedOverrides("gather.delay=60"));
    REQUIRE(level.plants[0].overrides.size() == 1);
    CHECK(level.plants[0].overrides[0].valueMilli == 60000);
    CHECK(editor.selectedOverridesText() == "gather.delay=60");
    CHECK_FALSE(editor.setSelectedOverrides("gather.speed=2"));
    CHECK_FALSE(editor.setSelectedOverrides("nothing.delay=2"));
    CHECK_FALSE(editor.setSelectedOverrides("gather.delay=soon"));
    CHECK(level.plants[0].overrides.size() == 1); // the mistakes changed nothing
    CHECK(editor.setSelectedOverrides("gather.delay=60 gather.duration=2.5"));
    CHECK(level.plants[0].overrides.size() == 2);
    CHECK(editor.undo());
    CHECK(level.plants[0].overrides.size() == 1);
    CHECK(editor.undo());
    CHECK(level.plants[0].overrides.empty());
    CHECK(editor.redo());
    CHECK(editor.setSelectedOverrides(""));
    CHECK(level.plants[0].overrides.empty());
}

TEST_CASE("US-173 Attach: a conversation picked for a placed character is the one talking to it starts") {
    const fs::path data = dataCopy("us173-attach");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.characters.clear();
    level.pickups.clear();
    game::PlacedCharacter wanderer{level.nextId++, "wanderer", {level.heroStart.x + 60, level.heroStart.y}, game::Facing::South, "Ossa", 60, 4, {}};
    const int ossa = wanderer.id;
    level.characters = {wanderer};
    const fs::path file = data / "attach-level.json";
    game::saveLevel(level, definitions, file);
    luna::engine::RecordingRenderer renderer;
    game::OdysseyGame odyssey(data, file);
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);
    luna::engine::Intents toEditor;
    toEditor.set(luna::engine::Intent::ModeEditor, true, true);
    odyssey.update(toEditor);
    odyssey.editor().select(ossa);
    REQUIRE(odyssey.editor().selectedIsNpc());
    // The pick list is the folder's conversations.
    const std::vector<std::string> names = odyssey.editor().graphs().dialogueNames();
    CHECK(std::find(names.begin(), names.end(), "elder-fire") != names.end());
    // Nothing yet; then the pick; then the game talks with elder-fire.
    CHECK(odyssey.npcDialogueFor(ossa) == nullptr);
    odyssey.editor().setSelectedDialogue("player", "elder-fire.dlg");
    luna::engine::Intents toGame;
    toGame.set(luna::engine::Intent::ModeGame, true, true);
    odyssey.update(toGame);
    const odysseus::sim::rules::DlgScript* script = odyssey.npcDialogueFor(ossa);
    REQUIRE(script != nullptr);
    CHECK(script->name == "elder-fire");
    // Saved and loaded: still attached.
    odyssey.update(toEditor);
    REQUIRE(odyssey.editor().save());
    const game::Level again = game::loadLevel(file, definitions).level;
    CHECK(again.characters[0].dialogues == std::vector<std::pair<std::string, std::string>>{{"player", "elder-fire.dlg"}});
}
