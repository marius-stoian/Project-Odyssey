// US-243 Fires, torches and glowing effects: the point lights of the world, from the `light` fields of the catalogs and the torches of the clan.
#include "camp.h"

#include "game/level.h"
#include "game/lighting.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

#include <algorithm>
#include <functional>
#include <memory>
#include <cmath>
#include <fstream>
#include <sstream>

using namespace camp_support;

namespace {

std::string readText(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

void replaceIn(const fs::path& file, const std::string& from, const std::string& to) {
    std::string text = readText(file);
    const auto at = text.find(from);
    REQUIRE_MESSAGE(at != std::string::npos, from);
    text.replace(at, from.size(), to);
    writeText(file, text);
}

// A level with a clan and a camp fire (the effect "flame") 40 pixels east of the hero's start; the clan clock starts at midnight.
struct Night {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    explicit Night(const std::string& name, const std::function<void(const fs::path&)>& adjust = {}) : data(dataCopy(name)) {
        if (adjust) adjust(data);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.clan = true;
        level.effects = {{level.nextId++, "flame", {level.heroStart.x + 40, level.heroStart.y}}};
        game::saveLevel(level, definitions, data / "fire-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "fire-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    const luna::engine::LightFrame& lit() {
        renderer.clear();
        odyssey->render(renderer, 1.0);
        return renderer.lighting();
    }
    void advance(int ticks) {
        for (int i = 0; i < ticks; ++i) odyssey->clanMutable()->tick();
    }
};

} // namespace

TEST_CASE("US-243 Fire: a lit fire lights a circle around it at night, and nothing by day") {
    Night night("lights-fire", [](const fs::path& data) { replaceIn(data / "light" / "lights.json", "\"clanTorch\": \"torch\",", "\"clanTorch\": \"\","); });
    const luna::engine::LightFrame& frame = night.lit();
    REQUIRE(frame.lights.size() == 1); // the camp fire alone
    const luna::engine::PointLight light = frame.lights.front(); // a copy: the renderer keeps only the last frame it was given
    CHECK(light.radius == doctest::Approx(6.0 * 32.0)); // a campfire reaches 6 tiles (D-49)
    CHECK(light.strength == doctest::Approx(0.9).epsilon(0.02));
    CHECK(light.r > light.b); // warm
    const auto view = night.odyssey->camera().view(1.0);
    const auto hero = night.odyssey->level().heroStart;
    CHECK(light.x == doctest::Approx(static_cast<double>(hero.x + 40 - view.x)).epsilon(0.01));
    CHECK(light.y == doctest::Approx(static_cast<double>(hero.y - 8 - view.y)).epsilon(0.01));
    // At noon the ambient light is full and the fire adds nothing.
    night.advance(night.odyssey->clan()->calendar().ticksPerDay() / 2);
    CHECK(night.lit().lights.empty());
    // At dusk it shines again, less than at midnight.
    night.advance(night.odyssey->clan()->calendar().ticksPerDay() * 6 / 24 + 10); // about 18:06 (an hour is 100 ticks)
    const auto& dusk = night.lit();
    REQUIRE(dusk.lights.size() == 1);
    CHECK(dusk.lights.front().strength < light.strength);
    CHECK(dusk.lights.front().strength > 0.0F);
}

TEST_CASE("US-243 Data: a catalog entry with a light field emits it; without the field, none") {
    {
        Night without("lights-without", [](const fs::path& data) {
            replaceIn(data / "light" / "lights.json", "\"clanTorch\": \"torch\",", "\"clanTorch\": \"\",");
            replaceIn(data / "effects.json", "{\"name\":\"flame\",\"frames\":4,\"ticksPerFrame\":3,\"loop\":true,\"light\":\"campfire\"}",
                      "{\"name\":\"flame\",\"frames\":4,\"ticksPerFrame\":3,\"loop\":true}");
        });
        CHECK(without.lit().lights.empty());
    }
    {
        // Another kind of light, from the file: a green lantern of 3 tiles.
        Night lantern("lights-lantern", [](const fs::path& data) {
            replaceIn(data / "light" / "lights.json", "\"clanTorch\": \"torch\",", "\"clanTorch\": \"\",");
            replaceIn(data / "light" / "lights.json", "\"lights\": [", "\"lights\": [ { \"name\": \"lantern\", \"color\": [0, 255, 0], \"radiusTiles\": 3.0, \"strength\": 0.5, \"height\": 20 },");
            replaceIn(data / "effects.json", "\"name\":\"flame\",\"frames\":4,\"ticksPerFrame\":3,\"loop\":true,\"light\":\"campfire\"",
                      "\"name\":\"flame\",\"frames\":4,\"ticksPerFrame\":3,\"loop\":true,\"light\":\"lantern\"");
        });
        const auto& frame = lantern.lit();
        REQUIRE(frame.lights.size() == 1);
        CHECK(frame.lights.front().radius == doctest::Approx(96.0));
        CHECK(frame.lights.front().g > frame.lights.front().r);
        CHECK(frame.lights.front().strength == doctest::Approx(0.5).epsilon(0.02));
    }
    // A light that names no kind is an error that names the file and the field.
    const fs::path data = dataCopy("lights-bad-name");
    replaceIn(data / "effects.json", "\"loop\":true,\"light\":\"campfire\"},\n    {\"name\":\"big fire\"", "\"loop\":true,\"light\":\"nothing\"},\n    {\"name\":\"big fire\"");
    try {
        game::OdysseyGame odyssey(data, ODYSSEUS_DEMO_LEVEL);
        FAIL("an unknown kind of light should stop the game");
    } catch (const odysseus::sim::DataError& error) {
        const std::string text = error.what();
        CHECK(text.find("effects.json") != std::string::npos);
        CHECK(text.find("light") != std::string::npos);
        CHECK(text.find("nothing") != std::string::npos);
    }
}

TEST_CASE("US-243 Torch: a clan member carries a light that moves with them at night") {
    Night night("lights-torch", [](const fs::path& data) {
        replaceIn(data / "effects.json", "{\"name\":\"flame\",\"frames\":4,\"ticksPerFrame\":3,\"loop\":true,\"light\":\"campfire\"}",
                  "{\"name\":\"flame\",\"frames\":4,\"ticksPerFrame\":3,\"loop\":true}"); // no fire: the torches are the only lights
    });
    const auto matches = [&](const luna::engine::LightFrame& frame, std::size_t figure) {
        const auto& f = night.odyssey->clanView().figures()[figure];
        const auto view = night.odyssey->camera().view(1.0);
        return std::any_of(frame.lights.begin(), frame.lights.end(), [&](const luna::engine::PointLight& light) {
            return std::abs(light.x - (f.x - view.x)) < 1.5 && std::abs(light.y - (f.y - 18.0 - view.y)) < 1.5;
        });
    };
    const auto& frame = night.lit();
    const auto& figures = night.odyssey->clanView().figures();
    std::size_t present = 0;
    for (const auto& f : figures) present += f.present && f.placed ? 1U : 0U;
    REQUIRE(present > 3);
    CHECK(frame.lights.size() <= present); // those on the picture
    CHECK(frame.lights.size() > 0);
    for (const auto& light : frame.lights) CHECK(light.radius == doctest::Approx(4.0 * 32.0)); // a torch reaches 4 tiles
    // Walk: after some play the figures have moved and the lights went with them.
    std::vector<std::pair<double, double>> before;
    for (const auto& f : figures) before.emplace_back(f.x, f.y);
    for (int i = 0; i < 300; ++i) {
        night.odyssey->update({});
    }
    bool moved = false;
    for (std::size_t i = 0; i < figures.size(); ++i) {
        if (!figures[i].present || !figures[i].placed) continue;
        if (std::abs(figures[i].x - before[i].first) + std::abs(figures[i].y - before[i].second) > 4.0) {
            moved = true;
            const auto& after = night.lit();
            const auto view = night.odyssey->camera().view(1.0);
            const bool onPicture = figures[i].x - view.x > 0 && figures[i].x - view.x < view.width && figures[i].y - view.y > 0 && figures[i].y - view.y < view.height;
            if (onPicture) CHECK(matches(after, i));
            break;
        }
    }
    CHECK(moved);
    // By day the torches are out.
    night.advance(night.odyssey->clan()->calendar().ticksPerDay() / 2);
    CHECK(night.lit().lights.empty());
}

TEST_CASE("US-243 Flicker: steady by default (D-49), a seeded wobble when the data asks") {
    {
        Night steady("lights-steady", [](const fs::path& data) { replaceIn(data / "light" / "lights.json", "\"clanTorch\": \"torch\",", "\"clanTorch\": \"\","); });
        const float first = steady.lit().lights.front().strength;
        for (int i = 0; i < 40; ++i) {
            steady.odyssey->update({});
            CHECK(steady.lit().lights.front().strength == doctest::Approx(first)); // no flicker field, no wobble
        }
    }
    {
        Night wavering("lights-wavering", [](const fs::path& data) {
            replaceIn(data / "light" / "lights.json", "\"clanTorch\": \"torch\",", "\"clanTorch\": \"\",");
            replaceIn(data / "light" / "lights.json", "\"name\": \"campfire\", \"color\": [255, 199, 115], \"radiusTiles\": 6.0, \"strength\": 0.9, \"height\": 24",
                      "\"name\": \"campfire\", \"color\": [255, 199, 115], \"radiusTiles\": 6.0, \"strength\": 0.9, \"height\": 24, \"flicker\": 0.6");
        });
        float lowest = 10.0F, highest = 0.0F;
        for (int i = 0; i < 80; ++i) {
            wavering.odyssey->update({});
            const float strength = wavering.lit().lights.front().strength;
            lowest = std::min(lowest, strength);
            highest = std::max(highest, strength);
            CHECK(strength >= 0.9F * 0.4F - 0.001F); // never below strength x (1 - flicker)
            CHECK(strength <= 0.9F + 0.001F);
        }
        CHECK(highest - lowest > 0.05F); // it wavers
    }
    // The noise itself: always 0 to 1, the same every run, and smooth.
    for (double t = 0.0; t < 20.0; t += 0.037) {
        const double value = game::flickerNoise(7, t);
        CHECK(value >= 0.0);
        CHECK(value <= 1.0);
        CHECK(game::flickerNoise(7, t) == value);
        CHECK(std::abs(game::flickerNoise(7, t + 0.005) - value) < 0.15);
    }
    CHECK(game::flickerNoise(7, 3.3) != game::flickerNoise(8, 3.3)); // another light, another wobble
}

TEST_CASE("US-243 lights.json: flicker and clanTorch are checked") {
    const fs::path file = dataCopy("lights-fields") / "light" / "lights.json";
    const auto problem = [&](const std::string& text) {
        writeText(file, text);
        try {
            game::loadLighting(file);
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    CHECK(problem(R"({"version":1,"ambient":{"color":[1,2,3],"strength":1},"lights":[{"name":"a","color":[1,2,3],"radiusTiles":4,"strength":1,"height":24,"flicker":2}]})")
              .find("lights[0].flicker") != std::string::npos);
    CHECK(problem(R"({"version":1,"clanTorch":"nothing","ambient":{"color":[1,2,3],"strength":1},"lights":[]})").find("clanTorch") != std::string::npos);
    game::LightingData data;
    data.clanTorch = "torch";
    data.kinds.push_back({"torch", 255, 199, 115, 4.0, 0.9, 24.0, 0.3});
    writeText(file, game::lightingToText(data));
    CHECK(game::loadLighting(file) == data); // the round trip keeps the flicker and the torch
}
