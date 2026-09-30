#pragma once

#include "boundary.h"

#include "events.h"

#include "core/geometry.h"

#include <cstdint>
#include <filesystem>

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Gamepad;
struct SDL_Texture;

namespace luna::platform {

struct WindowSettings {
    std::string title;
    int width = 1280;
    int height = 720;
    int virtualWidth = 480;  // everything is drawn at this size, then scaled up
    int virtualHeight = 270;
    bool hidden = false;     // tests draw into a window nobody sees
};

// A screenshot: width x height pixels, 4 bytes each (red, green, blue, alpha).
struct Pixels {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};

// The game window and its drawing surface (SDL_Window + SDL_Renderer). RAII: both are
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
    struct RendererDeleter {
        void operator()(SDL_Renderer* renderer) const;
    };
    struct GamepadDeleter {
        void operator()(SDL_Gamepad* gamepad) const;
    };
    using GamepadHandle = std::unique_ptr<SDL_Gamepad, GamepadDeleter>;
    struct TextureDeleter {
        void operator()(SDL_Texture* texture) const;
    };

    std::unique_ptr<SDL_Window, WindowDeleter> window_;
    std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
    std::vector<std::pair<int, GamepadHandle>> gamepads_; // (gamepad id, open gamepad)
    std::vector<std::unique_ptr<SDL_Texture, TextureDeleter>> textures_; // index = texture number
    bool vsync_ = false;
};

} // namespace luna::platform
