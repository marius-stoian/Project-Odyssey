#include "core/presentation.h"

#include <doctest/doctest.h>

using odysseus::core::Rect;
using odysseus::core::ScalingMode;
using odysseus::core::presentationArea;

TEST_CASE("US-231 Scale") {
    CHECK(odysseus::core::kVirtualWidth == 960);
    CHECK(odysseus::core::kVirtualHeight == 540);
    CHECK(presentationArea(1920, 1080, 960, 540, ScalingMode::Whole) == Rect{0, 0, 1920, 1080});
    CHECK(presentationArea(3840, 2160, 960, 540, ScalingMode::Whole) == Rect{0, 0, 3840, 2160});
}

TEST_CASE("US-231 Odd sizes") {
    SUBCASE("Whole centres the largest integer scale with black bars") {
        CHECK(presentationArea(2560, 1440, 960, 540, ScalingMode::Whole) == Rect{320, 180, 1920, 1080});
        CHECK(presentationArea(1280, 720, 960, 540, ScalingMode::Whole) == Rect{160, 90, 960, 540});
        CHECK(presentationArea(1367, 769, 960, 540, ScalingMode::Whole) == Rect{203, 114, 960, 540});
    }
    SUBCASE("Fill uses the available 16 to 9 area") {
        CHECK(presentationArea(2560, 1440, 960, 540, ScalingMode::Fill) == Rect{0, 0, 2560, 1440});
        CHECK(presentationArea(1280, 720, 960, 540, ScalingMode::Fill) == Rect{0, 0, 1280, 720});
        CHECK(presentationArea(1600, 1000, 960, 540, ScalingMode::Fill) == Rect{0, 50, 1600, 900});
    }
    SUBCASE("a window below the virtual size stays contained") {
        CHECK(presentationArea(480, 270, 960, 540, ScalingMode::Whole) == Rect{0, 0, 480, 270});
        CHECK(presentationArea(480, 270, 960, 540, ScalingMode::Fill) == Rect{0, 0, 480, 270});
    }
}
