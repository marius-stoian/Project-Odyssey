#pragma once

#include "boundary.h"

#include "events.h"

#include "core/geometry.h"
#include "core/presentation.h"

#include <cstdint>
#include <filesystem>

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct SDL_Window;
struct SDL_Gamepad;

namespace luna::platform {

// What the window draws with (US-230): the GPU (SDL_GPU with shaders), SDL_Renderer, or Auto: the GPU when it works, else SDL_Renderer with the reason logged.
enum class RendererChoice { Auto, Gpu, Sdl };

struct WindowSettings {
    std::string title;
    int width = 1280;
    int height = 720;
    int virtualWidth = odysseus::core::kVirtualWidth;
    int virtualHeight = odysseus::core::kVirtualHeight;
    bool hidden = false;     // tests draw into a window nobody sees
    RendererChoice renderer = RendererChoice::Auto;
    odysseus::core::WindowMode mode = odysseus::core::WindowMode::Windowed;
    odysseus::core::ScalingMode scaling = odysseus::core::ScalingMode::Whole;
};

// A screenshot: width x height pixels, 4 bytes each (red, green, blue, alpha).
struct Pixels {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};

class RenderBackend;

// The game window and its drawing surface (an SDL_Window and a RenderBackend: SDL_GPU or SDL_Renderer). RAII: both are
// destroyed with the Window. Needs a live System.
class Window {
public:
    explicit Window(const WindowSettings& settings);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Appends everything that happened since the last call to `events`. Also opens
    // gamepads when they are plugged in and closes them when they are removed.
    void pollEvents(std::vector<Event>& events);

    void clear(int red, int green, int blue);
    void present();

    // True when presenting waits for the monitor's refresh (smooth, no wasted CPU).
    bool vsyncEnabled() const;
    void setGpuTiming(bool on);
    double gpuMilliseconds() const; // the last frame's time on the card; -1 when not measured (SDL renderer, or timing off)

    // Which backend draws: "gpu (direct3d12)" or "sdl (direct3d11)" (logged at start).
    std::string backendName() const;

    // Uploads an image (4 bytes per pixel: red, green, blue, alpha) and returns its number.
    // Textures are sampled nearest-neighbour: pixels stay square when scaled up.
    int createTexture(int width, int height, const std::uint8_t* rgba);

    // Draws part of a texture at a position on the virtual screen (virtual pixels).
    void drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination);
    // The same with see-through `alpha` (0-255) and, with `additive`, its colours added to what
    // is below (glows, light rain), used by effects and weather (M2d).
    void drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination, std::uint8_t alpha, bool additive);

    // Where the virtual screen lands in the window, in real pixels (the scaled picture
    // without the black bars).
    odysseus::core::Rect presentationRect() const;

    void setSize(int width, int height);
    // Full screen or a window (US-081): takes effect at once; the window keeps its size for going back.
    void setFullscreen(bool fullscreen);
    bool fullscreen() const;
    void applyResolution(const odysseus::core::Resolution& resolution);
    void setScalingMode(odysseus::core::ScalingMode mode);

    // The size of the drawing surface in real pixels (can differ from the requested
    // window size on high-DPI screens).
    odysseus::core::Rect outputRect() const;

    // Reads back the whole window as drawn this frame, black bars included. Slow; for
    // tests. Call before present().
    Pixels readPixels();

    // Saves this frame (whole window) as a .bmp picture. Call before present().
    void saveScreenshot(const std::filesystem::path& file);

private:
    // Deleters let unique_ptr call SDL's destroy functions for us.
    struct WindowDeleter {
        void operator()(SDL_Window* window) const;
    };
    struct GamepadDeleter {
        void operator()(SDL_Gamepad* gamepad) const;
    };
    using GamepadHandle = std::unique_ptr<SDL_Gamepad, GamepadDeleter>;
    // Destroyed in reverse order: the gamepads, the backend (its textures and device), then the window.
    std::unique_ptr<SDL_Window, WindowDeleter> window_;
    std::unique_ptr<RenderBackend> backend_;
    std::vector<std::pair<int, GamepadHandle>> gamepads_; // (gamepad id, open gamepad)
};

} // namespace luna::platform
