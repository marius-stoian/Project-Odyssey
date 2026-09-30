#include "luna/physics/material.h"

namespace luna::physics {

Fixed massOf(const Material& material, Fixed volume) {
    return material.density * volume;
}

Fixed kineticEnergy(Fixed mass, Vec3 velocity) {
    return mass * lengthSquared(velocity) / 2;
}

} // namespace luna::physics
