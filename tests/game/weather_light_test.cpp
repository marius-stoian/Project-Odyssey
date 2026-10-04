// US-246 Weather and light: rain dims and cools the world over the 3 s fade, lightning flashes the whole scene, and weather.json says how.
#include "camp.h"

#include "game/weather.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <functional>
#include <memory>

using namespace camp_support;

namespace {

const game::Catalogs& catalogs() {
    static const game::Catalogs loaded = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    return loaded;
}

int indexOf(const std::string& name) {
    const auto& weathers = catalogs().weather;
    for (std::size_t i = 0; i < weathers.size(); ++i) {
        if (weathers[i].name == name) return static_cast<int>(i);
    }
    FAIL("no weather named " << name);
    return -1;
}

// A level with a clan, at noon of day 0 (an hour is 100 ticks), so the sky is plain white day and only the weather tints the light.
struct Noon {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Noon(const std::string& name, const std::function<void(const fs::path&)>& adjust = {}) : data(dataCopy(name)) {
        if (adjust) adjust(data);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.clan = true;
        game::saveLevel(level, definitions, data / "weather-light-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "weather-light-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
        for (int i = 0; i < 1200; ++i) odyssey->clanMutable()->tick();
    }
    luna::engine::LightFrame lit() {
        renderer.clear();
        odyssey->render(renderer, 1.0);
        return renderer.lighting();
    }
    void tick(int count) {
        for (int i = 0; i < count; ++i) odyssey->update({});
    }
};

} // namespace

TEST_CASE("US-246 Dim: rain dims and cools the light over the same 3 s as its fade") {
    const auto& weathers = catalogs().weather;
    const int clear = indexOf("clear");
    const int rain = indexOf("steady rain");
    CHECK(weathers[static_cast<std::size_t>(rain)].lightDim < 1.0);
    const game::WeatherLight before = game::weatherLight(weathers, clear, rain, 0.0);
    const game::WeatherLight half = game::weatherLight(weathers, clear, rain, 0.5);
    const game::WeatherLight after = game::weatherLight(weathers, clear, rain, 1.0);
    // Starts at plain light, ends dimmer and bluer, in a straight line between.
    CHECK(before == game::WeatherLight{});
    CHECK(after.red < 1.0F);
    CHECK(after.blue > after.red); // cooler
    CHECK(half.red == doctest::Approx((before.red + after.red) / 2.0F));
    CHECK(half.blue == doctest::Approx((before.blue + after.blue) / 2.0F));
    // The fade of the cycle is the fade of the light: 60 ticks (3 s).
    CHECK(game::WeatherCycle::kFadeTicks == 60);

    Noon world("weather-light-dim");
    const float clearSky = world.lit().ambientR;
    CHECK(clearSky == doctest::Approx(1.0F).epsilon(0.02));
    REQUIRE(world.odyssey->setWeatherNamed("steady rain"));
    const luna::engine::LightFrame wet = world.lit();
    CHECK(wet.ambientR < clearSky - 0.05F);
    CHECK(wet.ambientB > wet.ambientR);
}

TEST_CASE("US-246 Lightning: a storm flashes the whole scene for a moment, the same way for the same seed") {
    // Pure: no flash without a rate, a flash that falls from 1 to 0 within 0.2 s, and the same strikes for the same seed.
    CHECK(game::lightningFlash(5, 12.3, 0.0) == doctest::Approx(0.0));
    int strikes = 0;
    double longest = 0.0;
    for (int slot = 0; slot < 3000; ++slot) { // ten minutes of 0.2 s slots
        const double flash = game::lightningFlash(5, slot * 0.2 + 0.001, 10.0);
        CHECK(flash >= 0.0);
        CHECK(flash <= 1.0);
        if (flash > 0.0) {
            ++strikes;
            CHECK(game::lightningFlash(5, slot * 0.2 + 0.1, 10.0) < flash); // half a slot later it has fallen
        }
        longest = std::max(longest, flash);
        CHECK(game::lightningFlash(5, slot * 0.2 + 0.001, 10.0) == doctest::Approx(flash)); // repeatable
    }
    CHECK(longest > 0.9);
    CHECK(strikes > 40); // about 10 a minute for 10 minutes = 100 expected
    CHECK(strikes < 200);

    // In the game: under a thunderstorm the ambient light jumps above its steady level for a moment.
    Noon world("weather-light-flash");
    REQUIRE(world.odyssey->setWeatherNamed("thunderstorm weather"));
    const float steady = world.lit().ambientR;
    float brightest = steady;
    for (int i = 0; i < 400; ++i) {
        world.tick(1);
        brightest = std::max(brightest, world.lit().ambientR);
    }
    CHECK(brightest > steady + 0.1F);
    CHECK(brightest <= 1.001F);
}

TEST_CASE("US-246 Data: weather.json sets a weather light and the game uses it; mistakes name the file and field") {
    const auto& weathers = catalogs().weather;
    CHECK(weathers[static_cast<std::size_t>(indexOf("clear"))].lightDim == doctest::Approx(1.0));
    CHECK(weathers[static_cast<std::size_t>(indexOf("thunderstorm weather"))].flashPerMinute > 0.0);
    CHECK(weathers[static_cast<std::size_t>(indexOf("close lightning"))].flashPerMinute > 0.0);
    CHECK(weathers[static_cast<std::size_t>(indexOf("fog"))].lightDim < 1.0);
    CHECK(weathers[static_cast<std::size_t>(indexOf("steady rain"))].flashPerMinute == doctest::Approx(0.0));

    // The owner makes "steady rain" nearly black and the game follows.
    Noon world("weather-light-data", [](const fs::path& data) {
        std::string text = readText(data / "weather.json");
        const std::size_t at = text.find("{\"name\":\"steady rain\"");
        REQUIRE(at != std::string::npos);
        const std::size_t open = text.find("\"light\"", at);
        REQUIRE(open != std::string::npos);
        const std::size_t end = text.find("]}", open);
        REQUIRE(end != std::string::npos);
        text.replace(open, end + 2 - open, "\"light\":{\"dim\":0.2,\"tint\":[255,255,255]}");
        writeText(data / "weather.json", text);
    });
    REQUIRE(world.odyssey->setWeatherNamed("steady rain"));
    CHECK(world.lit().ambientR == doctest::Approx(0.2F).epsilon(0.05));

    // Mistakes.
    const fs::path data = dataCopy("weather-light-bad");
    const auto broken = [&](const std::string& fieldText) {
        std::string text = readText(fs::path(ODYSSEUS_DATA_DIR) / "weather.json");
        const std::string first = "{\"name\":\"clear\"";
        text.replace(text.find(first), first.size(), first + "," + fieldText);
        writeText(data / "weather.json", text);
        try {
            game::loadCatalogs(data);
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    CHECK(broken("\"light\":{\"dim\":3}").find("weather[0].light.dim") != std::string::npos);
    CHECK(broken("\"light\":{\"tint\":[1,2]}").find("weather[0].light.tint") != std::string::npos);
    CHECK(broken("\"light\":{\"tint\":[1,2,300]}").find("weather[0].light.tint[2]") != std::string::npos);
    CHECK(broken("\"flash\":-1").find("weather[0].flash") != std::string::npos);
}
