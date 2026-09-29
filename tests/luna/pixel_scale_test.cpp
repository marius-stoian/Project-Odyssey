#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

using luna::engine::integerScale;
using luna::engine::Rect;

TEST_CASE("US-022 Whole-number scale") {
    SUBCASE("1920 x 1080 shows the 480 x 270 screen exactly 4 times bigger, no bars") {
        const auto pixels = integerScale(1920, 1080, 480, 270);
        CHECK(pixels.scale == 4);
        CHECK(pixels.area == Rect{0, 0, 1920, 1080});
    }
    SUBCASE("a size that is not a multiple uses the largest whole number and letterboxes") {
        const auto pixels = integerScale(1366, 768, 480, 270); // a common laptop screen
        CHECK(pixels.scale == 2);                              // 3 would need 1440 x 810
        CHECK(pixels.area == Rect{203, 114, 960, 540});        // centred, black bars around
    }
    SUBCASE("other sizes") {
        CHECK(integerScale(1280, 720, 480, 270).scale == 2);
        CHECK(integerScale(2560, 1440, 480, 270).scale == 5);
        CHECK(integerScale(3840, 2160, 480, 270).scale == 8);
        CHECK(integerScale(1920, 1200, 480, 270).area == Rect{0, 60, 1920, 1080});
    }
    SUBCASE("a window smaller than the virtual screen still draws at scale 1") {
        CHECK(integerScale(300, 200, 480, 270).scale == 1);
    }
}
