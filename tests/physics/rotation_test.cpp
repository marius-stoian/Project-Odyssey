#include "luna/physics/quat.h"

#include <doctest/doctest.h>

#include <cstdint>

using luna::physics::Fixed;
using luna::physics::Quat;
using luna::physics::Vec3;
namespace physics = luna::physics;

namespace {

// "Within 1/65536 of a unit" from the acceptance criterion, in raw Fixed steps.
constexpr std::int64_t kToleranceRaw = Fixed::kOneRaw / 65'536;

bool near(Vec3 a, Vec3 b, std::int64_t toleranceRaw = kToleranceRaw) {
    return physics::abs(a.x - b.x).raw() <= toleranceRaw && physics::abs(a.y - b.y).raw() <= toleranceRaw &&
           physics::abs(a.z - b.z).raw() <= toleranceRaw;
}

Vec3 vec(std::int64_t x, std::int64_t y, std::int64_t z) {
    return {Fixed::fromInt(x), Fixed::fromInt(y), Fixed::fromInt(z)};
}

} // namespace

TEST_CASE("US-025 Rotations") {
    const Quat quarterTurn = Quat::fromAxisAngle(physics::kUp, physics::degrees(90));
    const Vec3 start = vec(1, 2, 3);

    // One quarter turn about "up" takes east to south: (1, 2, 3) becomes (-2, 1, 3).
    CHECK(near(physics::rotate(quarterTurn, start), vec(-2, 1, 3)));

    Vec3 v = start;
    for (int turn = 0; turn < 4; ++turn) {
        v = physics::rotate(quarterTurn, v);
    }
    INFO("error in raw steps: x ", (v.x - start.x).raw(), " y ", (v.y - start.y).raw(), " z ", (v.z - start.z).raw());
    CHECK(near(v, start));

    // Joining the four turns into one quaternion gives the same full turn.
    const Quat fullTurn = quarterTurn * quarterTurn * quarterTurn * quarterTurn;
    CHECK(near(physics::rotate(fullTurn, start), start));
}

TEST_CASE("US-025 Vectors") {
    const Vec3 a = vec(1, 2, 3);
    const Vec3 b = vec(4, -5, 6);
    CHECK(physics::dot(a, b) == Fixed::fromInt(12));             // 4 - 10 + 18
    CHECK(physics::cross(a, b) == vec(27, 6, -13));              // right-hand rule
    CHECK(physics::cross(vec(1, 0, 0), vec(0, 1, 0)) == physics::kUp);
    CHECK(physics::length(vec(3, 4, 12)) == Fixed::fromInt(13)); // a 3-4-12-13 box
    CHECK(near(physics::normalized(vec(0, 3, 4)), {physics::kFixedZero, Fixed::fromRatio(3, 5), Fixed::fromRatio(4, 5)}, 2));

    // Undoing a rotation with its conjugate returns the vector.
    const Quat tilt = Quat::fromAxisAngle(physics::normalized(vec(1, 1, 1)), physics::degrees(50));
    CHECK(near(physics::rotate(physics::conjugate(tilt), physics::rotate(tilt, a)), a));
    const Quat unit = physics::normalized(tilt * tilt);
    const Fixed size = unit.w * unit.w + unit.x * unit.x + unit.y * unit.y + unit.z * unit.z;
    CHECK(physics::abs(size - physics::kFixedOne).raw() < 16);
}
