#include "luna/physics/spatial_grid.h"

#include "core/random.h"

#include <doctest/doctest.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

using luna::physics::Box;
using luna::physics::Capsule;
using luna::physics::Fixed;
using luna::physics::Shape;
using luna::physics::Sphere;
using luna::physics::Vec3;
namespace physics = luna::physics;

namespace {

// Tests may use doubles as a reference; physics code never does (Charter rule 10).
double toDouble(Fixed value) {
    return static_cast<double>(value.raw()) / 4294967296.0;
}

// Metres given in millimetres, so test positions stay exact integers.
Fixed mm(std::int64_t millimetres) {
    return Fixed::fromRatio(millimetres, 1000);
}

Vec3 at(std::int64_t xMm, std::int64_t yMm, std::int64_t zMm) {
    return {mm(xMm), mm(yMm), mm(zMm)};
}

bool near(Fixed a, double expected, double tolerance = 1e-8) {
    return std::abs(toDouble(a) - expected) <= tolerance;
}

bool near(Vec3 v, double x, double y, double z, double tolerance = 1e-8) {
    return near(v.x, x, tolerance) && near(v.y, y, tolerance) && near(v.z, z, tolerance);
}

} // namespace

TEST_CASE("US-026 Overlap") {
    // A 1 m sphere at the origin and a box whose left face is at x = 1: they just touch.
    const Sphere sphere{at(0, 0, 0), mm(1000)};
    const Box box{at(1000, -1000, -1000), at(3000, 1000, 1000)};
    const auto touching = physics::overlap(sphere, box);
    REQUIRE(touching.has_value());
    CHECK(touching->point == at(1000, 0, 0));
    CHECK(touching->normal == Vec3{physics::kFixedOne, physics::kFixedZero, physics::kFixedZero}); // sphere -> box
    CHECK(touching->depth == physics::kFixedZero);

    // Pushed 0.25 m into the box: 0.25 m deep; asking the other way round flips the normal.
    const Sphere inside{at(250, 0, 0), mm(1000)};
    const auto deep = physics::overlap(inside, box);
    REQUIRE(deep.has_value());
    CHECK(deep->depth == mm(250));
    const auto reversed = physics::overlap(box, inside);
    REQUIRE(reversed.has_value());
    CHECK(reversed->normal == -deep->normal);

    // One millimetre apart: no hit.
    CHECK_FALSE(physics::overlap(Sphere{at(-1, 0, 0), mm(1000)}, box).has_value());
}

TEST_CASE("US-026 Overlap between every pair of shape kinds") {
    const Shape sphere = Sphere{at(0, 0, 0), mm(500)};
    const Shape capsule = Capsule{at(800, -1000, 0), at(800, 1000, 0), mm(400)}; // upright post, 0.1 m overlap
    const Shape box = Box{at(1100, -500, -500), at(2000, 500, 500)};            // 0.1 m into the post
    const Shape farBox = Box{at(5000, 5000, 5000), at(6000, 6000, 6000)};

    const auto sphereCapsule = physics::overlap(sphere, capsule);
    REQUIRE(sphereCapsule.has_value());
    CHECK(near(sphereCapsule->depth, 0.1));
    CHECK(near(sphereCapsule->normal, 1, 0, 0));

    const auto capsuleBox = physics::overlap(capsule, box);
    REQUIRE(capsuleBox.has_value());
    CHECK(near(capsuleBox->depth, 0.1, 1e-6));
    CHECK(near(capsuleBox->normal, 1, 0, 0, 1e-6));

    const auto capsuleCapsule = physics::overlap(capsule, Capsule{at(1500, 0, -1000), at(1500, 0, 1000), mm(400)});
    REQUIRE(capsuleCapsule.has_value()); // crossing posts 0.7 m apart, radii 0.4 + 0.4
    CHECK(near(capsuleCapsule->depth, 0.1));

    const auto boxBox = physics::overlap(box, Box{at(1900, -200, -200), at(3000, 200, 200)});
    REQUIRE(boxBox.has_value());
    CHECK(near(boxBox->depth, 0.1));
    CHECK(near(boxBox->normal, 1, 0, 0));

    CHECK_FALSE(physics::overlap(sphere, farBox).has_value());
    CHECK_FALSE(physics::overlap(capsule, farBox).has_value());
    CHECK_FALSE(physics::overlap(farBox, box).has_value());
}

TEST_CASE("US-026 No tunnelling") {
    // A spear tip (2 cm sphere) moving 10 m per tick at a 0.2 m target 5 m away. Checking
    // only where it is at each tick misses the target completely...
    const Sphere tip{at(0, 0, 1000), mm(20)};
    const Vec3 perTick = at(10'000, 0, 0);
    const Shape target = Box{at(5000, -100, 900), at(5200, 100, 1100)};
    CHECK_FALSE(physics::overlap(tip, target).has_value());
    CHECK_FALSE(physics::overlap(Sphere{tip.center + perTick, tip.radius}, target).has_value());

    // ...but the swept path finds it: the tip's front (x + 0.02) reaches x = 5 after 4.98 m,
    // that is 0.498 of the tick = 0.0249 s at 20 ticks per second.
    const auto hit = physics::sweep(tip, perTick, target);
    REQUIRE(hit.has_value());
    CHECK(near(hit->time, 0.498, 1e-9));
    CHECK(near(hit->time / 20, 0.0249, 1e-9)); // seconds
    CHECK(near(hit->point, 5, 0, 1));
    CHECK(near(hit->normal, -1, 0, 0));

    // The same against a 0.2 m round target (radius 0.1 at x = 5): 4.88 m, time 0.488.
    const auto round = physics::sweep(tip, perTick, Sphere{at(5000, 0, 1000), mm(100)});
    REQUIRE(round.has_value());
    CHECK(near(round->time, 0.488, 1e-9));

    // And against a thin post (capsule), and past it: 0.2 m to the side is a miss.
    const Shape post = Capsule{at(5000, 0, 0), at(5000, 0, 2000), mm(100)};
    const auto postHit = physics::sweep(tip, perTick, post);
    REQUIRE(postHit.has_value());
    CHECK(near(postHit->time, 0.488, 1e-9));
    CHECK_FALSE(physics::sweep(Sphere{at(0, 200, 1000), mm(20)}, perTick, target).has_value());
}

TEST_CASE("US-026 Rays and rounded corners") {
    const Shape box = Box{at(0, 0, 0), at(1000, 1000, 1000)};
    const auto ray = physics::raycast(at(-2000, 500, 500), at(4000, 0, 0), box);
    REQUIRE(ray.has_value());
    CHECK(near(ray->time, 0.5));
    CHECK(near(ray->normal, -1, 0, 0));

    // A 1 m ball rolling diagonally at the box's vertical edge touches the rounded corner:
    // its centre is 1 m from the edge when 2 - 2t = 1/sqrt(2), so t = 1 - 1/(2 sqrt 2).
    const auto corner = physics::sweep(Sphere{at(-2000, -2000, 500), mm(1000)}, at(2000, 2000, 0), box);
    REQUIRE(corner.has_value());
    CHECK(near(corner->time, 1.0 - 1.0 / (2.0 * std::sqrt(2.0)), 1e-8));
    CHECK(near(corner->normal, -1.0 / std::sqrt(2.0), -1.0 / std::sqrt(2.0), 0, 1e-8));
    CHECK(near(corner->point, 0, 0, 0.5, 1e-8));

    // Starting inside counts as a hit at time 0.
    const auto inside = physics::raycast(at(500, 500, 500), at(100, 0, 0), box);
    REQUIRE(inside.has_value());
    CHECK(inside->time == physics::kFixedZero);
}

TEST_CASE("US-026 Many bodies") {
    // 1,000 spheres, capsules and boxes scattered over a 256 m x 256 m region.
    odysseus::core::Pcg32 random(26, 26);
    auto metres = [&random](std::uint32_t maxMm) { return mm(static_cast<std::int64_t>(random.below(maxMm))); };
    std::vector<Shape> shapes;
    for (int i = 0; i < 1000; ++i) {
        const Vec3 place{metres(256'000), metres(256'000), metres(2'000)};
        const Fixed size = mm(200) + metres(800);
        switch (i % 3) {
        case 0:
            shapes.push_back(Sphere{place, size});
            break;
        case 1:
            shapes.push_back(Capsule{place, place + Vec3{size, size, physics::kFixedZero}, mm(300)});
            break;
        default:
            shapes.push_back(Box{place, place + Vec3{size, size, size}});
            break;
        }
    }
    physics::SpatialGrid grid(physics::kFixedZero, physics::kFixedZero, Fixed::fromInt(256), Fixed::fromInt(256),
                              Fixed::fromInt(2));
    const auto report = physics::findContacts(shapes, grid);

    // Only nearby pairs were tested: a tiny fraction of the 499,500 possible pairs...
    MESSAGE("pairs tested: ", report.pairsTested, " of 499500; contacts: ", report.contacts.size());
    CHECK(report.pairsTested < 5000);
    // ...and nothing was missed: testing every pair finds exactly the same contacts.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> everyPair;
    for (std::uint32_t a = 0; a < shapes.size(); ++a) {
        for (std::uint32_t b = a + 1; b < shapes.size(); ++b) {
            if (physics::overlap(shapes[a], shapes[b])) {
                everyPair.emplace_back(a, b);
            }
        }
    }
    REQUIRE(report.contacts.size() == everyPair.size());
    for (std::size_t i = 0; i < everyPair.size(); ++i) {
        CHECK(report.contacts[i].first == everyPair[i].first);
        CHECK(report.contacts[i].second == everyPair[i].second);
    }

    // Speed: the fastest of 20 steps (the others include the PC doing other things).
    double fastestMs = 1e9;
    for (int run = 0; run < 20; ++run) {
        const auto start = std::chrono::steady_clock::now();
        const auto again = physics::findContacts(shapes, grid);
        const auto elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start);
        CHECK(again.contacts.size() == report.contacts.size());
        fastestMs = std::min(fastestMs, elapsed.count());
    }
    MESSAGE("one step with 1,000 bodies: ", fastestMs, " ms");
#if defined(NDEBUG)
    CHECK(fastestMs < 2.0); // the budget applies to the optimised build the game ships
#endif
}
