#include "luna/platform/window.h"

#include "luna/platform/sdl_events.h"

#include <SDL3/SDL.h>

#include <algorithm>

#include <stdexcept>
#include <string>

namespace luna::platform {

namespace {

[[noreturn]] void fail(const std::string& what) {
    throw std::runtime_error(what + ": " + SDL_GetError());
}

} // namespace

void Window::WindowDeleter::operator()(SDL_Window* window) const {
    SDL_DestroyWindow(window);
}

void Window::RendererDeleter::operator()(SDL_Renderer* renderer) const {
    SDL_DestroyRenderer(renderer);
}

void Window::GamepadDeleter::operator()(SDL_Gamepad* gamepad) const {
    SDL_CloseGamepad(gamepad);
}

Window::Window(const WindowSettings& settings) {
    window_.reset(SDL_CreateWindow(settings.title.c_str(), settings.width, settings.height, SDL_WINDOW_RESIZABLE));
    if (!window_) {
        fail("Cannot open the window");
    }
    renderer_.reset(SDL_CreateRenderer(window_.get(), nullptr));
    if (!renderer_) {
        fail("Cannot create the renderer");
    }
    // Draw at a small virtual size, scaled by the largest whole number that fits the
    // window, with black bars for the rest: pixel art stays crisp (US-022).
    if (!SDL_SetRenderLogicalPresentation(renderer_.get(), settings.virtualWidth, settings.virtualHeight,
                                          SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
        fail("Cannot set the virtual screen size");
    }
    vsync_ = SDL_SetRenderVSync(renderer_.get(), 1);
}

Window::~Window() = default; // the unique_ptrs destroy the renderer, then the window

void Window::pollEvents(std::vector<Event>& events) {
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent)) {
        const std::optional<Event> event = translateEvent(sdlEvent);
        if (!event) {
            continue;
        }
        if (event->type == EventType::GamepadAdded) {
            // A gamepad only sends button and stick events after it is opened.
            if (SDL_Gamepad* gamepad = SDL_OpenGamepad(static_cast<SDL_JoystickID>(event->gamepad))) {
                gamepads_.emplace_back(event->gamepad, GamepadHandle(gamepad));
            }
        } else if (event->type == EventType::GamepadRemoved) {
            std::erase_if(gamepads_, [&](const auto& entry) { return entry.first == event->gamepad; });
        }
        events.push_back(*event);
    }
}

void Window::clear(int red, int green, int blue) {
    // Black bars outside the virtual screen, then the requested colour inside it.
    SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
    SDL_RenderClear(renderer_.get());
    SDL_SetRenderDrawColor(renderer_.get(), static_cast<Uint8>(red), static_cast<Uint8>(green),
                           static_cast<Uint8>(blue), 255);
    SDL_RenderFillRect(renderer_.get(), nullptr);
}

void Window::present() {
    SDL_RenderPresent(renderer_.get());
}

bool Window::vsyncEnabled() const {
    return vsync_;
}

} // namespace luna::platform
