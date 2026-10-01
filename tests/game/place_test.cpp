// US-125 Place characters.
#include "game/editor.h"
#include "game/odyssey_game.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <filesystem>

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

// A world point (feet) as the editor shows it now.
std::pair<int, int> onScreen(const game::Editor& editor, int worldX, int worldY) {
    const auto view = editor.camera().view();
    return {worldX - view.x, worldY - view.y};
}

void clickAt(game::OdysseyGame& odyssey, int x, int y) {
    odyssey.update(mouse(x, y, true, true, false));
    odyssey.update(mouse(x, y, false, false, true));
}

fs::path demoCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us125" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy_file(ODYSSEUS_DEMO_LEVEL, folder / "level.json");
    return folder / "level.json";
}

int kindIndex(const game::Definitions& definitions, const std::string& name) {
    for (std::size_t i = 0; i < definitions.characters.size(); ++i) {
        if (definitions.characters[i].name == name) return static_cast<int>(i);
    }
    FAIL("no such kind: ", name);
    return -1;
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

TEST_CASE("US-125 Place") {
    Opened opened(demoCopy("place"));
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    editor.setTool(game::EditorTool::Place);
    editor.setKind(kindIndex(odyssey.definitions(), "skeleton"));
    const int firstFree = editor.level().nextId;
    const auto [x, y] = onScreen(editor, 1072, 1100);
    clickAt(odyssey, x, y);
    REQUIRE(editor.level().characters.size() == 2); // the demo's goblin, and the new skeleton
    const game::PlacedCharacter& placed = editor.level().characters.back();
    CHECK(placed.kind == "skeleton");
    CHECK(placed.feet == game::PixelPoint{1072, 1100}); // where the owner clicked
    CHECK(placed.facing == game::Facing::South);
    CHECK(placed.name == "Skeleton");
    CHECK(placed.hp == odyssey.definitions().character("skeleton")->hp);
    CHECK(placed.swordDamage == odyssey.definitions().character("skeleton")->swordDamage);
    CHECK(placed.id == firstFree);
    CHECK(editor.level().nextId == firstFree + 1);
    CHECK(editor.selected() == firstFree); // ready to be edited
    // One step of Undo takes it away; the id is never given again.
    CHECK(editor.undo());
    CHECK(editor.level().characters.size() == 1);
    clickAt(odyssey, x, y);
    CHECK(editor.level().characters.back().id == firstFree + 1);
}

TEST_CASE("US-125 Edit") {
    const fs::path file = demoCopy("edit");
    Opened opened(file);
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    editor.setTool(game::EditorTool::Select);
    const game::PlacedCharacter goblin = editor.level().characters.front();
    // Select: a click on the figure.
    const auto [gx, gy] = onScreen(editor, goblin.feet.x, goblin.feet.y - 20);
    clickAt(odyssey, gx, gy);
    REQUIRE(editor.selected() == goblin.id);
    // Move: drag it 40 pixels right and 32 down; one step of Undo.
    const std::size_t steps = editor.history().size();
    odyssey.update(mouse(gx, gy, true, true, false));
    odyssey.update(mouse(gx + 40, gy + 32, false, true, false));
    odyssey.update(mouse(gx + 40, gy + 32, false, false, true));
    CHECK(editor.level().characters.front().feet == game::PixelPoint{goblin.feet.x + 40, goblin.feet.y + 32});
    CHECK(editor.history().size() == steps + 1);
    // Turn: R, clockwise.
    odyssey.update(pressing(Intent::Rotate));
    CHECK(editor.level().characters.front().facing == game::Facing::SouthWest);
    // Properties: the HP field in the panel, typed into.
    odyssey.update(mouse(-1, -1, false, false, false)); // the panel now shows the goblin
    const auto hpField = std::pair{960 - 136 - 2 + 80, 22 + 32 + 5};
    odyssey.update(mouse(hpField.first, hpField.second, true, true, false));
    odyssey.update(mouse(hpField.first, hpField.second, false, false, true, "250"));
    Intents confirm = pressing(Intent::Confirm);
    odyssey.update(confirm);
    CHECK(editor.level().characters.front().hp == 250);
    editor.setSelectedName("Grak the Loud");
    editor.setSelectedSwordDamage(9);
    CHECK(editor.level().characters.front().name == "Grak the Loud");
    CHECK(editor.level().characters.front().swordDamage == 9);
    // Saved and read back: every change is kept.
    odyssey.update(pressing(Intent::Save));
    const game::Level saved = game::loadLevel(file, odyssey.definitions()).level;
    CHECK(saved.characters == editor.level().characters);
    // Delete removes it; Undo brings it back as it was.
    const auto before = editor.level().characters;
    odyssey.update(pressing(Intent::Delete));
    CHECK(editor.level().characters.empty());
    CHECK_FALSE(editor.selected().has_value());
    CHECK(editor.undo());
    CHECK(editor.level().characters == before);
}

TEST_CASE("US-125 Play") {
    Opened opened(demoCopy("play"));
    game::OdysseyGame& odyssey = opened.odyssey;
    game::Editor& editor = odyssey.editor();
    // A goblin with 12 HP right next to the hero's start, and a wanderer who is not an enemy.
    editor.setTool(game::EditorTool::Place);
    editor.setKind(kindIndex(odyssey.definitions(), "goblin"));
    const auto [ax, ay] = onScreen(editor, 1060, 1048);
    clickAt(odyssey, ax, ay);
    editor.setSelectedHp(12);
    editor.setKind(kindIndex(odyssey.definitions(), "wanderer"));
    const auto [bx, by] = onScreen(editor, 980, 1000);
    clickAt(odyssey, bx, by);
    odyssey.update(pressing(Intent::ModeGame));
    REQUIRE(odyssey.enemies().size() == 2);
    CHECK(odyssey.bystanders().size() == 1);
    const game::Enemy* placed = nullptr;
    for (const game::Enemy& enemy : odyssey.enemies()) {
        if (enemy.maxHp() == 12) placed = &enemy;
    }
    REQUIRE(placed != nullptr);
    // The sword: it takes damage, flashes red, and is defeated at 0 HP.
    odyssey.update(pressing(Intent::SwitchWeapon));
    odyssey.update(pressing(Intent::Interact));
    CHECK(placed->hp() == 12 - 5);
    CHECK(placed->isFlashing());
    for (int i = 0; i < 200 && placed->isAlive(); ++i) {
        odyssey.update(i % 2 == 0 ? pressing(Intent::Interact) : Intents{});
    }
    CHECK_FALSE(placed->isAlive());
    CHECK(placed->hp() == 0);
}
