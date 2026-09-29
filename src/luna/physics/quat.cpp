#include "luna/physics/quat.h"

namespace luna::physics {

Quat Quat::fromAxisAngle(Vec3 axis, Fixed radians) {
    const Fixed half = radians / 2;
    const Fixed s = sin(half);
    return {cos(half), axis.x * s, axis.y * s, axis.z * s};
}

Quat operator*(Quat a, Quat b) {
    // The Hamilton product, written out term by term.
    return {
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
    };
}

Quat conjugate(Quat q) {
    return {q.w, -q.x, -q.y, -q.z};
}

Vec3 rotate(Quat q, Vec3 v) {
    // v' = q v q*, simplified: t = 2 (u x v), v' = v + w t + u x t, with u = (x, y, z).
    // Two cross products instead of two full quaternion multiplications.
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t = cross(u, v) * Fixed::fromInt(2);
    return v + t * q.w + cross(u, t);
}

Quat normalized(Quat q) {
    const Fixed size = sqrt(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    ODYSSEUS_ASSERT(size > kFixedZero, "cannot normalise a zero quaternion");
    return {q.w / size, q.x / size, q.y / size, q.z / size};
}

} // namespace luna::physics
