#include "luna/engine/scaled_renderer.h"

namespace luna::engine {

void ScaledRenderer::draw(const Texture& texture, const Rect& source, Point at) {
    if (scale_ == 1) {
        inner_.draw(texture, source, at);
        return;
    }
    inner_.drawStyled(texture, source, {at.x * scale_, at.y * scale_, source.width * scale_, source.height * scale_}, DrawStyle{});
}

void ScaledRenderer::drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) {
    inner_.drawStyled(texture, source,
                      {destination.x * scale_, destination.y * scale_, destination.width * scale_, destination.height * scale_}, style);
}

Pointer scaledPointer(const Pointer& pointer, int scale) {
    Pointer out = pointer;
    if (scale > 1 && pointer.inside()) {
        out.x = pointer.x / scale;
        out.y = pointer.y / scale;
    }
    return out;
}

} // namespace luna::engine
