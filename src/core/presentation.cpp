#include "core/presentation.h"

#include <algorithm>
#include <cstdint>

namespace odysseus::core {

Rect presentationArea(int outputWidth, int outputHeight, int virtualWidth, int virtualHeight, ScalingMode mode) {
    if (outputWidth <= 0 || outputHeight <= 0 || virtualWidth <= 0 || virtualHeight <= 0) return {};
    int width = 0;
    int height = 0;
    const int whole = std::min(outputWidth / virtualWidth, outputHeight / virtualHeight);
    if (mode == ScalingMode::Whole && whole >= 1) {
        width = virtualWidth * whole;
        height = virtualHeight * whole;
    } else if (static_cast<std::int64_t>(outputWidth) * virtualHeight <= static_cast<std::int64_t>(outputHeight) * virtualWidth) {
        width = outputWidth;
        height = static_cast<int>(static_cast<std::int64_t>(outputWidth) * virtualHeight / virtualWidth);
    } else {
        height = outputHeight;
        width = static_cast<int>(static_cast<std::int64_t>(outputHeight) * virtualWidth / virtualHeight);
    }
    return {(outputWidth - width) / 2, (outputHeight - height) / 2, width, height};
}

} // namespace odysseus::core
