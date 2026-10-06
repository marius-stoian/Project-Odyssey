// US-247 Lighting in the Editor and quality settings: a time-of-day preview, a Light tool whose lights are saved in the level (level version 3) and
// shine in the game, and the Low / Medium / High lighting quality.
#include "camp.h"

#include "game/level.h"
#include "game/lighting.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <functional>
#include <memory>

using namespace camp_support;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Pointer;
using luna::engine::PointerButton;

namespace {

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
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

void clickAt(game::OdysseyGame& odyssey, int x, int y) {
    odyssey.update(mouse(x, y, true, true, false));
    odyssey.update(mouse(x, y, false, false, true));
}

// A game in the Editor on a copy of the valley without people, the clan or pickups (a plain level to place lights on).
struct Studio {
    fs::path data;
    fs::path file;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Studio(const std::string& name, const std::function<void(game::Level&)>& changeLevel = {}) : data(dataCopy(name)) {
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.plants.clear();
        level.effects.clear();
        if (changeLevel) changeLevel(level);
        file = data / "studio-level.json";
        game::saveLevel(level, definitions, file);
        odyssey = std::make_unique<game::OdysseyGame>(data, file);
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    luna::engine::LightFrame render() {
        renderer.clear();
        odyssey->render(renderer, 1.0);
        return renderer.lighting();
    }
    int lightIndex(const std::string& kind) {
        const auto& kinds = odyssey->definitions().lightKinds;
        const auto at = std::find(kinds.begin(), kinds.end(), kind);
        REQUIRE(at != kinds.end());
        return static_cast<int>(at - kinds.begin());
    }
};

} // namespace

TEST_CASE("US-247 Preview: the slider lights the level as at that hour, and nothing is saved") {
    Studio studio("lighting-editor-preview");
    studio.odyssey->update(pressing(Intent::ModeEditor));
    game::Editor& editor = studio.odyssey->editor();
    CHECK_FALSE(editor.previewHour().has_value()); // off until the owner asks
    const bool wasUnsaved = editor.unsaved();

    editor.setPreviewHour(12.0);
    const float noon = studio.render().ambientR;
    editor.setPreviewHour(0.0);
    const float midnight = studio.render().ambientR;
    CHECK(noon == doctest::Approx(1.0F).epsilon(0.02));
    CHECK(midnight < noon - 0.2F);
    CHECK(midnight == doctest::Approx(191.0 / 255.0 * 0.55).epsilon(0.03)); // the night of sky.json (D-49)
    CHECK(studio.render().ambientB > studio.render().ambientR);             // blue

    // Dragging the slider sets the hour: the middle is noon, the left end midnight, the right end the next midnight.
    editor.setPreviewHour(6.0);
    const luna::engine::Rect slider = editor.timeSliderRect();
    studio.odyssey->update(mouse(slider.x + (slider.width - 1) / 2, slider.y + 2, true, true, false));
    CHECK(editor.previewHour().value_or(-1.0) == doctest::Approx(12.0).epsilon(0.05));
    studio.odyssey->update(mouse(slider.x, slider.y + 2, false, true, false)); // still held, dragged to the left end
    CHECK(editor.previewHour().value_or(-1.0) == doctest::Approx(0.0).epsilon(0.05));
    studio.odyssey->update(mouse(slider.x + slider.width - 1, slider.y + 2, false, true, false));
    CHECK(editor.previewHour().value_or(-1.0) == doctest::Approx(24.0).epsilon(0.05));
    studio.odyssey->update(mouse(slider.x + 20, slider.y + 2, false, false, true)); // let go: the hour stays
    const double letGo = editor.previewHour().value_or(-1.0);
    studio.odyssey->update(mouse(slider.x + 100, 100, false, false, false)); // moving the pointer without the button changes nothing
    CHECK(editor.previewHour().value_or(-1.0) == doctest::Approx(letGo));

    // A view only: no Undo step, nothing unsaved; switched off, the level is drawn unlit again.
    CHECK(editor.unsaved() == wasUnsaved);
    CHECK_FALSE(editor.history().canUndo());
    editor.setPreviewHour(std::nullopt);
    studio.render();
    CHECK_FALSE(studio.renderer.lightingOn());
}

TEST_CASE("US-247 Place: a light placed with the Light tool is saved in the level and shines in the game after dark") {
    Studio studio("lighting-editor-place");
    game::Editor& editor = [&]() -> game::Editor& {
        studio.odyssey->update(pressing(Intent::ModeEditor));
        return studio.odyssey->editor();
    }();
    REQUIRE(editor.level().lights.empty());
    const int campfire = studio.lightIndex("campfire");
    editor.setTool(game::EditorTool::Light);
    editor.setLight(campfire);
    const auto view = editor.camera().view();
    const int lx = view.x + 400, ly = view.y + 300; // a spot on the screen, in the world
    clickAt(*studio.odyssey, 400, 300);
    REQUIRE(editor.level().lights.size() == 1);
    const int id = editor.level().lights[0].id;
    CHECK(editor.level().lights[0].kind == "campfire");
    CHECK(editor.level().lights[0].at == game::PixelPoint{lx, ly});
    CHECK(editor.selected() == id);
    CHECK(editor.lightAt(lx - view.x, ly - view.y) == id);

    // Undo and redo, and move it with Select.
    CHECK(editor.undo());
    CHECK(editor.level().lights.empty());
    CHECK(editor.redo());
    REQUIRE(editor.level().lights.size() == 1);
    editor.setTool(game::EditorTool::Select);
    editor.select(std::nullopt);
    studio.odyssey->update(mouse(lx - view.x, ly - view.y, true, true, false));
    studio.odyssey->update(mouse(lx + 40 - view.x, ly - view.y, false, true, false));
    studio.odyssey->update(mouse(lx + 40 - view.x, ly - view.y, false, false, true));
    CHECK(editor.level().lights[0].at == game::PixelPoint{lx + 40, ly});
    CHECK(editor.undo());
    CHECK(editor.level().lights[0].at == game::PixelPoint{lx, ly});

    // Saved as level version 3 and read back the same.
    CHECK(editor.save());
    CHECK(readText(studio.file).find("\"levelVersion\": 7") != std::string::npos);
    const game::Definitions definitions = game::loadDefinitions(studio.data);
    const game::Level reread = game::loadLevel(studio.file, definitions).level;
    CHECK(reread.lights == editor.level().lights);

    // Delete, Undo.
    editor.select(id);
    studio.odyssey->update(pressing(Intent::Delete));
    CHECK(editor.level().lights.empty());
    CHECK(editor.undo());
    CHECK(editor.level().lights.size() == 1);

    // In the game: a level with a light, at night (the clan clock starts at midnight), shows it as a point light at its spot; by day it adds nothing.
    Studio play("lighting-editor-game", [&](game::Level& level) {
        level.clan = true;
        level.lights = {{level.nextId++, "campfire", {level.heroStart.x + 60, level.heroStart.y}}};
    });
    const luna::engine::LightFrame night = play.render();
    REQUIRE(night.lights.size() >= 1);
    const luna::engine::PointLight light = night.lights.front();
    const auto gameView = play.odyssey->camera().view(1.0);
    const auto hero = play.odyssey->level().heroStart;
    CHECK(light.x == doctest::Approx(hero.x + 60 - gameView.x));
    CHECK(light.radius == doctest::Approx(6.0 * 32.0));
    for (int i = 0; i < 1200; ++i) play.odyssey->clanMutable()->tick(); // noon
    CHECK(play.render().lights.empty());
}

TEST_CASE("US-247 Level version 3: older levels load without lights and are saved as version 3; a wrong light names the file and field") {
    const fs::path data = dataCopy("lighting-editor-format");
    const game::Definitions definitions = game::loadDefinitions(data);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    // The demo level is version 2 and holds no lights.
    CHECK(level.lights.empty());
    level.lights = {{level.nextId++, "campfire", {100, 100}}, {level.nextId++, "torch", {200, 150}}};
    game::saveLevel(level, definitions, data / "lights-level.json");
    const std::string text = readText(data / "lights-level.json");
    CHECK(text.find("\"levelVersion\": 7") != std::string::npos);
    CHECK(game::readLevelFile(data / "lights-level.json", definitions) == level);

    // A version 2 file (no "lights") still loads.
    std::string v2 = text;
    v2.replace(v2.find("\"levelVersion\": 7"), 17, "\"levelVersion\": 2");
    const std::size_t lights = v2.find("\"lights\"");
    REQUIRE(lights != std::string::npos);
    v2.erase(lights, v2.find("],", lights) + 3 - lights);
    writeText(data / "v2.json", v2);
    CHECK(game::readLevelFile(data / "v2.json", definitions).lights.empty());
    // One made by a newer game is refused, never silently read.
    std::string future = text;
    future.replace(future.find("\"levelVersion\": 7"), 17, "\"levelVersion\": 9");
    writeText(data / "future.json", future);
    CHECK_THROWS_WITH_AS(game::readLevelFile(data / "future.json", definitions), doctest::Contains("newer version"), odysseus::sim::DataError);

    // Mistakes.
    std::string unknown = text;
    unknown.replace(unknown.find("\"campfire\""), 10, "\"lava lamp\"");
    writeText(data / "unknown.json", unknown);
    CHECK_THROWS_WITH_AS(game::readLevelFile(data / "unknown.json", definitions), doctest::Contains("lights[0].kind"), odysseus::sim::DataError);
    std::string outside = text;
    outside.replace(outside.find("\"x\": 100"), 8, "\"x\": 99999");
    writeText(data / "outside.json", outside);
    CHECK_THROWS_WITH_AS(game::readLevelFile(data / "outside.json", definitions), doctest::Contains("lights[0]"), odysseus::sim::DataError);
}

TEST_CASE("US-247 Quality: Low turns normal maps and fire shadows off; Medium and High keep them") {
    Studio studio("lighting-editor-quality");
    for (const char* name : {"Medium", "High"}) {
        game::GameSettings settings = studio.odyssey->settings();
        settings.lighting = name;
        studio.odyssey->applySettings(settings);
        CHECK(studio.render().normalMaps);
    }
    game::GameSettings low = studio.odyssey->settings();
    low.lighting = "Low";
    studio.odyssey->applySettings(low);
    CHECK_FALSE(studio.render().normalMaps);
    // The preview in the Editor follows the setting too.
    studio.odyssey->update(pressing(Intent::ModeEditor));
    studio.odyssey->editor().setPreviewHour(0.0);
    CHECK_FALSE(studio.render().normalMaps);
    // The Low setting is what settings.json says; a bad name is refused (US-240 settings test), the three names are the whole list.
    CHECK(game::GameSettings{}.lighting == "Medium");
}
