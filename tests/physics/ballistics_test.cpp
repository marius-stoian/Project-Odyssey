#include "luna/physics/ballistics.h"

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <cstdint>

using luna::physics::Air;
using luna::physics::Fixed;
using luna::physics::Projectile;
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

// A hunting spear: 1.5 kg, drag coefficient x area 0.01 m^2.
Projectile spear(Vec3 position) {
    return {position, {}, decimal(1500), decimal(10)};
}

// The textbook reference for drag: the same equation of motion, a = g - (rho Cd A / 2m)
// |v - w| (v - w), solved with doubles by 4th-order Runge-Kutta in 0.1 ms steps: a far more
// precise method than the game needs, so it serves as "what the formula predicts".
struct Reference {
    double x;
    double y;
};

Reference referenceLanding(double speed, double angleRadians, double windY, double dragPerMetre, double g) {
    using State = std::array<double, 6>; // x, y, z, vx, vy, vz
    auto derivative = [&](const State& s) {
        const double rx = s[3];
        const double ry = s[4] - windY;
        const double rz = s[5];
        const double relativeSpeed = std::sqrt(rx * rx + ry * ry + rz * rz);
        return State{s[3], s[4], s[5], -dragPerMetre * relativeSpeed * rx, -dragPerMetre * relativeSpeed * ry,
                     -g - dragPerMetre * relativeSpeed * rz};
    };
    State s{0, 0, 0, speed * std::cos(angleRadians), 0, speed * std::sin(angleRadians)};
    const double h = 1e-4;
    while (true) {
        const State before = s;
        auto add = [](const State& a, const State& b, double scale) {
            State r{};
            for (std::size_t i = 0; i < 6; ++i) {
                r[i] = a[i] + b[i] * scale;
            }
            return r;
        };
        const State k1 = derivative(s);
        const State k2 = derivative(add(s, k1, h / 2));
        const State k3 = derivative(add(s, k2, h / 2));
        const State k4 = derivative(add(s, k3, h));
        for (std::size_t i = 0; i < 6; ++i) {
            s[i] += h / 6 * (k1[i] + 2 * k2[i] + 2 * k3[i] + k4[i]);
        }
        if (s[2] <= 0 && before[2] > 0) {
            const double f = before[2] / (before[2] - s[2]);
            return {before[0] + (s[0] - before[0]) * f, before[1] + (s[1] - before[1]) * f};
        }
    }
}

} // namespace

TEST_CASE("US-027 Arc") {
    // 20 m/s at 45 degrees over flat ground, no air: range v^2 / g = 400 / 9.81 = 40.77 m.
    Projectile thrown = spear({});
    thrown.dragArea = physics::kFixedZero;
    thrown.velocity = physics::launchVelocity({}, {physics::kFixedOne, physics::kFixedZero, physics::kFixedZero},
                                              Fixed::fromInt(20), physics::degrees(45));
    const auto landing = physics::flyUntilLanding(thrown, Air{}, physics::kFixedZero, 10);
    REQUIRE(landing.has_value());
    const double textbookRange = 400.0 / 9.81;
    const double range = toDouble(landing->point.x);
    MESSAGE("range ", range, " m (textbook ", textbookRange, " m), flight ", toDouble(landing->flightTime), " s");
    CHECK(std::abs(range - textbookRange) / textbookRange < 0.01);
    CHECK(std::abs(toDouble(landing->flightTime) - 2 * 20 * std::sin(std::acos(-1.0) / 4) / 9.81) < 0.03);
    CHECK(landing->point.y == physics::kFixedZero);
}

TEST_CASE("US-027 Drag and wind") {
    const Fixed speed = Fixed::fromInt(20);
    const Vec3 east{physics::kFixedOne, physics::kFixedZero, physics::kFixedZero};
    Air windy;
    windy.wind = {physics::kFixedZero, Fixed::fromInt(5), physics::kFixedZero}; // 5 m/s crosswind blowing south (+y)

    Projectile thrown = spear({});
    thrown.velocity = physics::launchVelocity({}, east, speed, physics::degrees(45));
    const auto still = physics::flyUntilLanding(thrown, Air{}, physics::kFixedZero, 10);
    const auto blown = physics::flyUntilLanding(thrown, windy, physics::kFixedZero, 10);
    REQUIRE(still.has_value());
    REQUIRE(blown.has_value());

    const double dragPerMetre = toDouble(Air{}.density) * 0.01 / (2 * 1.5);
    const double g = toDouble(physics::kStandardGravity);
    const double angle = std::acos(-1.0) / 4;
    const Reference calm = referenceLanding(20, angle, 0, dragPerMetre, g);
    const Reference breeze = referenceLanding(20, angle, 5, dragPerMetre, g);
    MESSAGE("drag, no wind: ", toDouble(still->point.x), " m (formula ", calm.x, " m)");
    MESSAGE("drag and wind: ", toDouble(blown->point.x), " m, drift ", toDouble(blown->point.y), " m (formula ",
            breeze.x, " m, drift ", breeze.y, " m)");

    // It falls short of the 40.8 m vacuum range, by the amount the drag equation predicts...
    CHECK(toDouble(still->point.x) < 400.0 / 9.81 - 1.0);
    CHECK(std::abs(toDouble(still->point.x) - calm.x) / calm.x < 0.01);
    CHECK(still->point.y == physics::kFixedZero);
    // ...and the crosswind carries it downwind by the predicted amount.
    CHECK(toDouble(blown->point.y) > 0.5);
    CHECK(std::abs(toDouble(blown->point.x) - breeze.x) / breeze.x < 0.01);
    CHECK(std::abs(toDouble(blown->point.y) - breeze.y) / breeze.y < 0.01);
}

TEST_CASE("US-027 Aim") {
    // A straw target (0.3 m radius, centre 1 m up) 25 m away, north-east-ish; the hunter
    // throws from shoulder height (1.5 m) at 18 m/s, with air drag.
    const Vec3 hand{physics::kFixedZero, physics::kFixedZero, decimal(1500)};
    const Vec3 targetCentre{Fixed::fromInt(15), Fixed::fromInt(-20), physics::kFixedOne};
    CHECK(physics::length(Vec3{targetCentre.x, targetCentre.y, physics::kFixedZero}) == Fixed::fromInt(25));
    const Fixed speed = Fixed::fromInt(18);
    const Air air;

    const auto vacuum = physics::launchAngleWithoutDrag(speed, Fixed::fromInt(25), -decimal(500), air.gravity);
    REQUIRE(vacuum.has_value());
    const auto angle = physics::aimLaunchAngle(spear(hand), air, targetCentre, speed);
    REQUIRE(angle.has_value());
    MESSAGE("launch angle: vacuum ", toDouble(*vacuum) * 180 / std::acos(-1.0), " deg, with drag ",
            toDouble(*angle) * 180 / std::acos(-1.0), " deg");
    CHECK(*angle > *vacuum); // drag needs a slightly higher throw

    // Throw it for real, tick by tick, with swept collisions against the target.
    Projectile thrown = spear(hand);
    thrown.velocity = physics::launchVelocity(hand, targetCentre, speed, *angle);
    const std::vector<physics::Shape> obstacles{physics::Sphere{targetCentre, decimal(300)}};
    std::optional<physics::ProjectileHit> hit;
    int ticks = 0;
    for (; ticks < 100 && !hit; ++ticks) {
        hit = physics::flyTick(thrown, air, decimal(20), obstacles);
    }
    REQUIRE(hit.has_value());
    MESSAGE("hit after ", ticks, " ticks at (", toDouble(hit->hit.point.x), ", ", toDouble(hit->hit.point.y), ", ",
            toDouble(hit->hit.point.z), ")");
    // It strikes the target's surface (0.3 m from its centre) on the side facing the hunter;
    // coming down at an angle, it meets the front slightly above the centre.
    CHECK(std::abs(toDouble(physics::length(hit->hit.point - targetCentre)) - 0.3) < 0.001);
    CHECK(physics::length(hit->hit.point) < physics::length(targetCentre));
    CHECK(std::abs(toDouble(hit->hit.point.z) - 1.0) < 0.3);

    // Out of reach: 100 m at 18 m/s cannot be hit.
    CHECK_FALSE(physics::launchAngleWithoutDrag(speed, Fixed::fromInt(100), physics::kFixedZero, air.gravity).has_value());
    CHECK_FALSE(physics::aimLaunchAngle(spear(hand), air, {Fixed::fromInt(100), physics::kFixedZero, physics::kFixedOne},
                                        speed)
                    .has_value());
}

TEST_CASE("US-027 atan2 is accurate to 1e-9") {
    for (std::int64_t y = -40; y <= 40; y += 3) {
        for (std::int64_t x = -40; x <= 40; x += 7) {
            const Fixed fy = Fixed::fromRatio(y, 8);
            const Fixed fx = Fixed::fromRatio(x, 8);
            INFO("y ", y, " x ", x);
            CHECK(std::abs(toDouble(physics::atan2(fy, fx)) - std::atan2(y / 8.0, x / 8.0)) < 2e-9);
        }
    }
}
