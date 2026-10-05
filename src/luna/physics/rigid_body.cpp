#include "luna/physics/rigid_body.h"

namespace luna::physics {

namespace {

Vec3 horizontal(Vec3 v) {
    return {v.x, v.y, kFixedZero};
}

} // namespace

Fixed combinedRestitution(SurfaceMaterial a, SurfaceMaterial b) {
    return max(a.restitution, b.restitution);
}

Fixed combinedFriction(SurfaceMaterial a, SurfaceMaterial b) {
    return sqrt(a.friction * b.friction);
}

RigidBody::RigidBody(Fixed mass, Fixed bottom, Vec3 position, SurfaceMaterial surface)
    : mass_(mass), bottom_(bottom), position_(position), surface_(surface) {
    // The invariants are checked once, here; afterwards no function can break them.
    ODYSSEUS_ASSERT(mass > kFixedZero, "a rigid body needs a positive mass");
    ODYSSEUS_ASSERT(bottom >= kFixedZero, "the bottom distance cannot be negative");
    ODYSSEUS_ASSERT(surface.restitution >= kFixedZero && surface.restitution <= kFixedOne, "restitution is 0..1");
    ODYSSEUS_ASSERT(surface.friction >= kFixedZero, "friction cannot be negative");
}

void RigidBody::wake() {
    asleep_ = false;
    stillSteps_ = 0;
}

void RigidBody::applyImpulse(Vec3 impulse) {
    wake();
    velocity_ += impulse / mass_; // dividing once rounds once; x (1/m) would round twice
    if (velocity_.z > kFixedZero) {
        onGround_ = false; // knocked up into the air
    }
}

void RigidBody::applyForce(Vec3 force) {
    wake();
    force_ += force;
}

void RigidBody::setVelocity(Vec3 velocity) {
    wake();
    velocity_ = velocity;
    if (velocity_.z > kFixedZero) {
        onGround_ = false;
    }
}

void RigidBody::flyFor(Fixed seconds, Vec3 acceleration) {
    // Exact for a constant acceleration: x += v t + a t^2 / 2, then v += a t. Plain Euler
    // (x += v t) would make every bounce a little too high or too low.
    position_ += velocity_ * seconds + acceleration * (seconds * seconds / 2);
    velocity_ += acceleration * seconds;
}

void RigidBody::slideFor(Fixed seconds, Fixed frictionDeceleration, Vec3 pushAcceleration) {
    const Vec3 flat = horizontal(velocity_);
    const Fixed speed = length(flat);
    const Fixed push = length(pushAcceleration);
    if (speed == kFixedZero) {
        // Standing still: friction holds the body until the push is stronger than friction.
        if (push > frictionDeceleration) {
            flyFor(seconds, pushAcceleration * ((push - frictionDeceleration) / push));
        }
        return;
    }
    const Vec3 direction = flat / speed;
    if (push == kFixedZero) {
        // Coasting: friction brakes steadily, a = mu g, until the body stops.
        const Fixed timeToStop = speed / frictionDeceleration;
        if (frictionDeceleration == kFixedZero || timeToStop > seconds) {
            flyFor(seconds, direction * -frictionDeceleration);
        } else {
            position_ += direction * (speed * speed / (frictionDeceleration * 2)); // v^2 / (2 a)
            velocity_ = {};
        }
        return;
    }
    flyFor(seconds, pushAcceleration - direction * frictionDeceleration);
    if (dot(horizontal(velocity_), direction) < kFixedZero) {
        velocity_ = {}; // friction can stop a body, never push it backwards
    }
}

void RigidBody::step(const Ground& ground, Fixed gravity, Fixed dt) {
    const Vec3 push = force_ / mass_; // a = F / m
    force_ = {};
    if (asleep_) {
        return;
    }
    const Fixed floorZ = ground.height + bottom_;
    const Fixed friction = combinedFriction(surface_, ground.surface);

    if (onGround_ && push.z <= gravity) {
        // Resting on the ground: the ground holds it up; friction resists sliding.
        slideFor(dt, friction * (gravity - push.z), horizontal(push));
        position_.z = floorZ;
        velocity_.z = kFixedZero;
        if (horizontal(velocity_) == Vec3{} && horizontal(push) == Vec3{}) {
            if (++stillSteps_ >= kStepsBeforeSleep) {
                asleep_ = true; // nothing moves it: stop computing it until something does
            }
        } else {
            stillSteps_ = 0;
        }
        return;
    }

    onGround_ = false;
    const Vec3 acceleration = push + Vec3{kFixedZero, kFixedZero, -gravity};
    Fixed remaining = dt;
    // A fast ball can bounce more than once in a step, so handle each impact in turn.
    for (int impact = 0; impact < 8 && remaining > kFixedZero; ++impact) {
        // When does the bottom reach the floor? Solve z + v t + a t^2 / 2 = floor, written as
        // t = 2h / (-v + sqrt(v^2 - 2 a h)), which stays precise when t is tiny.
        const Fixed h = max(kFixedZero, position_.z - floorZ);
        const Fixed v = velocity_.z;
        const Fixed a = acceleration.z;
        const Fixed discriminant = v * v - a * h * 2;
        Fixed hitTime = remaining + kFixedOne; // "not in this step"
        if (discriminant >= kFixedZero) {
            const Fixed denominator = -v + sqrt(discriminant);
            if (denominator > kFixedZero) {
                hitTime = h * 2 / denominator;
            } else if (h == kFixedZero && v <= kFixedZero && a < kFixedZero) {
                hitTime = kFixedZero; // sitting on the floor with gravity pulling
            }
        }
        if (hitTime > remaining) {
            flyFor(remaining, acceleration);
            return;
        }
        flyFor(hitTime, acceleration);
        remaining -= hitTime;
        position_.z = floorZ;

        // The bounce: the ground sends back a fraction e of the impact speed, and friction
        // (Coulomb) can remove up to mu (1 + e) of it from the sideways speed.
        const Fixed impactSpeed = max(kFixedZero, -velocity_.z);
        const Fixed restitution = combinedRestitution(surface_, ground.surface);
        const Fixed bounceSpeed = impactSpeed * restitution;
        const Vec3 flat = horizontal(velocity_);
        const Fixed sideways = length(flat);
        const Fixed frictionChange = friction * (kFixedOne + restitution) * impactSpeed;
        velocity_ = sideways <= frictionChange ? Vec3{} : flat - flat / sideways * frictionChange;
        if (bounceSpeed < kRestSpeed) {
            onGround_ = true; // too weak to leave the ground again: it rests (and may slide)
            slideFor(remaining, friction * gravity, horizontal(push));
            return;
        }
        velocity_.z = bounceSpeed;
    }
}

} // namespace luna::physics
