#pragma once

#include "boundary.h"

#include "luna/physics/vec3.h"

namespace luna::physics {

// A rotation in 3D as a unit quaternion: w = cos(angle/2), (x, y, z) = axis * sin(angle/2).
// Four numbers instead of three angles, because quaternions never "lock" when two axes
// line up (gimbal lock), and joining two rotations is just one multiplication.
struct Quat {
    Fixed w = kFixedOne;
    Fixed x;
    Fixed y;
    Fixed z;

    // A turn of `radians` about `axis` (which must have length 1), right-handed:
    // a positive turn about kUp turns east (x) towards south (y).
    static Quat fromAxisAngle(Vec3 axis, Fixed radians);

    // a * b: first rotate by b, then by a.
    friend Quat operator*(Quat a, Quat b);

    friend bool operator==(const Quat&, const Quat&) = default;
};

// The rotation undone (for a unit quaternion, its inverse).
Quat conjugate(Quat q);
// Turns the vector by the rotation.
Vec3 rotate(Quat q, Vec3 v);
// Rounding slowly stretches a quaternion; this rescales it back to length 1.
Quat normalized(Quat q);

} // namespace luna::physics
