#include "luna/engine/effects.h"

#include <algorithm>
#include <cmath>

namespace luna::engine {

int EffectPlayer::start(const EffectSpec& spec, double x, double y, int size, bool onScreen) {
    if (spec.frames.empty()) {
        return -1;
    }
    const int handle = nextHandle_++;
    running_.push_back({handle, spec, x, y, size, onScreen});
    return handle;
}

void EffectPlayer::stop(int handle) {
    std::erase_if(running_, [handle](const Running& effect) { return effect.handle == handle; });
}

bool EffectPlayer::isRunning(int handle) const {
    return std::any_of(running_.begin(), running_.end(), [handle](const Running& effect) { return effect.handle == handle; });
}

void EffectPlayer::update() {
    for (Running& effect : running_) {
        ++effect.age;
    }
    // A one-shot effect is done once its last frame has had its time.
    std::erase_if(running_, [](const Running& effect) {
        return !effect.spec.loop && effect.age >= static_cast<int>(effect.spec.frames.size()) * std::max(1, effect.spec.ticksPerFrame);
    });
}

void EffectPlayer::draw(Renderer& renderer, const Rect& view) const {
    for (const Running& effect : running_) {
        const int count = static_cast<int>(effect.spec.frames.size());
        const int step = effect.age / std::max(1, effect.spec.ticksPerFrame);
        const Rect& frame = effect.spec.frames[static_cast<std::size_t>(effect.spec.loop ? step % count : std::min(step, count - 1))];
        // Keep the frame's proportions when a size is asked for: the longer side gets it.
        int width = frame.width;
        int height = frame.height;
        if (effect.size > 0) {
            const int longest = std::max(frame.width, frame.height);
            width = std::max(1, frame.width * effect.size / longest);
            height = std::max(1, frame.height * effect.size / longest);
        }
        const double left = effect.x - width / 2.0 - (effect.onScreen ? 0 : view.x);
        const double top = effect.y - height / 2.0 - (effect.onScreen ? 0 : view.y);
        renderer.drawStyled(effect.spec.texture, frame, {static_cast<int>(std::lround(left)), static_cast<int>(std::lround(top)), width, height},
                            effect.spec.style);
    }
}

} // namespace luna::engine
