#pragma once

#include "boundary.h"

#include "luna/physics/vec3.h"

#include <optional>
#include <variant>

namespace luna::physics {

// The three collision shapes (PHY-02). Plain structs: just data, no behaviour, so they are
// easy to copy, compare and save. The functions below do the geometry.
struct Sphere {
    Vec3 center;
    Fixed radius;
};

// A sphere swept along a line segment: a pill. Good for people, spears and tree trunks.
struct Capsule {
    Vec3 start;
    Vec3 end;
    Fixed radius;
};

// An axis-aligned box ("AABB"): its sides are parallel to x, y and z. Rocks, walls, crates.
struct Box {
    Vec3 min;
    Vec3 max;
};

// Any one of the shapes. std::variant holds exactly one of them and knows which.
using Shape = std::variant<Sphere, Capsule, Box>;

// Where two shapes touch. The normal has length 1 and points from the first shape towards
// the second; depth is how far they overlap (0 when they just touch).
struct Contact {
    Vec3 point;
    Vec3 normal;
    Fixed depth;
};

// Where a moving shape first touches another. time is the fraction of the movement
// (0 = at the start, 1 = at the end); the normal is the target's surface normal, pointing
// back towards the mover; point lies on the target's surface.
struct SweepHit {
    Fixed time;
    Vec3 point;
    Vec3 normal;
};

// The smallest box around a shape, for the spatial grid.
Box bounds(const Shape& shape);

// Do two shapes overlap or touch? If so, where and in which direction.
std::optional<Contact> overlap(const Shape& first, const Shape& second);

// Follows the line from `origin` to `origin + delta` and reports the first point where it
// enters the shape. A line that starts inside hits at time 0.
std::optional<SweepHit> raycast(Vec3 origin, Vec3 delta, const Shape& target);

// Moves a sphere by `delta` and reports the first moment it touches the target, however
// fast it goes: the whole path is tested, not just where it ends, so nothing tunnels.
std::optional<SweepHit> sweep(const Sphere& moving, Vec3 delta, const Shape& target);

// Closest points, used by the tests above and handy elsewhere.
Vec3 closestPointOnSegment(Vec3 point, Vec3 start, Vec3 end);
Vec3 closestPointInBox(Vec3 point, const Box& box);

} // namespace luna::physics
