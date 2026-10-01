// US-126 Level and character settings.
#include "game/editor.h"
#include "game/odyssey_game.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

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

Intents mouse(int x, int y, bool press, bool hold, bool release, const std::string& text = "") {
    Intents intents;
    luna::engine::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    const auto left = static_cast<std::size_t>(PointerButton::Left);
    pointer.pressed[left] = press;
    pointer.held[left] = hold;
    pointer.released[left] = release;
    intents.setPointer(pointer);
    intents.setText(text);
    return intents;
}

fs::path demoCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us126" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy_file(ODYSSEUS_DEMO_LEVEL, folder / "level.json");
    return folder / "level.json";
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

TEST_CASE("US-126 Settings") {
    const fs::path file = demoCopy("settings");
    Opened opened(file);
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    const int water = odyssey.definitions().tileNumber("water");
    editor.run(std::make_unique<game::PaintCommand>("paint", std::vector<game::CellChange>{{5, 5, editor.level().at(5, 5), water}}));
    const game::Level before = editor.level();

    // The width, typed into the settings panel; the height from the program.
    editor.showSettings(true);
    odyssey.update(mouse(-1, -1, false, false, false)); // the panel appears
    const int fieldX = 960 - 152 + 100;
    const int widthY = 22 + 32 + 5;
    odyssey.update(mouse(fieldX, widthY, true, true, false));
    odyssey.update(mouse(fieldX, widthY, false, false, true, "40"));
    odyssey.update(pressing(Intent::Confirm));
    CHECK(editor.level().width == 40);
    editor.setLevelSize(40, 30);
    CHECK(editor.level().height == 30);
    // A resize keeps the painted tiles where they are and drops what falls outside.
    CHECK(editor.level().at(5, 5) == water);
    CHECK(editor.level().ground.size() == 40U * 30U);
    CHECK(editor.level().characters.empty());     // the goblin stood at y = 1048, below 30 tiles
    CHECK(editor.level().heroStart.y < 30 * game::kTileSize);
    // Name and default ground.
    editor.setLevelName("The Small Valley");
    editor.setDefaultGround(odyssey.definitions().tileNumber("sand"));
    CHECK(editor.level().name == "The Small Valley");
    CHECK(editor.level().defaultGround == odyssey.definitions().tileNumber("sand"));
    // Growing again fills the new cells with the default ground.
    editor.setLevelSize(44, 30);
    CHECK(editor.level().at(43, 0) == odyssey.definitions().tileNumber("sand"));
    // Saved and reloaded: the same level.
    CHECK(editor.save());
    CHECK(game::loadLevel(file, odyssey.definitions()).level == editor.level());
    // And every setting is one step of Undo: back to the level as it was.
    for (int i = 0; i < 5; ++i) CHECK(editor.undo());
    CHECK(editor.level() == before);
}

TEST_CASE("US-126 Hero start") {
    Opened opened(demoCopy("start"));
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    editor.setTool(game::EditorTool::Select);
    const auto view = editor.camera().view();
    const int sx = editor.level().heroStart.x - view.x;
    const int sy = editor.level().heroStart.y - 20 - view.y; // on the marker's figure
    odyssey.update(mouse(sx, sy, true, true, false));
    odyssey.update(mouse(sx - 64, sy + 32, false, true, false));
    odyssey.update(mouse(sx - 64, sy + 32, false, false, true));
    CHECK(editor.level().heroStart == game::PixelPoint{1040 - 64, 1048 + 32});
    CHECK(editor.history().size() == 1);
    odyssey.update(pressing(Intent::ModeGame));
    CHECK(odyssey.hero().feetX() == doctest::Approx(1040.0 - 64));
    CHECK(odyssey.hero().feetY() == doctest::Approx(1048.0 + 32));
}

TEST_CASE("US-126 Other levels") {
    const fs::path file = demoCopy("levels");
    Opened opened(file);
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    editor.setLevelName("Changed");
    REQUIRE(editor.unsaved());
    // New, with unsaved changes: it asks first. Cancel keeps everything.
    editor.requestNew();
    CHECK(editor.asking());
    editor.answer(game::Editor::Answer::Cancel);
    CHECK_FALSE(editor.asking());
    CHECK(editor.level().name == "Changed");
    // Discard: a new 32 x 32 level in the first free file, saved from the start.
    editor.requestNew();
    editor.answer(game::Editor::Answer::Discard);
    CHECK(editor.level().width == 32);
    CHECK(editor.levelFile() == file.parent_path() / "level-1.json");
    CHECK(fs::exists(file.parent_path() / "level-1.json"));
    CHECK(odyssey.levelFile() == editor.levelFile());
    CHECK(editor.history().size() == 0); // Undo never crosses into another level
    // Open the first one again (nothing unsaved: no question); it is as it was on disk.
    const auto files = editor.levelFiles();
    REQUIRE(files.size() == 2);
    editor.requestOpen(file);
    CHECK_FALSE(editor.asking());
    CHECK(editor.level().name == "The Valley");
    // Save, answering the question: the changes are kept, then the other level opens.
    editor.setLevelName("Kept");
    editor.requestOpen(file.parent_path() / "level-1.json");
    editor.answer(game::Editor::Answer::Save);
    CHECK(editor.level().width == 32);
    CHECK(game::loadLevel(file, odyssey.definitions()).level.name == "Kept");
    // And the game plays the level the Editor has open.
    odyssey.update(pressing(Intent::ModeGame));
    CHECK(odyssey.level().width == 32);
}

TEST_CASE("US-126 Guide") {
    const fs::path guide = fs::path(ODYSSEUS_DATA_DIR).parent_path().parent_path() / "docs" / "guides" / "editor.md";
    std::ifstream in(guide);
    REQUIRE_MESSAGE(in.good(), guide.string());
    std::stringstream text;
    text << in.rdbuf();
    // Every control of the Editor is explained.
    for (const char* control : {"F1", "F2", "--editor", "--level", "Brush", "Rect", "Fill", "Erase", "Place", "Select", "Level", "Grid", "Undo",
                                "Redo", "Save", "Ctrl+Z", "Ctrl+Y", "Ctrl+S", "Delete", "R", "G", "right mouse button", "Name", "HP", "Sword",
                                "Width", "Height", "New", "Open", "START"}) {
        CHECK_MESSAGE(text.str().find(control) != std::string::npos, control);
    }
}
