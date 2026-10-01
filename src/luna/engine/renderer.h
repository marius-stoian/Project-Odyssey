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

// Light (US-240). Positions, radii and heights are in the pixels the game draws in (virtual pixels, or zoomed pixels under a ScaledRenderer).
struct PointLight {
    float x = 0.0F, y = 0.0F;
    float radius = 0.0F;
    float strength = 1.0F;
    float r = 1.0F, g = 1.0F, b = 1.0F;
    float height = 24.0F;
};

// What lights the world drawn after it is set: the ambient colour (already times its strength; 1 1 1 leaves every sprite as it is) and the
// point lights, at most kMaxLights (the rest are ignored).
struct LightFrame {
    static constexpr int kMaxLights = 64;
    float ambientR = 1.0F, ambientG = 1.0F, ambientB = 1.0F;
    std::vector<PointLight> lights;
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

    // Light (US-240): until `setLighting(nullptr)`, normal-blend draws are lit by `frame` (the interface is drawn after it is cleared, so it is
    // never dimmed). `setNormalMap` gives a texture a normal map of the same size (red: x, green: y, blue: z, each 0..255 for -1..1); without one a
    // sprite is flat. The fallback renderer only tints by the ambient colour.
    virtual void setLighting(const LightFrame* /*frame*/) {}
    virtual void setNormalMap(const Texture& /*texture*/, const Texture& /*normals*/) {}

    // GPU time of the last frame in milliseconds, -1 when it is not measured (US-234). measureGpu(true) turns the measuring on.
    virtual void measureGpu(bool) {}
    virtual double gpuMilliseconds() const { return -1.0; }
};

// The real renderer: draws into the Luna window.
class WindowRenderer final : public Renderer {
public:
    explicit WindowRenderer(platform::Window& window);

    Texture createTexture(const Image& image) override;
    void draw(const Texture& texture, const Rect& source, Point at) override;
    void drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) override;
    void measureGpu(bool on) override;
    double gpuMilliseconds() const override;
    void setLighting(const LightFrame* frame) override;
    void setNormalMap(const Texture& texture, const Texture& normals) override;

private:
    platform::Window& window_;
};

// A renderer that only writes down what it was asked to draw. For tests.
class RecordingRenderer : public Renderer {
public:
    struct Draw {
        int texture;
        Rect source;
        Point at;
        Rect destination{};  // drawStyled only: where the source was stretched to
        DrawStyle style{};
        bool styled = false;
        bool lit = false;    // drawn while a lighting frame was set (US-240)
    };

    Texture createTexture(const Image& image) override;
    void draw(const Texture& texture, const Rect& source, Point at) override;
    void drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) override;

    void setLighting(const LightFrame* frame) override {
        lighting_ = frame != nullptr;
        if (frame != nullptr) lastLighting_ = *frame;
    }
    void setNormalMap(const Texture& texture, const Texture& normals) override { normalMaps_.emplace_back(texture.id, normals.id); }

    const std::vector<Draw>& draws() const { return draws_; }
    void clear() { draws_.clear(); }
    const LightFrame& lighting() const { return lastLighting_; } // the last frame set
    bool lightingOn() const { return lighting_; }
    const std::vector<std::pair<int, int>>& normalMaps() const { return normalMaps_; }

private:
    bool lighting_ = false;
    LightFrame lastLighting_;
    std::vector<std::pair<int, int>> normalMaps_;
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
