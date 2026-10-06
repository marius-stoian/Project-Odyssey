#pragma once

#include "boundary.h"

#include "game/placeholder_art.h"
#include "luna/engine/collision.h"
#include "luna/engine/tile_map.h"

namespace odysseus::game {

// Tuning for the sword slash (M3 melee combat).
struct SwordConfig {
    double slashDurationTicks = 10.0;  // how long a slash animation lasts
    double slashRangeMetres = 1.5;     // reach of a sword slash
    int slashCooldownTicks = 15;       // ticks between allowed slashes
    int damagePerHit = 5;
};

enum class SlashState { Idle, Slashing, Cooldown };

struct SwordSlash {
    SlashState state = SlashState::Idle;
    int slashTicks = 0;      // ticks into current slash animation
    int cooldownTicks = 0;   // ticks remaining in cooldown
};

// Melee combat system for the hero's sword (US-025, M3).
class Sword {
public:
    Sword(SwordConfig config = {});

    // Start a slash attack; the game hits the nearest enemy in range (any direction).
    void slash();

    // One simulation tick: update slash animation and cooldown.
    void update();

    const SwordSlash& state() const { return state_; }
    SwordConfig config() const { return config_; }

    // Animation frame for the current state: 0-3 for slash, 0 for idle/cooldown.
    int animationFrame() const;

    // Is the sword currently attacking (slashing)?
    bool isAttacking() const { return state_.state == SlashState::Slashing; }

    // Has the sword already hit in this slash? (to prevent hitting multiple times per swing)
    bool hasHitInThisSlash() const { return hasHitInThisSlash_; }
    void markHit() { hasHitInThisSlash_ = true; }

private:
    SwordConfig config_;
    SwordSlash state_;
    bool hasHitInThisSlash_ = false;
};

} // namespace odysseus::game
