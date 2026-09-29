#include "luna/physics/quat.h"

#include "core/hash.h"
#include "core/random.h"

#include <doctest/doctest.h>

#include <cstdint>

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
