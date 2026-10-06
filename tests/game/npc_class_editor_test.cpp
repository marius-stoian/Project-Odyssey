// US-260 NPC Classes in the Editor: create and save a class, refuse to delete one that placed NPCs use, report a bad file and load the rest.
#include "camp.h"

#include "game/npc_class_book.h"
#include "sim/npc_class.h"

#include <functional>
#include <memory>

using namespace camp_support;
namespace rules = odysseus::sim::rules;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// A game in the Editor on a copy of the data folder and a plain copy of the valley with two placed goblins.
struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name, const std::function<void(const fs::path&)>& adjust = {}) : data(dataCopy(name)) {
        if (adjust) adjust(data);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.characters.push_back({level.nextId++, "goblin", {level.heroStart.x + 64, level.heroStart.y}, game::Facing::South, "Ossa", 60, 4, {"trader", "elder"}});
        level.characters.push_back({level.nextId++, "goblin", {level.heroStart.x + 96, level.heroStart.y}, game::Facing::South, "Brek", 60, 4, {"trader"}});
        level.characters.push_back({level.nextId++, "goblin", {level.heroStart.x + 128, level.heroStart.y}, game::Facing::South, "Idle", 60, 4, {}});
        game::saveLevel(level, definitions, data / "classes-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "classes-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    }
};

} // namespace

TEST_CASE("US-260 Create: a new class is written to its file and offered in the catalog") {
    Studio studio("npc-class-create");
    game::Editor& editor = studio.odyssey->editor();
    editor.showClasses(true);
    CHECK(editor.classesShown());
    CHECK(studio.odyssey->npcClasses().catalog().find("healer") == nullptr);

    editor.newClass();
    CHECK(editor.classDraftIsNew());
    rules::NpcClass& draft = editor.classDraft();
    draft.id = "healer";
    draft.label = "Healer";
    draft.colour = 0x3A9A4C; // green
    draft.icon = "cross";
    draft.tags = {"healer"};
    CHECK(editor.saveClass());

    CHECK(fs::exists(studio.data / "npc-classes" / "healer.json"));
    const rules::NpcClass* saved = studio.odyssey->npcClasses().catalog().find("healer");
    REQUIRE(saved != nullptr);
    CHECK(saved->colour == 0x3A9A4C);
    CHECK(saved->tags == std::vector<std::string>{"healer"});
    CHECK(studio.odyssey->npcClasses().catalog().ids().size() == 8); // the 7 shipped ones and the healer
    CHECK_FALSE(editor.classDraftIsNew());

    // A second class with the same id is refused, and a mistake (an unknown icon) never reaches the disk.
    editor.newClass();
    editor.classDraft().id = "healer";
    CHECK_FALSE(editor.saveClass());
    editor.classDraft().id = "seer";
    editor.classDraft().icon = "banana";
    CHECK_FALSE(editor.saveClass());
    CHECK(editor.status().find("icon") != std::string::npos);
    CHECK_FALSE(fs::exists(studio.data / "npc-classes" / "seer.json"));

    // Editing: select, change, save, and the file holds the change.
    editor.selectClass("healer");
    editor.classDraft().label = "Wise healer";
    CHECK(editor.saveClass());
    CHECK(readText(studio.data / "npc-classes" / "healer.json").find("Wise healer") != std::string::npos);
}

TEST_CASE("US-260 Delete in use: the Editor refuses and names both NPCs; an unused class goes") {
    Studio studio("npc-class-delete");
    game::Editor& editor = studio.odyssey->editor();
    editor.selectClass("trader");
    CHECK_FALSE(editor.deleteClass());
    CHECK(editor.status().find("Ossa") != std::string::npos);
    CHECK(editor.status().find("Brek") != std::string::npos);
    CHECK(editor.status().find("Idle") == std::string::npos);
    CHECK(fs::exists(studio.data / "npc-classes" / "trader.json"));
    CHECK(game::NpcClassBook::usersOf("elder", editor.level()) == std::vector<std::string>{"Ossa"});

    editor.selectClass("guard"); // nobody uses it
    CHECK(editor.deleteClass());
    CHECK_FALSE(fs::exists(studio.data / "npc-classes" / "guard.json"));
    CHECK(studio.odyssey->npcClasses().catalog().find("guard") == nullptr);
}

TEST_CASE("US-260 Mistake: a class file with an unknown icon is named, the other classes load, F5 keeps the good ones") {
    Studio studio("npc-class-mistake", [](const fs::path& data) {
        writeText(data / "npc-classes" / "weird.json",
                  "{\n  \"id\": \"weird\",\n  \"label\": \"Weird\",\n  \"colour\": \"#101010\",\n  \"icon\": \"banana\",\n  \"tags\": []\n}\n");
    });
    const game::NpcClassBook& book = studio.odyssey->npcClasses();
    REQUIRE(book.report().errors.size() == 1);
    CHECK(book.report().errors[0].text().find("npc-classes/weird.json:5:") == 0);
    CHECK(book.catalog().find("weird") == nullptr);
    CHECK(book.catalog().find("trader") != nullptr);

    // F5 with the mistake still there changes nothing; once fixed, F5 loads the class.
    CHECK_FALSE(studio.odyssey->npcClasses().reload());
    CHECK(book.catalog().find("trader") != nullptr);
    writeText(studio.data / "npc-classes" / "weird.json",
              "{\n  \"id\": \"weird\",\n  \"label\": \"Weird\",\n  \"colour\": \"#101010\",\n  \"icon\": \"eye\",\n  \"tags\": []\n}\n");
    studio.odyssey->update(reloadPressed());
    CHECK(book.catalog().find("weird") != nullptr);
}

TEST_CASE("US-260 Level: placed NPCs keep their classes (level version 4); older levels load without them") {
    const fs::path data = dataCopy("npc-class-level");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.characters.clear();
    level.characters.push_back({level.nextId++, "goblin", {100, 100}, game::Facing::South, "Ossa", 60, 4, {"trader", "elder"}});
    level.characters.push_back({level.nextId++, "goblin", {140, 100}, game::Facing::South, "Plain", 60, 4, {}});
    game::saveLevel(level, definitions, data / "classes.json");
    const std::string text = readText(data / "classes.json");
    CHECK(text.find("\"levelVersion\": 7") != std::string::npos);
    CHECK(game::readLevelFile(data / "classes.json", definitions) == level);
    CHECK(game::readLevelFile(data / "classes.json", definitions).characters[0].classes == std::vector<std::string>{"trader", "elder"});
    // Only NPCs with classes write the field.
    CHECK(text.find("\"classes\"") != std::string::npos);
    CHECK(text.find("\"classes\"", text.find("\"classes\"") + 1) == std::string::npos);
    // The demo level (an older version) has no classes.
    for (const auto& placed : game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level.characters) CHECK(placed.classes.empty());
}
