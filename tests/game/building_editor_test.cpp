// US-256 Prefab editor and placing buildings: the Build tool, the panel of a placed building and the Prefab tab of the Editor.
#include "camp.h"

#include "game/game_rules.h"

#include <algorithm>
#include <memory>
#include <string>

using namespace camp_support;
namespace buildings = odysseus::sim::buildings;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// A game with no run (a plain level): the Editor and the game can switch back and forth.
struct Plain {
    fs::path data;
    fs::path file;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    explicit Plain(const std::string& name) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        file = data / "plain-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
};

// A free spot (a 3 x 3 square of cells that are neither rock nor occupied) near the hero start of the level, for placing a building by its middle.
std::pair<int, int> freeSpot(const game::Level& level, const luna::engine::TileMap& map, const std::vector<buildings::LayoutPiece>* /*unused*/ = nullptr) {
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

void composeLodge(game::Editor& editor) {
    auto& edit = editor.buildings();
    edit.newPrefab();
    edit.setPrefabId("test-lodge");
    edit.setPrefabLabel("Test lodge");
    edit.setSize(4, 3);
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 3; ++y) {
            const bool edge = x == 0 || y == 0 || x == 3 || y == 2;
            if (edge) CHECK(edit.setPiece(x, y, x == 2 && y == 2 ? "door-wood" : "wall-wood"));
            else CHECK(edit.setPiece(x, y, "floor-planks"));
            CHECK(edit.setPiece(x, y, "roof-thatch"));
        }
    }
    edit.setPrefabInterior("map", "lodge-inside");
    edit.togglePrefabUse("shelter");
    edit.togglePrefabUse("sleep");
    edit.setPrefabFlags(true, true);
}

} // namespace

TEST_CASE("US-256 Compose: pieces on the grid, materials, build time, interior mode and uses are saved as a prefab file that appears in the palette") {
    Camp camp("us256-compose");
    game::OdysseyGame& odyssey = camp.odyssey;
    odyssey.update(pressing(luna::engine::Intent::ModeEditor));
    REQUIRE(odyssey.mode() == game::Mode::Editor);
    game::Editor& editor = odyssey.editor();
    auto& edit = editor.buildings();
    REQUIRE(edit.ready());
    const std::size_t kindsBefore = edit.kinds().size();
    CHECK(kindsBefore >= 5);
    composeLodge(editor);
    edit.setPrefabCost("wood=20 fur=2");
    edit.setPrefabSeconds(30);
    CHECK_FALSE(edit.setPrefabCost("wood=banana")); // a mistake is said and changes nothing
    REQUIRE(edit.savePrefab());
    const fs::path file = camp.data / "buildings" / "prefabs" / "test-lodge.json";
    REQUIRE(fs::exists(file));
    const std::string text = readText(file);
    CHECK(text.find("\"interiorLevel\": \"lodge-inside\"") != std::string::npos);
    CHECK(text.find("\"wood\": 20") != std::string::npos);
    CHECK(text.find("\"buildSeconds\": 30") != std::string::npos);

    // It joined the palette (kinds, then prefabs), the data and the level's list of known kinds.
    const std::vector<std::string> kinds = edit.kinds();
    CHECK(kinds.size() == kindsBefore + 1);
    CHECK(kinds.back() == "test-lodge");
    CHECK(odyssey.definitions().hasBuildingKind("test-lodge"));
    const buildings::KindDef* lodge = odyssey.buildings().data().kind("test-lodge");
    REQUIRE(lodge != nullptr);
    CHECK(lodge->prefab);
    CHECK(lodge->cost.at("wood") == 20);
    CHECK(lodge->buildMilli == 30000);
    CHECK(lodge->interior == buildings::InteriorMode::Map);
    CHECK(odyssey.buildings().knows("test-lodge")); // marked known: buildable at once

    // Reading the folder again (the next start) gives the same kind back.
    odysseus::sim::rules::LoadReport report;
    const buildings::BuildingData again = buildings::BuildingData::load(camp.data / "buildings", report);
    CHECK(report.errors.empty());
    REQUIRE(again.kind("test-lodge") != nullptr);
    CHECK(*again.kind("test-lodge") == *lodge);
}

TEST_CASE("US-256 Compose: mistakes are said and nothing is written") {
    Camp camp("us256-mistakes");
    game::OdysseyGame& odyssey = camp.odyssey;
    odyssey.update(pressing(luna::engine::Intent::ModeEditor));
    auto& edit = odyssey.editor().buildings();
    edit.newPrefab();
    CHECK_FALSE(edit.savePrefab()); // no id
    edit.setPrefabId("Bad Id");
    CHECK_FALSE(edit.savePrefab());
    edit.setPrefabId("hut"); // an id of kinds.json
    edit.setPiece(0, 0, "wall-wood");
    CHECK_FALSE(edit.savePrefab());
    CHECK(odyssey.editor().status().find("kinds.json") != std::string::npos);
    edit.setPrefabId("empty");
    edit.setPiece(0, 0, ""); // erase: the grid is empty
    CHECK_FALSE(edit.savePrefab());
    CHECK(odyssey.editor().status().find("at least one piece") != std::string::npos);
    CHECK_FALSE(edit.setPiece(9, 9, "wall-wood")); // outside the 3 x 3 grid
    CHECK(edit.setSize(2, 2));
    CHECK(edit.draft().width == 2);
    CHECK_FALSE(fs::exists(camp.data / "buildings" / "prefabs" / "hut.json"));
    CHECK_FALSE(fs::exists(camp.data / "buildings" / "prefabs" / "empty.json"));
    // Erase takes the top piece first: roof, then wall, then floor.
    edit.setPiece(0, 0, "floor-straw");
    edit.setPiece(0, 0, "wall-wood");
    edit.setPiece(0, 0, "roof-thatch");
    CHECK(edit.draft().layout.size() == 3);
    edit.setPiece(0, 0, "");
    CHECK(edit.draft().layout.size() == 2);
    for (const auto& lp : edit.draft().layout) CHECK(lp.piece != "roof-thatch");
    // A piece of the same layer replaces the one that was there.
    edit.setPiece(0, 0, "door-hide");
    CHECK(edit.draft().layout.size() == 2);
}

TEST_CASE("US-256 Place: the finished building stands in the level and in the game; undo and the panel work; the level round-trips") {
    Plain plain("us256-place");
    game::OdysseyGame& odyssey = *plain.odyssey;
    odyssey.update(pressing(luna::engine::Intent::ModeEditor));
    game::Editor& editor = odyssey.editor();
    composeLodge(editor);
    REQUIRE(editor.buildings().savePrefab());
    const std::vector<std::string> kinds = editor.buildings().kinds();
    const auto at = std::find(kinds.begin(), kinds.end(), "test-lodge");
    REQUIRE(at != kinds.end());
    editor.setTool(game::EditorTool::Building);
    editor.buildings().setKind(static_cast<int>(at - kinds.begin()));
    CHECK(editor.buildings().currentKind() == "test-lodge");
    const auto [cx, cy] = freeSpot(editor.level(), odyssey.tileMap());
    CHECK(editor.buildings().placeAt(odyssey.tileMap(), cx, cy));
    REQUIRE(editor.level().buildings.size() == 1);
    const game::PlacedBuildingSpec placed = editor.level().buildings.front();
    CHECK(placed.kind == "test-lodge");
    CHECK(placed.finished);
    // Another building may not overlap it.
    CHECK_FALSE(editor.buildings().placeAt(odyssey.tileMap(), cx, cy));
    CHECK(editor.status().find("Another building") != std::string::npos);
    // The panel: turn, interior and owner are steps of Undo.
    editor.buildings().selectAt(placed.x * game::kTileSize + 4, placed.y * game::kTileSize + 4);
    REQUIRE(editor.buildings().hasSelection());
    const std::size_t steps = editor.history().size();
    CHECK(editor.buildings().turnSelected());
    CHECK(editor.level().buildings.front().turns == 1);
    CHECK(editor.buildings().setInterior("fade", ""));
    CHECK(editor.buildings().setOwner(4));
    CHECK(editor.history().size() == steps + 3);
    CHECK(editor.level().buildings.front().interior == "fade");
    CHECK(editor.level().buildings.front().owner == 4);
    CHECK_FALSE(editor.buildings().setInterior("sideways", ""));
    CHECK(editor.undo());
    CHECK(editor.level().buildings.front().owner == -1);
    CHECK(editor.redo());
    CHECK(editor.level().buildings.front().owner == 4);

    // Save and reload the level: the building is in its own list.
    REQUIRE(editor.save());
    CHECK(game::loadLevel(editor.levelFile(), odyssey.definitions()).level == editor.level());

    // Back in the game: it stands there finished, its walls block, it is a room with a door.
    odyssey.update(pressing(luna::engine::Intent::ModeGame));
    REQUIRE(odyssey.mode() == game::Mode::Game);
    odyssey.update({});
    const auto& store = odyssey.buildings().store();
    REQUIRE(store.all().size() == 1);
    const buildings::PlacedBuilding& built = store.all().front();
    CHECK(built.state == buildings::State::Finished);
    CHECK(built.owner == 4);
    CHECK(store.interiorMode(built) == "fade"); // the owner's override wins over the prefab's map
    CHECK(store.rooms().size() == 1);
    int blocked = 0;
    for (int y = built.y; y < built.y + built.height; ++y) {
        for (int x = built.x; x < built.x + built.width; ++x) blocked += odyssey.tileMap().obstacleHeight(x, y) > 0.0 ? 1 : 0;
    }
    CHECK(blocked >= 8);

    // Back to the Editor: delete it with the key.
    odyssey.update(pressing(luna::engine::Intent::ModeEditor));
    editor.buildings().selectAt(placed.x * game::kTileSize + 4, placed.y * game::kTileSize + 4);
    luna::engine::Intents del;
    del.set(luna::engine::Intent::Delete, true, true);
    odyssey.update(del);
    CHECK(editor.level().buildings.empty());
}

TEST_CASE("US-256 Blueprint: a prefab marked buildable is offered in the Build menu when a game starts, and one that is not known waits to be learned") {
    const fs::path data = dataCopy("us256-blueprint");
    fs::create_directories(data / "buildings" / "prefabs");
    writeText(data / "buildings" / "prefabs" / "shed.json", R"({ "id": "shed", "label": "Shed", "footprint": [2, 1],
  "layout": [ { "piece": "wall-wood", "x": 0, "y": 0 }, { "piece": "wall-wood", "x": 1, "y": 0 } ], "buildable": true, "known": true })");
    writeText(data / "buildings" / "prefabs" / "secret.json", R"({ "id": "secret", "label": "Secret hall", "footprint": [1, 1],
  "layout": [ { "piece": "wall-wood", "x": 0, "y": 0 } ], "buildable": true, "known": false })");
    writeText(data / "buildings" / "prefabs" / "ruin.json", R"({ "id": "ruin", "label": "Ruin", "footprint": [1, 1],
  "layout": [ { "piece": "wall-wood", "x": 0, "y": 0 } ], "buildable": false, "known": true })");
    Camp camp("us256-blueprint", data);
    game::OdysseyGame& odyssey = camp.odyssey;
    CHECK(odyssey.buildings().notes().empty());
    const std::vector<std::string> menu = odyssey.buildings().menuKinds(false);
    CHECK(std::find(menu.begin(), menu.end(), "shed") != menu.end());
    CHECK(std::find(menu.begin(), menu.end(), "secret") == menu.end()); // not learned yet
    CHECK(std::find(menu.begin(), menu.end(), "ruin") == menu.end());   // not buildable: only the owner places it
    CHECK(odyssey.definitions().hasBuildingKind("ruin"));               // ... which the Editor and levels may
    odyssey.buildings().learn("secret");
    const std::vector<std::string> later = odyssey.buildings().menuKinds(false);
    CHECK(std::find(later.begin(), later.end(), "secret") != later.end());
    // The shed can be built like any blueprint: place it, bring the wood, work on it.
    odyssey.setViewScales(1, 1);
    const auto [hx, hy] = game::BuildingLayer::cellOf(odyssey.hero().feetX(), odyssey.hero().feetY());
    std::pair<int, int> spot{-1, -1};
    for (int x = hx - 6; x <= hx + 6 && spot.first < 0; ++x) {
        if (odyssey.buildings().whyNot(odyssey, "shed", x, hy + 2, 0).empty()) spot = {x, hy + 2};
    }
    REQUIRE(spot.first >= 0);
    odyssey.buildings().select("shed");
    CHECK(odyssey.buildings().placeSelected(odyssey, spot.first, spot.second).empty());
    REQUIRE(odyssey.buildings().store().all().size() == 1);
    odyssey.life()->give("wood", 4);
    odyssey.buildings().deliver(odyssey, odyssey.buildings().store().all().front().id);
    CHECK(odyssey.buildings().store().ruleState(odyssey.buildings().store().all().front()) == "ready");
}

TEST_CASE("US-256 A mistake in a prefab file is shown and the other prefabs still load") {
    const fs::path data = dataCopy("us256-badprefab");
    fs::create_directories(data / "buildings" / "prefabs");
    writeText(data / "buildings" / "prefabs" / "good.json", R"({ "id": "good", "footprint": [1, 1], "layout": [ { "piece": "wall-wood", "x": 0, "y": 0 } ] })");
    writeText(data / "buildings" / "prefabs" / "bad.json", R"({ "id": "bad", "footprint": [1, 1], "layout": [ { "piece": "wall-gold", "x": 0, "y": 0 } ] })");
    Camp camp("us256-badprefab", data);
    CHECK(camp.odyssey.buildings().data().kind("good") != nullptr);
    CHECK(camp.odyssey.buildings().data().kind("bad") == nullptr);
    REQUIRE_FALSE(camp.odyssey.buildings().notes().empty());
    CHECK(camp.odyssey.buildings().notes().front().find("buildings/prefabs/bad.json:1:") != std::string::npos);
    CHECK(camp.odyssey.buildings().notes().front().find("wall-gold") != std::string::npos);
}
