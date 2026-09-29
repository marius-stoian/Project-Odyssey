#pragma once

#include "boundary.h"

namespace luna::engine {

// What a game gives Luna. Luna owns the window and the loop and calls these; the game
// never sees SDL. An interface: "virtual ... = 0" means every game must provide it.
class Game {
public:
    virtual ~Game() = default;

    // Called at a fixed rate (20 times per second): change the world here.
    virtual void update() = 0;

    // Called once per frame. `alpha` (0..1) says how far we are towards the next tick,
    // so movement can be drawn smoothly between ticks.
    virtual void render(double alpha) = 0;
};

} // namespace luna::engine
