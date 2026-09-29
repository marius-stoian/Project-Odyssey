// Window tests (ctest label "window"): a real, hidden Luna window. They draw a
// checkerboard of single art pixels and read the screen back. Crisp means every art
// pixel became an exact scale x scale block of its own colour; any blurring would
// produce in-between colours at the block edges.
#include "luna/engine/image.h"
#include "luna/engine/renderer.h"
#include "luna/platform/system.h"
#include "luna/platform/window.h"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

using luna::engine::Color;
using luna::engine::Image;
using luna::engine::integerScale;
using luna::engine::Rect;

namespace {

constexpr int kVirtualWidth = 480;
constexpr int kVirtualHeight = 270;
constexpr Color kRed{220, 40, 40};
constexpr Color kBlue{40, 60, 220};
constexpr int kChecker = 8;       // 8 x 8 art pixels
constexpr int kAtX = 10;          // drawn at virtual (10, 10)
constexpr int kAtY = 10;

Image checkerboard() {
    Image image(kChecker, kChecker);
    for (int y = 0; y < kChecker; ++y) {
        for (int x = 0; x < kChecker; ++x) {
            image.set(x, y, (x + y) % 2 == 0 ? kRed : kBlue);
        }
    }
    return image;
}

Color pixelAt(const luna::platform::Pixels& pixels, int x, int y) {
    const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(pixels.width) + static_cast<std::size_t>(x)) * 4;
    return {pixels.rgba[at], pixels.rgba[at + 1], pixels.rgba[at + 2], pixels.rgba[at + 3]};
}

// Draws the checkerboard at the given window size and checks every screen pixel of it.
void checkCrispAt(int windowWidth, int windowHeight) {
    luna::platform::Window window({"US-022 test", 1280, 720, kVirtualWidth, kVirtualHeight, true});
    luna::engine::WindowRenderer renderer(window);
    const auto texture = renderer.createTexture(checkerboard());

    window.setSize(windowWidth, windowHeight);
    std::vector<luna::platform::Event> events;
    window.pollEvents(events); // let the resize arrive
    const auto output = window.outputRect();
    MESSAGE("requested ", windowWidth, "x", windowHeight, ", drawing surface ", output.width, "x", output.height);
    window.clear(0, 0, 0);
    renderer.draw(texture, {0, 0, kChecker, kChecker}, {kAtX, kAtY});
    const luna::platform::Pixels screen = window.readPixels();
    window.present();

    INFO("window pixels: ", screen.width, " x ", screen.height);
    const auto expected = integerScale(screen.width, screen.height, kVirtualWidth, kVirtualHeight);
    CHECK(window.presentationRect() == expected.area); // SDL and Luna agree on the scale
    const int scale = expected.scale;
    int wrongPixels = 0;
    for (int artY = 0; artY < kChecker; ++artY) {
        for (int artX = 0; artX < kChecker; ++artX) {
            const Color want = (artX + artY) % 2 == 0 ? kRed : kBlue;
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    const int x = expected.area.x + (kAtX + artX) * scale + dx;
                    const int y = expected.area.y + (kAtY + artY) * scale + dy;
                    const Color got = pixelAt(screen, x, y);
                    if (got.red != want.red || got.green != want.green || got.blue != want.blue) {
                        ++wrongPixels;
                    }
                }
            }
        }
    }
    CHECK(wrongPixels == 0);
    if (expected.area.x > 0) {
        const Color bar = pixelAt(screen, expected.area.x / 2, screen.height / 2);
        CHECK(bar.red == 0);
        CHECK(bar.green == 0);
        CHECK(bar.blue == 0); // letterbox
    }
    MESSAGE("window ", screen.width, "x", screen.height, ": scale x", scale, ", picture at (", expected.area.x, ", ",
            expected.area.y, "), ", kChecker * kChecker * scale * scale, " pixels checked, ", wrongPixels, " wrong");
}

} // namespace

TEST_CASE("US-022 Crisp pixels") {
    luna::platform::System system;
    SUBCASE("1920 x 1080: exactly 4 times bigger, no blurred pixels") {
        checkCrispAt(1920, 1080);
    }
    SUBCASE("1366 x 768: largest whole-number scale, letterboxed") {
        checkCrispAt(1366, 768);
    }
}
