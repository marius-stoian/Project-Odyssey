#include "luna/engine/renderer.h"

#include "luna/platform/window.h"

#include <algorithm>

namespace luna::engine {

WindowRenderer::WindowRenderer(platform::Window& window) : window_(window) {}

Texture WindowRenderer::createTexture(const Image& image) {
    return {window_.createTexture(image.width(), image.height(), image.data()), image.width(), image.height()};
}

void WindowRenderer::draw(const Texture& texture, const Rect& source, Point at) {
    window_.drawTexture(texture.id, source, {at.x, at.y, source.width, source.height});
}

void WindowRenderer::drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) {
    window_.drawTexture(texture.id, source, destination, style.alpha, style.blend == Blend::Add);
}

Texture RecordingRenderer::createTexture(const Image& image) {
    return {nextTexture_++, image.width(), image.height()};
}

void RecordingRenderer::draw(const Texture& texture, const Rect& source, Point at) {
    draws_.push_back({texture.id, source, at});
}

void RecordingRenderer::drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) {
    draws_.push_back({texture.id, source, {destination.x, destination.y}, destination, style, true});
}

PixelScale integerScale(int windowWidth, int windowHeight, int virtualWidth, int virtualHeight) {
    const int scale = std::max(1, std::min(windowWidth / virtualWidth, windowHeight / virtualHeight));
    const int width = virtualWidth * scale;
    const int height = virtualHeight * scale;
    return {scale, {(windowWidth - width) / 2, (windowHeight - height) / 2, width, height}};
}

} // namespace luna::engine
