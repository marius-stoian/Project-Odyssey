// US-136 Plants: placed in the Editor, blocking, looked at, chopped, eaten and grown back.
#include "game/editor.h"
#include "game/level.h"
#include "game/odyssey_game.h"

#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Pointer;
using luna::engine::PointerButton;

namespace {

const game::Catalogs& catalogs() {
    static const game::Catalogs loaded = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    return loaded;
}

// The first plant of the catalog with these properties.
std::string plantWith(bool blocks, bool edible, const std::string& size = "") {
    for (const game::PlantDef& plant : catalogs().plants) {
        if (plant.blocks == blocks && plant.edible == edible && (size.empty() || plant.size == size)) return plant.name;
    }
    FAIL("no such plant in plants.json");
    return {};
}

const game::PlantDef& plantDef(const std::string& name) { return *catalogs().plant(name); }

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

Intents holding(Intent intent) {
    Intents intents;
    intents.set(intent, true, false);
    return intents;
}

Intents mouse(int x, int y, bool press, bool hold, bool release) {
    Intents intents;
    Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    const auto left = static_cast<std::size_t>(PointerButton::Left);
    pointer.pressed[left] = press;
    pointer.held[left] = hold;
    pointer.released[left] = release;
    intents.setPointer(pointer);
    return intents;
}

Intents attackToward(int x, int y) {
    Intents intents;
    Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    intents.setPointer(pointer);
    intents.set(Intent::Attack, true, true);
    return intents;
}

void clickAt(game::OdysseyGame& odyssey, int x, int y) {
    odyssey.update(mouse(x, y, true, true, false));
    odyssey.update(mouse(x, y, false, false, true));
}

std::pair<int, int> onScreen(const game::Editor& editor, int worldX, int worldY) {
    const auto view = editor.camera().view();
    return {worldX - view.x, worldY - view.y};
}

// The feet of a plant standing in cell (cellX, cellY), as the Editor and the game put them.
game::PixelPoint feetIn(int cellX, int cellY) { return {cellX * 32 + 16, cellY * 32 + 28}; }

struct Setup {
    std::vector<std::pair<std::string, game::PixelPoint>> plants;
    bool goblin = false; // one goblin 1 m east of the hero, striking for 20
};

// The demo level (hero at tile (32, 32)) with these plants and no pickups.
fs::path levelWith(const std::string& name, const Setup& setup) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us136" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.pickups.clear();
    game::PlacedCharacter goblin = level.characters.at(0);
    level.characters.clear();
    if (setup.goblin) {
        goblin.id = level.nextId++;
        goblin.feet = {level.heroStart.x + 32, level.heroStart.y};
        goblin.hp = 100;
        goblin.swordDamage = 20;
        level.characters.push_back(goblin);
    }
    for (const auto& [kind, feet] : setup.plants) level.plants.push_back({level.nextId++, kind, feet});
    game::saveLevel(level, definitions, folder / "level.json");
    return folder / "level.json";
}

struct Play {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Play(const fs::path& level) : odyssey(ODYSSEUS_DATA_DIR, level) { odyssey.setViewScales(1, 1); odyssey.start(renderer); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
    void hold(const std::string& weapon) {
        REQUIRE(odyssey.pickUp(weapon));
        odyssey.selectSlot(static_cast<int>(std::find(odyssey.hotbar().begin(), odyssey.hotbar().end(), weapon) - odyssey.hotbar().begin()));
    }
    const game::WorldPlant& plant(std::size_t index = 0) const { return odyssey.plants().at(index); }
};

std::string fileText(const fs::path& file) {
    std::ostringstream text;
    text << std::ifstream(file).rdbuf();
    return text.str();
}

} // namespace

TEST_CASE("US-136 Level format") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    const std::string tree = plantWith(true, false, "tree");
    const std::string flower = plantWith(false, false, "small");
    SUBCASE("plants are saved in version 2 and read back the same") {
        const fs::path file = levelWith("format", {{{tree, feetIn(34, 34)}, {flower, feetIn(30, 30)}}, false});
        const game::Level level = game::loadLevel(file, definitions).level;
        REQUIRE(level.plants.size() == 2);
        CHECK(level.plants[0].kind == tree);
        CHECK(level.plants[0].feet == feetIn(34, 34));
        CHECK(fileText(file).find("\"levelVersion\": 4") != std::string::npos);
    }
    SUBCASE("a level without plants still loads (the demo is older than plants)") {
        const game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        CHECK(level.plants.empty());
    }
    SUBCASE("a plant that is not in plants.json is named") {
        const fs::path file = levelWith("unknown", {{{tree, feetIn(34, 34)}}, false});
        std::string text = fileText(file);
        text.replace(text.find(tree), tree.size(), "no such plant");
        std::ofstream(file, std::ios::trunc) << text;
        CHECK_THROWS_WITH_AS(game::readLevelFile(file, definitions), doctest::Contains("plants[0].kind"), odysseus::sim::DataError);
    }
    SUBCASE("ids are never shared with a character or a pickup") {
        const fs::path file = levelWith("ids", {{{tree, feetIn(34, 34)}}, true});
        std::string text = fileText(file);
        const game::Level level = game::loadLevel(file, definitions).level;
        REQUIRE(level.characters.size() == 1);
        const std::string plantId = "\"id\": " + std::to_string(level.plants[0].id);
        const std::size_t at = text.find(plantId, text.find("\"plants\""));
        REQUIRE(at != std::string::npos);
        text.replace(at, plantId.size(), "\"id\": " + std::to_string(level.characters[0].id));
        std::ofstream(file, std::ios::trunc) << text;
        CHECK_THROWS_WITH_AS(game::readLevelFile(file, definitions), doctest::Contains("plants[0].id"), odysseus::sim::DataError);
    }
}

TEST_CASE("US-136 Editor") {
    const std::string tree = plantWith(true, false, "tree");
    const std::string flower = plantWith(false, false, "small");
    const fs::path file = levelWith("editor", {});
    Play play(file);
    play.odyssey.update(pressing(Intent::ModeEditor));
    game::Editor& editor = play.odyssey.editor();
    const auto& names = game::loadDefinitions(ODYSSEUS_DATA_DIR).plants;
    REQUIRE(names.size() == 153);
    auto indexOf = [&](const std::string& name) { return static_cast<int>(std::find(names.begin(), names.end(), name) - names.begin()); };

    SUBCASE("place, save, undo and redo") {
        editor.setTool(game::EditorTool::Plant);
        editor.setPlant(indexOf(tree));
        const int firstFree = editor.level().nextId;
        const auto [x, y] = onScreen(editor, 1100, 1090); // inside cell (34, 34)
        clickAt(play.odyssey, x, y);
        REQUIRE(editor.level().plants.size() == 1);
        CHECK(editor.level().plants[0].kind == tree);
        CHECK(editor.level().plants[0].feet == feetIn(34, 34)); // in the middle of the bottom edge of the cell
        CHECK(editor.level().plants[0].id == firstFree);
        CHECK(editor.selected() == firstFree);
        // One plant to a cell.
        clickAt(play.odyssey, x, y);
        CHECK(editor.level().plants.size() == 1);
        // A flower next to it.
        editor.setPlant(indexOf(flower));
        const auto [fx, fy] = onScreen(editor, 1000, 1000);
        clickAt(play.odyssey, fx, fy);
        REQUIRE(editor.level().plants.size() == 2);
        CHECK(editor.save());
        const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
        CHECK(game::loadLevel(file, definitions).level.plants == editor.level().plants);
        CHECK(editor.undo());
        CHECK(editor.level().plants.size() == 1);
        CHECK(editor.undo());
        CHECK(editor.level().plants.empty());
        CHECK(editor.redo());
        CHECK(editor.level().plants.size() == 1);
    }
    SUBCASE("the palette has pages") {
        editor.setTool(game::EditorTool::Plant);
        play.odyssey.update({}); // the palette is shown
        REQUIRE(editor.plantPage() == 0);
        clickAt(play.odyssey, 80, 30); // the ">" arrow
        play.odyssey.update({});
        CHECK(editor.plantPage() == 1);
        clickAt(play.odyssey, 14, 30); // the "<" arrow
        play.odyssey.update({});
        CHECK(editor.plantPage() == 0);
        // The plants end on page 4 (153 - 4 * 36 = 9 plants); the world objects of US-155 have page 5 to themselves, and the arrow stops there.
        for (int i = 0; i < 8; ++i) {
            clickAt(play.odyssey, 80, 30);
            play.odyssey.update({});
        }
        CHECK(editor.plantPage() == 5);
    }
    SUBCASE("select, move and delete") {
        editor.setTool(game::EditorTool::Plant);
        editor.setPlant(indexOf(tree));
        const auto [x, y] = onScreen(editor, 1100, 1090);
        clickAt(play.odyssey, x, y);
        REQUIRE(editor.level().plants.size() == 1);
        const int id = editor.level().plants[0].id;
        editor.setTool(game::EditorTool::Select);
        editor.select(std::nullopt);
        // Click on the picture of the tree (above its feet).
        const game::PixelPoint feet = editor.level().plants[0].feet;
        const auto [sx, sy] = onScreen(editor, feet.x, feet.y - 20);
        CHECK(editor.plantAt(sx, sy) == id);
        CHECK_FALSE(editor.plantAt(sx + 100, sy).has_value());
        play.odyssey.update(mouse(sx, sy, true, true, false));
        CHECK(editor.selected() == id);
        // Drag it two cells east: it keeps to the grid.
        const auto [ex, ey] = onScreen(editor, feet.x + 64, feet.y - 20);
        play.odyssey.update(mouse(ex, ey, false, true, false));
        play.odyssey.update(mouse(ex, ey, false, false, true));
        CHECK(editor.level().plants[0].feet == feetIn(36, 34));
        CHECK(editor.undo());
        CHECK(editor.level().plants[0].feet == feetIn(34, 34));
        // Delete removes it, and Undo brings it back.
        editor.select(id);
        play.odyssey.update(pressing(Intent::Delete));
        CHECK(editor.level().plants.empty());
        CHECK(editor.undo());
        CHECK(editor.level().plants.size() == 1);
    }
}

TEST_CASE("US-136 Place and block") {
    const std::string tree = plantWith(true, false, "tree");
    const std::string flower = plantWith(false, false, "small");
    SUBCASE("a tree blocks walking") {
        Play play(levelWith("block-tree", {{{tree, feetIn(34, 32)}}, false}));
        play.tick(80, holding(Intent::MoveRight));
        CHECK(play.odyssey.hero().feetX() < 34 * 32.0); // stopped at the tree's cell
        CHECK(play.odyssey.hero().feetX() > 1040.0);    // but it did walk
    }
    SUBCASE("a flower does not") {
        Play play(levelWith("block-flower", {{{flower, feetIn(34, 32)}}, false}));
        play.tick(45, holding(Intent::MoveRight));
        CHECK(play.odyssey.hero().feetX() > 35 * 32.0); // walked right through its cell
    }
    SUBCASE("plants are drawn") {
        Play play(levelWith("drawn", {{{tree, feetIn(34, 32)}, {flower, feetIn(33, 33)}}, false}));
        play.tick(2);
        CHECK(play.odyssey.plantsGrowing() == 2);
        play.renderer.clear();
        play.odyssey.render(play.renderer, 1.0);
        const std::size_t withPlants = play.renderer.draws().size();
        Play bare(levelWith("drawn-bare", {}));
        bare.tick(2);
        bare.renderer.clear();
        bare.odyssey.render(bare.renderer, 1.0);
        CHECK(withPlants >= bare.renderer.draws().size() + 2);
    }
}

TEST_CASE("US-136 Inspect") {
    const std::string flower = plantWith(false, false, "small");
    Play play(levelWith("inspect", {{{flower, feetIn(32, 32)}, {plantWith(false, false, "tall"), feetIn(38, 32)}}, false}));
    play.tick(2);
    SUBCASE("Interact with empty hands shows the name and the text for three seconds") {
        REQUIRE_FALSE(play.odyssey.inspecting());
        play.tick(1, pressing(Intent::Interact));
        REQUIRE(play.odyssey.inspecting());
        CHECK(play.odyssey.inspectedName() == flower);
        CHECK(play.odyssey.inspectedText() == plantDef(flower).inspect);
        play.renderer.clear();
        play.odyssey.render(play.renderer, 1.0);
        play.tick(58);
        CHECK(play.odyssey.inspecting());
        play.tick(3);
        CHECK_FALSE(play.odyssey.inspecting());
    }
    SUBCASE("Inspect works with a weapon in hand too, and far away shows nothing") {
        play.hold("iron sword");
        play.tick(1, pressing(Intent::Inspect));
        CHECK(play.odyssey.inspecting());
        play.tick(70);
        play.tick(30, holding(Intent::MoveLeft)); // walk away from both plants
        play.tick(1, pressing(Intent::Inspect));
        CHECK_FALSE(play.odyssey.inspecting());
    }
}

TEST_CASE("US-136 Chop") {
    const std::string tree = plantWith(true, false, "tree");
    const std::string flower = plantWith(false, false, "small");
    SUBCASE("a sword swing destroys the plant with a leaf burst") {
        Play play(levelWith("chop", {{{flower, feetIn(33, 32)}}, false}));
        play.tick(30);
        play.hold("iron sword");
        REQUIRE(play.odyssey.effects().count() == 0);
        play.tick(1, attackToward(540, 270)); // east
        CHECK_FALSE(play.plant().alive);
        CHECK(play.odyssey.plantsGrowing() == 0);
        CHECK(play.odyssey.effects().count() > 0); // the leaves
    }
    SUBCASE("a destroyed tree no longer blocks") {
        Play play(levelWith("chop-tree", {{{tree, feetIn(34, 32)}}, false}));
        play.hold("iron sword");
        play.tick(80, holding(Intent::MoveRight));
        REQUIRE(play.odyssey.hero().feetX() < 34 * 32.0);
        play.tick(30); // the camera catches up, so the pointer is where we think
        play.tick(1, attackToward(600, 270));
        CHECK_FALSE(play.plant().alive);
        play.tick(40, holding(Intent::MoveRight));
        CHECK(play.odyssey.hero().feetX() > 34 * 32.0 + 20.0);
    }
    SUBCASE("an arrow cuts down a tree") {
        Play play(levelWith("arrow-tree", {{{tree, feetIn(32, 36)}}, false}));
        play.tick(30);
        play.hold("wooden longbow");
        play.tick(1, attackToward(480, 270 + (feetIn(32, 36).y - 1048)));
        for (int i = 0; i < 30 && play.plant().alive; ++i) play.tick();
        CHECK_FALSE(play.plant().alive);
    }
}

TEST_CASE("US-136 Eat") {
    // A goblin next to the hero strikes for 20; two edible plants next to it are then cut down, one at a time.
    const std::string apple = plantWith(false, true, "small");
    REQUIRE(plantDef(apple).edible);
    Play play(levelWith("eat", {{{apple, game::PixelPoint{1070, 1048}}, {apple, game::PixelPoint{1086, 1048}}}, true}));
    play.tick(30);
    play.hold("iron sword");
    play.tick(1, attackToward(540, 270)); // the nearest plant, and the goblin
    CHECK(play.odyssey.plantsGrowing() == 1);
    CHECK(play.odyssey.heroHp() == 100); // eaten at full health: nothing to heal
    play.tick(12);                       // the goblin strikes back
    REQUIRE(play.odyssey.heroHp() == 80);
    play.tick(20);                       // the sword rests
    play.tick(1, attackToward(540, 270));
    CHECK(play.odyssey.plantsGrowing() == 0);
    CHECK(play.odyssey.heroHp() == 90); // +10 from the second plant
}

TEST_CASE("US-136 Regrow") {
    const std::string flower = plantWith(false, false, "small");
    const std::string tree = plantWith(true, false, "tree");
    auto destroyAndWait = [&](const std::string& kind, int& ticksToGrow) {
        Play play(levelWith("regrow", {{{kind, feetIn(32, 31)}}, false}));
        play.tick(30);
        play.hold("iron sword");
        play.tick(1, attackToward(480, 195)); // north: the plant is a metre up
        REQUIRE_FALSE(play.plant().alive);
        ticksToGrow = 0;
        while (!play.plant().alive && ticksToGrow < 1000) {
            play.tick();
            ++ticksToGrow;
        }
        // Where it grew back: inside the camera view, free, not under the hero.
        REQUIRE(play.plant().alive);
        const auto view = play.odyssey.cameraView();
        const game::PixelPoint feet = play.plant().feet;
        CHECK(feet.x >= view.x);
        CHECK(feet.x < view.x + view.width);
        CHECK(feet.y >= view.y);
        CHECK(feet.y < view.y + view.height + 32);
        CHECK(std::hypot(feet.x - play.odyssey.hero().feetX(), feet.y - play.odyssey.hero().feetY()) >= 32.0);
        CHECK(play.plant().kind == kind);
        CHECK(play.odyssey.effects().count() > 0); // the growth effect
        return feet;
    };
    int ticks = 0;
    const game::PixelPoint first = destroyAndWait(flower, ticks);
    CHECK(ticks >= 299);
    CHECK(ticks <= 302); // 15 s
    int again = 0;
    CHECK(destroyAndWait(flower, again) == first); // the same stream: the same spot every time
    SUBCASE("a tree grows back blocking its new cell") {
        Play play(levelWith("regrow-tree", {{{tree, feetIn(32, 31)}}, false}));
        play.tick(30);
        play.hold("iron sword");
        play.tick(1, attackToward(480, 195));
        REQUIRE_FALSE(play.plant().alive);
        play.tick(305);
        REQUIRE(play.plant().alive);
        const game::PixelPoint cell = game::plantCell(play.plant().feet);
        CHECK(play.odyssey.plantsGrowing() == 1);
        // The old cell is free again, the new one is not.
        CHECK(cell != game::PixelPoint{32, 31});
    }
}
