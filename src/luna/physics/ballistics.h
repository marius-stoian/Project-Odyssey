#pragma once

#include "boundary.h"

#include "luna/physics/shapes.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace luna::physics {

// Standard gravity, 9.81 m/s^2 (the raw value is 9.81 x 2^32, rounded).
inline constexpr Fixed kStandardGravity = Fixed::fromRaw(42'133'629'174);

// The simulation ticks 20 times per second (ADR-006). Fast projectiles move in smaller
// sub-steps inside each tick so their arcs stay accurate.
inline constexpr std::int64_t kTicksPerSecond = 20;
inline constexpr std::int64_t kProjectileSubsteps = 10;

// The air a projectile flies through.
struct Air {
    Fixed density = Fixed::fromRaw(5'261'334'938); // 1.225 kg/m^3, sea level at 15 degrees C
    Vec3 wind;                                     // the air's own velocity, m/s
    Fixed gravity = kStandardGravity;
};

// A thrown or shot object: a spear, a dart, a stone (PHY-03).
struct Projectile {
    Vec3 position;       // m
    Vec3 velocity;       // m/s
    Fixed mass;          // kg
    Fixed dragArea;      // drag coefficient x front area, Cd*A in m^2 (0 = no air drag)
};

// The acceleration on a projectile: gravity down, plus quadratic air drag against its
// motion through the air, F = 1/2 rho Cd A |v - w| (v - w), divided by the mass.
Vec3 projectileAcceleration(const Projectile& projectile, const Air& air);

// Advances by `dt` seconds with semi-implicit Euler: first the velocity from the
// acceleration, then the position from the new velocity. Simple, stable and deterministic.
void stepProjectile(Projectile& projectile, const Air& air, Fixed dt);

// Advances by one simulation tick (1/20 s) in kProjectileSubsteps sub-steps.
void stepProjectileTick(Projectile& projectile, const Air& air);

// What a projectile hit during a tick: which obstacle (its index), and where.
struct ProjectileHit {
    std::size_t obstacle = 0;
    SweepHit hit; // hit.time is the fraction of the sub-step, see secondsIntoTick
    Fixed secondsIntoTick;
};

// One tick of flight with collisions: each sub-step's path is swept (the projectile's tip is
// a small sphere) against every obstacle, so even a fast spear cannot pass through one. On a
// hit the projectile stops at the point of impact and the earliest hit is returned.
std::optional<ProjectileHit> flyTick(Projectile& projectile, const Air& air, Fixed tipRadius,
                                     const std::vector<Shape>& obstacles);

struct Landing {
    Vec3 point;       // where it came down on the ground plane
    Fixed flightTime; // seconds
};

// Flies until the projectile comes down through the ground height `groundZ` (the crossing
// is interpolated inside the last sub-step), or gives up after `maxSeconds`.
std::optional<Landing> flyUntilLanding(Projectile projectile, const Air& air, Fixed groundZ, std::int64_t maxSeconds);

// The launch angle above the horizon (radians) that hits a target `distance` metres away
// horizontally and `height` metres above the launch point, at launch speed `speed`, in a
// vacuum: theta = atan((v^2 - sqrt(v^4 - g (g x^2 + 2 y v^2))) / (g x)), the low arc.
// Empty when the target is out of reach at that speed.
std::optional<Fixed> launchAngleWithoutDrag(Fixed speed, Fixed distance, Fixed height, Fixed gravity);

// The same, but with air drag: starts from the vacuum answer and corrects it by flying test
// throws (secant method) until the arc passes through the target. `projectile` supplies
// the start position, mass and drag; its velocity is ignored. Wind is not corrected for.
std::optional<Fixed> aimLaunchAngle(const Projectile& projectile, const Air& air, Vec3 target, Fixed speed);

// The launch velocity for `speed` at `angle` above the horizon, heading from `from`
// towards `target` along the ground.
Vec3 launchVelocity(Vec3 from, Vec3 target, Fixed speed, Fixed angle);

} // namespace luna::physics
