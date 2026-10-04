#pragma once

#include "boundary.h"

#include "window.h"

#include "core/geometry.h"
#include "core/presentation.h"

#include <cstdint>
#include <memory>
#include <string>

struct SDL_Window;

namespace luna::platform {

// What the Window draws with (US-230, ADR-021, docs/plans/M8b-renderer-design.md section 2). The Window keeps the operating-system window, the events
// and the gamepads; a backend draws the picture and puts it in the window. There are two: SDL_Renderer (the first one, kept as fallback and for
// tests) and SDL_GPU with shaders. Both draw the same pictures, to the pixel (the tests compare them).
class RenderBackend {
public:
    virtual ~RenderBackend() = default;

    virtual std::string name() const = 0; // "sdl (direct3d11)", "gpu (direct3d12)"
    virtual bool vsyncEnabled() const = 0;
    // GPU time (US-234): SDL_GPU has no timestamp queries, so with timing on, present() waits for the card to finish the frame and
    // measures submit-to-done. It costs a little speed, so it is off unless the overlay (F3) or a performance run asks. -1: not measured.
    // Light (US-240): see Window::setLighting. The SDL_Renderer fallback only tints by the ambient colour.
    virtual void setLighting(const LightingState*) {}
    virtual void setNormalMap(int /*texture*/, int /*normals*/) {}
    virtual void setGpuTiming(bool) {}
    virtual double gpuMilliseconds() const { return -1.0; }

    // A new frame: black bars outside the virtual screen, `red green blue` inside it.
    virtual void clear(int red, int green, int blue) = 0;
    virtual void present() = 0;

    virtual int createTexture(int width, int height, const std::uint8_t* rgba) = 0;
    virtual void drawTexture(int texture, const odysseus::core::Rect& source, const odysseus::core::Rect& destination, std::uint8_t alpha, bool additive) = 0;

    virtual odysseus::core::Rect presentationRect() const = 0; // where the virtual screen lands in the window, in real pixels
    virtual odysseus::core::Rect outputRect() const = 0;       // the size of the drawing surface in real pixels
    virtual Pixels readPixels() = 0;                           // the frame as drawn so far, black bars included
    virtual void setScalingMode(odysseus::core::ScalingMode mode) = 0;
};

// When a sprite is drawn at another size than its own (three quarters, for the children), some pixels of it look at exactly the line between two
// texels, and which texel they take then depends on how the last bit of a float rounds: the two backends could choose differently. Both look a
// five-hundredth of a texel further right and down, which settles every such case the same way (and moves no other pixel).
inline constexpr float kTexelNudge = 1.0F / 512.0F;

std::unique_ptr<RenderBackend> makeSdlRendererBackend(SDL_Window* window, int virtualWidth, int virtualHeight, odysseus::core::ScalingMode scaling = odysseus::core::ScalingMode::Whole);

// True when this build has the GPU backend (the shaders could be compiled with the Windows SDK's dxc.exe).
bool gpuBackendCompiledIn();
// Throws std::runtime_error with the reason when the GPU cannot be used (no device, no swapchain, a shader that does not load). Never returns null.
std::unique_ptr<RenderBackend> makeGpuBackend(SDL_Window* window, int virtualWidth, int virtualHeight, odysseus::core::ScalingMode scaling = odysseus::core::ScalingMode::Whole);

} // namespace luna::platform
