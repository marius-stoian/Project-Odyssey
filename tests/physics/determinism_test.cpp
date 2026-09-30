#include "luna/physics/ballistics.h"
#include "luna/physics/quat.h"
#include "luna/physics/rigid_body.h"
#include "luna/physics/spatial_grid.h"

#include "core/hash.h"
#include "core/random.h"

#include <doctest/doctest.h>

#include <cstdint>
#include <vector>

using luna::physics::Fixed;
using luna::physics::Quat;
using luna::physics::Vec3;
namespace physics = luna::physics;

namespace {

// One million mixed operations on seeded random inputs; every result goes into the hash.
std::uint64_t runMixedMath(std::uint64_t seed) {
    odysseus::core::Pcg32 random(seed, 25);
    odysseus::core::Hasher hasher;
    // A random value between -range and +range metres, with a random fraction.
    auto randomFixed = [&random](std::int64_t range) {
        const std::int64_t raw = static_cast<std::int64_t>((static_cast<std::uint64_t>(random.next()) << 32) | random.next());
        return Fixed::fromRaw(raw % (range * Fixed::kOneRaw));
    };
    Vec3 v{randomFixed(10), randomFixed(10), randomFixed(10)};
    Quat spin = Quat::fromAxisAngle(physics::kUp, randomFixed(3));

    int operations = 0;
    while (operations < 1'000'000) {
        const Fixed a = randomFixed(1000);
        const Fixed b = randomFixed(1000);
        const Fixed small = randomFixed(4);
        hasher.add((a + b).raw());
        hasher.add((a - b).raw());
        hasher.add((a * small).raw());
        hasher.add((b != physics::kFixedZero ? a / b : a).raw());
        hasher.add(physics::sqrt(physics::abs(a)).raw());
        hasher.add(physics::sin(small).raw());
        hasher.add(physics::cos(a).raw());
        v = physics::rotate(spin, v);
        hasher.add(v.x.raw());
        hasher.add(v.y.raw());
        hasher.add(v.z.raw());
        operations += 10;
        if (operations % 10'000 == 0) {
            spin = physics::normalized(spin * Quat::fromAxisAngle(physics::normalized(Vec3{a, b, small}), small));
            hasher.add(spin.w.raw());
            ++operations;
        }
    }
    return hasher.value();
}

} // namespace

TEST_CASE("US-025 Determinism") {
    const std::uint64_t first = runMixedMath(42);
    const std::uint64_t second = runMixedMath(42);
    CHECK(first == second);
    CHECK(runMixedMath(43) != first);
    // Pinned: the same number in Debug, Release and CI proves every build computes the
    // same bits. If a change to the math alters this on purpose, update it and say why.
    MESSAGE("mixed-math hash: ", first);
    CHECK(first == 17'224'312'723'153'614'174ULL);
}

namespace {

// Every part of Luna Physics in one run: throws with drag and wind, bouncing and sliding
// bodies, and a 1,000-body contact step. All results go into one hash.
std::uint64_t runWholePhysics() {
    odysseus::core::Hasher hasher;
    auto addVec = [&hasher](Vec3 v) {
        hasher.add(v.x.raw());
        hasher.add(v.y.raw());
        hasher.add(v.z.raw());
    };
    physics::Air air;
    air.wind = {Fixed::fromRatio(3, 2), Fixed::fromInt(-2), physics::kFixedZero};
    for (std::int64_t degreesUp = 5; degreesUp <= 60; degreesUp += 5) {
        physics::Projectile spear{{}, {}, Fixed::fromRatio(3, 2), Fixed::fromRatio(1, 100)};
        spear.velocity = physics::launchVelocity({}, {physics::kFixedOne, physics::kFixedOne, physics::kFixedZero},
                                                 Fixed::fromInt(20), physics::degrees(degreesUp));
        const auto landing = physics::flyUntilLanding(spear, air, physics::kFixedZero, 10);
        REQUIRE(landing.has_value());
        addVec(landing->point);
        hasher.add(landing->flightTime.raw());
    }
    const physics::Ground grass{physics::kFixedZero, {physics::kFixedZero, Fixed::fromRatio(7, 20)}};
    physics::RigidBody ball(Fixed::fromRatio(1, 2), Fixed::fromRatio(1, 10), {physics::kFixedZero, physics::kFixedZero, Fixed::fromInt(2)},
                            {Fixed::fromRatio(1, 2), Fixed::fromRatio(1, 2)});
    ball.setVelocity({Fixed::fromInt(2), Fixed::fromInt(1), physics::kFixedZero});
    for (int tick = 0; tick < 400; ++tick) {
        ball.step(grass, physics::kStandardGravity, Fixed::fromRatio(1, 20));
        addVec(ball.position());
        addVec(ball.velocity());
    }
    odysseus::core::Pcg32 random(1, 29);
    std::vector<physics::Shape> shapes;
    for (int i = 0; i < 1000; ++i) {
        const Vec3 place{Fixed::fromRatio(random.below(256'000), 1000), Fixed::fromRatio(random.below(256'000), 1000),
                         Fixed::fromRatio(random.below(2'000), 1000)};
        shapes.push_back(physics::Sphere{place, Fixed::fromRatio(200 + random.below(800), 1000)});
    }
    physics::SpatialGrid grid(physics::kFixedZero, physics::kFixedZero, Fixed::fromInt(256), Fixed::fromInt(256), Fixed::fromInt(2));
    for (const auto& contact : physics::findContacts(shapes, grid).contacts) {
        hasher.add(static_cast<std::uint64_t>(contact.first));
        hasher.add(static_cast<std::uint64_t>(contact.second));
        addVec(contact.contact.point);
        addVec(contact.contact.normal);
        hasher.add(contact.contact.depth.raw());
    }
    return hasher.value();
}

} // namespace

TEST_CASE("M1b Whole physics is identical on every build") {
    const std::uint64_t first = runWholePhysics();
    CHECK(first == runWholePhysics());
    // Pinned like the mixed-math hash: Debug, Release and CI must all compute this number.
    MESSAGE("whole-physics hash: ", first);
    CHECK(first == 17'309'765'312'882'650'619ULL);
}
