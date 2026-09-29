#pragma once

#include "boundary.h"

#include "events.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Gamepad;

namespace luna::platform {

struct WindowSettings {
    std::string title;
    int width = 1280;
    int height = 720;
    int virtualWidth = 480;  // everything is drawn at this size, then scaled up
    int virtualHeight = 270;
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

    std::unique_ptr<SDL_Window, WindowDeleter> window_;
    std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
    std::vector<std::pair<int, GamepadHandle>> gamepads_; // (gamepad id, open gamepad)
    bool vsync_ = false;
};

} // namespace luna::platform
