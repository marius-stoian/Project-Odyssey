#pragma once

#include "boundary.h"

#include "core/assertions.h"

#include <compare>
#include <cstdint>

namespace luna::physics {

// A fixed-point number: a 64-bit integer that counts units of 1/2^32 (ADR-017, "32.32").
// 32 bits hold the whole part (about +/-2.1 billion) and 32 bits the fraction (steps of
// 0.00000000023). Every operation is plain integer math, so every compiler and CPU gives
// exactly the same bits. Floats would round differently on different machines, and a
// physics world that drifts apart breaks replays, saves and multiplayer (Charter rule 10).
//
// Physics works in SI units: metres, seconds, kilograms. One 32-pixel tile is 1 metre.
class Fixed {
public:
    static constexpr int kFractionBits = 32;
    static constexpr std::int64_t kOneRaw = std::int64_t{1} << kFractionBits;
    // Whole numbers from -2^31 to 2^31 - 1 fit; anything larger overflows.
    static constexpr std::int64_t kMaxWhole = (std::int64_t{1} << 31) - 1;
    static constexpr std::int64_t kMinWhole = -(std::int64_t{1} << 31);

    // Zero.
    constexpr Fixed() = default;

    // The stored integer itself: raw / 2^32 is the value.
    static constexpr Fixed fromRaw(std::int64_t raw) {
        Fixed result;
        result.raw_ = raw;
        return result;
    }

    // An exact whole number, for example Fixed::fromInt(3) is 3.0.
    static constexpr Fixed fromInt(std::int64_t whole) {
        ODYSSEUS_ASSERT(whole >= kMinWhole && whole <= kMaxWhole, "whole number too large for Fixed");
        return fromRaw(whole * kOneRaw);
    }

    // numerator / denominator, rounded to the nearest step: fromRatio(981, 100) is 9.81.
    // This is how we write decimal constants without ever touching floating point.
    static Fixed fromRatio(std::int64_t numerator, std::int64_t denominator);

    constexpr std::int64_t raw() const { return raw_; }

    // The whole part, rounded towards minus infinity (floor(-0.5) is -1).
    constexpr std::int64_t floorToInt() const { return raw_ >> kFractionBits; }
    // Rounded to the nearest whole number, halves away from zero.
    std::int64_t roundToInt() const;

    // Adding and subtracting are plain integer add and subtract.
    friend Fixed operator+(Fixed a, Fixed b) { return fromRaw(checkedAdd(a.raw_, b.raw_)); }
    friend Fixed operator-(Fixed a, Fixed b) { return fromRaw(checkedAdd(a.raw_, negateRaw(b.raw_))); }
    friend Fixed operator-(Fixed a) { return fromRaw(negateRaw(a.raw_)); }

    // Multiplying and dividing need the 128-bit intermediate result (see fixed.cpp).
    friend Fixed operator*(Fixed a, Fixed b);
    friend Fixed operator/(Fixed a, Fixed b);
    // Scaling by a whole number is exact (apart from rounding for division).
    friend Fixed operator*(Fixed a, std::int64_t whole);
    friend Fixed operator*(std::int64_t whole, Fixed a) { return a * whole; }
    friend Fixed operator/(Fixed a, std::int64_t whole);

    Fixed& operator+=(Fixed other) { return *this = *this + other; }
    Fixed& operator-=(Fixed other) { return *this = *this - other; }
    Fixed& operator*=(Fixed other) { return *this = *this * other; }
    Fixed& operator/=(Fixed other) { return *this = *this / other; }

    // Comparisons compare the raw integers, which orders the values correctly.
    friend constexpr auto operator<=>(const Fixed&, const Fixed&) = default;
    friend constexpr bool operator==(const Fixed&, const Fixed&) = default;

private:
    // Overflow is a bug, never a silent wrap-around: asserted in Debug (ADR-015).
    static std::int64_t checkedAdd(std::int64_t a, std::int64_t b) {
        // Unsigned addition wraps without undefined behaviour; the sign test finds overflow.
        const auto sum = static_cast<std::int64_t>(static_cast<std::uint64_t>(a) + static_cast<std::uint64_t>(b));
        ODYSSEUS_ASSERT(((a ^ sum) & (b ^ sum)) >= 0, "Fixed addition overflowed");
        return sum;
    }
    static std::int64_t negateRaw(std::int64_t raw) {
        ODYSSEUS_ASSERT(raw != INT64_MIN, "Fixed negation overflowed");
        return -raw;
    }

    std::int64_t raw_ = 0;
};

// Handy constants, rounded to the nearest step (pi * 2^32 = 13493037704.52...).
inline constexpr Fixed kFixedZero = Fixed::fromRaw(0);
inline constexpr Fixed kFixedOne = Fixed::fromRaw(Fixed::kOneRaw);
inline constexpr Fixed kFixedHalf = Fixed::fromRaw(Fixed::kOneRaw / 2);
inline constexpr Fixed kPi = Fixed::fromRaw(13'493'037'705);
inline constexpr Fixed kHalfPi = Fixed::fromRaw(6'746'518'852);
inline constexpr Fixed kTwoPi = Fixed::fromRaw(26'986'075'409);

Fixed abs(Fixed value);
Fixed min(Fixed a, Fixed b);
Fixed max(Fixed a, Fixed b);
Fixed clamp(Fixed value, Fixed low, Fixed high);

// Square root, exact to the last bit (rounded down). The value must not be negative.
Fixed sqrt(Fixed value);

// Sine and cosine of an angle in radians, accurate to about 1e-9.
Fixed sin(Fixed radians);
Fixed cos(Fixed radians);

// The angle of the point (x, y) seen from the origin, in radians from -pi to pi
// (atan2(1, 1) is pi/4). Accurate to about 1e-9. atan2(0, 0) is 0.
Fixed atan2(Fixed y, Fixed x);

// An angle given in whole degrees, in radians: degrees(90) is pi/2.
Fixed degrees(std::int64_t wholeDegrees);

} // namespace luna::physics
