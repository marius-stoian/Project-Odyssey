#include "luna/physics/vec3.h"

namespace luna::physics {

Fixed dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Fixed lengthSquared(Vec3 v) {
    return dot(v, v);
}

Fixed length(Vec3 v) {
    return sqrt(lengthSquared(v));
}

Vec3 normalized(Vec3 v) {
    const Fixed size = length(v);
    ODYSSEUS_ASSERT(size > kFixedZero, "cannot normalise a zero vector");
    return v / size;
}

} // namespace luna::physics
