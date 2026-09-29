#include "luna/physics/fixed.h"

namespace luna::physics {

namespace {

// A 128-bit unsigned number as two 64-bit halves. We build it ourselves instead of using a
// compiler's 128-bit type because those differ between compilers (MSVC has none), and
// every platform must produce the same bits.
struct Wide {
    std::uint64_t high = 0;
    std::uint64_t low = 0;
};

constexpr std::uint64_t kLow32 = 0xFFFF'FFFFULL;
constexpr std::uint64_t kSignBit = std::uint64_t{1} << 63;

bool lessOrEqual(Wide a, Wide b) {
    return a.high < b.high || (a.high == b.high && a.low <= b.low);
}

// Schoolbook multiplication with 32-bit "digits": (aH*2^32 + aL) * (bH*2^32 + bL).
// Each partial product of two 32-bit digits fits in 64 bits.
Wide multiplyWide(std::uint64_t a, std::uint64_t b) {
    const std::uint64_t aLow = a & kLow32;
    const std::uint64_t aHigh = a >> 32;
    const std::uint64_t bLow = b & kLow32;
    const std::uint64_t bHigh = b >> 32;

    const std::uint64_t lowLow = aLow * bLow;
    const std::uint64_t lowHigh = aLow * bHigh;
    const std::uint64_t highLow = aHigh * bLow;
    const std::uint64_t highHigh = aHigh * bHigh;

    // The middle column collects three 32-bit pieces; it cannot overflow 64 bits.
    const std::uint64_t middle = (lowLow >> 32) + (lowHigh & kLow32) + (highLow & kLow32);
    Wide product;
    product.low = (middle << 32) | (lowLow & kLow32);
    product.high = highHigh + (lowHigh >> 32) + (highLow >> 32) + (middle >> 32);
    return product;
}

// The size of a raw value without its sign. Works even for the most negative number.
std::uint64_t magnitude(std::int64_t raw) {
    return raw < 0 ? 0 - static_cast<std::uint64_t>(raw) : static_cast<std::uint64_t>(raw);
}

// Puts the sign back. The magnitude must fit: up to 2^63 - 1, or 2^63 when negative.
std::int64_t applySign(std::uint64_t size, bool negative) {
    ODYSSEUS_ASSERT(size < kSignBit || (negative && size == kSignBit), "Fixed result overflowed");
    return negative ? static_cast<std::int64_t>(0 - size) : static_cast<std::int64_t>(size);
}

// (numerator * 2^shift) / denominator on magnitudes, rounded to nearest (halves up).
// Long division exactly as on paper, one binary digit at a time.
std::uint64_t divideShifted(std::uint64_t numerator, int shift, std::uint64_t denominator) {
    ODYSSEUS_ASSERT(denominator != 0, "Fixed division by zero");
    Wide dividend;
    if (shift == 0) {
        dividend.low = numerator;
    } else {
        dividend.high = numerator >> (64 - shift);
        dividend.low = numerator << shift;
    }

    Wide quotient;
    std::uint64_t remainder = 0;
    for (int bit = 127; bit >= 0; --bit) {
        const std::uint64_t nextBit =
            bit >= 64 ? (dividend.high >> (bit - 64)) & 1 : (dividend.low >> bit) & 1;
        // The remainder briefly needs 65 bits; the carry remembers the lost top bit.
        const bool carry = (remainder & kSignBit) != 0;
        remainder = (remainder << 1) | nextBit;
        if (carry || remainder >= denominator) {
            remainder -= denominator;
            if (bit >= 64) {
                quotient.high |= std::uint64_t{1} << (bit - 64);
            } else {
                quotient.low |= std::uint64_t{1} << bit;
            }
        }
    }
    // Round to nearest: add one when the remainder is at least half the denominator.
    if (remainder >= denominator - remainder) {
        ++quotient.low;
        if (quotient.low == 0) {
            ++quotient.high;
        }
    }
    ODYSSEUS_ASSERT(quotient.high == 0, "Fixed division overflowed");
    return quotient.low;
}

} // namespace

Fixed Fixed::fromRatio(std::int64_t numerator, std::int64_t denominator) {
    // raw = numerator * 2^32 / denominator: the same long division as operator/.
    const bool negative = (numerator < 0) != (denominator < 0);
    return fromRaw(applySign(divideShifted(magnitude(numerator), kFractionBits, magnitude(denominator)), negative));
}

std::int64_t Fixed::roundToInt() const {
    const std::uint64_t size = magnitude(raw_);
    const std::uint64_t whole = (size >> kFractionBits) + (((size >> (kFractionBits - 1)) & 1) != 0 ? 1 : 0);
    return applySign(whole, raw_ < 0);
}

Fixed operator*(Fixed a, Fixed b) {
    // (a * 2^32) * (b * 2^32) = a*b * 2^64, so the product is shifted back by 32 bits.
    const bool negative = (a.raw_ < 0) != (b.raw_ < 0);
    Wide product = multiplyWide(magnitude(a.raw_), magnitude(b.raw_));
    // Round to nearest: add half of the bits we are about to drop.
    const std::uint64_t half = std::uint64_t{1} << (Fixed::kFractionBits - 1);
    product.low += half;
    if (product.low < half) {
        ++product.high;
    }
    ODYSSEUS_ASSERT((product.high >> 32) == 0, "Fixed multiplication overflowed");
    const std::uint64_t size = (product.high << 32) | (product.low >> 32);
    return Fixed::fromRaw(applySign(size, negative));
}

Fixed operator/(Fixed a, Fixed b) {
    // (a * 2^32) / (b * 2^32) loses the scale, so the dividend is shifted up by 32 first.
    const bool negative = (a.raw_ < 0) != (b.raw_ < 0);
    return Fixed::fromRaw(applySign(divideShifted(magnitude(a.raw_), Fixed::kFractionBits, magnitude(b.raw_)), negative));
}

Fixed operator*(Fixed a, std::int64_t whole) {
    const bool negative = (a.raw_ < 0) != (whole < 0);
    const Wide product = multiplyWide(magnitude(a.raw_), magnitude(whole));
    ODYSSEUS_ASSERT(product.high == 0, "Fixed multiplication overflowed");
    return Fixed::fromRaw(applySign(product.low, negative));
}

Fixed operator/(Fixed a, std::int64_t whole) {
    const bool negative = (a.raw_ < 0) != (whole < 0);
    return Fixed::fromRaw(applySign(divideShifted(magnitude(a.raw_), 0, magnitude(whole)), negative));
}

Fixed abs(Fixed value) {
    return value < kFixedZero ? -value : value;
}

Fixed min(Fixed a, Fixed b) {
    return b < a ? b : a;
}

Fixed max(Fixed a, Fixed b) {
    return a < b ? b : a;
}

Fixed clamp(Fixed value, Fixed low, Fixed high) {
    return min(max(value, low), high);
}

Fixed sqrt(Fixed value) {
    ODYSSEUS_ASSERT(value >= kFixedZero, "square root of a negative number");
    // sqrt(raw / 2^32) = sqrt(raw * 2^32) / 2^32, so we need the integer square root of the
    // 96-bit number raw * 2^32. The answer is below 2^48; we decide its bits from the top,
    // keeping each bit whose square still fits (like long division, for square roots).
    const std::uint64_t raw = static_cast<std::uint64_t>(value.raw());
    const Wide target{raw >> 32, raw << 32};
    std::uint64_t root = 0;
    for (int bit = 47; bit >= 0; --bit) {
        const std::uint64_t candidate = root | (std::uint64_t{1} << bit);
        if (lessOrEqual(multiplyWide(candidate, candidate), target)) {
            root = candidate;
        }
    }
    return Fixed::fromRaw(static_cast<std::int64_t>(root));
}

namespace {

// Taylor series around 0, good for |x| <= pi/4 where the terms shrink fast:
// sin x = x - x^3/3! + x^5/5! - ...   cos x = 1 - x^2/2! + x^4/4! - ...
// Seven terms leave an error far below one Fixed step's worth of 1e-9 accuracy.
Fixed sinSmall(Fixed x) {
    const Fixed xSquared = x * x;
    Fixed term = x;
    Fixed sum = x;
    for (std::int64_t n = 1; n <= 7; ++n) {
        term = -(term * xSquared) / ((2 * n) * (2 * n + 1));
        sum += term;
    }
    return sum;
}

Fixed cosSmall(Fixed x) {
    const Fixed xSquared = x * x;
    Fixed term = kFixedOne;
    Fixed sum = kFixedOne;
    for (std::int64_t n = 1; n <= 7; ++n) {
        term = -(term * xSquared) / ((2 * n - 1) * (2 * n));
        sum += term;
    }
    return sum;
}

// sin for 0 <= x <= pi/2: the series near 0, or cos of the rest near pi/2.
Fixed sinQuarter(Fixed x) {
    const Fixed quarterPi = kHalfPi / 2;
    return x <= quarterPi ? sinSmall(x) : cosSmall(kHalfPi - x);
}

} // namespace

Fixed sin(Fixed radians) {
    // Fold any angle into [0, 2pi), then into a quarter turn plus the quadrant it came from.
    std::int64_t turn = radians.raw() % kTwoPi.raw();
    if (turn < 0) {
        turn += kTwoPi.raw();
    }
    const std::int64_t quadrant = turn / kHalfPi.raw();
    const Fixed rest = Fixed::fromRaw(turn - quadrant * kHalfPi.raw());
    switch (quadrant) {
    case 0:
        return sinQuarter(rest);
    case 1:
        return sinQuarter(kHalfPi - rest);
    case 2:
        return -sinQuarter(rest);
    case 3:
        return -sinQuarter(kHalfPi - rest);
    default:
        // 4 x (rounded pi/2) is one step short of (rounded 2pi): that last step is a full
        // turn, where the sine is 0 again.
        return sinQuarter(rest);
    }
}

Fixed cos(Fixed radians) {
    // cos x = sin(x + pi/2). Folding first keeps the addition far from overflow.
    std::int64_t turn = radians.raw() % kTwoPi.raw();
    return sin(Fixed::fromRaw(turn) + kHalfPi);
}

Fixed degrees(std::int64_t wholeDegrees) {
    return kPi * wholeDegrees / 180;
}

} // namespace luna::physics
