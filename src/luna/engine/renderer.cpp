#include "luna/engine/renderer.h"

#include "luna/platform/window.h"

#include <algorithm>

namespace luna::engine {

WindowRenderer::WindowRenderer(platform::Window& window) : window_(window) {}

void WindowRenderer::measureGpu(bool on) {
    window_.setGpuTiming(on);
}

double WindowRenderer::gpuMilliseconds() const {
    return window_.gpuMilliseconds();
}

Texture WindowRenderer::createTexture(const Image& image) {
    return {window_.createTexture(image.width(), image.height(), image.data()), image.width(), image.height()};
}

void WindowRenderer::destroyTexture(const Texture& texture) {
    window_.destroyTexture(texture.id);
}

void WindowRenderer::draw(const Texture& texture, const Rect& source, Point at) {
    window_.drawTexture(texture.id, source, {at.x, at.y, source.width, source.height});
}

void WindowRenderer::drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) {
    window_.drawTexture(texture.id, source, destination, style.alpha, style.blend == Blend::Add);
}

void WindowRenderer::setLighting(const LightFrame* frame) {
    if (frame == nullptr) {
        window_.setLighting(nullptr);
        return;
    }
    platform::LightingState state;
    state.ambient[0] = frame->ambientR;
    state.ambient[1] = frame->ambientG;
    state.ambient[2] = frame->ambientB;
    state.normalMaps = frame->normalMaps;
    state.count = std::min(static_cast<int>(frame->lights.size()), LightFrame::kMaxLights);
    for (int i = 0; i < state.count; ++i) {
        const PointLight& light = frame->lights[static_cast<std::size_t>(i)];
        state.lights[i] = {light.x, light.y, light.radius, light.strength, light.r, light.g, light.b, light.height};
    }
    window_.setLighting(&state);
}

void WindowRenderer::setNormalMap(const Texture& texture, const Texture& normals) {
    window_.setNormalMap(texture.id, normals.id);
}

Texture RecordingRenderer::createTexture(const Image& image) {
    return {nextTexture_++, image.width(), image.height()};
}

void RecordingRenderer::draw(const Texture& texture, const Rect& source, Point at) {
    draws_.push_back({texture.id, source, at});
    draws_.back().lit = lighting_;
}

void RecordingRenderer::drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) {
    draws_.push_back({texture.id, source, {destination.x, destination.y}, destination, style, true});
    draws_.back().lit = lighting_ && style.blend != Blend::Add;
}

PixelScale integerScale(int windowWidth, int windowHeight, int virtualWidth, int virtualHeight) {
    const int scale = std::max(1, std::min(windowWidth / virtualWidth, windowHeight / virtualHeight));
    const int width = virtualWidth * scale;
    const int height = virtualHeight * scale;
    return {scale, {(windowWidth - width) / 2, (windowHeight - height) / 2, width, height}};
}

} // namespace luna::engine
