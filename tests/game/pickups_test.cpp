// US-134 Pickups and the hotbar.
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
using luna::engine::PointerButton;

namespace {

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

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

std::pair<int, int> onScreen(const game::Editor& editor, int worldX, int worldY) {
    const auto view = editor.camera().view();
    return {worldX - view.x, worldY - view.y};
}

void clickAt(game::OdysseyGame& odyssey, int x, int y) {
    odyssey.update(mouse(x, y, true, true, false));
    odyssey.update(mouse(x, y, false, false, true));
}

// The demo level with its own pickups replaced by these weapons, all lying on the hero's start.
fs::path levelWith(const std::string& name, const std::vector<std::string>& weapons) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us134" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.pickups.clear();
    for (const std::string& weapon : weapons) level.pickups.push_back({level.nextId++, weapon, level.heroStart});
    game::saveLevel(level, definitions, folder / "level.json");
    return folder / "level.json";
}

struct Play {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Play(const fs::path& level) : odyssey(ODYSSEUS_DATA_DIR, level) { odyssey.start(renderer); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
};

int paletteIndex(const game::Editor& editor, const std::string& weapon) {
    const auto& names = editor.weaponPalette();
    const auto found = std::find(names.begin(), names.end(), weapon);
    REQUIRE(found != names.end());
    return static_cast<int>(found - names.begin());
}

std::string fileText(const fs::path& file) {
    std::stringstream text;
    text << std::ifstream(file, std::ios::binary).rdbuf();
    return text.str();
}

} // namespace

TEST_CASE("US-134 Place") {
    const fs::path file = levelWith("place", {});
    Play play(file);
    play.odyssey.update(pressing(Intent::ModeEditor));
    game::Editor& editor = play.odyssey.editor();
    REQUIRE(editor.level().pickups.empty());
    editor.setTool(game::EditorTool::Weapon);
    editor.setWeapon(paletteIndex(editor, "flame sword"));
    const int firstFree = editor.level().nextId;
    const auto [x, y] = onScreen(editor, 1100, 1090);
    clickAt(play.odyssey, x, y);
    REQUIRE(editor.level().pickups.size() == 1);
    CHECK(editor.level().pickups[0].weapon == "flame sword");
    CHECK(editor.level().pickups[0].at == game::PixelPoint{1100, 1090});
    CHECK(editor.level().pickups[0].id == firstFree);
    CHECK(editor.selected() == firstFree);

    // Saved as version 3, and read back the same.
    CHECK(editor.save());
    CHECK(fileText(file).find("\"levelVersion\": 6") != std::string::npos);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    CHECK(game::loadLevel(file, definitions).level.pickups == editor.level().pickups);

    // Undo and redo, like characters; the id is never given again.
    CHECK(editor.undo());
    CHECK(editor.level().pickups.empty());
    CHECK(editor.redo());
    CHECK(editor.level().pickups.size() == 1);
    CHECK(editor.undo());
    clickAt(play.odyssey, x, y);
    CHECK(editor.level().pickups.back().id == firstFree + 1);
}

TEST_CASE("US-134 Move and delete") {
    Play play(levelWith("move", {}));
    play.odyssey.update(pressing(Intent::ModeEditor));
    game::Editor& editor = play.odyssey.editor();
    editor.setTool(game::EditorTool::Weapon);
    editor.setWeapon(paletteIndex(editor, "iron spear"));
    const auto [x, y] = onScreen(editor, 1100, 1090);
    clickAt(play.odyssey, x, y);
    const int id = editor.level().pickups.at(0).id;
    // Select it and drag it 40 pixels east: one step of Undo.
    editor.setTool(game::EditorTool::Select);
    play.odyssey.update(mouse(x, y, true, true, false));
    play.odyssey.update(mouse(x + 20, y, false, true, false));
    play.odyssey.update(mouse(x + 40, y, false, true, false));
    play.odyssey.update(mouse(x + 40, y, false, false, true));
    CHECK(editor.level().pickups.at(0).at == game::PixelPoint{1140, 1090});
    CHECK(editor.selected() == id);
    CHECK(editor.undo());
    CHECK(editor.level().pickups.at(0).at == game::PixelPoint{1100, 1090});
    CHECK(editor.redo());
    // Delete removes it; Undo brings it back.
    play.odyssey.update(pressing(Intent::Delete));
    CHECK(editor.level().pickups.empty());
    CHECK(editor.undo());
    CHECK(editor.level().pickups.size() == 1);
}

TEST_CASE("US-134 Level versions") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    const fs::path folder = fs::temp_directory_path() / "odysseus-us134" / "versions";
    fs::remove_all(folder);
    fs::create_directories(folder);
    // A version 1 file (the valley as it was before pickups) still loads, with none.
    std::string v1 = fileText(ODYSSEUS_DEMO_LEVEL);
    const std::size_t pickups = v1.find(" \"pickups\"");
    REQUIRE(pickups != std::string::npos);
    v1.erase(pickups, v1.find(" ],", pickups) + 4 - pickups);
    v1.replace(v1.find("\"levelVersion\": 2"), 17, "\"levelVersion\": 1");
    std::ofstream(folder / "v1.json", std::ios::binary) << v1;
    const game::Level old = game::readLevelFile(folder / "v1.json", definitions);
    CHECK(old.pickups.empty());
    CHECK(old.characters.size() == 1);
    // Saving it writes version 3 and keeps everything else.
    game::saveLevel(old, definitions, folder / "upgraded.json");
    CHECK(fileText(folder / "upgraded.json").find("\"levelVersion\": 6") != std::string::npos);
    CHECK(game::readLevelFile(folder / "upgraded.json", definitions) == old);
    // A pickup naming a weapon that does not exist says where.
    std::string bad = fileText(ODYSSEUS_DEMO_LEVEL);
    bad.replace(bad.find("Spear throw"), 11, "laser cannon");
    std::ofstream(folder / "bad.json", std::ios::binary) << bad;
    try {
        (void)game::readLevelFile(folder / "bad.json", definitions);
        FAIL("an unknown weapon should be refused");
    } catch (const odysseus::sim::DataError& error) {
        CHECK(std::string(error.what()).find("pickups[0].weapon") != std::string::npos);
    }
}

TEST_CASE("US-134 Pick up") {
    Play play(levelWith("pickup", {"iron sword", "katana"}));
    REQUIRE(play.odyssey.carriedCount() == 0); // empty hands at the start
    REQUIRE(play.odyssey.pickupsLeft() == 2);
    play.tick();
    CHECK(play.odyssey.hotbar()[0] == "iron sword"); // the first free slot, then the next
    CHECK(play.odyssey.hotbar()[1] == "katana");
    CHECK(play.odyssey.hotbar()[2].empty());
    CHECK(play.odyssey.pickupsLeft() == 0);
    CHECK(play.odyssey.heldName() == "iron sword"); // slot 1 is held, now filled
    CHECK(play.odyssey.effects().count() == 2); // One short spark per pickup (D-23).
    // A pickup is gone for good until the level restarts.
    play.tick(5);
    CHECK(play.odyssey.carriedCount() == 2);
    // A restart: the hotbar is emptied and the pickups lie in the level again, so the hero takes them
    // anew (two weapons, not four) on the first tick of the new game.
    play.odyssey.update(pressing(Intent::ModeEditor));
    CHECK(play.odyssey.pickupsLeft() == 0);
    play.odyssey.update(pressing(Intent::ModeGame));
    CHECK(play.odyssey.carriedCount() == 2);
    CHECK(play.odyssey.hotbar()[2].empty());
}

TEST_CASE("US-134 Full hotbar") {
    std::vector<std::string> ten(10, "iron sword");
    ten[9] = "katana";
    Play play(levelWith("full", ten));
    play.tick();
    CHECK(play.odyssey.carriedCount() == 9);
    CHECK(play.odyssey.pickupsLeft() == 1); // the tenth stays lying
    CHECK(play.odyssey.hotbarFullShown());
    play.tick(60);
    CHECK_FALSE(play.odyssey.hotbar()[8] == "katana");
    CHECK(play.odyssey.pickupsLeft() == 1);
    // Standing on it keeps the message up; dropping nothing, taking nothing.
    CHECK(play.odyssey.hotbarFullShown());
}

TEST_CASE("US-134 Select") {
    Play play(levelWith("select", {"iron sword", "katana", "wooden longbow"}));
    play.tick();
    REQUIRE(play.odyssey.heldName() == "iron sword");
    // Keys 1-9 hold that slot; a free slot means empty hands.
    play.tick(1, pressing(Intent::Slot3));
    CHECK(play.odyssey.heldName() == "wooden longbow");
    play.tick(1, pressing(Intent::Slot2));
    CHECK(play.odyssey.heldName() == "katana");
    play.tick(1, pressing(Intent::Slot9));
    CHECK(play.odyssey.heldSlot() == 8);
    CHECK(play.odyssey.heldName() == "Empty hands");
    CHECK(play.odyssey.heldWeapon() == nullptr);
    // Shift goes to the next filled slot and wraps round, skipping the free ones.
    play.tick(1, pressing(Intent::SwitchWeapon));
    CHECK(play.odyssey.heldName() == "iron sword");
    play.tick(1, pressing(Intent::SwitchWeapon));
    CHECK(play.odyssey.heldName() == "katana");
    play.tick(1, pressing(Intent::SwitchWeapon));
    CHECK(play.odyssey.heldName() == "wooden longbow");
    play.tick(1, pressing(Intent::SwitchWeapon));
    CHECK(play.odyssey.heldName() == "iron sword");
}

TEST_CASE("US-134 Hotbar drawn") {
    Play play(levelWith("drawn", {"iron sword", "katana", "wooden longbow"}));
    play.tick();
    play.renderer.clear();
    play.odyssey.render(play.renderer, 1.0);
    // Three weapons in the hotbar: three 18-pixel icons, at the bottom centre of the 960 x 540 picture.
    int icons = 0;
    for (const auto& draw : play.renderer.draws()) {
        if (draw.styled && draw.destination.width == 18) {
            ++icons;
            CHECK(draw.destination.y > 460);
            CHECK(draw.destination.x > 200);
            CHECK(draw.destination.x < 760);
        }
    }
    CHECK(icons == 3);
}
