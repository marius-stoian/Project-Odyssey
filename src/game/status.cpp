#include "game/status.h"

#include <cmath>

namespace odysseus::game {

namespace {

int ticksFor(double seconds) { return static_cast<int>(std::lround(seconds * StatusEffects::kTicksPerSecond)); }

int dripTick(StatusEffects::Drip& drip) {
    if (!drip.active()) {
        return 0;
    }
    drip.carry += drip.perSecond / StatusEffects::kTicksPerSecond;
    --drip.ticksLeft;
    const int whole = static_cast<int>(std::floor(drip.carry + 1e-9));
    drip.carry -= whole;
    if (!drip.active()) {
        drip.carry = 0.0;
    }
    return whole;
}

} // namespace

void StatusEffects::apply(Element element, const ElementDef& numbers) {
    switch (element) {
    case Element::Fire:
        burn = Drip{numbers.perSecond, ticksFor(numbers.seconds), 0.0};
        break;
    case Element::Poison:
        poison = Drip{numbers.perSecond, ticksFor(numbers.seconds), 0.0};
        break;
    case Element::Ice:
        slowFactor = numbers.slowTo;
        slowTicksLeft = ticksFor(numbers.seconds);
        break;
    default:
        break;
    }
}

int StatusEffects::tick() {
    int lost = dripTick(burn) + dripTick(poison);
    if (slowTicksLeft > 0 && --slowTicksLeft == 0) {
        slowFactor = 1.0;
    }
    return lost;
}

} // namespace odysseus::game
