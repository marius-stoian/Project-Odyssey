#include "luna/physics/ballistics.h"

namespace luna::physics {

namespace {

Fixed substepSeconds() {
    return Fixed::fromRatio(1, kTicksPerSecond * kProjectileSubsteps);
}

Fixed horizontalDistance(Vec3 from, Vec3 to) {
    return length(Vec3{to.x - from.x, to.y - from.y, kFixedZero});
}

// How far above (+) or below (-) the target height the throw passes when it has travelled
// the target's horizontal distance. A throw that falls short gets a negative value that
// keeps shrinking the further short it lands, so the aim search always knows which way to go.
Fixed missHeight(Projectile projectile, const Air& air, Vec3 target) {
    const Vec3 start = projectile.position;
    const Fixed distance = horizontalDistance(start, target);
    const Fixed dt = substepSeconds();
    const Fixed floor = min(start.z, target.z) - Fixed::fromInt(100);
    const std::int64_t maxSteps = 30 * kTicksPerSecond * kProjectileSubsteps; // 30 seconds
    Vec3 previous = start;
    Fixed previousDistance = kFixedZero;
    for (std::int64_t step = 0; step < maxSteps; ++step) {
        stepProjectile(projectile, air, dt);
        const Fixed travelled = horizontalDistance(start, projectile.position);
        if (travelled >= distance) {
            // Interpolate the height where the path crosses the target's distance.
            const Fixed fraction = (distance - previousDistance) / (travelled - previousDistance);
            const Fixed z = previous.z + (projectile.position.z - previous.z) * fraction;
            return z - target.z;
        }
        if (projectile.position.z < floor) {
            break;
        }
        previous = projectile.position;
        previousDistance = travelled;
    }
    return (projectile.position.z - target.z) - (distance - horizontalDistance(start, projectile.position));
}

} // namespace

Vec3 projectileAcceleration(const Projectile& projectile, const Air& air) {
    Vec3 acceleration{kFixedZero, kFixedZero, -air.gravity};
    if (projectile.dragArea > kFixedZero) {
        // Drag pushes against the motion through the air (so wind counts), growing with
        // the square of the speed: a = -(rho Cd A / 2m) |v - w| (v - w).
        const Vec3 throughAir = projectile.velocity - air.wind;
        const Fixed dragPerMetre = air.density * projectile.dragArea / (projectile.mass * 2);
        acceleration -= throughAir * (dragPerMetre * length(throughAir));
    }
    return acceleration;
}

void stepProjectile(Projectile& projectile, const Air& air, Fixed dt) {
    projectile.velocity += projectileAcceleration(projectile, air) * dt;
    projectile.position += projectile.velocity * dt;
}

void stepProjectileTick(Projectile& projectile, const Air& air) {
    const Fixed dt = substepSeconds();
    for (std::int64_t step = 0; step < kProjectileSubsteps; ++step) {
        stepProjectile(projectile, air, dt);
    }
}

std::optional<ProjectileHit> flyTick(Projectile& projectile, const Air& air, Fixed tipRadius,
                                     const std::vector<Shape>& obstacles) {
    const Fixed dt = substepSeconds();
    for (std::int64_t step = 0; step < kProjectileSubsteps; ++step) {
        const Vec3 before = projectile.position;
        stepProjectile(projectile, air, dt);
        const Vec3 moved = projectile.position - before;
        std::optional<ProjectileHit> earliest;
        for (std::size_t i = 0; i < obstacles.size(); ++i) {
            const auto hit = sweep(Sphere{before, tipRadius}, moved, obstacles[i]);
            if (hit && (!earliest || hit->time < earliest->hit.time)) {
                earliest = ProjectileHit{i, *hit, dt * step + dt * hit->time};
            }
        }
        if (earliest) {
            projectile.position = before + moved * earliest->hit.time;
            return earliest;
        }
    }
    return std::nullopt;
}

std::optional<Landing> flyUntilLanding(Projectile projectile, const Air& air, Fixed groundZ, std::int64_t maxSeconds) {
    const Fixed dt = substepSeconds();
    const std::int64_t maxSteps = maxSeconds * kTicksPerSecond * kProjectileSubsteps;
    Fixed time = kFixedZero;
    for (std::int64_t step = 0; step < maxSteps; ++step) {
        const Vec3 before = projectile.position;
        stepProjectile(projectile, air, dt);
        if (before.z > groundZ && projectile.position.z <= groundZ) {
            // It crossed the ground during this sub-step: find where along the step.
            const Fixed fraction = (before.z - groundZ) / (before.z - projectile.position.z);
            return Landing{before + (projectile.position - before) * fraction, time + dt * fraction};
        }
        time += dt;
    }
    return std::nullopt;
}

std::optional<Fixed> launchAngleWithoutDrag(Fixed speed, Fixed distance, Fixed height, Fixed gravity) {
    ODYSSEUS_ASSERT(distance > kFixedZero, "the target must be some distance away");
    const Fixed v2 = speed * speed;
    const Fixed discriminant = v2 * v2 - gravity * (gravity * distance * distance + height * v2 * 2);
    if (discriminant < kFixedZero) {
        return std::nullopt; // too far for this speed
    }
    // atan2(a, b) is atan(a / b) for positive b, without dividing first.
    return atan2(v2 - sqrt(discriminant), gravity * distance);
}

Vec3 launchVelocity(Vec3 from, Vec3 target, Fixed speed, Fixed angle) {
    const Vec3 flat{target.x - from.x, target.y - from.y, kFixedZero};
    const Vec3 heading = lengthSquared(flat) > kFixedZero ? normalized(flat) : Vec3{};
    return heading * (speed * cos(angle)) + kUp * (speed * sin(angle));
}

std::optional<Fixed> aimLaunchAngle(const Projectile& projectile, const Air& air, Vec3 target, Fixed speed) {
    const Fixed distance = horizontalDistance(projectile.position, target);
    const auto vacuum = launchAngleWithoutDrag(speed, distance, target.z - projectile.position.z, air.gravity);
    // Drag always shortens a throw, so the vacuum angle is a good first guess; out of
    // vacuum reach, start at 45 degrees and let the search report failure.
    Fixed angle = vacuum ? *vacuum : degrees(45);
    auto missFor = [&](Fixed launchAngle) {
        Projectile test = projectile;
        test.velocity = launchVelocity(projectile.position, target, speed, launchAngle);
        return missHeight(test, air, target);
    };
    // Secant method: fit a straight line through the last two tries and jump to where it
    // crosses zero miss. It needs only a handful of test throws.
    Fixed previousAngle = angle + Fixed::fromRatio(2, 100);
    Fixed previousMiss = missFor(previousAngle);
    Fixed miss = missFor(angle);
    const Fixed goodEnough = Fixed::fromRatio(1, 1000); // 1 mm
    const Fixed maxJump = Fixed::fromRatio(3, 10);       // radians
    const Fixed lowest = -degrees(80);
    const Fixed highest = degrees(45);                   // the low arc only
    for (int round = 0; round < 16 && abs(miss) > goodEnough; ++round) {
        const Fixed slope = miss - previousMiss;
        if (abs(slope) < Fixed::fromRatio(1, 1'000'000)) {
            break; // the angle no longer changes the result: out of reach
        }
        Fixed jump = clamp((angle - previousAngle) / slope * miss, -maxJump, maxJump);
        previousAngle = angle;
        previousMiss = miss;
        angle = clamp(angle - jump, lowest, highest);
        miss = missFor(angle);
    }
    if (abs(miss) > Fixed::fromRatio(5, 100)) {
        return std::nullopt; // cannot reach it at this speed
    }
    return angle;
}

} // namespace luna::physics
