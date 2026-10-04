// US-248 Celestial bodies: the sun and the moon are objects of the world; their position decides the light direction (D-50).
#include "camp.h"

#include "game/celestial.h"
#include "game/sky.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <cmath>
#include <functional>
#include <memory>

using namespace camp_support;

namespace {

void replaceInFile(const fs::path& file, const std::string& from, const std::string& to) {
    std::string text = readText(file);
    const auto at = text.find(from);
    REQUIRE_MESSAGE(at != std::string::npos, from);
    text.replace(at, from.size(), to);
    writeText(file, text);
}

struct Shipped {
    fs::path data{ODYSSEUS_DATA_DIR};
    game::SkyData sky = game::loadSky(data / "light" / "sky.json", data / "sim" / "calendar.json");
    game::LightingData lighting = game::loadLighting(data / "light" / "lights.json");
    game::Catalogs catalogs = game::loadCatalogs(data);
    game::CelestialEvents none;
    std::vector<const game::PlantDef*> defaults;

    Shipped() {
        for (const game::PlantDef& def : catalogs.plants) {
            if (def.celestial && def.sky.followsClock) defaults.push_back(&def);
        }
    }
    const game::PlantDef* plant(const std::string& name) const { return catalogs.plant(name); }
    game::CelestialLight lightAt(const std::vector<game::CelestialBody>& bodies, double hour, int season = 0, const game::CelestialEvents* events = nullptr,
                                 int day = 0) const {
        return game::currentLight(bodies, sky, lighting, events != nullptr ? *events : none, {hour, day, season}, 0.0, 0.0);
    }
};

// A level with a clan; the clan clock starts at midnight of day 0.
struct World {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit World(const std::string& name, const std::function<void(const fs::path&)>& adjust = {},
                   const std::function<void(game::Level&)>& changeLevel = {})
        : data(dataCopy(name)) {
        if (adjust) adjust(data);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.clan = true;
        if (changeLevel) changeLevel(level);
        game::saveLevel(level, definitions, data / "celestial-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "celestial-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    void advance(int ticks) {
        for (int i = 0; i < ticks; ++i) odyssey->clanMutable()->tick();
    }
    void advanceToHour(double hour) { advance(static_cast<int>(hour * 100.0)); } // an hour is 100 ticks
    const luna::engine::LightFrame& lit() {
        renderer.clear();
        odyssey->render(renderer, 1.0);
        return renderer.lighting();
    }
};

} // namespace

TEST_CASE("US-248 Default pair: a level that places nothing gets a sun by day and a moon by night, travelling by the clock") {
    const Shipped s;
    REQUIRE(s.defaults.size() == 2);
    const std::vector<game::CelestialBody> bodies = game::bodiesOf(s.defaults, {});
    CHECK(bodies.size() == 2);

    const game::CelestialLight morning = s.lightAt(bodies, 9.0);
    const game::CelestialLight noon = s.lightAt(bodies, 12.0);
    const game::CelestialLight evening = s.lightAt(bodies, 15.5);
    const game::CelestialLight night = s.lightAt(bodies, 0.0);
    REQUIRE(morning.valid);
    CHECK(morning.source == "sun");
    CHECK(night.source == "moon");
    CHECK(night.moon);
    // The sun rises in the east: a shadow falls west (left, x negative) in the morning, east in the evening, north (up) at noon.
    CHECK(morning.dirX < -0.3);
    CHECK(evening.dirX > 0.3);
    CHECK(noon.dirY < -0.9);
    CHECK(std::abs(noon.dirX) < 0.05);
    // Lower sun, longer shadow; the noon sun is highest.
    CHECK(morning.lengthPerHeight > noon.lengthPerHeight);
    CHECK(evening.lengthPerHeight > noon.lengthPerHeight);
    CHECK(noon.elevation == doctest::Approx(s.sky.sunPeakDegrees[0]).epsilon(0.01)); // spring noon
    // The direction follows the clock: it turns smoothly over the day.
    CHECK(s.lightAt(bodies, 10.0).dirX < s.lightAt(bodies, 14.0).dirX);
}

TEST_CASE("US-248 Orbit: the season changes how high the sun climbs; the tilt turns the orbit") {
    const Shipped s;
    const std::vector<game::CelestialBody> bodies = game::bodiesOf(s.defaults, {});
    const game::CelestialLight spring = s.lightAt(bodies, 12.0, 0);
    const game::CelestialLight summer = s.lightAt(bodies, 12.0, 1);
    const game::CelestialLight winter = s.lightAt(bodies, 12.0, 3);
    CHECK(summer.elevation > spring.elevation);
    CHECK(winter.elevation < spring.elevation);
    CHECK(winter.lengthPerHeight > summer.lengthPerHeight);
    // A tilted sun: the same hour gives another direction.
    game::PlantDef tilted = *s.plant("sun");
    tilted.sky.tilt = 30.0;
    const game::CelestialLight turned = s.lightAt({{&tilted, 0.0, 0.0}, {s.plant("moon"), 0.0, 0.0}}, 9.0);
    const game::CelestialLight straight = s.lightAt(bodies, 9.0);
    CHECK(std::abs(turned.dirX - straight.dirX) > 0.05);
}

TEST_CASE("US-248 Placed body: its position, not the default orbit, gives the direction and the elevation") {
    const Shipped s;
    const game::PlantDef* placedSun = s.plant("sun (placed)");
    REQUIRE(placedSun != nullptr);
    REQUIRE_FALSE(placedSun->sky.followsClock);
    // 10 metres east of the reference point (320 pixels), 40 metres up.
    const std::vector<game::CelestialBody> east = game::bodiesOf(s.defaults, {{placedSun, 320.0, 0.0}});
    const game::CelestialLight light = s.lightAt(east, 12.0);
    CHECK(light.source == "sun (placed)");
    CHECK(light.dirX == doctest::Approx(-1.0).epsilon(0.01)); // away from the east
    CHECK(std::abs(light.dirY) < 0.01);
    CHECK(light.elevation == doctest::Approx(std::atan2(40.0, 10.0) * 180.0 / 3.14159265358979).epsilon(0.001));
    // Moved to the south, the shadow falls north (up); the clock no longer matters for a placed sun by day.
    const std::vector<game::CelestialBody> south = game::bodiesOf(s.defaults, {{placedSun, 0.0, 640.0}});
    const game::CelestialLight southLight = s.lightAt(south, 9.0);
    CHECK(southLight.dirY < -0.99);
    CHECK(southLight.elevation == doctest::Approx(std::atan2(40.0, 20.0) * 180.0 / 3.14159265358979).epsilon(0.001));
    // The moon is still the default one at night, because the level places no moon.
    CHECK(s.lightAt(south, 0.0).source == "moon");
}

TEST_CASE("US-248 Low body: the elevation is clamped, so a body on the horizon gives a long but finite shadow with a cap") {
    const Shipped s;
    const game::PlantDef* placedSun = s.plant("sun (placed)");
    // 20 kilometres away and 40 metres up: nearly on the horizon.
    const std::vector<game::CelestialBody> far = game::bodiesOf(s.defaults, {{placedSun, 20000.0 * 32.0, 0.0}});
    const game::CelestialLight light = s.lightAt(far, 12.0);
    REQUIRE(light.valid);
    CHECK(light.elevation >= game::kMinLightElevation);
    CHECK(light.lengthPerHeight <= game::kSunShadowCap + 1e-9);
    CHECK(light.lengthPerHeight == doctest::Approx(game::kSunShadowCap));
    // Straight overhead: no direction at all.
    const std::vector<game::CelestialBody> above = game::bodiesOf(s.defaults, {{placedSun, 0.0, 0.0}});
    const game::CelestialLight overhead = s.lightAt(above, 12.0);
    CHECK(overhead.dirX == 0.0);
    CHECK(overhead.dirY == 0.0);
    CHECK(overhead.lengthPerHeight < 0.05);
}

TEST_CASE("US-248 Several suns: the strongest is used and nothing is summed") {
    Shipped s;
    s.lighting.kinds.push_back({"dim sun", 255, 255, 255, 30.0, 0.4, 200.0, 0.0});
    game::PlantDef weak = *s.plant("sun (placed)");
    weak.name = "weak sun";
    weak.sky.lightKind = "dim sun";
    const game::PlantDef* strong = s.plant("sun (placed)");
    const game::CelestialLight light = s.lightAt({{&weak, 320.0, 0.0}, {strong, 0.0, 640.0}}, 12.0);
    CHECK(light.source == "sun (placed)"); // strength 1.0 beats 0.4
    CHECK(light.strength == doctest::Approx(1.0));
    CHECK(light.dirY < -0.99); // the strong one's direction alone
}

TEST_CASE("US-248 Seen: the sprites appear in the sky band and move with the clock") {
    const Shipped s;
    const std::vector<game::CelestialBody> bodies = game::bodiesOf(s.defaults, {});
    const auto at = [&](double hour) { return game::skySprites(bodies, s.sky, s.none, {hour, 0, 0}, 0.0, 0.0); };
    const auto morning = at(8.0);
    const auto noon = at(12.0);
    const auto evening = at(16.0);
    REQUIRE(morning.size() == 1);
    REQUIRE(noon.size() == 1);
    REQUIRE(evening.size() == 1);
    CHECK(morning.front().name == "sun");
    CHECK(morning.front().x < noon.front().x);
    CHECK(noon.front().x < evening.front().x);
    CHECK(noon.front().y < morning.front().y); // higher at noon
    CHECK(noon.front().x == doctest::Approx(0.5).epsilon(0.02));
    const auto night = at(0.0);
    REQUIRE(night.size() == 1);
    CHECK(night.front().name == "moon");
    CHECK(night.front().moon);
}

TEST_CASE("US-248 Eclipse: the light dims by the stated depth for the stated length and then returns") {
    const Shipped s;
    game::CelestialEvents events;
    events.events.push_back({"sun", 3, 11.0, 2.0, 0.7});
    CHECK(game::eclipseFactor(events, "sun", 3, 10.5) == doctest::Approx(1.0));
    CHECK(game::eclipseFactor(events, "sun", 3, 12.0) == doctest::Approx(0.3)); // the middle holds the full depth
    CHECK(game::eclipseFactor(events, "sun", 3, 11.1) > 0.3);                   // eases in
    CHECK(game::eclipseFactor(events, "sun", 3, 11.1) < 1.0);
    CHECK(game::eclipseFactor(events, "sun", 3, 13.5) == doctest::Approx(1.0)); // and it is over
    CHECK(game::eclipseFactor(events, "sun", 2, 12.0) == doctest::Approx(1.0)); // another day
    CHECK(game::eclipseFactor(events, "moon", 3, 12.0) == doctest::Approx(1.0)); // another body
    const std::vector<game::CelestialBody> bodies = game::bodiesOf(s.defaults, {});
    const game::CelestialLight dimmed = s.lightAt(bodies, 12.0, 0, &events, 3);
    CHECK(dimmed.dimming == doctest::Approx(0.3));
    CHECK(dimmed.strength == doctest::Approx(0.3));
    const game::CelestialLight later = s.lightAt(bodies, 13.5, 0, &events, 3);
    CHECK(later.dimming == doctest::Approx(1.0));
    // An event that runs past midnight: the next day's first hour is still inside it.
    game::CelestialEvents late;
    late.events.push_back({"moon", 0, 23.0, 2.0, 0.5});
    CHECK(game::eclipseFactor(late, "moon", 1, 0.0) == doctest::Approx(0.5));
    CHECK(game::eclipseFactor(late, "moon", 1, 1.5) == doctest::Approx(1.0));
}

TEST_CASE("US-248 Eclipse in the game: the world gets darker for the length of the event and then brightens") {
    World world("eclipse-game", [](const fs::path& data) {
        writeText(data / "light" / "celestial-events.json",
                  "{\"version\":1,\"events\":[{\"body\":\"sun\",\"startDay\":0,\"startHour\":11.0,\"lengthHours\":2.0,\"depth\":0.7}]}");
    });
    world.advanceToHour(10.0);
    const float before = world.lit().ambientR;
    world.advanceToHour(2.0); // 12:00, the middle of the eclipse
    const float during = world.lit().ambientR;
    world.advanceToHour(2.0); // 14:00
    const float after = world.lit().ambientR;
    CHECK(before == doctest::Approx(1.0F).epsilon(0.02));
    CHECK(during == doctest::Approx(0.3F).epsilon(0.05));
    CHECK(after == doctest::Approx(1.0F).epsilon(0.02));
    CHECK(world.odyssey->celestialLight().source == "sun");
}

TEST_CASE("US-248 Game: a placed sun changes the light direction in a played level") {
    World world("placed-game", {}, [](game::Level& level) {
        level.plants.push_back({level.nextId++, "sun (placed)", {level.heroStart.x + 320, level.heroStart.y}}); // 10 m east of the hero
    });
    world.advanceToHour(9.0);
    const game::CelestialLight light = world.odyssey->celestialLight();
    CHECK(light.source == "sun (placed)");
    CHECK(light.dirX == doctest::Approx(-1.0).epsilon(0.02));
    CHECK(light.elevation > 70.0);
    // A placed body is a body in the sky, not a plant on the ground: the player cannot hit or inspect it.
    for (const game::WorldPlant& plant : world.odyssey->plants()) {
        if (plant.def != nullptr && plant.def->celestial) CHECK_FALSE(plant.present());
    }
}

TEST_CASE("US-248 Data: events and entries round-trip, and a mistake names the file and the field") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us248";
    fs::create_directories(folder);
    // Round trip of the events file.
    game::CelestialEvents events;
    events.events.push_back({"sun", 20, 11.5, 2.0, 0.7});
    events.events.push_back({"moon", 41, 22.0, 1.5, 0.6});
    writeText(folder / "celestial-events.json", game::celestialEventsToText(events));
    CHECK(game::loadCelestialEvents(folder / "celestial-events.json") == events);
    // The shipped file loads.
    CHECK(game::loadCelestialEvents(fs::path(ODYSSEUS_DATA_DIR) / "light" / "celestial-events.json").events.size() == 2);
    // A missing file is no events.
    CHECK(game::loadCelestialEvents(folder / "nothing.json").events.empty());
    // Mistakes.
    const auto message = [&](const std::string& text) {
        writeText(folder / "bad.json", text);
        try {
            game::loadCelestialEvents(folder / "bad.json");
        } catch (const odysseus::sim::DataError& e) {
            return std::string(e.what());
        }
        return std::string("no error");
    };
    const std::string good = "{\"body\":\"sun\",\"startDay\":1,\"startHour\":2.0,\"lengthHours\":1.0,\"depth\":0.5}";
    CHECK(message("{\"version\":1,\"events\":[" + good + "]}") == "no error");
    const std::string depth = message("{\"version\":1,\"events\":[{\"body\":\"sun\",\"startDay\":1,\"startHour\":2.0,\"lengthHours\":1.0,\"depth\":1.5}]}");
    CHECK(depth.find("bad.json") != std::string::npos);
    CHECK(depth.find("events[0].depth") != std::string::npos);
    const std::string body = message("{\"version\":1,\"events\":[{\"body\":\"comet\",\"startDay\":1,\"startHour\":2.0,\"lengthHours\":1.0,\"depth\":0.5}]}");
    CHECK(body.find("events[0].body") != std::string::npos);
    CHECK(message("{\"version\":2,\"events\":[]}").find("version") != std::string::npos);
}

TEST_CASE("US-248 Data in the game: a mistake is shown with file and field, and the game keeps the default pair") {
    {
        World world("bad-events", [](const fs::path& data) {
            writeText(data / "light" / "celestial-events.json",
                      "{\"version\":1,\"events\":[{\"body\":\"sun\",\"startDay\":0,\"startHour\":11.0,\"lengthHours\":2.0,\"depth\":9}]}");
        });
        CHECK(world.odyssey->message().find("celestial-events.json") != std::string::npos);
        CHECK(world.odyssey->message().find("events[0].depth") != std::string::npos);
        world.advanceToHour(12.0);
        CHECK(world.odyssey->celestialLight().valid); // the default pair still lights the world
        CHECK(world.odyssey->celestialLight().dimming == doctest::Approx(1.0)); // and no eclipse
    }
    {
        // A sun with a wrong field in objects.json is left out, and the built-in default sun takes its place.
        World world("bad-sun", [](const fs::path& data) { replaceInFile(data / "objects.json", "\"follows\": \"clock\", \"orbitRadius\": 400", "\"follows\": \"always\", \"orbitRadius\": 400"); });
        CHECK(world.odyssey->message().find("objects.json") != std::string::npos);
        CHECK(world.odyssey->message().find("follows") != std::string::npos);
        world.advanceToHour(12.0);
        const game::CelestialLight light = world.odyssey->celestialLight();
        CHECK(light.valid);
        CHECK(light.source == "sun");
    }
    {
        // A sun naming a kind of light that does not exist.
        World world("bad-light", [](const fs::path& data) { replaceInFile(data / "objects.json", "\"light\": \"moon\", \"follows\": \"clock\"", "\"light\": \"nothing\", \"follows\": \"clock\""); });
        CHECK(world.odyssey->message().find("light/lights.json") != std::string::npos);
        world.advanceToHour(0.0);
        CHECK(world.odyssey->celestialLight().valid);
        CHECK(world.odyssey->celestialLight().source == "moon");
    }
}
