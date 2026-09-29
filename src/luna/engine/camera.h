#pragma once

#include "boundary.h"

#include "core/geometry.h"

namespace luna::engine {

using odysseus::core::Rect;

// A window onto the world, as big as the virtual screen. Each tick it moves part of the
// way towards its target (smooth, never a jump), and it never shows beyond the world.
class Camera {
public:
    Camera(int viewWidth, int viewHeight, int worldWidth, int worldHeight);

    // Centres on a point at once (for the first frame, or teleports).
    void centreOn(double x, double y);

    // Once per tick: moves `smoothing` of the way (0..1) towards centring on (x, y).
    void follow(double x, double y, double smoothing = kDefaultSmoothing);

    // The visible part of the world, in world pixels, blended between the previous and
    // the current tick by `alpha` (0..1) so the picture moves every frame, not every tick.
    Rect view(double alpha = 1.0) const;

    static constexpr double kDefaultSmoothing = 0.25;

private:
    double clampX(double x) const;
    double clampY(double y) const;

    int viewWidth_;
    int viewHeight_;
    int worldWidth_;
    int worldHeight_;
    double previousX_ = 0.0; // top-left corner at the previous tick
    double previousY_ = 0.0;
    double x_ = 0.0;         // top-left corner now
    double y_ = 0.0;
};

} // namespace luna::engine
