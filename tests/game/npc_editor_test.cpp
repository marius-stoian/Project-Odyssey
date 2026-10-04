// US-268 The Editor's NPC panel: classes, attitude, family, dialogues by partner type and action ticks for a placed NPC; only the differences from its classes and
// kind are saved; every change is one step of Undo.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"

#include <functional>
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
    int ossa = 0;

    explicit Studio(const std::string& name) : data(dataCopy(name)) {
        writeText(data / "dialogue" / "npc-ossa.dlg", "@who npc-ossa\n\n=== start\nOssa: Well met, {hero}.\n-> Leave => END\n");
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        game::PlacedCharacter wanderer{level.nextId++, "wanderer", {level.heroStart.x + 60, level.heroStart.y}, game::Facing::South, "Ossa", 60, 4, {}};
        ossa = wanderer.id;
        level.characters = {wanderer};
        file = data / "npc-editor-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
        odyssey->editor().select(ossa);
    }
    game::Editor& editor() { return odyssey->editor(); }
    const game::PlacedCharacter& placed() { return editor().level().characters[0]; }
};

} // namespace

TEST_CASE("US-268 Edit: classes, attitude and a dialogue of its own are saved as differences only, and F1 plays with them") {
    Studio studio("npc-editor-edit");
    game::Editor& editor = studio.editor();
    REQUIRE(editor.selectedIsNpc());
    CHECK(studio.placed().classes.empty());
    // The wanderer kind says talker and neutral: the owner gives it trader and elder, friendly, and its own player dialogue.
    editor.toggleSelectedClass("trader");
    editor.toggleSelectedClass("elder");
    editor.toggleSelectedClass("talker"); // last: an NPC with no class at all would inherit its kind again
    editor.setSelectedAttitude("friendly");
    editor.setSelectedDialogue("player", "npc-ossa.dlg");
    CHECK(studio.placed().classes == std::vector<std::string>{"trader", "elder"});
    CHECK(studio.placed().attitude == "friendly");
    CHECK(studio.placed().dialogues == std::vector<std::pair<std::string, std::string>>{{"player", "npc-ossa.dlg"}});
    CHECK(editor.save());
    // The file holds only those differences.
    const std::string text = readText(studio.file);
    CHECK(text.find("\"classes\"") != std::string::npos);
    CHECK(text.find("\"attitude\": \"friendly\"") != std::string::npos);
    CHECK(text.find("\"dialogues\"") != std::string::npos);
    CHECK(text.find("\"tags\"") == std::string::npos);
    CHECK(text.find("\"actions\"") == std::string::npos);
    CHECK(text.find("\"family\"") == std::string::npos);
    const game::Level reread = game::loadLevel(studio.file, studio.odyssey->definitions()).level;
    CHECK(reread.characters == editor.level().characters);

    // F1: the game plays with them.
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    const sim::rules::ResolvedNpc resolved = studio.odyssey->npcClasses().resolve(studio.odyssey->level().characters[0]);
    CHECK(resolved.classes == std::vector<std::string>{"trader", "elder"});
    CHECK(resolved.attitude == "friendly");
    CHECK(studio.odyssey->attitudeWordOf(studio.ossa) == "friendly");
    CHECK(studio.odyssey->npcDialogueFor(studio.ossa) != nullptr);
}

TEST_CASE("US-268 Differences only: a value equal to what the NPC inherits is not kept") {
    Studio studio("npc-editor-differences");
    game::Editor& editor = studio.editor();
    // Classes equal to the kind's: nothing kept. Attitude equal to the kind's: nothing kept.
    editor.setSelectedClasses({"talker"});
    CHECK(studio.placed().classes.empty());
    editor.setSelectedAttitude("neutral");
    CHECK(studio.placed().attitude.empty());
    // A dialogue equal to what the class gives: nothing kept.
    editor.setSelectedClasses({"trader"});
    sim::rules::NpcClass trader = *studio.odyssey->npcClasses().catalog().find("trader");
    trader.dialogues = {{"player", "npc-ossa.dlg"}};
    REQUIRE_FALSE(studio.odyssey->npcClasses().save(trader).has_value());
    editor.setSelectedDialogue("player", "npc-ossa.dlg");
    CHECK(studio.placed().dialogues.empty());
    editor.setSelectedDialogue("player", "other.dlg"); // a different one is kept
    CHECK(studio.placed().dialogues.size() == 1);
    editor.setSelectedDialogue("player", "");           // the default again
    CHECK(studio.placed().dialogues.empty());
    // An action the class already denies is not denied again; allowing it is kept; the default is the class's own.
    sim::rules::NpcClass elder = *studio.odyssey->npcClasses().catalog().find("elder");
    elder.deny = {"talk"};
    REQUIRE_FALSE(studio.odyssey->npcClasses().save(elder).has_value());
    editor.setSelectedClasses({"elder"});
    editor.setSelectedActionDenied("talk", true);
    CHECK(studio.placed().deny.empty());
    editor.setSelectedActionDenied("talk", false);
    CHECK(studio.placed().allow == std::vector<std::string>{"talk"});
    editor.setSelectedActionDenied("talk", true); // back to the class's own state
    CHECK(studio.placed().allow.empty());
    CHECK(studio.placed().deny.empty());
    // Reset to defaults forgets everything the NPC set itself.
    editor.setSelectedFamily(3);
    editor.setSelectedAttitude("hostile");
    editor.resetSelectedNpc();
    const game::PlacedCharacter& placed = studio.placed();
    CHECK(placed.classes.empty());
    CHECK(placed.attitude.empty());
    CHECK(placed.family == 0);
    CHECK(placed.dialogues.empty());
    CHECK(placed.allow.empty());
    CHECK(placed.deny.empty());
    // Setting a value that changes nothing makes no step of Undo.
    const std::size_t steps = editor.history().size();
    editor.setSelectedFamily(0);
    CHECK(editor.history().size() == steps);
}

TEST_CASE("US-268 Actions: an action unticked for an NPC is gone from its menu") {
    Studio studio("npc-editor-actions");
    game::Editor& editor = studio.editor();
    editor.setSelectedDialogue("player", "npc-ossa.dlg"); // so Talk exists
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    const game::PlacedCharacter* placed = studio.odyssey->placedCharacter(studio.ossa);
    REQUIRE(placed != nullptr);
    const auto menu = [&]() {
        REQUIRE(studio.odyssey->run().openContext(*studio.odyssey, placed->feet.x, placed->feet.y - 20));
        std::vector<std::string> labels;
        for (const auto& entry : studio.odyssey->run().contextEntries()) labels.push_back(entry.label);
        studio.odyssey->run().close();
        return labels;
    };
    CHECK(menu().front() == "Talk");
    // The checkboxes of the panel are the interactions of the registry.
    const std::vector<std::string> ids = studio.odyssey->interactions().all().empty() ? std::vector<std::string>{} : std::vector<std::string>{studio.odyssey->interactions().all().front().id};
    CHECK_FALSE(ids.empty());
    studio.odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    editor.select(studio.ossa);
    editor.setSelectedActionDenied("talk", true);
    CHECK(studio.placed().deny == std::vector<std::string>{"talk"});
    studio.odyssey->update(pressing(luna::engine::Intent::ModeGame));
    placed = studio.odyssey->placedCharacter(studio.ossa);
    const std::vector<std::string> after = menu();
    CHECK(std::find(after.begin(), after.end(), "Talk") == after.end());
    CHECK(std::find(after.begin(), after.end(), "Confront...") != after.end());
}

TEST_CASE("US-268 Undo: five panel changes are taken back by five Undo steps") {
    Studio studio("npc-editor-undo");
    game::Editor& editor = studio.editor();
    const game::PlacedCharacter before = studio.placed();
    const std::size_t steps = editor.history().size();
    editor.toggleSelectedClass("trader");
    editor.setSelectedAttitude("wary");
    editor.setSelectedFamily(7);
    editor.setSelectedDialogue("player", "npc-ossa.dlg");
    editor.setSelectedActionDenied("talk", true);
    CHECK(editor.history().size() == steps + 5);
    CHECK_FALSE(studio.placed() == before);
    for (int i = 0; i < 5; ++i) CHECK(editor.undo());
    CHECK(studio.placed() == before);
    CHECK(editor.history().size() == steps);
    // And redo brings them all back.
    for (int i = 0; i < 5; ++i) CHECK(editor.redo());
    CHECK(studio.placed().family == 7);
    CHECK(studio.placed().deny == std::vector<std::string>{"talk"});
}

TEST_CASE("US-268 Partner types: the dialogue row offers the player, animals, the environment and every class, from data") {
    Studio studio("npc-editor-partners");
    const std::vector<std::string> types = studio.editor().partnerTypes();
    CHECK(types[0] == "player");
    CHECK(types[1] == "animal");
    CHECK(types[2] == "environment");
    for (const char* id : {"trader", "elder", "guard"}) CHECK(std::find(types.begin(), types.end(), std::string("class:") + id) != types.end());
    // A class made later is in the list.
    sim::rules::NpcClass healer;
    healer.id = "healer";
    healer.label = "Healer";
    healer.icon = "cross";
    REQUIRE_FALSE(studio.odyssey->npcClasses().save(healer).has_value());
    CHECK(std::find(studio.editor().partnerTypes().begin(), studio.editor().partnerTypes().end(), "class:healer") != studio.editor().partnerTypes().end());
    // A dialogue for another NPC class is kept per partner type.
    studio.editor().setSelectedDialogue("class:guard", "npc-ossa.dlg");
    CHECK(studio.placed().dialogues == std::vector<std::pair<std::string, std::string>>{{"class:guard", "npc-ossa.dlg"}});
    CHECK(studio.odyssey->npcClasses().resolve(studio.placed()).dialogues.at("class:guard") == "npc-ossa.dlg");
}
