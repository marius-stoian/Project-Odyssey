#include "luna/engine/camera.h"

#include <algorithm>
#include <cmath>

namespace luna::engine {

Camera::Camera(int viewWidth, int viewHeight, int worldWidth, int worldHeight)
    : viewWidth_(viewWidth), viewHeight_(viewHeight), worldWidth_(worldWidth), worldHeight_(worldHeight) {}

double Camera::clampX(double x) const {
    // A world narrower than the view stays centred instead.
    const double maxX = static_cast<double>(worldWidth_ - viewWidth_);
    return maxX < 0 ? maxX / 2 : std::clamp(x, 0.0, maxX);
}

double Camera::clampY(double y) const {
    const double maxY = static_cast<double>(worldHeight_ - viewHeight_);
    return maxY < 0 ? maxY / 2 : std::clamp(y, 0.0, maxY);
}

void Camera::centreOn(double x, double y) {
    x_ = clampX(x - viewWidth_ / 2.0);
    y_ = clampY(y - viewHeight_ / 2.0);
    previousX_ = x_;
    previousY_ = y_;
}

void Camera::follow(double x, double y, double smoothing) {
    previousX_ = x_;
    previousY_ = y_;
    const double targetX = clampX(x - viewWidth_ / 2.0);
    const double targetY = clampY(y - viewHeight_ / 2.0);
    x_ += (targetX - x_) * smoothing;
    y_ += (targetY - y_) * smoothing;
    // Snap the last fraction of a pixel, so a resting camera is exactly still.
    if (std::abs(targetX - x_) < 0.05) {
        x_ = targetX;
    }
    if (std::abs(targetY - y_) < 0.05) {
        y_ = targetY;
    }
}

Rect Camera::view(double alpha) const {
    const double x = previousX_ + (x_ - previousX_) * alpha;
    const double y = previousY_ + (y_ - previousY_) * alpha;
    // Whole pixels: pixel art must not land between screen pixels.
    return {static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)), viewWidth_, viewHeight_};
}

} // namespace luna::engine
