// The SDL_Renderer backend: the drawing code the Window had before US-230, moved here. One change, made so that both backends draw the very same
// pictures (the pixel-for-pixel test of US-230): everything is drawn onto a texture of the virtual-screen size first and that texture is enlarged into the
// window, as the GPU backend does. Before, SDL enlarged each sprite by itself, so a sprite drawn at three quarters of its size (the children) got uneven
// pixels; now every virtual pixel is one even block.
#include "luna/platform/backend.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace luna::platform {

namespace {

[[noreturn]] void fail(const std::string& what) {
    throw std::runtime_error(what + ": " + SDL_GetError());
}

class SdlRendererBackend final : public RenderBackend {
public:
    SdlRendererBackend(SDL_Window* window, int virtualWidth, int virtualHeight) : window_(window) {
        renderer_.reset(SDL_CreateRenderer(window, nullptr));
        if (!renderer_) {
            fail("Cannot create the renderer");
        }
        // Draw at a small virtual size, scaled by the largest whole number that fits the
        // window, with black bars for the rest: pixel art stays crisp (US-022).
        if (!SDL_SetRenderLogicalPresentation(renderer_.get(), virtualWidth, virtualHeight, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE)) {
            fail("Cannot set the virtual screen size");
        }
        virtualScreen_.reset(SDL_CreateTexture(renderer_.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET, virtualWidth, virtualHeight));
        if (!virtualScreen_) {
            fail("Cannot create the virtual screen");
        }
        SDL_SetTextureScaleMode(virtualScreen_.get(), SDL_SCALEMODE_NEAREST);
        vsync_ = SDL_SetRenderVSync(renderer_.get(), 1);
    }

    std::string name() const override { return std::string("sdl (") + SDL_GetRendererName(renderer_.get()) + ")"; }
    bool vsyncEnabled() const override { return vsync_; }

    void clear(int red, int green, int blue) override {
        // A new frame starts on the virtual screen: the requested colour all over it.
        SDL_SetRenderTarget(renderer_.get(), virtualScreen_.get());
        SDL_SetRenderDrawColor(renderer_.get(), static_cast<Uint8>(red), static_cast<Uint8>(green), static_cast<Uint8>(blue), 255);
        SDL_RenderClear(renderer_.get());
    }

    void present() override {
        compose();
        SDL_RenderPresent(renderer_.get());
    }

    int createTexture(int width, int height, const std::uint8_t* rgba) override {
        SDL_Texture* texture = SDL_CreateTexture(renderer_.get(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, width, height);
        if (!texture) {
            fail("Cannot create a texture");
        }
        textures_.emplace_back(texture);
        SDL_UpdateTexture(texture, nullptr, rgba, width * 4);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);   // transparent pixels stay transparent
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST); // square pixels, never blurred
        return static_cast<int>(textures_.size()) - 1;
    }

    void drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination, std::uint8_t alpha, bool additive) override {
        SDL_Texture* picture = textures_.at(static_cast<std::size_t>(texture)).get();
        const bool plain = alpha == 255 && !additive;
        if (!plain) {
            SDL_SetTextureAlphaMod(picture, alpha);
            SDL_SetTextureBlendMode(picture, additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        }
        const SDL_FRect from{static_cast<float>(source.x) + kTexelNudge, static_cast<float>(source.y) + kTexelNudge, static_cast<float>(source.width), static_cast<float>(source.height)};
        const SDL_FRect to{static_cast<float>(destination.x), static_cast<float>(destination.y), static_cast<float>(destination.width),
                           static_cast<float>(destination.height)};
        SDL_RenderTexture(renderer_.get(), picture, &from, &to);
        if (!plain) {
            // Back to plain drawing: other draws of this texture must not inherit the style.
            SDL_SetTextureAlphaMod(picture, 255);
            SDL_SetTextureBlendMode(picture, SDL_BLENDMODE_BLEND);
        }
    }

    odysseus::core::Rect presentationRect() const override {
        SDL_FRect rect{};
        SDL_GetRenderLogicalPresentationRect(renderer_.get(), &rect);
        return {static_cast<int>(rect.x), static_cast<int>(rect.y), static_cast<int>(rect.w), static_cast<int>(rect.h)};
    }

    odysseus::core::Rect outputRect() const override {
        int width = 0;
        int height = 0;
        SDL_GetRenderOutputSize(renderer_.get(), &width, &height);
        return {0, 0, width, height};
    }

    Pixels readPixels() override {
        // With the virtual screen active, SDL reads only the picture area; switch it off for
        // the read so the black bars are included, then switch it back on.
        int logicalWidth = 0;
        int logicalHeight = 0;
        SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        compose(); // the frame so far, enlarged into the window
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
            std::copy(source, source + static_cast<std::ptrdiff_t>(rgba->w) * 4, pixels.rgba.begin() + static_cast<std::ptrdiff_t>(row) * rgba->w * 4);
        }
        SDL_DestroySurface(rgba);
        return pixels;
    }

private:
    // The virtual screen into the window: black bars, then the picture enlarged by a whole number (the logical presentation does the enlarging).
    void compose() {
        SDL_SetRenderTarget(renderer_.get(), nullptr);
        SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
        SDL_RenderClear(renderer_.get());
        SDL_RenderTexture(renderer_.get(), virtualScreen_.get(), nullptr, nullptr);
    }

    struct RendererDeleter {
        void operator()(SDL_Renderer* renderer) const { SDL_DestroyRenderer(renderer); }
    };
    struct TextureDeleter {
        void operator()(SDL_Texture* texture) const { SDL_DestroyTexture(texture); }
    };

    SDL_Window* window_ = nullptr; // owned by the Window, which outlives its backend
    // Members are destroyed in reverse order: the textures first, then the renderer.
    std::unique_ptr<SDL_Renderer, RendererDeleter> renderer_;
    std::vector<std::unique_ptr<SDL_Texture, TextureDeleter>> textures_; // index = texture number
    std::unique_ptr<SDL_Texture, TextureDeleter> virtualScreen_;         // destroyed before the renderer too (declared after the renderer)
    bool vsync_ = false;
};

} // namespace

odysseus::core::Rect wholeStepArea(int windowWidth, int windowHeight, int virtualWidth, int virtualHeight) {
    const int scale = std::max(1, std::min(windowWidth / virtualWidth, windowHeight / virtualHeight));
    const int width = virtualWidth * scale;
    const int height = virtualHeight * scale;
    return {(windowWidth - width) / 2, (windowHeight - height) / 2, width, height};
}

std::unique_ptr<RenderBackend> makeSdlRendererBackend(SDL_Window* window, int virtualWidth, int virtualHeight) {
    return std::make_unique<SdlRendererBackend>(window, virtualWidth, virtualHeight);
}

} // namespace luna::platform
