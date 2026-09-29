#include "luna/physics/rigid_body.h"

#include "luna/physics/ballistics.h"

#include <doctest/doctest.h>

#include <cmath>
#include <vector>

using luna::physics::Fixed;
using luna::physics::Ground;
using luna::physics::RigidBody;
using luna::physics::SurfaceMaterial;
using luna::physics::Vec3;
namespace physics = luna::physics;

namespace {

// Tests may use doubles as a reference; physics code never does (Charter rule 10).
double toDouble(Fixed value) {
    return static_cast<double>(value.raw()) / 4294967296.0;
}

Fixed decimal(std::int64_t thousandths) {
    return Fixed::fromRatio(thousandths, 1000);
}

const Fixed kTick = Fixed::fromRatio(1, 20); // the simulation tick, 1/20 s
const Ground kGrass{physics::kFixedZero, {physics::kFixedZero, decimal(350)}};
const Ground kIce{physics::kFixedZero, {physics::kFixedZero, physics::kFixedZero}};

} // namespace

TEST_CASE("US-028 Impulse") {
    // A 70 kg person at rest, pushed with 140 N s towards (3, 4): v = J / m = 2 m/s that way.
    RigidBody person(Fixed::fromInt(70), decimal(750), {}, {physics::kFixedZero, decimal(500)});
    const Vec3 direction = physics::normalized({Fixed::fromInt(3), Fixed::fromInt(4), physics::kFixedZero});
    person.applyImpulse(direction * Fixed::fromInt(140));
    CHECK(std::abs(toDouble(person.velocity().x) - 1.2) < 1e-9);
    CHECK(std::abs(toDouble(person.velocity().y) - 1.6) < 1e-9);
    CHECK(std::abs(toDouble(physics::length(person.velocity())) - 2.0) < 1e-9);

    // The same momentum from a steady push: 140 N for one second (20 ticks) on ice.
    RigidBody sled(Fixed::fromInt(70), decimal(750), {physics::kFixedZero, physics::kFixedZero, decimal(750)},
                   {physics::kFixedZero, physics::kFixedZero});
    for (int tick = 0; tick < 20; ++tick) {
        sled.applyForce({Fixed::fromInt(140), physics::kFixedZero, physics::kFixedZero});
        sled.step(kIce, physics::kStandardGravity, kTick);
    }
    CHECK(std::abs(toDouble(sled.velocity().x) - 2.0) < 1e-6);
    CHECK(sled.position().z == decimal(750)); // it stayed on the ground
}

TEST_CASE("US-028 Bounce and rest") {
    // A 0.1 m ball with restitution 0.5 dropped from 1 m onto grass. Each bounce should reach
    // e^2 = 0.25 of the previous height: 1 m, 0.25 m, 0.0625 m, 0.0156 m, ...
    const Fixed radius = decimal(100);
    RigidBody ball(decimal(500), radius, {physics::kFixedZero, physics::kFixedZero, radius + physics::kFixedOne},
                   {decimal(500), decimal(500)});
    // Step every millisecond so the highest point of each hop is measured precisely.
    const Fixed millisecond = Fixed::fromRatio(1, 1000);
    std::vector<double> peaks{1.0};
    double highest = 0;
    bool rising = false;
    for (int step = 0; step < 10'000; ++step) {
        const double before = toDouble(ball.position().z - radius);
        ball.step(kGrass, physics::kStandardGravity, millisecond);
        const double height = toDouble(ball.position().z - radius);
        if (height > before) {
            rising = true;
            highest = height;
        } else if (rising && height < before) {
            peaks.push_back(highest); // it just passed the top of a hop
            rising = false;
        }
    }
    REQUIRE(peaks.size() >= 5);
    for (std::size_t i = 1; i < 5; ++i) {
        MESSAGE("bounce ", i, ": ", peaks[i], " m, ratio ", peaks[i] / peaks[i - 1]);
        CHECK(std::abs(peaks[i] / peaks[i - 1] - 0.25) < 0.0025);
    }
    // After ten seconds it lies still on the ground, asleep.
    CHECK(ball.asleep());
    CHECK(ball.position().z == radius);
    CHECK(ball.velocity() == Vec3{});

    // At the game's own 20 ticks per second it comes to rest too.
    RigidBody ticked(decimal(500), radius, {physics::kFixedZero, physics::kFixedZero, radius + physics::kFixedOne},
                     {decimal(500), decimal(500)});
    for (int tick = 0; tick < 200; ++tick) {
        ticked.step(kGrass, physics::kStandardGravity, kTick);
    }
    CHECK(ticked.asleep());
    CHECK(ticked.position().z == radius);
}

TEST_CASE("US-028 Friction") {
    // A 20 kg wooden crate (mu 0.4) sliding at 3 m/s on grass (mu 0.35): mu = sqrt(0.4 x 0.35).
    // Friction brakes at mu g, so it stops after v^2 / (2 mu g) metres.
    const SurfaceMaterial wood{decimal(100), decimal(400)};
    RigidBody crate(Fixed::fromInt(20), decimal(250), {physics::kFixedZero, physics::kFixedZero, decimal(250)}, wood);
    crate.setVelocity({Fixed::fromInt(3), physics::kFixedZero, physics::kFixedZero});
    int ticks = 0;
    for (; ticks < 200 && !crate.asleep(); ++ticks) {
        crate.step(kGrass, physics::kStandardGravity, kTick);
    }
    const double mu = std::sqrt(0.4 * 0.35);
    const double predicted = 9.0 / (2 * mu * 9.81);
    const double slid = toDouble(crate.position().x);
    MESSAGE("slid ", slid, " m (friction predicts ", predicted, " m), asleep after ", ticks, " ticks");
    CHECK(std::abs(slid - predicted) / predicted < 0.01);
    CHECK(crate.velocity() == Vec3{});
    CHECK(crate.position().z == decimal(250));
    CHECK(physics::combinedFriction(wood, kGrass.surface) == physics::sqrt(decimal(400) * decimal(350)));

    // A sleeping crate wakes up when pushed again.
    crate.applyImpulse({Fixed::fromInt(20), physics::kFixedZero, physics::kFixedZero});
    CHECK_FALSE(crate.asleep());
    crate.step(kGrass, physics::kStandardGravity, kTick);
    CHECK(toDouble(crate.position().x) > slid);
}
