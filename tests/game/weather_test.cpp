// US-138 Placed effects and random weather.
#include "game/catalogs.h"
#include "game/editor.h"
#include "game/level.h"
#include "game/odyssey_game.h"
#include "game/weather.h"
#include "sim/data.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
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

fs::path levelWithEffects(const std::string& name, const std::vector<std::pair<std::string, game::PixelPoint>>& effects) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us138" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    level.pickups.clear();
    for (const auto& [effect, at] : effects) level.effects.push_back({level.nextId++, effect, at});
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

std::string fileText(const fs::path& file) {
    std::ostringstream text;
    text << std::ifstream(file).rdbuf();
    return text.str();
}

} // namespace

TEST_CASE("US-138 Weather cycle") {
    const auto& weathers = catalogs().weather;
    REQUIRE(weathers.size() == 101);
    SUBCASE("starts clear, changes every 60 to 120 seconds, fades in over 3 seconds") {
        game::WeatherCycle cycle(weathers, 7);
        CHECK(weathers[static_cast<std::size_t>(cycle.current())].name == "clear");
        CHECK(cycle.ticksUntilChange() >= game::WeatherCycle::kMinTicks);
        CHECK(cycle.ticksUntilChange() <= game::WeatherCycle::kMaxTicks);
        CHECK_FALSE(cycle.fading());
        int ticks = 0;
        std::vector<int> gaps;
        int last = 0;
        while (gaps.size() < 10) {
            const int before = cycle.changes();
            cycle.update();
            ++ticks;
            if (cycle.changes() > before) {
                gaps.push_back(ticks - last);
                last = ticks;
                CHECK(cycle.fading());
                CHECK(cycle.fade() == 0.0);
                // Halfway through the fade the new weather is half there.
                game::WeatherCycle copy = cycle;
                for (int i = 0; i < game::WeatherCycle::kFadeTicks / 2; ++i) copy.update();
                CHECK(copy.fade() == doctest::Approx(0.5));
                for (int i = 0; i < game::WeatherCycle::kFadeTicks / 2; ++i) copy.update();
                CHECK_FALSE(copy.fading());
            }
        }
        for (const int gap : gaps) {
            CHECK(gap >= game::WeatherCycle::kMinTicks);
            CHECK(gap <= game::WeatherCycle::kMaxTicks);
        }
    }
    SUBCASE("clear sky comes about one time in three") {
        game::WeatherCycle cycle(weathers, 11);
        int clear = 0;
        const int changes = 3000;
        while (cycle.changes() < changes) cycle.update();
        for (const int index : cycle.history()) {
            if (weathers[static_cast<std::size_t>(index)].name == "clear") ++clear;
        }
        CHECK(clear > changes * 0.28);
        CHECK(clear < changes * 0.38);
    }
    SUBCASE("the same seed gives the same weathers in the same order, another seed another order") {
        game::WeatherCycle a(weathers, 42);
        game::WeatherCycle b(weathers, 42);
        game::WeatherCycle c(weathers, 43);
        while (a.changes() < 30) a.update();
        while (b.changes() < 30) b.update();
        while (c.changes() < 30) c.update();
        CHECK(a.history() == b.history());
        CHECK(a.history() != c.history());
    }
    SUBCASE("the default seed is the hash of a text, the same every time") {
        CHECK(game::WeatherCycle::seedFromText("The Valley") == game::WeatherCycle::seedFromText("The Valley"));
        CHECK(game::WeatherCycle::seedFromText("The Valley") != game::WeatherCycle::seedFromText("The Meadow"));
    }
}

TEST_CASE("US-138 Weather in the game") {
    Play first(levelWithEffects("weather-a", {}));
    Play second(levelWithEffects("weather-b", {}));
    first.odyssey.setWeatherSeed(5);
    second.odyssey.setWeatherSeed(5);
    // Run past the first change: both games are under the same weather, which fades in over 60 ticks.
    int ticks = 0;
    while (first.odyssey.weather().changes() == 0 && ticks < 3000) {
        first.tick();
        second.tick();
        ++ticks;
    }
    REQUIRE(first.odyssey.weather().changes() == 1);
    CHECK(second.odyssey.weather().changes() == 1);
    CHECK(first.odyssey.weather().history() == second.odyssey.weather().history());
    CHECK(first.odyssey.weather().fading());
    // The weather is drawn over the world: more draws than under a clear sky (unless the new weather is clear).
    const std::string name = catalogs().weather[static_cast<std::size_t>(first.odyssey.weather().current())].name;
    first.tick(30);
    first.renderer.clear();
    first.odyssey.render(first.renderer, 1.0);
    const std::size_t drawn = first.renderer.draws().size();
    Play clear(levelWithEffects("weather-clear", {}));
    clear.tick(2);
    clear.renderer.clear();
    clear.odyssey.render(clear.renderer, 1.0);
    if (name != "clear") CHECK(drawn > clear.renderer.draws().size());
    // The Editor shows no weather.
    first.odyssey.update(pressing(Intent::ModeEditor));
    first.renderer.clear();
    first.odyssey.render(first.renderer, 1.0);
    const std::size_t inEditor = first.renderer.draws().size();
    CHECK(inEditor < drawn + 4000);
}

TEST_CASE("US-138 Placed effects") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    REQUIRE(definitions.loopingEffects.size() == 17);
    const std::string campfire = definitions.hasLoopingEffect("flame") ? "flame" : definitions.loopingEffects[0];
    const std::string fireflies = "fireflies";
    const std::string portal = "portal idle";
    for (const std::string& name : {campfire, fireflies, portal}) CHECK(definitions.hasLoopingEffect(name));
    CHECK_FALSE(definitions.hasLoopingEffect("spark")); // a one-shot effect cannot be placed

    SUBCASE("saved in version 2 and read back the same; an unknown or one-shot effect is refused") {
        const fs::path file = levelWithEffects("format", {{campfire, {1100, 1000}}, {fireflies, {1200, 1100}}});
        const game::Level level = game::loadLevel(file, definitions).level;
        REQUIRE(level.effects.size() == 2);
        CHECK(level.effects[0].name == campfire);
        CHECK(level.effects[1].at == game::PixelPoint{1200, 1100});
        std::string text = fileText(file);
        text.replace(text.find(fireflies), fireflies.size(), "spark");
        std::ofstream(file, std::ios::trunc) << text;
        CHECK_THROWS_WITH_AS(game::readLevelFile(file, definitions), doctest::Contains("effects[1].name"), odysseus::sim::DataError);
    }
    SUBCASE("they loop in the game after a reload") {
        const fs::path file = levelWithEffects("loop", {{campfire, {1100, 1000}}});
        Play play(file);
        play.tick(3);
        CHECK(play.odyssey.effects().count() == 1);
        play.tick(400); // far longer than any one play of the effect: it is still there
        CHECK(play.odyssey.effects().count() == 1);
    }
    SUBCASE("the Editor places, moves, deletes, undoes and saves them") {
        const fs::path file = levelWithEffects("editor", {});
        Play play(file);
        play.odyssey.update(pressing(Intent::ModeEditor));
        game::Editor& editor = play.odyssey.editor();
        const auto& names = definitions.loopingEffects;
        const int index = static_cast<int>(std::find(names.begin(), names.end(), portal) - names.begin());
        editor.setTool(game::EditorTool::Effect);
        editor.setEffect(index);
        const auto view = editor.camera().view();
        clickAt(play.odyssey, 1100 - view.x, 1090 - view.y);
        REQUIRE(editor.level().effects.size() == 1);
        CHECK(editor.level().effects[0].name == portal);
        CHECK(editor.level().effects[0].at == game::PixelPoint{1100, 1090});
        const int id = editor.level().effects[0].id;
        CHECK(editor.selected() == id);
        CHECK(editor.effectAt(1100 - view.x, 1090 - view.y) == id);
        CHECK(editor.save());
        CHECK(game::loadLevel(file, definitions).level.effects == editor.level().effects);
        // Move it with Select, then Undo.
        editor.setTool(game::EditorTool::Select);
        editor.select(std::nullopt);
        play.odyssey.update(mouse(1100 - view.x, 1090 - view.y, true, true, false));
        play.odyssey.update(mouse(1140 - view.x, 1090 - view.y, false, true, false));
        play.odyssey.update(mouse(1140 - view.x, 1090 - view.y, false, false, true));
        CHECK(editor.level().effects[0].at == game::PixelPoint{1140, 1090});
        CHECK(editor.undo());
        CHECK(editor.level().effects[0].at == game::PixelPoint{1100, 1090});
        // Delete, Undo, Redo.
        editor.select(id);
        play.odyssey.update(pressing(Intent::Delete));
        CHECK(editor.level().effects.empty());
        CHECK(editor.undo());
        CHECK(editor.level().effects.size() == 1);
        CHECK(editor.redo());
        CHECK(editor.level().effects.empty());
    }
}
