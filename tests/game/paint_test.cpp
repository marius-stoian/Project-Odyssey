// US-124 Paint ground tiles.
#include "game/editor.h"
#include "game/odyssey_game.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <random>
#include <vector>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::PointerButton;

namespace {

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// The pointer at a screen point, with the left button going down, held, or coming up.
Intents mouse(int x, int y, bool press, bool hold, bool release) {
    Intents intents;
    luna::engine::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    const auto left = static_cast<std::size_t>(PointerButton::Left);
    pointer.pressed[left] = press;
    pointer.held[left] = hold;
    pointer.released[left] = release;
    intents.setPointer(pointer);
    return intents;
}

// Where on screen the centre of map cell (x, y) is, in the editor's view now.
std::pair<int, int> onScreen(const game::Editor& editor, int x, int y) {
    const auto view = editor.camera().view();
    return {x * game::kTileSize + game::kTileSize / 2 - view.x, y * game::kTileSize + game::kTileSize / 2 - view.y};
}

void click(game::OdysseyGame& odyssey, int cx, int cy) {
    const auto [x, y] = onScreen(odyssey.editor(), cx, cy);
    odyssey.update(mouse(x, y, true, true, false));
    odyssey.update(mouse(x, y, false, false, true));
}

// A copy of the valley to edit and save without touching the real one.
fs::path valleyCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us124" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy_file(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels" / "valley.json", folder / "valley.json");
    return folder / "valley.json";
}

struct Opened {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Opened(const fs::path& level) : odyssey(ODYSSEUS_DATA_DIR, level) {
        odyssey.start(renderer);
        odyssey.update(pressing(Intent::ModeEditor));
    }
};

} // namespace

TEST_CASE("US-124 Paint") {
    Opened opened(valleyCopy("paint"));
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    const int water = odyssey.definitions().tileNumber("water");
    editor.setTile(water);
    // A click paints one cell.
    click(odyssey, 33, 32); // just east of the hero's start (32, 32)
    CHECK(editor.level().at(33, 32) == water);
    CHECK(editor.history().size() == 1);
    // A drag paints every cell it crosses, even when the pointer jumps between two ticks.
    const auto [ax, ay] = onScreen(editor, 34, 35);
    const auto [bx, by] = onScreen(editor, 40, 35);
    odyssey.update(mouse(ax, ay, true, true, false));
    odyssey.update(mouse(bx, by, false, true, false)); // six cells in one tick
    odyssey.update(mouse(bx, by, false, false, true));
    for (int x = 34; x <= 40; ++x) CHECK(editor.level().at(x, 35) == water);
    CHECK(editor.history().size() == 2); // one stroke, one step of Undo
    // The eraser paints the default ground.
    editor.setTool(game::EditorTool::Eraser);
    click(odyssey, 40, 35);
    CHECK(editor.level().at(40, 35) == editor.level().defaultGround);
    // Clicks on the palette never paint the map beneath it.
    editor.setTool(game::EditorTool::Brush);
    const int before = static_cast<int>(editor.history().size());
    odyssey.update(mouse(10, 30, true, true, false));
    odyssey.update(mouse(10, 30, false, false, true));
    CHECK(static_cast<int>(editor.history().size()) == before);

    // Solid ground blocks walking in Game mode: the water east of the start stops the hero.
    odyssey.update(pressing(Intent::ModeGame));
    Intents right;
    right.set(Intent::MoveRight, true, false);
    for (int i = 0; i < 40; ++i) odyssey.update(right);
    CHECK(odyssey.hero().feetX() < 33 * game::kTileSize);
}

TEST_CASE("US-124 Fill") {
    Opened opened(valleyCopy("fill"));
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    const int sand = odyssey.definitions().tileNumber("sand");
    editor.setTile(sand);
    SUBCASE("a rectangle, dragged from corner to corner, in one step") {
        editor.setTool(game::EditorTool::Rectangle);
        const auto [ax, ay] = onScreen(editor, 36, 33);
        const auto [bx, by] = onScreen(editor, 39, 35);
        odyssey.update(mouse(ax, ay, true, true, false));
        odyssey.update(mouse(bx, by, false, true, false));
        odyssey.update(mouse(bx, by, false, false, true));
        for (int y = 33; y <= 35; ++y)
            for (int x = 36; x <= 39; ++x) CHECK(editor.level().at(x, y) == sand);
        CHECK(editor.history().size() == 1);
    }
    SUBCASE("a flood fill: the whole pond, and only the pond, in one step") {
        editor.setTool(game::EditorTool::Fill);
        const int water = odyssey.definitions().tileNumber("water");
        int pond = 0;
        for (const int cell : editor.level().ground) pond += cell == water ? 1 : 0;
        REQUIRE(pond > 5);
        editor.enter(42 * game::kTileSize, 23 * game::kTileSize); // look at the pond
        click(odyssey, 42, 23);                                   // its middle
        int left = 0;
        int sandy = 0;
        for (const int cell : editor.level().ground) {
            left += cell == water ? 1 : 0;
            sandy += cell == sand ? 1 : 0;
        }
        CHECK(left == 0);
        CHECK(sandy == pond);
        CHECK(editor.history().size() == 1);
        CHECK(editor.undo());
        for (const int cell : editor.level().ground) CHECK(cell != sand);
    }
}

TEST_CASE("US-124 Undo") {
    // Random edits on a small level; after each, the level is remembered. Undo and redo must
    // step exactly through those remembered levels, however they are mixed.
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    for (unsigned seed = 1; seed <= 5; ++seed) {
        game::Level level = game::makeLevel("Test", 12, 10, 0);
        game::Editor editor(level, definitions, fs::temp_directory_path() / "odysseus-us124-undo.json", 480, 270);
        std::mt19937 random(seed);
        auto pick = [&](int n) { return static_cast<int>(random() % static_cast<unsigned>(n)); };
        const int kinds = static_cast<int>(definitions.tiles.size());
        std::vector<game::Level> model{level};
        std::size_t at = 0; // which remembered level we are on
        for (int step = 0; step < 120; ++step) {
            const int action = pick(10);
            if (action < 6) { // a new edit: a stroke, a rectangle or a fill
                std::vector<game::CellChange> changes;
                const int tile = pick(kinds);
                if (action < 2) {
                    changes = game::rectangleFill(level, pick(12), pick(10), pick(12), pick(10), tile);
                } else if (action < 4) {
                    changes = game::floodFill(level, pick(12), pick(10), tile);
                } else {
                    for (const auto& [x, y] : game::lineCells(pick(12), pick(10), pick(12), pick(10))) {
                        if (level.at(x, y) != tile) changes.push_back({x, y, level.at(x, y), tile});
                    }
                }
                if (changes.empty()) continue;
                editor.run(std::make_unique<game::PaintCommand>("edit", std::move(changes)));
                model.resize(at + 1); // what was undone is gone
                model.push_back(level);
                ++at;
            } else if (action < 8) {
                const bool undone = editor.undo();
                CHECK(undone == (at > 0));
                if (undone) --at;
            } else {
                const bool redone = editor.redo();
                CHECK(redone == (at + 1 < model.size()));
                if (redone) ++at;
            }
            REQUIRE(level == model[at]);
        }
    }
    // At most 100 steps are kept: the 101st edit makes the first one permanent.
    game::Level level = game::makeLevel("Long", 12, 10, 0);
    game::Editor editor(level, definitions, fs::temp_directory_path() / "odysseus-us124-long.json", 480, 270);
    for (int i = 0; i < 101; ++i) {
        editor.run(std::make_unique<game::PaintCommand>("edit", std::vector<game::CellChange>{{i % 12, i / 12, level.at(i % 12, i / 12), 1}}));
    }
    int undone = 0;
    while (editor.undo()) ++undone;
    CHECK(undone == 100);
    CHECK(level.at(0, 0) == 1);
}

TEST_CASE("US-124 Save") {
    const fs::path file = valleyCopy("save");
    Opened opened(file);
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    editor.setTile(odyssey.definitions().tileNumber("lava"));
    click(odyssey, 30, 30);
    CHECK(editor.unsaved());
    odyssey.update(pressing(Intent::Save)); // Ctrl+S
    CHECK_FALSE(editor.unsaved());
    CHECK(editor.status() == "Saved valley.json");
    CHECK(game::loadLevel(file, odyssey.definitions()).level == editor.level());
    CHECK(fs::exists(fs::path(file.string() + ".bak1"))); // the previous version is kept
    // G hides and shows the grid.
    const bool grid = editor.gridShown();
    odyssey.update(pressing(Intent::ToggleGrid));
    CHECK(editor.gridShown() != grid);
}
