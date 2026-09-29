#include "luna/physics/fixed.h"

#include "core/random.h"

#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

using luna::physics::Fixed;
namespace physics = luna::physics;

namespace {

// Tests may use doubles as a reference; physics code never does (Charter rule 10).
double toDouble(Fixed value) {
    return static_cast<double>(value.raw()) / 4294967296.0;
}

} // namespace

// Expected raw values were computed independently with exact fractions (Python's
// fractions.Fraction), so equality here means "bit for bit", in Debug, Release and CI.
TEST_CASE("US-025 Exact arithmetic") {
    const Fixed a = Fixed::fromRatio(3, 2);  // 1.5
    const Fixed b = Fixed::fromRatio(9, 4);  // 2.25
    CHECK(a.raw() == 6'442'450'944);
    CHECK(b.raw() == 9'663'676'416);

    CHECK((a + b).raw() == Fixed::fromRatio(15, 4).raw());   // 3.75
    CHECK((a - b).raw() == -Fixed::fromRatio(3, 4).raw());   // -0.75
    CHECK((a * b).raw() == 14'495'514'624);                  // 3.375 exactly
    CHECK((a / b).raw() == 2'863'311'531);                   // 2/3, rounded to nearest
    CHECK((-a * b).raw() == -14'495'514'624);                // signs are symmetric
    CHECK((-a / b).raw() == -2'863'311'531);

    CHECK(Fixed::fromRatio(1, 3).raw() == 1'431'655'765);
    CHECK(Fixed::fromRatio(-2, 3).raw() == -2'863'311'531);
    CHECK(Fixed::fromRatio(981, 100).raw() == 42'133'629'174); // g = 9.81
    CHECK((Fixed::fromRatio(1, 3) * Fixed::fromInt(3)).raw() == 4'294'967'295); // one step short of 1
    CHECK((Fixed::fromInt(-7) / Fixed::fromInt(2)) == -Fixed::fromRatio(7, 2));
    CHECK((Fixed::fromInt(40'000) * Fixed::fromInt(40'000)) == Fixed::fromInt(1'600'000'000));

    // Half a step times one half rounds to nearest: 0.5 steps rounds up to 1 step.
    CHECK((Fixed::fromRaw(1) * physics::kFixedHalf).raw() == 1);
    CHECK((Fixed::fromRaw(-1) * physics::kFixedHalf).raw() == -1);

    CHECK(Fixed::fromRatio(5, 2).roundToInt() == 3);
    CHECK(Fixed::fromRatio(-5, 2).roundToInt() == -3);
    CHECK(Fixed::fromRatio(-1, 2).floorToInt() == -1);

    CHECK(physics::sqrt(Fixed::fromInt(2)).raw() == 6'074'000'999);
    CHECK(physics::sqrt(Fixed::fromInt(4)) == Fixed::fromInt(2));
    CHECK(physics::sqrt(Fixed::fromRatio(1, 4)) == physics::kFixedHalf);
    CHECK(physics::degrees(30).raw() == 2'248'839'618);
}

#if defined(_MSC_VER)
// A second, independent oracle: the processor's own 128-bit multiply and divide
// instructions, over 100,000 random pairs.
TEST_CASE("US-025 Exact arithmetic matches the 128-bit hardware instructions") {
    odysseus::core::Pcg32 random(2025, 25);
    auto randomRaw = [&random](int bits) {
        const std::uint64_t value = (static_cast<std::uint64_t>(random.next()) << 32) | random.next();
        const auto magnitude = static_cast<std::int64_t>(value >> (64 - bits));
        return random.chance(50) ? -magnitude : magnitude;
    };
    auto roundedSign = [](std::uint64_t size, bool negative) {
        return negative ? -static_cast<std::int64_t>(size) : static_cast<std::int64_t>(size);
    };
    for (int i = 0; i < 100'000; ++i) {
        // Multiply: values up to about +/-45,000, so the product always fits.
        const std::int64_t a = randomRaw(47);
        const std::int64_t b = randomRaw(47);
        std::uint64_t high = 0;
        std::uint64_t low = _umul128(static_cast<std::uint64_t>(std::llabs(a)), static_cast<std::uint64_t>(std::llabs(b)), &high);
        low += std::uint64_t{1} << 31; // round to nearest, as Fixed does
        if (low < (std::uint64_t{1} << 31)) {
            ++high;
        }
        const std::int64_t expectedProduct = roundedSign((high << 32) | (low >> 32), (a < 0) != (b < 0));
        REQUIRE((Fixed::fromRaw(a) * Fixed::fromRaw(b)).raw() == expectedProduct);

        // Divide: a up to +/-256, b at least 1/4096 in size, so the quotient fits.
        const std::int64_t n = randomRaw(41);
        std::int64_t d = randomRaw(52);
        if (std::llabs(d) < (std::int64_t{1} << 20)) {
            d = std::int64_t{1} << 20;
        }
        const auto un = static_cast<std::uint64_t>(std::llabs(n));
        const auto ud = static_cast<std::uint64_t>(std::llabs(d));
        std::uint64_t remainder = 0;
        std::uint64_t quotient = _udiv128(un >> 32, un << 32, ud, &remainder);
        if (remainder >= ud - remainder) {
            ++quotient;
        }
        REQUIRE((Fixed::fromRaw(n) / Fixed::fromRaw(d)).raw() == roundedSign(quotient, (n < 0) != (d < 0)));

        // Square root: floor(sqrt(raw * 2^32)) means root^2 <= raw * 2^32 < (root + 1)^2.
        const auto s = static_cast<std::uint64_t>(std::llabs(randomRaw(62)));
        const auto root = static_cast<std::uint64_t>(physics::sqrt(Fixed::fromRaw(static_cast<std::int64_t>(s))).raw());
        std::uint64_t squareHigh = 0;
        std::uint64_t squareLow = _umul128(root, root, &squareHigh);
        const std::uint64_t targetHigh = s >> 32;
        const std::uint64_t targetLow = s << 32;
        const bool rootFits = squareHigh < targetHigh || (squareHigh == targetHigh && squareLow <= targetLow);
        squareLow = _umul128(root + 1, root + 1, &squareHigh);
        const bool nextTooBig = squareHigh > targetHigh || (squareHigh == targetHigh && squareLow > targetLow);
        REQUIRE(rootFits);
        REQUIRE(nextTooBig);
    }
}
#endif

TEST_CASE("US-025 Sine and cosine are accurate to 1e-9") {
    // 2,000 angles from -4 turns to +4 turns.
    for (std::int64_t step = -1000; step <= 1000; ++step) {
        const Fixed angle = Fixed::fromRatio(step * 8 * 314'159'265, std::int64_t{100'000'000'000});
        const double reference = toDouble(angle);
        INFO("angle ", reference);
        CHECK(std::abs(toDouble(physics::sin(angle)) - std::sin(reference)) < 2e-9);
        CHECK(std::abs(toDouble(physics::cos(angle)) - std::cos(reference)) < 2e-9);
    }
    CHECK(physics::sin(physics::kFixedZero) == physics::kFixedZero);
    CHECK(physics::cos(physics::kFixedZero) == physics::kFixedOne);
}
