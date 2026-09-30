#pragma once

#include "boundary.h"

#include "luna/physics/vec3.h"

namespace luna::engine {

// How the Engine shows Luna Physics' 3D world on a top-down 2D screen (ADR-017). This is the
// one place where physics numbers become floating point, and only for drawing.

// One 32-pixel tile is one metre.
inline constexpr double kPixelsPerMetre = 32.0;

double toDouble(physics::Fixed value);
// A pixel position in the world as metres, rounded to the nearest 1/1024 m (deterministic).
physics::Fixed metresFromPixels(double pixels);

struct ScreenPoint {
    double x = 0.0;
    double y = 0.0;
};

// Where a 3D point appears on the ground map, in world pixels: height lifts it up the
// screen, so screen y = (y - z) x 32.
ScreenPoint topDownPosition(physics::Vec3 metres);
// Where its shadow falls: straight below it on the ground (z ignored).
ScreenPoint groundShadow(physics::Vec3 metres);
// A 3D direction as seen on screen (for choosing which way a sprite points).
ScreenPoint topDownDirection(physics::Vec3 direction);

} // namespace luna::engine
