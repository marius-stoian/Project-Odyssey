#pragma once

#include "boundary.h"

#include "game/placeholder_art.h"
#include "game/status.h"

#include <string>

namespace odysseus::game {

// A character placed in the level whom the sword can hit (M2c: they stand still, D-19).
// Position is in world pixels, like the hero's.
class Enemy {
public:
    Enemy(double feetX, double feetY, int maxHp);

    // Returns true when this hit brings HP to zero.
    // `flash`: the red hit flash (burning and poison do not flash on every tick).
    bool takeDamage(int damage, bool flash = true);

    // Hit and still standing: it winds up to strike back (US-131), unless it already is.
    void provoke();
    // One simulation tick: counts down the red hit flash and the wind-up. True on the tick the
    // wind-up ends: the strike lands now, if the hero is still within reach.
    bool update();

    // An animal that walks (US-154): the game moves it, one step at a time, after checking the ground.
    void setFeet(double x, double y) { feetX_ = x; feetY_ = y; }

    double feetX() const { return feetX_; }
    double feetY() const { return feetY_; }
    int hp() const { return hp_; }
    int maxHp() const { return maxHp_; }
    bool isAlive() const { return hp_ > 0; }
    bool isFlashing() const { return damageFlashTicks_ > 0; }
    // The telegraph: winding up to strike, so the hero can step away.
    bool isWindingUp() const { return state_ == Strike::WindUp; }

    // Who it is, from the level (US-122).
    int id = 0;
    std::string name = "Enemy";
    std::string frames = "goblin"; // its art: an atlas frame, or a frame set with 8 directions
    int directions = 1;
    std::string kindName;          // the kind in characters.json / animals.json (an animal's picture is found by it)
    bool animal = false;           // drawn from the animal art (a side view) instead of the character sheet (US-137)
    Facing facing = Facing::South;
    int swordDamage = 5;          // what its own strike does to the hero
    double reachMetres = 1.5;     // how far its strike reaches (D-21)
    StatusEffects status;         // burning, poison and slow left by elements (US-135); a slowed enemy winds up slower

private:
    double feetX_;
    double feetY_;
    int hp_;
    int maxHp_;
    int damageFlashTicks_ = 0;
    enum class Strike { Idle, WindUp };
    Strike state_ = Strike::Idle;
    double windUpLeft_ = 0.0; // ticks; a slowed enemy loses less than one per tick
};

} // namespace odysseus::game
