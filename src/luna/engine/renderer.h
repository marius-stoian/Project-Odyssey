#pragma once

#include "boundary.h"

#include "image.h"

#include "core/geometry.h"

#include <cstdint>
#include <vector>

namespace luna::platform {
class Window;
}

namespace luna::engine {

using odysseus::core::Point;
using odysseus::core::Rect;

enum class Blend { Normal, Add };

struct DrawStyle {
    std::uint8_t alpha = 255;
    Blend blend = Blend::Normal;
};

// A texture uploaded to the renderer: its number and size.
struct Texture {
    int id = -1;
    int width = 0;
    int height = 0;
};

// How games draw (ADR-003): an interface, so the real one can later switch from
// SDL_Renderer to SDL_GPU in one place, and tests can record draws without a window.
// All positions are in virtual-screen pixels (480 x 270).
class Renderer {
public:
    virtual ~Renderer() = default;

    virtual Texture createTexture(const Image& image) = 0;

    // Draws the `source` part of `texture` with its top-left corner at `at`.
    virtual void draw(const Texture& texture, const Rect& source, Point at) = 0;

    // Draws `source` stretched onto `destination`, see-through by `style.alpha` (255: solid)
    // and, with Blend::Add, adding light (M2d: effects and weather).
    virtual void drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) = 0;
};

// The real renderer: draws into the Luna window.
class WindowRenderer final : public Renderer {
public:
    explicit WindowRenderer(platform::Window& window);

    Texture createTexture(const Image& image) override;
    void draw(const Texture& texture, const Rect& source, Point at) override;
    void drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) override;

private:
    platform::Window& window_;
};

// A renderer that only writes down what it was asked to draw. For tests.
class RecordingRenderer final : public Renderer {
public:
    struct Draw {
        int texture;
        Rect source;
        Point at;
        Rect destination{};  // drawStyled only: where the source was stretched to
        DrawStyle style{};
        bool styled = false;
    };

    Texture createTexture(const Image& image) override;
    void draw(const Texture& texture, const Rect& source, Point at) override;
    void drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) override;

    const std::vector<Draw>& draws() const { return draws_; }
    void clear() { draws_.clear(); }

private:
    int nextTexture_ = 0;
    std::vector<Draw> draws_;
};

// The largest whole-number scale at which the virtual screen fits the window, and where
// it lands (centred; the rest are black bars). Never below 1.
struct PixelScale {
    int scale = 1;
    Rect area; // in window pixels
};

PixelScale integerScale(int windowWidth, int windowHeight, int virtualWidth, int virtualHeight);

} // namespace luna::engine
