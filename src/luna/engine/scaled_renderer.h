#pragma once

#include "boundary.h"

#include "luna/engine/input.h"
#include "luna/engine/renderer.h"

namespace luna::engine {

// Draws everything a whole number of times larger (US-232): the world at camera zoom 2x, the interface
// at UI scale 2x. The game keeps laying things out in its own small pixels; this turns each one into
// a scale x scale block, so pixel art and the 5x7 font stay crisp.
class ScaledRenderer final : public Renderer {
public:
    ScaledRenderer(Renderer& inner, int scale) : inner_(inner), scale_(scale < 1 ? 1 : scale) {}

    Texture createTexture(const Image& image) override { return inner_.createTexture(image); }
    void draw(const Texture& texture, const Rect& source, Point at) override;
    void drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) override;

    int scale() const { return scale_; }

private:
    Renderer& inner_;
    int scale_;
};

// The pointer as the scaled picture sees it: window-virtual pixels divided by the scale.
Pointer scaledPointer(const Pointer& pointer, int scale);

} // namespace luna::engine
