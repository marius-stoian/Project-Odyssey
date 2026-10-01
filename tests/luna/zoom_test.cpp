// US-232 Camera zoom and UI scale: the scaled picture, the camera view and the pointer.
#include "luna/engine/camera.h"
#include "luna/engine/renderer.h"
#include "luna/engine/scaled_renderer.h"

#include <doctest/doctest.h>

using namespace luna::engine;

TEST_CASE("US-232 Scaled drawing") {
    RecordingRenderer recorder;
    const Texture texture = recorder.createTexture(Image(8, 8));
    ScaledRenderer twice(recorder, 2);
    twice.draw(texture, {0, 0, 8, 8}, {10, 20});
    twice.drawStyled(texture, {0, 0, 4, 4}, {3, 5, 6, 7}, DrawStyle{128, Blend::Add});
    REQUIRE(recorder.draws().size() == 2);
    CHECK(recorder.draws()[0].destination == Rect{20, 40, 16, 16}); // whole-number blocks, so pixels stay crisp
    CHECK(recorder.draws()[1].destination == Rect{6, 10, 12, 14});
    CHECK(recorder.draws()[1].style.alpha == 128);
    CHECK(recorder.draws()[1].style.blend == Blend::Add);

    RecordingRenderer plain;
    const Texture again = plain.createTexture(Image(8, 8));
    ScaledRenderer once(plain, 1);
    once.draw(again, {0, 0, 8, 8}, {10, 20});
    CHECK(plain.draws()[0].at == Point{10, 20}); // 1x passes straight through
}

TEST_CASE("US-232 Zoom changes the view and keeps the centre") {
    Camera camera(480, 270, 2048, 2048);
    camera.centreOn(1000, 1000);
    CHECK(camera.view().x == 760);
    camera.setViewSize(960, 540); // zoomed out: 30 x 17 tiles
    CHECK(camera.view().width == 960);
    CHECK(camera.view().x == 520);
    CHECK(camera.view().y == 730);
    camera.setViewSize(480, 270);
    CHECK(camera.view().x == 760);
}

TEST_CASE("US-232 The pointer hits the same world spot at any zoom") {
    Pointer pointer;
    pointer.x = 600;
    pointer.y = 340;
    for (const int zoom : {1, 2}) {
        Camera camera(960 / zoom, 540 / zoom, 2048, 2048);
        camera.centreOn(1000, 1000);
        const Pointer seen = scaledPointer(pointer, zoom);
        const Rect view = camera.view();
        // The hero is at the middle of the picture; a pointer 120 px right and 70 px below the middle is
        // 120/zoom and 70/zoom world pixels from him.
        CHECK(view.x + seen.x == 1000 + 120 / zoom);
        CHECK(view.y + seen.y == 1000 + 70 / zoom);
    }
    Pointer outside;
    CHECK_FALSE(scaledPointer(outside, 2).inside());
}
