#include "luna/engine/physics_view.h"

#include <cmath>
#include <cstdint>

namespace luna::engine {

double toDouble(physics::Fixed value) {
    return static_cast<double>(value.raw()) / static_cast<double>(physics::Fixed::kOneRaw);
}

physics::Fixed metresFromPixels(double pixels) {
    const auto units = static_cast<std::int64_t>(std::llround(pixels / kPixelsPerMetre * 1024.0));
    return physics::Fixed::fromRatio(units, 1024);
}

ScreenPoint topDownPosition(physics::Vec3 metres) {
    return {toDouble(metres.x) * kPixelsPerMetre, (toDouble(metres.y) - toDouble(metres.z)) * kPixelsPerMetre};
}

ScreenPoint groundShadow(physics::Vec3 metres) {
    return {toDouble(metres.x) * kPixelsPerMetre, toDouble(metres.y) * kPixelsPerMetre};
}

ScreenPoint topDownDirection(physics::Vec3 direction) {
    return {toDouble(direction.x), toDouble(direction.y) - toDouble(direction.z)};
}

} // namespace luna::engine
