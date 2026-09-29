#include "luna/physics/shapes.h"

#include <array>

namespace luna::physics {

namespace {

// n / d limited to [0, 1] without ever dividing something huge by something tiny, which
// would overflow Fixed. Used for "how far along a segment" parameters.
Fixed ratio01(Fixed numerator, Fixed denominator) {
    if (denominator < kFixedZero) {
        numerator = -numerator;
        denominator = -denominator;
    }
    if (numerator <= kFixedZero || denominator == kFixedZero) {
        return kFixedZero;
    }
    if (numerator >= denominator) {
        return kFixedOne;
    }
    return numerator / denominator;
}

// A direction of length 1, or `fallback` when the vector is (almost) zero.
Vec3 directionOr(Vec3 v, Vec3 fallback) {
    return lengthSquared(v) > kFixedZero ? normalized(v) : fallback;
}

Fixed component(Vec3 v, int axis) {
    return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

Vec3 axisVector(int axis, Fixed size) {
    Vec3 v;
    (axis == 0 ? v.x : (axis == 1 ? v.y : v.z)) = size;
    return v;
}

Vec3 boxCenter(const Box& box) {
    return (box.min + box.max) / Fixed::fromInt(2);
}

// --- Overlap tests. Each returns the contact with the normal from the first to the second. ---

std::optional<Contact> sphereSphere(const Sphere& a, const Sphere& b) {
    const Vec3 between = b.center - a.center;
    const Fixed reach = a.radius + b.radius;
    const Fixed distanceSquared = lengthSquared(between);
    if (distanceSquared > reach * reach) {
        return std::nullopt;
    }
    const Fixed distance = sqrt(distanceSquared);
    const Vec3 normal = directionOr(between, kUp);
    const Fixed depth = reach - distance;
    // Halfway through the overlap, on the line between the centres.
    return Contact{a.center + normal * (a.radius - depth / 2), normal, depth};
}

std::optional<Contact> sphereBox(const Sphere& sphere, const Box& box) {
    const Vec3 closest = closestPointInBox(sphere.center, box);
    const Vec3 toBox = closest - sphere.center;
    const Fixed distanceSquared = lengthSquared(toBox);
    if (distanceSquared > sphere.radius * sphere.radius) {
        return std::nullopt;
    }
    if (distanceSquared > kFixedZero) {
        const Fixed distance = sqrt(distanceSquared);
        return Contact{closest, toBox / distance, sphere.radius - distance};
    }
    // The centre is inside the box: leave through the nearest face.
    int bestAxis = 0;
    bool towardsMax = false;
    Fixed bestDistance = kFixedZero;
    for (int axis = 0; axis < 3; ++axis) {
        const Fixed toMin = component(sphere.center, axis) - component(box.min, axis);
        const Fixed toMax = component(box.max, axis) - component(sphere.center, axis);
        const Fixed nearest = min(toMin, toMax);
        if (axis == 0 || nearest < bestDistance) {
            bestAxis = axis;
            bestDistance = nearest;
            towardsMax = toMax < toMin;
        }
    }
    // The sphere is pushed out through that face, so the box lies the opposite way.
    const Vec3 outward = axisVector(bestAxis, towardsMax ? kFixedOne : -kFixedOne);
    return Contact{sphere.center + outward * bestDistance, -outward, sphere.radius + bestDistance};
}

std::optional<Contact> boxBox(const Box& a, const Box& b) {
    int bestAxis = -1;
    Fixed bestOverlap = kFixedZero;
    Vec3 low;
    Vec3 high;
    for (int axis = 0; axis < 3; ++axis) {
        const Fixed lo = max(component(a.min, axis), component(b.min, axis));
        const Fixed hi = min(component(a.max, axis), component(b.max, axis));
        if (hi < lo) {
            return std::nullopt;
        }
        (axis == 0 ? low.x : (axis == 1 ? low.y : low.z)) = lo;
        (axis == 0 ? high.x : (axis == 1 ? high.y : high.z)) = hi;
        if (bestAxis < 0 || hi - lo < bestOverlap) {
            bestAxis = axis;
            bestOverlap = hi - lo;
        }
    }
    // Separate along the axis of least overlap, from a's centre towards b's.
    const bool bIsAhead = component(boxCenter(b), bestAxis) >= component(boxCenter(a), bestAxis);
    const Vec3 normal = axisVector(bestAxis, bIsAhead ? kFixedOne : -kFixedOne);
    return Contact{(low + high) / Fixed::fromInt(2), normal, bestOverlap};
}

// The closest points between two segments (Ericson, Real-Time Collision Detection, 5.1.9),
// written with ratio01 so that nearly parallel segments never divide by almost zero.
void closestPointsBetweenSegments(Vec3 p1, Vec3 q1, Vec3 p2, Vec3 q2, Vec3& onFirst, Vec3& onSecond) {
    const Vec3 d1 = q1 - p1;
    const Vec3 d2 = q2 - p2;
    const Vec3 r = p1 - p2;
    const Fixed a = dot(d1, d1);
    const Fixed e = dot(d2, d2);
    const Fixed f = dot(d2, r);
    Fixed s = kFixedZero;
    Fixed t = kFixedZero;
    if (a == kFixedZero && e == kFixedZero) {
        // Both are points.
    } else if (a == kFixedZero) {
        t = ratio01(f, e);
    } else {
        const Fixed c = dot(d1, r);
        if (e == kFixedZero) {
            s = ratio01(-c, a);
        } else {
            const Fixed b = dot(d1, d2);
            const Fixed denominator = a * e - b * b; // never negative
            s = ratio01(b * f - c * e, denominator);
            const Fixed tNumerator = b * s + f;
            if (tNumerator <= kFixedZero) {
                t = kFixedZero;
                s = ratio01(-c, a);
            } else if (tNumerator >= e) {
                t = kFixedOne;
                s = ratio01(b - c, a);
            } else {
                t = tNumerator / e;
            }
        }
    }
    onFirst = p1 + d1 * s;
    onSecond = p2 + d2 * t;
}

std::optional<Contact> capsuleCapsule(const Capsule& a, const Capsule& b) {
    Vec3 onA;
    Vec3 onB;
    closestPointsBetweenSegments(a.start, a.end, b.start, b.end, onA, onB);
    return sphereSphere({onA, a.radius}, {onB, b.radius});
}

Fixed squaredDistanceToBox(Vec3 point, const Box& box) {
    return lengthSquared(closestPointInBox(point, box) - point);
}

std::optional<Contact> capsuleBox(const Capsule& capsule, const Box& box) {
    // The distance from a box to the points of a segment first falls, then rises (it is
    // convex), so a ternary search finds the segment point nearest the box.
    Fixed low = kFixedZero;
    Fixed high = kFixedOne;
    const Vec3 along = capsule.end - capsule.start;
    for (int step = 0; step < 48; ++step) {
        const Fixed third = (high - low) / 3;
        const Fixed left = low + third;
        const Fixed right = high - third;
        if (squaredDistanceToBox(capsule.start + along * left, box) <=
            squaredDistanceToBox(capsule.start + along * right, box)) {
            high = right;
        } else {
            low = left;
        }
    }
    // Near the minimum, distances differ by less than one Fixed step, so the search stops a
    // little off. Stepping back and forth between the two shapes' closest points lands on
    // the exact closest pair (alternating projection).
    Vec3 onSegment = capsule.start + along * ((low + high) / 2);
    for (int refine = 0; refine < 2; ++refine) {
        onSegment = closestPointOnSegment(closestPointInBox(onSegment, box), capsule.start, capsule.end);
    }
    return sphereBox({onSegment, capsule.radius}, box);
}

Contact flipped(Contact contact) {
    contact.normal = -contact.normal;
    return contact;
}

std::optional<Contact> flipped(std::optional<Contact> contact) {
    if (contact) {
        return flipped(*contact);
    }
    return std::nullopt;
}

// --- Rays. Each finds the entry time in [0, 1] of origin + delta * time. ---

struct RayHit {
    Fixed time;
    Vec3 normal; // outward surface normal where the ray enters
};

std::optional<RayHit> earlier(std::optional<RayHit> a, std::optional<RayHit> b) {
    if (!a) {
        return b;
    }
    if (!b) {
        return a;
    }
    return b->time < a->time ? b : a;
}

std::optional<RayHit> raySphere(Vec3 origin, Vec3 delta, Vec3 center, Fixed radius) {
    // Solve |m + t d|^2 = r^2 for the smallest t, with m = origin - center.
    const Vec3 m = origin - center;
    const Fixed c = lengthSquared(m) - radius * radius;
    if (c <= kFixedZero) {
        return RayHit{kFixedZero, directionOr(m, kUp)}; // starts inside
    }
    const Fixed b = dot(m, delta);
    if (b >= kFixedZero) {
        return std::nullopt; // outside and moving away
    }
    const Fixed a = lengthSquared(delta);
    const Fixed discriminant = b * b - a * c;
    if (discriminant < kFixedZero) {
        return std::nullopt; // passes by
    }
    const Fixed numerator = -b - sqrt(discriminant); // >= 0 here
    if (numerator > a) {
        return std::nullopt; // would hit only after the end of the movement
    }
    const Fixed time = numerator / a;
    return RayHit{time, directionOr(m + delta * time, kUp)};
}

std::optional<RayHit> rayCylinderSide(Vec3 origin, Vec3 delta, Vec3 start, Vec3 end, Fixed radius) {
    const Vec3 axis = end - start;
    const Fixed axisSquared = lengthSquared(axis);
    if (axisSquared == kFixedZero) {
        return std::nullopt;
    }
    // Remove the part along the axis: what is left moves in the circle's plane.
    const Vec3 w = origin - start;
    const Vec3 wPerp = w - axis * (dot(w, axis) / axisSquared);
    const Vec3 dPerp = delta - axis * (dot(delta, axis) / axisSquared);
    const Fixed a = lengthSquared(dPerp);
    const Fixed b = dot(wPerp, dPerp);
    const Fixed c = lengthSquared(wPerp) - radius * radius;
    if (a == kFixedZero || c <= kFixedZero || b >= kFixedZero) {
        return std::nullopt; // parallel to the axis, already within the tube, or moving away
    }
    const Fixed discriminant = b * b - a * c;
    if (discriminant < kFixedZero) {
        return std::nullopt;
    }
    const Fixed numerator = -b - sqrt(discriminant);
    if (numerator > a) {
        return std::nullopt;
    }
    const Fixed time = numerator / a;
    const Vec3 hit = origin + delta * time;
    const Fixed along = dot(hit - start, axis);
    if (along < kFixedZero || along > axisSquared) {
        return std::nullopt; // beyond the ends: the end caps (spheres) handle those
    }
    const Vec3 onAxis = start + axis * (along / axisSquared);
    return RayHit{time, directionOr(hit - onAxis, kUp)};
}

std::optional<RayHit> rayCapsule(Vec3 origin, Vec3 delta, Vec3 start, Vec3 end, Fixed radius) {
    // A capsule is a tube plus two end spheres; the ray enters whichever it meets first.
    if (lengthSquared(closestPointOnSegment(origin, start, end) - origin) <= radius * radius) {
        return RayHit{kFixedZero, directionOr(origin - closestPointOnSegment(origin, start, end), kUp)};
    }
    return earlier(rayCylinderSide(origin, delta, start, end, radius),
                   earlier(raySphere(origin, delta, start, radius), raySphere(origin, delta, end, radius)));
}

// t = n / d, limited to +/-1024 so that a nearly-still ray never overflows. Only times in
// [0, 1] matter, so the limit changes no answer.
Fixed limitedRatio(Fixed numerator, Fixed denominator) {
    constexpr std::int64_t kLimit = 1024;
    const bool negative = (numerator < kFixedZero) != (denominator < kFixedZero);
    if (abs(numerator) >= abs(denominator) * kLimit) {
        return Fixed::fromInt(negative ? -kLimit : kLimit);
    }
    return numerator / denominator;
}

// The slab method: the ray is inside the box while it is between both walls on all three
// axes at once. Entry = the latest of the three "enter" times.
std::optional<RayHit> rayBox(Vec3 origin, Vec3 delta, const Box& box) {
    Fixed enter = Fixed::fromInt(-1024);
    Fixed exit = Fixed::fromInt(1024);
    int enterAxis = -1;
    for (int axis = 0; axis < 3; ++axis) {
        const Fixed o = component(origin, axis);
        const Fixed d = component(delta, axis);
        const Fixed lo = component(box.min, axis);
        const Fixed hi = component(box.max, axis);
        if (d == kFixedZero) {
            if (o < lo || o > hi) {
                return std::nullopt; // moving parallel to this slab, outside it
            }
            continue;
        }
        Fixed tLo = limitedRatio(lo - o, d);
        Fixed tHi = limitedRatio(hi - o, d);
        if (tHi < tLo) {
            const Fixed swap = tLo;
            tLo = tHi;
            tHi = swap;
        }
        if (tLo > enter) {
            enter = tLo;
            enterAxis = axis;
        }
        exit = min(exit, tHi);
        if (enter > exit) {
            return std::nullopt;
        }
    }
    if (exit < kFixedZero || enter > kFixedOne) {
        return std::nullopt;
    }
    if (enter <= kFixedZero || enterAxis < 0) {
        return RayHit{kFixedZero, directionOr(origin - boxCenter(box), kUp)}; // starts inside
    }
    const bool fromBelow = component(delta, enterAxis) > kFixedZero;
    return RayHit{enter, axisVector(enterAxis, fromBelow ? -kFixedOne : kFixedOne)};
}

// A box grown by `radius` in every direction has rounded edges and corners: it is three
// boxes (each grown along one axis) plus twelve capsules along the edges. The ray enters
// the rounded box where it enters the first of those parts.
std::optional<RayHit> rayRoundedBox(Vec3 origin, Vec3 delta, const Box& box, Fixed radius) {
    if (radius == kFixedZero) {
        return rayBox(origin, delta, box);
    }
    if (squaredDistanceToBox(origin, box) <= radius * radius) {
        return RayHit{kFixedZero, directionOr(origin - closestPointInBox(origin, box), kUp)};
    }
    std::optional<RayHit> best;
    for (int axis = 0; axis < 3; ++axis) {
        const Vec3 grow = axisVector(axis, radius);
        best = earlier(best, rayBox(origin, delta, {box.min - grow, box.max + grow}));
    }
    const std::array<Vec3, 2> corners{box.min, box.max};
    for (int axis = 0; axis < 3; ++axis) {
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                // An edge parallel to `axis`: the other two coordinates are at min or max.
                Vec3 start = box.min;
                Vec3 end = box.max;
                const int other1 = (axis + 1) % 3;
                const int other2 = (axis + 2) % 3;
                const Fixed c1 = component(corners[static_cast<std::size_t>(i)], other1);
                const Fixed c2 = component(corners[static_cast<std::size_t>(j)], other2);
                (other1 == 0 ? start.x : (other1 == 1 ? start.y : start.z)) = c1;
                (other1 == 0 ? end.x : (other1 == 1 ? end.y : end.z)) = c1;
                (other2 == 0 ? start.x : (other2 == 1 ? start.y : start.z)) = c2;
                (other2 == 0 ? end.x : (other2 == 1 ? end.y : end.z)) = c2;
                best = earlier(best, rayCapsule(origin, delta, start, end, radius));
            }
        }
    }
    return best;
}

std::optional<RayHit> rayShape(Vec3 origin, Vec3 delta, const Shape& target, Fixed grow) {
    if (const auto* sphere = std::get_if<Sphere>(&target)) {
        return raySphere(origin, delta, sphere->center, sphere->radius + grow);
    }
    if (const auto* capsule = std::get_if<Capsule>(&target)) {
        return rayCapsule(origin, delta, capsule->start, capsule->end, capsule->radius + grow);
    }
    return rayRoundedBox(origin, delta, std::get<Box>(target), grow);
}

} // namespace

Vec3 closestPointOnSegment(Vec3 point, Vec3 start, Vec3 end) {
    const Vec3 along = end - start;
    return start + along * ratio01(dot(point - start, along), lengthSquared(along));
}

Vec3 closestPointInBox(Vec3 point, const Box& box) {
    return {clamp(point.x, box.min.x, box.max.x), clamp(point.y, box.min.y, box.max.y),
            clamp(point.z, box.min.z, box.max.z)};
}

Box bounds(const Shape& shape) {
    if (const auto* sphere = std::get_if<Sphere>(&shape)) {
        const Vec3 r{sphere->radius, sphere->radius, sphere->radius};
        return {sphere->center - r, sphere->center + r};
    }
    if (const auto* capsule = std::get_if<Capsule>(&shape)) {
        const Vec3 r{capsule->radius, capsule->radius, capsule->radius};
        const Vec3 low{min(capsule->start.x, capsule->end.x), min(capsule->start.y, capsule->end.y),
                       min(capsule->start.z, capsule->end.z)};
        const Vec3 high{max(capsule->start.x, capsule->end.x), max(capsule->start.y, capsule->end.y),
                        max(capsule->start.z, capsule->end.z)};
        return {low - r, high + r};
    }
    return std::get<Box>(shape);
}

std::optional<Contact> overlap(const Shape& first, const Shape& second) {
    // Shapes can only touch if their bounding boxes do; that cheap test skips most work.
    const Box boundsA = bounds(first);
    const Box boundsB = bounds(second);
    if (boundsA.max.x < boundsB.min.x || boundsB.max.x < boundsA.min.x || boundsA.max.y < boundsB.min.y ||
        boundsB.max.y < boundsA.min.y || boundsA.max.z < boundsB.min.z || boundsB.max.z < boundsA.min.z) {
        return std::nullopt;
    }
    // Six pairings; the other three are the same tests with the roles swapped.
    if (const auto* a = std::get_if<Sphere>(&first)) {
        if (const auto* b = std::get_if<Sphere>(&second)) {
            return sphereSphere(*a, *b);
        }
        if (const auto* b = std::get_if<Capsule>(&second)) {
            return flipped(capsuleCapsule(*b, {a->center, a->center, a->radius}));
        }
        return sphereBox(*a, std::get<Box>(second));
    }
    if (const auto* a = std::get_if<Capsule>(&first)) {
        if (const auto* b = std::get_if<Sphere>(&second)) {
            return capsuleCapsule(*a, {b->center, b->center, b->radius});
        }
        if (const auto* b = std::get_if<Capsule>(&second)) {
            return capsuleCapsule(*a, *b);
        }
        return capsuleBox(*a, std::get<Box>(second));
    }
    const Box& a = std::get<Box>(first);
    if (const auto* b = std::get_if<Sphere>(&second)) {
        return flipped(sphereBox(*b, a));
    }
    if (const auto* b = std::get_if<Capsule>(&second)) {
        return flipped(capsuleBox(*b, a));
    }
    return boxBox(a, std::get<Box>(second));
}

std::optional<SweepHit> raycast(Vec3 origin, Vec3 delta, const Shape& target) {
    const auto hit = rayShape(origin, delta, target, kFixedZero);
    if (!hit) {
        return std::nullopt;
    }
    return SweepHit{hit->time, origin + delta * hit->time, hit->normal};
}

std::optional<SweepHit> sweep(const Sphere& moving, Vec3 delta, const Shape& target) {
    // A sphere touching a shape is the same as its centre touching the shape grown by the
    // sphere's radius, so a sweep is a ray against the grown shape (a Minkowski sum).
    const auto hit = rayShape(moving.center, delta, target, moving.radius);
    if (!hit) {
        return std::nullopt;
    }
    const Vec3 centerAtHit = moving.center + delta * hit->time;
    return SweepHit{hit->time, centerAtHit - hit->normal * moving.radius, hit->normal};
}

} // namespace luna::physics
