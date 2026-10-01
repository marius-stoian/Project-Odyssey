// US-242 Day, night and seasons: the light follows the game clock through dawn, day, dusk and night (D-49), with longer summer days.
#include "camp.h"

#include "game/sky.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <cmath>

using namespace camp_support;

namespace {

game::SkyData shipped() {
    const fs::path data(ODYSSEUS_DATA_DIR);
    return game::loadSky(data / "light" / "sky.json", data / "sim" / "calendar.json");
}

double brightness(const game::SkyState& s) { return (s.ambientR + s.ambientG + s.ambientB) / 3.0; }

} // namespace

TEST_CASE("US-242 Cycle: night, dawn, day, dusk and night again, from sky.json") {
    const game::SkyData sky = shipped();
    REQUIRE(sky.enabled);
    const game::SkyState midnight = game::skyAt(sky, 0.0, 0);
    const game::SkyState sunrise = game::skyAt(sky, sky.daylight[0].sunrise, 0);
    const game::SkyState noon = game::skyAt(sky, 12.0, 0);
    const game::SkyState sunset = game::skyAt(sky, sky.daylight[0].sunset, 0);
    const game::SkyState late = game::skyAt(sky, 23.9, 0);
    // Day is white at full strength; night a dimming of 55% with a blue tint (D-49); dawn and dusk are orange.
    CHECK(noon.ambientR == doctest::Approx(1.0F));
    CHECK(noon.ambientB == doctest::Approx(1.0F));
    CHECK(midnight.ambientR == doctest::Approx(191.0 / 255.0 * 0.55).epsilon(0.01));
    CHECK(midnight.ambientB == doctest::Approx(0.55).epsilon(0.01));
    CHECK(midnight.ambientB > midnight.ambientR); // blue
    CHECK(sunrise.ambientR > sunrise.ambientB);   // orange
    CHECK(sunset.ambientR > sunset.ambientB);
    CHECK(sunrise.ambientR > midnight.ambientR);
    CHECK(sunrise.ambientR < noon.ambientR + 0.001F);
    CHECK(brightness(noon) > brightness(sunset));
    CHECK(brightness(sunset) > brightness(midnight));
    // Midnight joins smoothly: no jump from 23.9 to 0.0 to 0.1.
    CHECK(std::abs(late.ambientB - midnight.ambientB) < 0.01F);
    // The change is gradual all through the day: no step bigger than a few percent in a tenth of an hour.
    game::SkyState before = game::skyAt(sky, 0.0, 1);
    for (double hour = 0.1; hour < 24.0; hour += 0.1) {
        const game::SkyState now = game::skyAt(sky, hour, 1);
        CHECK(std::abs(now.ambientR - before.ambientR) < 0.06F);
        CHECK(std::abs(now.ambientB - before.ambientB) < 0.06F);
        before = now;
    }
}

TEST_CASE("US-242 Seasons: the summer day is longer than the winter day, as calendar.json says") {
    const game::SkyData sky = shipped();
    const double summer = sky.daylight[1].sunset - sky.daylight[1].sunrise;
    const double winter = sky.daylight[3].sunset - sky.daylight[3].sunrise;
    CHECK(summer > winter + 5.0);
    CHECK(summer == doctest::Approx(15.0));
    CHECK(winter == doctest::Approx(8.0));
    // 7 o'clock on a summer morning is day; on a winter morning it is still dawn-dark.
    CHECK(game::skyAt(sky, 7.0, 1).sunUp);
    CHECK_FALSE(game::skyAt(sky, 7.0, 3).sunUp);
    CHECK(brightness(game::skyAt(sky, 7.0, 1)) > brightness(game::skyAt(sky, 7.0, 3)));
    // 19:00: summer dusk, winter night.
    CHECK(game::skyAt(sky, 19.0, 1).sunUp);
    CHECK_FALSE(game::skyAt(sky, 19.0, 3).sunUp);
    // The sun climbs higher in summer.
    CHECK(game::skyAt(sky, 12.5, 1).sunElevation > game::skyAt(sky, 12.0, 3).sunElevation + 20.0);
}

TEST_CASE("US-242 Moon: at night it is the light, dim and blue, with a direction for shadows") {
    const game::SkyData sky = shipped();
    const game::SkyState night = game::skyAt(sky, 0.5, 0);
    CHECK_FALSE(night.sunUp);
    CHECK(night.moonlit);
    CHECK(night.moonElevation > 0.0); // the moon is up
    CHECK(night.sunElevation < 0.0);  // the sun is not
    CHECK(night.lightElevation == doctest::Approx(night.moonElevation));
    CHECK(brightness(night) < 0.6);   // dim
    CHECK(night.ambientB > night.ambientR); // blue
    CHECK(std::hypot(night.shadowDirX, night.shadowDirY) == doctest::Approx(1.0).epsilon(0.001)); // a direction to cast shadows along
    // The sun and the moon cross the sky from east to west: by morning the sun is in the east and shadows fall west.
    const game::SkyState morning = game::skyAt(sky, sky.daylight[0].sunrise + 1.0, 0);
    CHECK(morning.sunAzimuth < 135.0);
    CHECK(morning.shadowDirX < 0.0); // the sun is in the east, the shadow falls west
    const game::SkyState evening = game::skyAt(sky, sky.daylight[0].sunset - 1.0, 0);
    CHECK(evening.sunAzimuth > 225.0);
    CHECK(evening.shadowDirX > 0.0);
}

TEST_CASE("US-242 sky.json: round trip and mistakes name the file and the field") {
    const fs::path folder = dataCopy("sky-data");
    const game::SkyData sky = game::loadSky(folder / "light" / "sky.json", folder / "sim" / "calendar.json");
    const fs::path copy = folder / "light" / "sky-copy.json";
    writeText(copy, game::skyToText(sky));
    CHECK(game::loadSky(copy, folder / "sim" / "calendar.json") == sky);

    const auto problem = [&](const fs::path& file, const fs::path& calendar) {
        try {
            game::loadSky(file, calendar);
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    const auto skyWith = [&](const std::string& keyframes) {
        writeText(copy, R"({"version":1,"keyframes":)" + keyframes + "}");
        return problem(copy, folder / "sim" / "calendar.json");
    };
    CHECK(skyWith(R"([{"anchor":"sunrise","offset":0,"color":[1,2,3],"strength":1,"shadow":0.5}])").find("keyframes") != std::string::npos);
    CHECK(skyWith(R"([{"anchor":"noon","offset":0,"color":[1,2,3],"strength":1,"shadow":0.5},{"anchor":"sunset","offset":0,"color":[1,2,3],"strength":1,"shadow":0.5}])")
              .find("keyframes[0].anchor") != std::string::npos);
    CHECK(skyWith(R"([{"anchor":"sunrise","offset":0,"color":[1,2,300],"strength":1,"shadow":0.5},{"anchor":"sunset","offset":0,"color":[1,2,3],"strength":1,"shadow":0.5}])")
              .find("keyframes[0].color[2]") != std::string::npos);
    CHECK(skyWith(R"([{"anchor":"sunrise","offset":0,"color":[1,2,3],"strength":1,"shadow":2},{"anchor":"sunset","offset":0,"color":[1,2,3],"strength":1,"shadow":0.5}])")
              .find("keyframes[0].shadow") != std::string::npos);
    // calendar.json: a day shorter than four hours is refused, naming the season.
    const fs::path calendar = folder / "sim" / "calendar-short.json";
    writeText(calendar, R"({"ticksPerDay":2400,"daysPerSeason":7,"daylight":{"Spring":{"sunrise":6,"sunset":8},"Summer":{"sunrise":5,"sunset":20},"Autumn":{"sunrise":6,"sunset":17},"Winter":{"sunrise":8,"sunset":16}}})");
    CHECK(problem(folder / "light" / "sky.json", calendar).find("daylight.Spring") != std::string::npos);
    // No sky.json: the light stays plain day.
    CHECK_FALSE(game::loadSky(folder / "light" / "missing.json", folder / "sim" / "calendar.json").enabled);
    CHECK(game::skyAt(game::loadSky(folder / "light" / "missing.json", folder / "sim" / "calendar.json"), 0.0, 0).ambientR == doctest::Approx(1.0F));
}

TEST_CASE("US-242 The world is lit by the time of day of the clan's clock") {
    Camp camp("sky-clock");
    luna::engine::RecordingRenderer& renderer = camp.renderer;
    // The day starts at midnight: dim and blue.
    renderer.clear();
    camp.odyssey.render(renderer, 1.0);
    const luna::engine::LightFrame night = renderer.lighting();
    CHECK(night.ambientR < 0.5F);
    CHECK(night.ambientB > night.ambientR);
    // At noon (half a day of ticks later) it is plain day.
    const int perDay = camp.odyssey.clan()->calendar().ticksPerDay();
    for (int i = 0; i < perDay / 2; ++i) camp.odyssey.clanMutable()->tick();
    renderer.clear();
    camp.odyssey.render(renderer, 1.0);
    CHECK(renderer.lighting().ambientR == doctest::Approx(1.0F).epsilon(0.01));
    CHECK(renderer.lighting().ambientB == doctest::Approx(1.0F).epsilon(0.01));
    CHECK(camp.odyssey.sky().sunUp);
}
