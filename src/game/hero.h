#pragma once

#include "boundary.h"

#include "game/placeholder_art.h"
#include "luna/engine/collision.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"

namespace odysseus::game {

// Tuning for the hero. Change these to change how walking feels.
struct HeroConfig {
    double speedPixelsPerSecond = 96.0; // 3 tiles per second
    int ticksPerSecond = 20;            // ADR-006
    int animationFramesPerSecond = 8;   // walking cycle speed
    double feetWidth = 20.0;            // the part that collides: the feet, not the head,
    double feetHeight = 10.0;           // so the hero can stand "in front of" a rock
};

// The player's character in the M1 walking skeleton (US-024). Moves in 8 directions (D-17)
// at a fixed speed, collides with solid tiles, animates while walking, and faces the last
// direction it moved when it stops.
class Hero {
public:
    Hero(double feetX, double feetY, HeroConfig config = {});

    // One fixed tick: read the movement intents and move through the map.
    void update(const luna::engine::Intents& intents, const luna::engine::TileMap& map);

    // Where the feet are (bottom centre), blended between the last two ticks.
    double feetX(double alpha = 1.0) const;
    double feetY(double alpha = 1.0) const;

    Facing facing() const { return facing_; }
    bool walking() const { return walking_; }
    int animationFrame() const; // 0..3 while walking, 0 when idle

    // The feet box used for collisions, in world pixels.
    luna::engine::Box feetBox() const;

    // The part of the character sheet to draw now.
    luna::engine::Rect spriteFrame() const;

private:
    HeroConfig config_;
    double x_;
    double y_;
    double previousX_;
    double previousY_;
    Facing facing_ = Facing::South;
    bool walking_ = false;
    int walkTicks_ = 0;
};

// "East", "SouthWest", ... for logs.
const char* facingName(Facing facing);

// The facing for a movement direction (x, y each -1, 0 or +1); `current` if not moving.
Facing facingFor(int moveX, int moveY, Facing current);

} // namespace odysseus::game
