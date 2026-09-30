// US-132 Effect player (Luna).
#include "luna/engine/effects.h"
#include "luna/engine/ui.h"

#include <doctest/doctest.h>

using namespace luna::engine;

namespace {

EffectSpec threeFrames(bool loop) {
    EffectSpec spec;
    spec.texture = {7, 96, 32};
    spec.frames = {{0, 0, 32, 32}, {32, 0, 32, 32}, {64, 0, 32, 32}};
    spec.ticksPerFrame = 2;
    spec.loop = loop;
    return spec;
}

} // namespace

TEST_CASE("US-132 One-shot") {
    EffectPlayer player;
    const int handle = player.start(threeFrames(false), 100.0, 50.0);
    RecordingRenderer renderer;
    // Frames 0, 0, 1, 1, 2, 2, then gone: 3 frames x 2 ticks.
    std::vector<int> shown;
    for (int tick = 0; tick < 6; ++tick) {
        renderer.clear();
        player.draw(renderer, {0, 0, 480, 270});
        REQUIRE(renderer.draws().size() == 1);
        shown.push_back(renderer.draws()[0].source.x / 32);
        player.update();
    }
    CHECK(shown == std::vector<int>{0, 0, 1, 1, 2, 2});
    CHECK_FALSE(player.isRunning(handle));
    CHECK(player.count() == 0);
}

TEST_CASE("US-132 Looping") {
    EffectPlayer player;
    const int handle = player.start(threeFrames(true), 0.0, 0.0);
    for (int tick = 0; tick < 100; ++tick) player.update();
    CHECK(player.isRunning(handle));
    RecordingRenderer renderer;
    player.draw(renderer, {0, 0, 480, 270});
    CHECK(renderer.draws()[0].source.x == 64); // tick 100: step 50, frame 50 % 3 = 2
    player.stop(handle);
    CHECK(player.count() == 0);
}

TEST_CASE("US-132 Placement and style") {
    EffectPlayer player;
    EffectSpec spec = threeFrames(false);
    spec.style = {128, Blend::Add};
    player.start(spec, 200.0, 100.0, 16);                 // in the world, 16 pixels across
    player.start(spec, 20.0, 10.0, 0, true);              // on the screen, its own size
    RecordingRenderer renderer;
    player.draw(renderer, {150, 60, 480, 270});           // the camera looks at (150, 60)
    REQUIRE(renderer.draws().size() == 2);
    const auto& world = renderer.draws()[0];
    CHECK(world.styled);
    CHECK(world.destination.x == 200 - 150 - 8);
    CHECK(world.destination.y == 100 - 60 - 8);
    CHECK(world.destination.width == 16);
    CHECK(world.style.alpha == 128);
    CHECK(world.style.blend == Blend::Add);
    const auto& screen = renderer.draws()[1];
    CHECK(screen.destination.x == 20 - 16);
    CHECK(screen.destination.width == 32);
    // No frames, no effect.
    CHECK(player.start(EffectSpec{}, 0.0, 0.0) == -1);
}

TEST_CASE("US-132 Additive light") {
    // Drawn additively, light adds to what is below; half alpha adds half.
    ImageRenderer image(2, 1);
    image.clear({100, 100, 100});
    Image glow(1, 1);
    glow.set(0, 0, {200, 100, 0, 255});
    const Texture texture = image.createTexture(glow);
    image.drawStyled(texture, {0, 0, 1, 1}, {0, 0, 1, 1}, {255, Blend::Add});
    image.drawStyled(texture, {0, 0, 1, 1}, {1, 0, 1, 1}, {128, Blend::Add});
    CHECK(image.image().get(0, 0).red == 255);
    CHECK(image.image().get(0, 0).green == 200);
    CHECK(image.image().get(1, 0).red == 100 + 200 * 128 / 255);
}
