// US-240 The lighting pipeline, in a real hidden window (ctest label "window"): the ambient colour tints lit sprites, a point light brightens the
// side of a sprite that faces it (using its normal map), and 64 lights fit the frame budget. The shader half needs a graphics card with Direct3D 12:
// where there is none (a CI runner) those tests say so and pass; the ambient tint is checked on both renderers.
#include "luna/engine/image.h"
#include "luna/engine/renderer.h"
#include "luna/platform/system.h"
#include "luna/platform/window.h"

#include <doctest/doctest.h>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <cstddef>
#include <memory>
#include <stdexcept>

using luna::engine::Color;
using luna::engine::Image;
using luna::engine::LightFrame;
using luna::engine::PointLight;
using luna::engine::Rect;
using luna::engine::Texture;

namespace {

constexpr int kVirtualWidth = 480;
constexpr int kVirtualHeight = 270;
constexpr int kScale = 2;      // a 1280 x 720 window shows the 480 x 270 picture twice as large,
constexpr int kLeft = 160;     // at (160, 90)
constexpr int kTop = 90;

Image white(int width, int height) {
    Image image(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) image.set(x, y, Color{255, 255, 255, 255});
    }
    return image;
}

// A dome: the surface leans away from the middle, so the side that faces a light is the side nearest to it.
Image dome(int size) {
    Image image(size, size);
    const double half = size / 2.0;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double nx = (x + 0.5 - half) / half;
            const double ny = (y + 0.5 - half) / half;
            const double nz = std::sqrt(std::max(0.0, 1.0 - nx * nx - ny * ny));
            const auto encode = [](double v) { return static_cast<std::uint8_t>(std::lround(v * 127.5 + 127.5)); };
            image.set(x, y, Color{encode(nx), encode(ny), encode(nz), 255});
        }
    }
    return image;
}

Color screenAt(const luna::platform::Pixels& pixels, int virtualX, int virtualY) {
    const int x = kLeft + virtualX * kScale + 1;
    const int y = kTop + virtualY * kScale + 1;
    const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(pixels.width) + static_cast<std::size_t>(x)) * 4;
    return {pixels.rgba[at], pixels.rgba[at + 1], pixels.rgba[at + 2], pixels.rgba[at + 3]};
}

std::unique_ptr<luna::platform::Window> openWindow(luna::platform::RendererChoice choice) {
    try {
        return std::make_unique<luna::platform::Window>(luna::platform::WindowSettings{"US-240 test", 1280, 720, kVirtualWidth, kVirtualHeight, true, choice});
    } catch (const std::exception& error) {
        MESSAGE("renderer could not run: ", error.what());
        return nullptr;
    }
}

} // namespace

TEST_CASE("US-240 Ambient: every lit sprite is tinted by the ambient colour, the interface is not") {
    luna::platform::System system;
    for (const auto choice : {luna::platform::RendererChoice::Sdl, luna::platform::RendererChoice::Gpu}) {
        auto window = openWindow(choice);
        if (!window) continue;
        luna::engine::WindowRenderer renderer(*window);
        const Texture picture = renderer.createTexture(white(8, 8));
        LightFrame frame;
        frame.ambientR = 0.5F;
        frame.ambientG = 0.25F;
        frame.ambientB = 1.0F;
        window->clear(0, 0, 0);
        renderer.setLighting(&frame);
        renderer.draw(picture, {0, 0, 8, 8}, {10, 10});
        renderer.setLighting(nullptr);
        renderer.draw(picture, {0, 0, 8, 8}, {30, 10}); // drawn after the lighting ended: untouched
        const luna::platform::Pixels pixels = window->readPixels();
        window->present();
        const Color lit = screenAt(pixels, 12, 12);
        const Color plain = screenAt(pixels, 32, 12);
        CAPTURE(window->backendName());
        CHECK(std::abs(static_cast<int>(lit.red) - 128) <= 2);
        CHECK(std::abs(static_cast<int>(lit.green) - 64) <= 2);
        CHECK(std::abs(static_cast<int>(lit.blue) - 255) <= 2);
        CHECK(plain.red == 255);
        CHECK(plain.green == 255);
        CHECK(plain.blue == 255);
    }
}

TEST_CASE("US-240 Point light: the side of a sprite that faces the light is the lit one") {
    luna::platform::System system;
    auto window = openWindow(luna::platform::RendererChoice::Gpu);
    if (!window) return; // no graphics card here: the shader path is not run
    luna::engine::WindowRenderer renderer(*window);
    const Texture picture = renderer.createTexture(white(64, 64));
    const Texture normals = renderer.createTexture(dome(64));
    renderer.setNormalMap(picture, normals);
    LightFrame frame;
    frame.ambientR = frame.ambientG = frame.ambientB = 0.15F;
    PointLight light;
    light.x = 100.0F; // to the left of the sprite, level with its middle
    light.y = 135.0F;
    light.radius = 160.0F;
    light.strength = 1.0F;
    light.r = light.g = light.b = 1.0F;
    light.height = 16.0F;
    frame.lights.push_back(light);
    window->clear(0, 0, 0);
    renderer.setLighting(&frame);
    renderer.draw(picture, {0, 0, 64, 64}, {120, 103});
    renderer.setLighting(nullptr);
    const luna::platform::Pixels pixels = window->readPixels();
#pragma warning(suppress : 4996) // getenv is fine here: a test reads one optional setting
    if (const char* folder = std::getenv("ODYSSEUS_EVIDENCE_DIR")) window->saveScreenshot(std::filesystem::path(folder) / "point-light.bmp"); // for docs/evidence
    window->present();
    const Color nearSide = screenAt(pixels, 120 + 6, 103 + 32);  // its left edge: the surface leans toward the light
    const Color farSide = screenAt(pixels, 120 + 58, 103 + 32);  // its right edge: it leans away
    CHECK(nearSide.red > farSide.red + 40);
    CHECK(farSide.red < 80); // the far side keeps little more than the ambient
}

TEST_CASE("US-240 Budget: 64 lights on a screen-sized sprite still fit the frame") {
    luna::platform::System system;
    auto window = openWindow(luna::platform::RendererChoice::Gpu);
    if (!window) return;
    luna::engine::WindowRenderer renderer(*window);
    const Texture picture = renderer.createTexture(white(kVirtualWidth, kVirtualHeight));
    LightFrame frame;
    frame.ambientR = frame.ambientG = frame.ambientB = 0.3F;
    for (int i = 0; i < 64; ++i) {
        PointLight light;
        light.x = 20.0F + static_cast<float>(i % 8) * 60.0F;
        light.y = 20.0F + static_cast<float>(i / 8) * 32.0F;
        light.radius = 90.0F;
        light.strength = 0.5F;
        frame.lights.push_back(light);
    }
    window->setGpuTiming(true);
    double worst = 0.0;
    for (int i = 0; i < 20; ++i) {
        window->clear(0, 0, 0);
        renderer.setLighting(&frame);
        renderer.draw(picture, {0, 0, kVirtualWidth, kVirtualHeight}, {0, 0});
        renderer.setLighting(nullptr);
        window->present();
        worst = std::max(worst, window->gpuMilliseconds());
    }
    MESSAGE("64 lights over the whole screen: the card needed at most ", worst, " ms a frame (the frame is 16.7 ms)");
    CHECK(worst < 8.0);
}
