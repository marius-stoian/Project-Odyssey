// US-244 Sun and moon shadows: characters, plants and objects cast shadows that turn and stretch with the light (D-49, D-50).
#include "camp.h"

#include "game/level.h"
#include "luna/engine/renderer.h"
#include "luna/engine/shadow_draw.h"
#include "sim/data.h"

#include <cmath>
#include <memory>

using namespace camp_support;

namespace {

// A level with a clan, an olive tree 10 tiles east of the hero, a bush 10 tiles west and a goblin 6 tiles south. The art is copied too, so the
// plants and the characters have pictures to cast shadows from. The clan clock starts at midnight (an hour is 100 ticks).
struct Day {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    game::PixelPoint hero;

    explicit Day(const std::string& name) : data(dataCopy(name)) {
        fs::create_directories(data.parent_path() / "sprites" / "atlas");
        fs::copy(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "sprites" / "atlas", data.parent_path() / "sprites" / "atlas", fs::copy_options::recursive);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.plants.clear();
        level.effects.clear();
        level.clan = true;
        hero = level.heroStart;
        level.plants.push_back({level.nextId++, "olive tree", {hero.x + 320, hero.y}});
        level.plants.push_back({level.nextId++, "bush", {hero.x - 320, hero.y}});
        game::PlacedCharacter goblin;
        goblin.id = level.nextId++;
        goblin.kind = "goblin";
        goblin.name = "Gob";
        goblin.feet = {hero.x, hero.y + 192};
        level.characters.push_back(goblin);
        game::saveLevel(level, definitions, data / "shadow-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "shadow-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    void advanceToHour(double hours) {
        for (int i = 0; i < static_cast<int>(hours * 100.0); ++i) odyssey->clanMutable()->tick();
    }

    struct Shadow {
        luna::engine::Rect destination;
        int alpha;
    };
    // The shadow strips drawn within `radius` pixels of a thing's feet (world position given in tiles from the hero).
    std::vector<Shadow> shadowsAt(double eastTiles, double southTiles, double radius = 110.0) {
        renderer.clear();
        odyssey->render(renderer, 1.0);
        const auto view = odyssey->camera().view(1.0);
        const double feetX = hero.x + eastTiles * 32.0 - view.x;
        const double feetY = hero.y + southTiles * 32.0 - view.y;
        std::vector<Shadow> found;
        for (const auto& draw : renderer.draws()) {
            if (!draw.styled || !odyssey->isShadowTexture(draw.texture)) continue;
            const double x = draw.destination.x + draw.destination.width / 2.0;
            const double y = draw.destination.y + draw.destination.height / 2.0;
            if (std::hypot(x - feetX, y - feetY) <= radius) found.push_back({draw.destination, draw.style.alpha});
        }
        return found;
    }
    // How far from the feet the shadow reaches, and to which side (negative: west, left).
    struct Reach {
        double length = 0.0;
        double sideways = 0.0;
        double down = 0.0;
        int strips = 0;
    };
    Reach reachOf(double eastTiles, double southTiles) {
        const auto view = odyssey->camera().view(1.0);
        Reach reach;
        const std::vector<Shadow> strips = shadowsAt(eastTiles, southTiles, 160.0);
        const double feetX = hero.x + eastTiles * 32.0 - view.x;
        const double feetY = hero.y + southTiles * 32.0 - view.y;
        double sum = 0.0;
        for (const Shadow& strip : strips) {
            const double x = strip.destination.x + strip.destination.width / 2.0 - feetX;
            const double y = strip.destination.y + strip.destination.height / 2.0 - feetY;
            reach.length = std::max(reach.length, std::hypot(x, y));
            reach.down = std::max(reach.down, y);
            sum += x;
        }
        reach.strips = static_cast<int>(strips.size());
        reach.sideways = strips.empty() ? 0.0 : sum / static_cast<double>(strips.size());
        return reach;
    }
};

} // namespace

TEST_CASE("US-244 Engine: a sprite is sheared along the light, one ground row at a time") {
    luna::engine::RecordingRenderer renderer;
    const luna::engine::Texture black = renderer.createTexture(luna::engine::Image(32, 48));
    // Shadow as long as the sprite is tall (1.0), falling to the east and a little down the screen.
    luna::engine::drawShadow(renderer, black, {0, 0, 32, 48}, {100, 100}, 0.8, 0.6, 1.0, 120);
    const auto& draws = renderer.draws();
    REQUIRE(draws.size() > 4);
    int lowestShift = 1000, highestShift = -1000;
    int previousBottom = 100;
    for (const auto& draw : draws) {
        CHECK(draw.styled);
        CHECK(draw.style.alpha == 120);
        CHECK(draw.destination.y == previousBottom); // rows follow each other: no ground pixel is darkened twice
        previousBottom = draw.destination.y + draw.destination.height;
        const int shift = draw.destination.x - (100 - 16);
        lowestShift = std::min(lowestShift, shift);
        highestShift = std::max(highestShift, shift);
    }
    CHECK(lowestShift >= 0);           // the foot of the shadow is at the feet
    CHECK(highestShift > 20);          // the head of it is far to the east: 0.8 * 48 pixels, give or take a strip
    CHECK(draws.front().source.y > draws.back().source.y); // the feet are drawn first, then up the sprite
    // Falling up the screen (light from the south): the rows go up.
    renderer.clear();
    luna::engine::drawShadow(renderer, black, {0, 0, 32, 48}, {100, 100}, 0.0, -1.0, 1.0, 120);
    REQUIRE_FALSE(renderer.draws().empty());
    CHECK(renderer.draws().front().destination.y < 100);
    // No darkness, no shadow.
    renderer.clear();
    luna::engine::drawShadow(renderer, black, {0, 0, 32, 48}, {100, 100}, 1.0, 0.0, 1.0, 0);
    CHECK(renderer.draws().empty());
}

TEST_CASE("US-244 Day: a tree's shadow points away from the sun and is longer in the morning and evening") {
    Day day("shadow-day");
    day.advanceToHour(9.0);
    const Day::Reach morning = day.reachOf(10.0, 0.0);
    day.advanceToHour(3.0);
    const Day::Reach noon = day.reachOf(10.0, 0.0);
    day.advanceToHour(5.0);
    const Day::Reach evening = day.reachOf(10.0, 0.0);
    REQUIRE(morning.strips > 0);
    REQUIRE(noon.strips > 0);
    REQUIRE(evening.strips > 0);
    CHECK(morning.sideways < -5.0); // the sun is in the east in the morning: the shadow falls west
    CHECK(evening.sideways > 5.0);  // and east in the evening
    CHECK(morning.length > noon.length);
    CHECK(evening.length > noon.length);
    CHECK(noon.down < 0.0 + 1.0);   // at noon the sun is in the south: the shadow falls north (up the picture), not down
}

TEST_CASE("US-244 Casters: characters, plants and objects each cast a shadow sized by their catalog height") {
    Day day("shadow-casters");
    day.advanceToHour(9.0);
    const Day::Reach tree = day.reachOf(10.0, 0.0);
    const Day::Reach bush = day.reachOf(-10.0, 0.0);
    const Day::Reach goblin = day.reachOf(0.0, 6.0);
    CHECK(tree.strips > 0);
    CHECK(bush.strips > 0);
    CHECK(goblin.strips > 0);
    CHECK(tree.length > bush.length); // 4 m against 1.5 m
    // The hero casts one too, at the camera's middle.
    CHECK(day.reachOf(0.0, 0.0).strips > 0);
    // At night the moon's shadows are lighter than the sun's.
    const auto sun = day.shadowsAt(10.0, 0.0);
    REQUIRE_FALSE(sun.empty());
    day.advanceToHour(15.0); // 24:00, midnight
    const auto moon = day.shadowsAt(10.0, 0.0);
    REQUIRE_FALSE(moon.empty());
    CHECK(moon.front().alpha < sun.front().alpha);
}

TEST_CASE("US-244 Overcast: fog and heavy cloud fade the shadows") {
    Day day("shadow-fog");
    day.advanceToHour(9.0);
    const auto clear = day.shadowsAt(10.0, 0.0);
    REQUIRE_FALSE(clear.empty());
    REQUIRE(day.odyssey->setWeatherNamed("fog"));
    CHECK(day.shadowsAt(10.0, 0.0).empty());
    REQUIRE(day.odyssey->setWeatherNamed("overcast bands"));
    CHECK(day.shadowsAt(10.0, 0.0).empty());
    REQUIRE(day.odyssey->setWeatherNamed("sunrise haze")); // haze takes half
    const auto haze = day.shadowsAt(10.0, 0.0);
    REQUIRE_FALSE(haze.empty());
    CHECK(haze.front().alpha < clear.front().alpha);
    REQUIRE(day.odyssey->setWeatherNamed("clear"));
    CHECK_FALSE(day.shadowsAt(10.0, 0.0).empty());
}

TEST_CASE("US-244 Data: height and shadow fields are read, small plants cast nothing, and a mistake names the file and the field") {
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    CHECK(catalogs.plant("olive tree")->height == doctest::Approx(4.0));
    CHECK(catalogs.plant("bush")->height == doctest::Approx(1.5));
    CHECK(catalogs.plant("grass")->height == doctest::Approx(0.0));
    CHECK(catalogs.plant("fire pit")->height == doctest::Approx(0.8));
    CHECK(catalogs.plant("olive tree")->shadow);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    CHECK(definitions.character("hero")->height == doctest::Approx(1.7));
    CHECK(definitions.character("grey wolf")->height == doctest::Approx(1.0)); // an animal
    for (const game::WeatherDef& weather : catalogs.weather) {
        if (weather.name == "fog") CHECK(weather.shadowFade == doctest::Approx(1.0));
        if (weather.name == "clear") CHECK(weather.shadowFade == doctest::Approx(0.0));
    }
    // Edits: a plant with its own height and no shadow, a mistake in a height, a mistake in a shadow flag.
    const fs::path data = dataCopy("shadow-data");
    const std::string plants = readText(data / "plants.json");
    const std::string marker = "{\"name\":\"bush\",";
    const auto at = plants.find(marker);
    REQUIRE(at != std::string::npos);
    std::string edited = plants;
    edited.insert(at + marker.size(), "\"height\":2.5,\"shadow\":false,");
    writeText(data / "plants.json", edited);
    const game::Catalogs changed = game::loadCatalogs(data);
    CHECK(changed.plant("bush")->height == doctest::Approx(2.5));
    CHECK_FALSE(changed.plant("bush")->shadow);
    edited = plants;
    edited.insert(at + marker.size(), "\"height\":500,");
    writeText(data / "plants.json", edited);
    try {
        game::loadCatalogs(data);
        FAIL("a height of 500 metres must be refused");
    } catch (const odysseus::sim::DataError& e) {
        CHECK(std::string(e.what()).find("plants.json") != std::string::npos);
        CHECK(std::string(e.what()).find(".height") != std::string::npos);
    }
}
