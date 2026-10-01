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

void Window::TextureDeleter::operator()(SDL_Texture* texture) const {
    SDL_DestroyTexture(texture);
}

Window::Window(const WindowSettings& settings) {
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | (settings.hidden ? SDL_WINDOW_HIDDEN : 0);
    window_.reset(SDL_CreateWindow(settings.title.c_str(), settings.width, settings.height, flags));
    if (!window_) {
        fail("Cannot open the window");
    }
    SDL_StartTextInput(window_.get()); // typed characters arrive as TextInput (for text fields)
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

Window::~Window() = default; // members are destroyed in reverse order: textures, gamepads, renderer, window

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

int Window::createTexture(int width, int height, const std::uint8_t* rgba) {
    SDL_Texture* texture = SDL_CreateTexture(renderer_.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
    if (!texture) {
        fail("Cannot create a texture");
    }
    textures_.emplace_back(texture);
    SDL_UpdateTexture(texture, nullptr, rgba, width * 4);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND); // transparent pixels stay transparent
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST); // square pixels, never blurred
    return static_cast<int>(textures_.size()) - 1;
}

void Window::drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination) {
    const SDL_FRect from{static_cast<float>(source.x), static_cast<float>(source.y), static_cast<float>(source.width),
                         static_cast<float>(source.height)};
    const SDL_FRect to{static_cast<float>(destination.x), static_cast<float>(destination.y),
                       static_cast<float>(destination.width), static_cast<float>(destination.height)};
    SDL_RenderTexture(renderer_.get(), textures_.at(static_cast<std::size_t>(texture)).get(), &from, &to);
}

void Window::drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination, std::uint8_t alpha, bool additive) {
    SDL_Texture* picture = textures_.at(static_cast<std::size_t>(texture)).get();
    SDL_SetTextureAlphaMod(picture, alpha);
    SDL_SetTextureBlendMode(picture, additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    drawTexture(texture, source, destination);
    // Back to plain drawing: other draws of this texture must not inherit the style.
    SDL_SetTextureAlphaMod(picture, 255);
    SDL_SetTextureBlendMode(picture, SDL_BLENDMODE_BLEND);
}

odysseus::core::Rect Window::presentationRect() const {
    SDL_FRect rect{};
    SDL_GetRenderLogicalPresentationRect(renderer_.get(), &rect);
    return {static_cast<int>(rect.x), static_cast<int>(rect.y), static_cast<int>(rect.w), static_cast<int>(rect.h)};
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
    int width = 0;
    int height = 0;
    SDL_GetRenderOutputSize(renderer_.get(), &width, &height);
    return {0, 0, width, height};
}

Pixels Window::readPixels() {
    // With the virtual screen active, SDL reads only the picture area; switch it off for
    // the read so the black bars are included, then switch it back on.
    int logicalWidth = 0;
    int logicalHeight = 0;
    SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
    SDL_GetRenderLogicalPresentation(renderer_.get(), &logicalWidth, &logicalHeight, &mode);
    SDL_SetRenderLogicalPresentation(renderer_.get(), 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_Surface* raw = SDL_RenderReadPixels(renderer_.get(), nullptr);
    SDL_SetRenderLogicalPresentation(renderer_.get(), logicalWidth, logicalHeight, mode);
    if (!raw) {
        fail("Cannot read the screen");
    }
    SDL_Surface* rgba = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(raw);
    if (!rgba) {
        fail("Cannot convert the screen pixels");
    }
    Pixels pixels;
    pixels.width = rgba->w;
    pixels.height = rgba->h;
    pixels.rgba.resize(static_cast<std::size_t>(rgba->w) * static_cast<std::size_t>(rgba->h) * 4);
    for (int row = 0; row < rgba->h; ++row) {
        const auto* source = static_cast<const std::uint8_t*>(rgba->pixels) + static_cast<std::ptrdiff_t>(row) * rgba->pitch;
        std::copy(source, source + static_cast<std::ptrdiff_t>(rgba->w) * 4,
                  pixels.rgba.begin() + static_cast<std::ptrdiff_t>(row) * rgba->w * 4);
    }
    SDL_DestroySurface(rgba);
    return pixels;
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
