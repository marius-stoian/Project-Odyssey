#include "luna/platform/window.h"

#include "luna/platform/backend.h"
#include "luna/platform/sdl_events.h"

#include "core/log.h"

#include <SDL3/SDL.h>

#include <algorithm>

#include <format>
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

void Window::GamepadDeleter::operator()(SDL_Gamepad* gamepad) const {
    SDL_CloseGamepad(gamepad);
}

Window::Window(const WindowSettings& settings) {
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | (settings.hidden ? SDL_WINDOW_HIDDEN : 0);
    const auto openWindow = [&] {
        window_.reset(SDL_CreateWindow(settings.title.c_str(), settings.width, settings.height, flags));
        if (!window_) {
            fail("Cannot open the window");
        }
        SDL_StartTextInput(window_.get()); // typed characters arrive as TextInput (for text fields)
    };
    openWindow();

    // The GPU first, unless asked not to. When it cannot start, Auto says why and goes on with SDL_Renderer (a fresh window: a window the GPU
    // device claimed is not used for SDL_Renderer); asking for the GPU by name fails loudly instead.
    if (settings.renderer != RendererChoice::Sdl) {
        try {
            if (!gpuBackendCompiledIn()) {
                throw std::runtime_error("this build has no GPU backend (the Windows SDK's dxc.exe was not found when it was made)");
            }
            backend_ = makeGpuBackend(window_.get(), settings.virtualWidth, settings.virtualHeight);
        } catch (const std::exception& error) {
            if (settings.renderer == RendererChoice::Gpu) {
                throw;
            }
            odysseus::core::logWarning(std::format("The GPU renderer cannot start ({}); using SDL_Renderer instead", error.what()));
            openWindow();
        }
    }
    if (!backend_) {
        backend_ = makeSdlRendererBackend(window_.get(), settings.virtualWidth, settings.virtualHeight);
    }
}

Window::~Window() = default; // members are destroyed in reverse order: gamepads, the backend (textures and device), the window

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
        Event translated = *event;
        if (translated.type == EventType::MouseMoved || translated.type == EventType::MouseButtonDown ||
            translated.type == EventType::MouseButtonUp) {
            // SDL reports the mouse in window points; Luna works in real pixels (high-DPI screens).
            const float density = SDL_GetWindowPixelDensity(window_.get());
            translated.x *= density;
            translated.y *= density;
        }
        events.push_back(translated);
    }
}

void Window::clear(int red, int green, int blue) {
    backend_->clear(red, green, blue);
}

void Window::present() {
    backend_->present();
}

bool Window::vsyncEnabled() const {
    return backend_->vsyncEnabled();
}

std::string Window::backendName() const {
    return backend_->name();
}

int Window::createTexture(int width, int height, const std::uint8_t* rgba) {
    return backend_->createTexture(width, height, rgba);
}

void Window::drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination) {
    backend_->drawTexture(texture, source, destination, 255, false);
}

void Window::drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination, std::uint8_t alpha, bool additive) {
    backend_->drawTexture(texture, source, destination, alpha, additive);
}

odysseus::core::Rect Window::presentationRect() const {
    return backend_->presentationRect();
}

void Window::setFullscreen(bool fullscreen) {
    SDL_SetWindowFullscreen(window_.get(), fullscreen);
    SDL_SyncWindow(window_.get());
}

bool Window::fullscreen() const {
    return (SDL_GetWindowFlags(window_.get()) & SDL_WINDOW_FULLSCREEN) != 0;
}

void Window::setSize(int width, int height) {
    SDL_SetWindowSize(window_.get(), width, height);
    SDL_SyncWindow(window_.get()); // wait until the operating system has applied it
}

odysseus::core::Rect Window::outputRect() const {
    return backend_->outputRect();
}

Pixels Window::readPixels() {
    return backend_->readPixels();
}

void Window::saveScreenshot(const std::filesystem::path& file) {
    Pixels pixels = readPixels();
    SDL_Surface* surface = SDL_CreateSurfaceFrom(pixels.width, pixels.height, SDL_PIXELFORMAT_RGBA32, pixels.rgba.data(),
                                                 pixels.width * 4);
    if (!surface) {
        fail("Cannot prepare the screenshot");
    }
    const std::u8string path = file.u8string();
    const bool saved = SDL_SaveBMP(surface, reinterpret_cast<const char*>(path.c_str()));
    SDL_DestroySurface(surface);
    if (!saved) {
        fail("Cannot save the screenshot");
    }
}

} // namespace luna::platform
