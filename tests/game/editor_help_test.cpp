// US-300 Field tooltips from help.json: every field of the Level, Building, Graph and Story event editors has an entry (purpose, range, example), a broken
// file never stops the Editor, and a field shows its entry after the pointer has rested on it.
#include "camp.h"

#include "game/dialogue_graph.h"
#include "game/editor_help.h"
#include "game/interaction_graph.h"
#include "game/quest_graph.h"
#include "luna/engine/image_io.h"

#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace camp_support;
namespace buildings = odysseus::sim::buildings;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// A game in the Editor on a copy of the data folder and a level with one NPC, one plant and room to build.
struct Studio {
    fs::path data;
    fs::path file;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int npc = 0;
    int plant = 0;

    explicit Studio(const std::string& name, const std::function<void(const fs::path&)>& adjust = {}) : data(dataCopy(name)) {
        if (adjust) adjust(data);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        npc = level.nextId++;
        level.characters.push_back({npc, "goblin", {level.heroStart.x + 64, level.heroStart.y}, game::Facing::South, "Ossa", 60, 4, {"trader"}});
        plant = level.nextId++;
        level.plants.push_back({plant, "wheat", {level.heroStart.x + 96, level.heroStart.y}});
        file = data / "help-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        odyssey->update(pressing(luna::engine::Intent::ModeEditor));
    }
    game::Editor& editor() { return odyssey->editor(); }
    // Two ticks: the panels are built by the first, and given their help at the start of the next.
    void tick() {
        odyssey->update({});
        odyssey->update({});
    }
};

// A 5 x 5 square of free cells near the hero start, for a building's middle.
std::pair<int, int> freeSpot(const game::Level& level, const luna::engine::TileMap& map) {
    const int hx = level.heroStart.x / game::kTileSize;
    const int hy = level.heroStart.y / game::kTileSize;
    for (int radius = 4; radius < 14; ++radius) {
        for (int y = hy - radius; y <= hy + radius; ++y) {
            for (int x = hx - radius; x <= hx + radius; ++x) {
                bool free = true;
                for (int dy = -2; dy <= 2 && free; ++dy) {
                    for (int dx = -2; dx <= 2; ++dx) free = free && level.inside(x + dx, y + dy) && !map.isSolid(x + dx, y + dy) && map.obstacleHeight(x + dx, y + dy) == 0.0;
                }
                if (free) return {x, y};
            }
        }
    }
    FAIL("no free spot");
    return {0, 0};
}

// The folder named by the environment variable ODYSSEUS_EVIDENCE_DIR, or empty: a story's screenshots are written there only when asked for.
std::string evidenceFolder() {
#ifdef _MSC_VER
    char* value = nullptr;
    std::size_t size = 0;
    std::string folder;
    if (_dupenv_s(&value, &size, "ODYSSEUS_EVIDENCE_DIR") == 0 && value != nullptr) folder = value;
    std::free(value);
    return folder;
#else
    const char* value = std::getenv("ODYSSEUS_EVIDENCE_DIR");
    return value != nullptr ? value : "";
#endif
}

std::string joined(const std::vector<std::string>& names) {
    std::ostringstream out;
    for (const std::string& name : names) out << name << "  ";
    return out.str();
}

// Opens every panel of every editor, one after the other, so each field is built (and given its help) at least once.
void tour(Studio& studio) {
    game::Editor& editor = studio.editor();
    // Level settings, economy.
    editor.showSettings(true);
    editor.showEconomy(true);
    studio.tick();
    // NPC Classes: the classes tab with a draft, then the kinds tab.
    editor.showClasses(true);
    editor.newClass();
    studio.tick();
    editor.showKinds(true);
    editor.selectKind("goblin");
    studio.tick();
    editor.showClasses(false);
    // A placed NPC (properties, NPC panel, trade section), then a placed plant (its own values).
    editor.select(studio.npc);
    studio.tick();
    editor.select(studio.plant);
    studio.tick();
    editor.select(std::nullopt);
    // Buildings: the Prefab tab, then a placed building.
    editor.buildings().showPrefabs(true);
    editor.buildings().newPrefab();
    studio.tick();
    editor.buildings().showPrefabs(false);
    editor.setTool(game::EditorTool::Building);
    editor.buildings().setKind(0);
    const auto [cx, cy] = freeSpot(editor.level(), studio.odyssey->tileMap());
    if (editor.buildings().placeAt(studio.odyssey->tileMap(), cx, cy) && !editor.level().buildings.empty()) {
        const game::PlacedBuildingSpec& placed = editor.level().buildings.front();
        editor.buildings().selectAt(placed.x * game::kTileSize + 4, placed.y * game::kTileSize + 4);
    }
    studio.tick();
    // The graph editor: the file panel and each card of the three kinds, and the Test-play card.
    game::GraphEditor& graphs = editor.graphs();
    graphs.show(true);
    const auto cards = [&](game::GraphEditor::Kind kind, const std::string& name, const std::vector<std::string>& types) {
        graphs.showKind(kind);
        if (!graphs.open(name)) graphs.createNew(name);
        studio.tick();
        for (const std::string& type : types) {
            graphs.addCard(type);
            studio.tick();
        }
    };
    cards(game::GraphEditor::Kind::Dialogue, "elder-fire", {game::dlg_card::kNode, game::dlg_card::kLine, game::dlg_card::kChoice, game::dlg_card::kCondition, game::dlg_card::kEffect,
                                                              game::dlg_card::kGoto, game::dlg_card::kComment});
    graphs.showTest(true);
    studio.tick();
    cards(game::GraphEditor::Kind::Interaction, "help-tour-verb", {game::rule_card::kActor, game::rule_card::kVerb, game::rule_card::kTarget,
                                                                     game::rule_card::kRequirement, game::rule_card::kEffects, game::rule_card::kNpcRule,
                                                                     game::rule_card::kChronicle});
    cards(game::GraphEditor::Kind::Quest, "help-tour-quest", {game::quest_card::kQuest, game::quest_card::kStep, game::quest_card::kBranch, game::quest_card::kEnd,
                                                                game::quest_card::kRequires, game::quest_card::kFail, game::quest_card::kRewards});
    graphs.show(false);
    // The Story events list.
    game::StoryEventEditor& events = editor.storyEvents();
    events.show(true);
    if (events.files().empty() || !events.open(events.files().front())) events.createNew("help-tour");
    studio.tick();
    events.show(false);
}

} // namespace

TEST_CASE("US-300 Format: the shipped help.json is clean and every entry has a purpose and an example") {
    game::EditorHelp help;
    help.load(fs::path(ODYSSEUS_DATA_DIR) / "editor" / "help.json");
    CHECK_MESSAGE(help.problems().empty(), joined(help.problems()));
    CHECK_FALSE(help.entries().empty());
    for (const auto& [id, entry] : help.entries()) {
        CHECK_MESSAGE(!entry.purpose.empty(), id);
        CHECK_MESSAGE(!entry.example.empty(), id);
    }
}

TEST_CASE("US-300 Ids: a field's id is its panel and its label, lower case") {
    CHECK(game::EditorHelp::fieldId("npc", "Sword") == "npc.sword");
    CHECK(game::EditorHelp::fieldId("graph.line", "says: ") == "graph.line.says");
    CHECK(game::EditorHelp::fieldId("event", "who (elder or friend): ") == "event.who");
    CHECK(game::EditorHelp::fieldId("event", "  chronicle ({hero}, {other}): ") == "event.sub-chronicle");
    CHECK(game::EditorHelp::fieldId("npc", "  Does") == "npc.sub-does");
    CHECK(game::EditorHelp::fieldId("npc-life", "Restock/day") == "npc-life.restock-day");
}

TEST_CASE("US-300 Hover: a field shows purpose, range and example from help.json") {
    const fs::path folder = dataCopy("us300-hover").parent_path();
    writeText(folder / "help.json", R"({ // comments are allowed
        "version": 1,
        "fields": {
            "npc.sword": { "purpose": "Damage of one strike", "example": "wolf 8", "suggest": "number" },
            "npc.name": { "purpose": "The name shown over the head", "range": "up to 18 letters", "example": "Ossa", "suggest": "none" } } })");
    game::EditorHelp help;
    help.load(folder / "help.json");
    REQUIRE(help.problems().empty());

    luna::engine::Panel panel({0, 0, 200, 60});
    auto& sword = panel.add<luna::engine::NumberField>(luna::engine::Rect{2, 2, 150, 11}, "Sword", 4, 0, 999, [](int) {});
    auto& name = panel.add<luna::engine::TextField>(luna::engine::Rect{2, 16, 150, 11}, "Name", "Ossa", 18, [](const std::string&) {});
    auto& other = panel.add<luna::engine::TextField>(luna::engine::Rect{2, 30, 150, 11}, "Colour", "", 7, [](const std::string&) {});
    help.apply(panel, "npc");
    CHECK(sword.helpId == "npc.sword");
    CHECK(sword.tip.text == "Damage of one strike\nRange: 0 to 999\nExample: wolf 8"); // the range of a number comes from the field itself
    CHECK(name.tip.text == "The name shown over the head\nRange: up to 18 letters\nExample: Ossa");
    CHECK(other.tip.text.empty()); // no entry: no tooltip
    CHECK(help.missing() == std::vector<std::string>{"npc.colour"});
}

TEST_CASE("US-300 Broken file: no tooltips, the status line names file and line, nothing crashes") {
    Studio studio("us300-broken", [](const fs::path& data) { writeText(data / "editor" / "help.json", "{ \"version\": 1,\n  \"fields\": {\n    \"npc.sword\": { \"purpose\": \n} }"); });
    game::Editor& editor = studio.editor();
    CHECK(studio.odyssey->editorHelp().entries().empty());
    REQUIRE_FALSE(studio.odyssey->editorHelp().problems().empty());
    CHECK(studio.odyssey->editorHelp().problem().rfind("help.json:", 0) == 0);
    CHECK(editor.status().find("help.json:") != std::string::npos);
    editor.select(studio.npc);
    studio.tick(); // the panels still build and the Editor still runs
    CHECK(editor.selected().has_value());
}

TEST_CASE("US-300 Missing file: the Editor opens without tooltips and says so") {
    Studio studio("us300-missing", [](const fs::path& data) { fs::remove(data / "editor" / "help.json"); });
    CHECK(studio.odyssey->editorHelp().entries().empty());
    CHECK(studio.editor().status().find("help.json") != std::string::npos);
    studio.tick();
}

TEST_CASE("US-300 Mistakes: a missing purpose, an unknown source and a wrong version are named with their line") {
    const fs::path folder = dataCopy("us300-mistakes").parent_path();
    writeText(folder / "help.json", "{ \"version\": 2,\n \"fields\": {\n  \"a.b\": { \"example\": \"x\" },\n  \"a.c\": { \"purpose\": \"p\", \"example\": \"x\", \"suggest\": \"catalog:nope\" } } }");
    game::EditorHelp help;
    help.load(folder / "help.json");
    const std::string all = joined(help.problems());
    CHECK(all.find("help.json:1: version must be 1") != std::string::npos);
    CHECK(all.find("help.json:3: \"a.b\" has no purpose") != std::string::npos);
    CHECK(all.find("help.json:4: \"a.c\": \"nope\" is not a catalog") != std::string::npos);
    CHECK(help.entries().empty()); // all or nothing
}

TEST_CASE("US-300 Coverage: every field of the Level, Building, Graph and Story event editors has a help entry, and every entry has a field") {
    Studio studio("us300-coverage");
    REQUIRE_MESSAGE(studio.odyssey->editorHelp().problems().empty(), joined(studio.odyssey->editorHelp().problems()));
    tour(studio);
    const game::EditorHelp& help = studio.odyssey->editorHelp();
    CHECK_MESSAGE(help.missing().empty(), "fields without a help entry: " << joined(help.missing()));
    std::vector<std::string> unused;
    for (const auto& [id, entry] : help.entries()) {
        if (!help.asked().contains(id)) unused.push_back(id);
    }
    CHECK_MESSAGE(unused.empty(), "help entries no field uses: " << joined(unused));
    std::vector<std::string> unwired; // an entry that offers values must have reached its field's list (US-302)
    for (const auto& [id, entry] : help.entries()) {
        if (entry.suggest != "none" && !help.wired().contains(id)) unwired.push_back(id);
    }
    CHECK_MESSAGE(unwired.empty(), "entries that offer values but no field got them: " << joined(unwired));
    MESSAGE("fields met: " << help.asked().size());
}

TEST_CASE("US-300 Screen: resting the pointer on Sword in the NPC panel shows its tooltip in the Editor") {
    const fs::path data = dataCopy("us300-screen");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.characters.clear();
    level.pickups.clear();
    const int npc = level.nextId++;
    level.characters.push_back({npc, "goblin", {level.heroStart.x + 64, level.heroStart.y}, game::Facing::South, "Ossa", 60, 4, {"trader"}});
    game::saveLevel(level, definitions, data / "screen-level.json");

    luna::engine::ImageRenderer renderer(960, 540);
    game::OdysseyGame odyssey(data, data / "screen-level.json");
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);
    odyssey.update(pressing(luna::engine::Intent::ModeEditor));
    odyssey.editor().select(npc);
    odyssey.update({});
    odyssey.update({});

    // The Sword field is the fourth row of the properties panel, top right.
    luna::engine::Pointer pointer;
    pointer.x = 960 - 136 - 2 + 20;
    pointer.y = 18 + 4 + 46 + 4;
    luna::engine::Intents resting;
    resting.setPointer(pointer);
    for (int tick = 0; tick < luna::engine::FieldHint::kDelayTicks + 2; ++tick) odyssey.update(resting);
    renderer.clear({0, 0, 0, 255});
    odyssey.render(renderer, 0.0);
    CHECK(renderer.image().get(pointer.x + 8, pointer.y + 10) == luna::engine::Color{118, 104, 72}); // the tooltip box's border at its corner
    if (const std::string evidence = evidenceFolder(); !evidence.empty()) { // the story's screenshot, when asked for
        fs::create_directories(evidence);
        luna::engine::savePng(renderer.image(), fs::path(evidence) / "tooltip-npc-sword.png");
    }
}
