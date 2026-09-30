#pragma once

#include "boundary.h"

#include "luna/physics/fixed.h"

namespace luna::physics {

// A point or direction in 3D space, in metres (ADR-017). Axes match the top-down screen:
// x points east (right), y points south (down the screen), z points up out of the ground.
struct Vec3 {
    Fixed x;
    Fixed y;
    Fixed z;

    friend Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
    friend Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
    friend Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
    friend Vec3 operator*(Vec3 v, Fixed s) { return {v.x * s, v.y * s, v.z * s}; }
    friend Vec3 operator*(Fixed s, Vec3 v) { return v * s; }
    friend Vec3 operator/(Vec3 v, Fixed s) { return {v.x / s, v.y / s, v.z / s}; }

    Vec3& operator+=(Vec3 other) { return *this = *this + other; }
    Vec3& operator-=(Vec3 other) { return *this = *this - other; }

    friend bool operator==(const Vec3&, const Vec3&) = default;
};

inline constexpr Vec3 kUp{kFixedZero, kFixedZero, kFixedOne};

// a . b: how much two directions agree (|a| |b| cos of the angle between them).
Fixed dot(Vec3 a, Vec3 b);
// a x b: a vector at right angles to both, following the right-hand rule.
Vec3 cross(Vec3 a, Vec3 b);
Fixed lengthSquared(Vec3 v);
// Squares of lengths above about 46,000 m overflow Fixed; our region is 256 m across.
Fixed length(Vec3 v);
// The same direction with length 1. The vector must not be zero.
Vec3 normalized(Vec3 v);

} // namespace luna::physics
