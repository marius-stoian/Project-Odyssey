// US-240 The lighting pipeline, game side: assets/data/light/lights.json is read and checked, and the world is drawn lit by its ambient colour
// while the interface is not.
#include "camp.h"

#include "game/lighting.h"
#include "luna/engine/renderer.h"
#include "sim/data.h"

using namespace camp_support;

TEST_CASE("US-240 lights.json: the shipped file and a round trip") {
    const game::LightingData shipped = game::loadLighting(fs::path(ODYSSEUS_DATA_DIR) / "light" / "lights.json");
    CHECK(shipped.ambientStrength == doctest::Approx(1.0)); // white at strength 1 changes nothing
    REQUIRE(shipped.kind("campfire") != nullptr);
    REQUIRE(shipped.kind("torch") != nullptr);
    CHECK(shipped.kind("campfire")->radiusTiles == doctest::Approx(6.0)); // D-49
    CHECK(shipped.kind("torch")->radiusTiles == doctest::Approx(4.0));
    CHECK(shipped.kind("nothing") == nullptr);

    game::LightingData made;
    made.ambientRed = 180;
    made.ambientGreen = 200;
    made.ambientBlue = 255;
    made.ambientStrength = 0.55;
    made.kinds.push_back({"lantern", 255, 220, 160, 3.5, 0.75, 30.0});
    const fs::path file = dataCopy("lighting-roundtrip") / "light" / "lights.json";
    writeText(file, game::lightingToText(made));
    CHECK(game::loadLighting(file) == made); // the same data comes back
}

TEST_CASE("US-240 lights.json: mistakes name the file and the field") {
    const fs::path folder = dataCopy("lighting-errors") / "light";
    const fs::path file = folder / "lights.json";
    const auto problem = [&](const std::string& text) {
        writeText(file, text);
        try {
            game::loadLighting(file);
        } catch (const odysseus::sim::DataError& error) {
            return std::string(error.what());
        }
        return std::string();
    };
    CHECK(problem(R"({"version":1,"ambient":{"color":[255,255],"strength":1},"lights":[]})").find("ambient.color") != std::string::npos);
    CHECK(problem(R"({"version":1,"ambient":{"color":[255,255,300],"strength":1},"lights":[]})").find("ambient.color[2]") != std::string::npos);
    CHECK(problem(R"({"version":1,"ambient":{"color":[1,2,3],"strength":9},"lights":[]})").find("ambient.strength") != std::string::npos);
    CHECK(problem(R"({"version":1,"ambient":{"color":[1,2,3],"strength":1},"lights":[{"name":"a","color":[1,2,3],"radiusTiles":4,"strength":1,"height":24},{"name":"a","color":[1,2,3],"radiusTiles":4,"strength":1,"height":24}]})")
                  .find("lights[1].name") != std::string::npos);
    CHECK(problem(R"({"version":1,"ambient":{"color":[1,2,3],"strength":1},"lights":[{"name":"a","color":[1,2,3],"radiusTiles":0,"strength":1,"height":24}]})").find("lights[0].radiusTiles") != std::string::npos);
    CHECK(problem(R"({"version":7})").find("version") != std::string::npos);
    CHECK(game::loadLighting(folder / "missing.json") == game::LightingData{}); // no file: unlit, not an error
}

TEST_CASE("US-240 Ambient: the world is drawn lit, the interface is not") {
    const fs::path data = dataCopy("lighting-ambient");
    writeText(data / "light" / "lights.json", R"({"version":1,"ambient":{"color":[200,220,255],"strength":0.5},"lights":[]})");
    game::OdysseyGame odyssey(data, ODYSSEUS_DEMO_LEVEL);
    luna::engine::RecordingRenderer renderer;
    odyssey.start(renderer);
    odyssey.update({});
    renderer.clear();
    odyssey.render(renderer, 1.0);
    REQUIRE(renderer.lighting().ambientR > 0.0F);
    CHECK(renderer.lighting().ambientR == doctest::Approx(200.0 / 255.0 * 0.5));
    CHECK(renderer.lighting().ambientG == doctest::Approx(220.0 / 255.0 * 0.5));
    CHECK(renderer.lighting().ambientB == doctest::Approx(255.0 / 255.0 * 0.5));
    // The first draws (the ground) are lit; the last ones (the hotbar and the labels) are not.
    REQUIRE(renderer.draws().size() > 100);
    CHECK(renderer.draws().front().lit);
    CHECK_FALSE(renderer.draws().back().lit);
    CHECK_FALSE(renderer.lightingOn()); // and the lighting is switched off again at the end of the frame
}
