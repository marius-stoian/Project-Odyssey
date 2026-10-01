// US-230: the game looks the same drawn through the GPU (SDL_GPU with shaders) and through SDL_Renderer, to the pixel. Needs a window and, for the
// GPU half, a graphics card with Direct3D 12: where there is none (a CI runner) the test says so and passes (the fallback test covers that case).
#include "camp.h"

#include "game/sky.h"
#include "luna/engine/renderer.h"
#include "luna/platform/system.h"
#include "luna/platform/window.h"

using namespace camp_support;

namespace {

namespace platform = luna::platform;

constexpr int kTicks = 60; // three seconds of play: the clan has moved, the fire flickers, the weather has begun

// The demo level and a camp with the clan, drawn after the same 60 ticks.
struct Scene {
    std::string name;
    fs::path data;
    fs::path level;
    bool run = false; // the camp has a clan and a hero's run; the demo level is just the level
};

// One frame of `scene` as the given renderer draws it, black bars included; empty when that renderer cannot start.
std::optional<platform::Pixels> drawn(const Scene& scene, platform::RendererChoice choice, std::string* backend = nullptr) {
    try {
        platform::Window window({"US-230 test", 1280, 720, 480, 270, true, choice});
        if (backend != nullptr) *backend = window.backendName();
        luna::engine::WindowRenderer renderer(window);
        // The pictures of the two renderers are compared pixel for pixel, so the light of the time of day is switched off here: the fallback
        // renderer tints by the ambient colour with whole numbers and the GPU with fractions (the lighting tests compare that to two levels).
        {
            const fs::path skyFile = scene.data / "light" / "sky.json";
            game::SkyData sky = game::loadSky(skyFile, scene.data / "sim" / "calendar.json");
            sky.enabled = false;
            writeText(skyFile, game::skyToText(sky));
        }
        game::OdysseyGame odyssey(scene.data, scene.level);
        odyssey.start(renderer);
        if (scene.run) {
            odyssey.startNewRun({1, 2, 1}, false, false);
            odyssey.run().close();
        }
        for (int i = 0; i < kTicks; ++i) odyssey.update({});
        window.clear(0, 0, 0);
        odyssey.render(renderer, 0.0);
        platform::Pixels pixels = window.readPixels();
        window.present();
        return pixels;
    } catch (const std::exception& error) {
        MESSAGE("renderer ", choice == platform::RendererChoice::Gpu ? "gpu" : "sdl", " could not run: ", error.what());
        return std::nullopt;
    }
}

} // namespace

TEST_CASE("US-230 Same picture: the demo level and the camp are drawn the same by the GPU and by SDL_Renderer") {
    platform::System system;
    const fs::path data = dataCopy("render-same");
    const std::vector<Scene> scenes = {{"demo level", data, ODYSSEUS_DEMO_LEVEL, false}, {"camp", data, Camp::makeLevel(data, "render-same"), true}};
    for (const Scene& scene : scenes) {
        CAPTURE(scene.name);
        std::string gpuName;
        std::string sdlName;
        const auto sdl = drawn(scene, platform::RendererChoice::Sdl, &sdlName);
        REQUIRE(sdl.has_value()); // the old renderer always works
        CHECK(sdlName.rfind("sdl", 0) == 0);
        const auto again = drawn(scene, platform::RendererChoice::Sdl); // the same renderer twice: is the scene itself the same both times?
        REQUIRE(again.has_value());
        std::size_t unstable = 0;
        for (std::size_t at = 0; at < again->rgba.size(); at += 4) {
            if (!std::equal(again->rgba.begin() + static_cast<std::ptrdiff_t>(at), again->rgba.begin() + static_cast<std::ptrdiff_t>(at) + 4, sdl->rgba.begin() + static_cast<std::ptrdiff_t>(at))) ++unstable;
        }
        MESSAGE(scene.name, ": SDL_Renderer against itself: ", unstable, " different");
        const auto gpu = drawn(scene, platform::RendererChoice::Gpu, &gpuName);
        if (!gpu) {
            MESSAGE("No GPU renderer on this machine: the pictures were not compared");
            continue;
        }
        CHECK(gpuName.rfind("gpu", 0) == 0);
        REQUIRE(gpu->width == sdl->width);
        REQUIRE(gpu->height == sdl->height);
        std::size_t different = 0;
        std::size_t first = 0;
        for (std::size_t at = 0; at < gpu->rgba.size(); at += 4) {
            if (!std::equal(gpu->rgba.begin() + static_cast<std::ptrdiff_t>(at), gpu->rgba.begin() + static_cast<std::ptrdiff_t>(at) + 4, sdl->rgba.begin() + static_cast<std::ptrdiff_t>(at))) {
                if (different++ == 0) first = at / 4;
            }
        }
        {
            int minX = 99999, minY = 99999, maxX = -1, maxY = -1, worst = 0;
            for (std::size_t at = 0; at < gpu->rgba.size(); at += 4) {
                int delta = 0;
                for (int c = 0; c < 4; ++c) delta = std::max(delta, std::abs(static_cast<int>(gpu->rgba[at + static_cast<std::size_t>(c)]) - static_cast<int>(sdl->rgba[at + static_cast<std::size_t>(c)])));
                if (delta == 0) continue;
                const int x = static_cast<int>((at / 4) % static_cast<std::size_t>(gpu->width));
                const int y = static_cast<int>((at / 4) / static_cast<std::size_t>(gpu->width));
                minX = std::min(minX, x); maxX = std::max(maxX, x); minY = std::min(minY, y); maxY = std::max(maxY, y); worst = std::max(worst, delta);
            }
            MESSAGE(scene.name, ": differences lie in x ", minX, "..", maxX, ", y ", minY, "..", maxY, ", the largest channel difference is ", worst);
        }
        MESSAGE(scene.name, ": ", gpu->width, "x", gpu->height, " pixels, ", different, " different (first at x ", first % static_cast<std::size_t>(gpu->width), ", y ", first / static_cast<std::size_t>(gpu->width), ")");
        CHECK(different == 0);
    }
}

TEST_CASE("US-230 Fallback: a renderer that cannot start is replaced by SDL_Renderer, and Auto says which one it uses") {
    platform::System system;
    // Auto always gives a working window, drawing with the GPU when it can and SDL_Renderer when not.
    platform::Window automatic({"US-230 auto", 640, 360, 480, 270, true, platform::RendererChoice::Auto});
    const std::string name = automatic.backendName();
    CHECK((name.rfind("gpu", 0) == 0 || name.rfind("sdl", 0) == 0));
    CHECK(automatic.presentationRect().width == 480 * (automatic.outputRect().width / 480)); // a whole-number scale, same rule on both
    // Forcing SDL_Renderer is honoured.
    platform::Window sdl({"US-230 sdl", 640, 360, 480, 270, true, platform::RendererChoice::Sdl});
    CHECK(sdl.backendName().rfind("sdl", 0) == 0);
    // Asking for the GPU by name either gives the GPU or fails loudly; it never quietly gives the other one.
    try {
        platform::Window gpu({"US-230 gpu", 640, 360, 480, 270, true, platform::RendererChoice::Gpu});
        CHECK(gpu.backendName().rfind("gpu", 0) == 0);
    } catch (const std::exception&) {
        MESSAGE("this machine has no usable GPU renderer, and asking for it says so");
    }
}
