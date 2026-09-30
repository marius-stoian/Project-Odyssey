#pragma once

#include "boundary.h"

#include "game/catalogs.h"

namespace odysseus::game {

// What an element leaves on a character after a hit (US-135): burning and poison take HP
// every tick, ice slows. A small struct held by the character, counted down by the game tick
// (20 per second). A new hit of the same element starts its timer again (D-24); it never stacks.
struct StatusEffects {
    static constexpr int kTicksPerSecond = 20;

    struct Drip {
        double perSecond = 0.0;
        int ticksLeft = 0;
        double carry = 0.0; // the part of an HP not yet lost
        bool active() const { return ticksLeft > 0; }
    };
    Drip burn;
    Drip poison;
    double slowFactor = 1.0;
    int slowTicksLeft = 0;

    // Element::Fire, Poison and Ice leave a status; the others leave nothing on the target.
    void apply(Element element, const ElementDef& numbers);
    // One tick: counts everything down and returns the whole HP lost this tick.
    int tick();

    bool burning() const { return burn.active(); }
    bool poisoned() const { return poison.active(); }
    bool slowed() const { return slowTicksLeft > 0; }
    bool any() const { return burning() || poisoned() || slowed(); }
    // How fast the character acts: 1 normally, the slow factor while slowed.
    double speed() const { return slowed() ? slowFactor : 1.0; }
};

} // namespace odysseus::game
