#pragma once

#include "boundary.h"
#include "geometry.h"

namespace odysseus::core {

inline constexpr int kVirtualWidth = 960;
inline constexpr int kVirtualHeight = 540;

enum class ScalingMode { Whole, Fill };
enum class WindowMode { Windowed, Borderless, Exclusive };

struct Resolution {
    int width = 1280;
    int height = 720;
    WindowMode mode = WindowMode::Windowed;
    ScalingMode scaling = ScalingMode::Whole;
    friend bool operator==(const Resolution&, const Resolution&) = default;
};

Rect presentationArea(int outputWidth, int outputHeight, int virtualWidth, int virtualHeight, ScalingMode mode);

} // namespace odysseus::core
