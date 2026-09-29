#pragma once

#include "boundary.h"

#include "luna/physics/vec3.h"

namespace luna::physics {

// How a surface behaves in a contact (PHY-04). Restitution: 0 = stops dead, 1 = bounces back
// to the same height. Friction: Coulomb's coefficient mu (about 0.35 for wood on grass).
struct SurfaceMaterial {
    Fixed restitution;
    Fixed friction;
};

// Two surfaces meeting: the bouncier one decides the bounce and the frictions are averaged
// geometrically, sqrt(mu1 mu2) (the rules most physics engines use).
Fixed combinedRestitution(SurfaceMaterial a, SurfaceMaterial b);
Fixed combinedFriction(SurfaceMaterial a, SurfaceMaterial b);

// Flat ground at height `height`, for example grass.
struct Ground {
    Fixed height;
    SurfaceMaterial surface;
};

// A body with mass that can be pushed, slide, bounce and come to rest.
//
// A class, not a plain struct, because it has invariants: rules that must always hold.
// The mass is positive and its inverse matches it; a sleeping body does not move. The data is
// private, so the only way to change it is through functions that keep those rules.
class RigidBody {
public:
    // `bottom` is the distance from the centre down to the lowest point (a ball's radius,
    // half a crate's height): where the body meets the ground.
    RigidBody(Fixed mass, Fixed bottom, Vec3 position, SurfaceMaterial surface);

    Fixed mass() const { return mass_; }
    Fixed inverseMass() const { return inverseMass_; }
    Fixed bottom() const { return bottom_; }
    Vec3 position() const { return position_; }
    Vec3 velocity() const { return velocity_; }
    SurfaceMaterial surface() const { return surface_; }
    bool asleep() const { return asleep_; }
    bool onGround() const { return onGround_; }

    // A sudden push (N s = kg m/s): the velocity changes at once by impulse / mass.
    void applyImpulse(Vec3 impulse);
    // A steady push (N) during the next step only, like a shove or the wind.
    void applyForce(Vec3 force);
    void setVelocity(Vec3 velocity);

    // One step of `dt` seconds under gravity, bouncing on and sliding along the ground.
    void step(const Ground& ground, Fixed gravity, Fixed dt);

private:
    void wake();
    void flyFor(Fixed seconds, Vec3 acceleration);
    void slideFor(Fixed seconds, Fixed frictionDeceleration, Vec3 pushAcceleration);

    Fixed mass_;
    Fixed inverseMass_;
    Fixed bottom_;
    Vec3 position_;
    Vec3 velocity_;
    Vec3 force_;
    SurfaceMaterial surface_;
    bool onGround_ = false;
    bool asleep_ = false;
    int stillSteps_ = 0;
};

// Below this speed a bounce ends and the body rests; after this many still steps it sleeps.
inline constexpr Fixed kRestSpeed = Fixed::fromRaw(Fixed::kOneRaw / 20); // 0.05 m/s
inline constexpr int kStepsBeforeSleep = 10;

} // namespace luna::physics
