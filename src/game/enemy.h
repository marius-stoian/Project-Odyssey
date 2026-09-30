#pragma once

#include "boundary.h"

#include "game/placeholder_art.h"

#include <string>

namespace odysseus::game {

// A character placed in the level whom the sword can hit (M2c: they stand still, D-19).
// Position is in world pixels, like the hero's.
class Enemy {
public:
    Enemy(double feetX, double feetY, int maxHp);

    // Returns true when this hit brings HP to zero.
    bool takeDamage(int damage);

    // Hit and still standing: it winds up to strike back (US-131), unless it already is.
    void provoke();
    // One simulation tick: counts down the red hit flash and the wind-up. True on the tick the
    // wind-up ends: the strike lands now, if the hero is still within reach.
    bool update();

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
    Facing facing = Facing::South;
    int swordDamage = 5;          // what its own strike does to the hero
    double reachMetres = 1.5;     // how far its strike reaches (D-21)

private:
    double feetX_;
    double feetY_;
    int hp_;
    int maxHp_;
    int damageFlashTicks_ = 0;
    enum class Strike { Idle, WindUp };
    Strike state_ = Strike::Idle;
    int windUpTicks_ = 0;
};

} // namespace odysseus::game
