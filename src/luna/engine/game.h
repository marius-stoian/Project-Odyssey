#pragma once

#include "boundary.h"

#include "input.h"
#include "renderer.h"

#include <optional>

namespace luna::engine {

// What a game gives Luna. Luna owns the window and the loop and calls these; the game
// never sees SDL. An interface: "virtual ... = 0" means every game must provide it.
class Game {
public:
    virtual ~Game() = default;

    // Called once before the first frame: create textures here.
    virtual void start(Renderer& /*renderer*/) {}

    // Called at a fixed rate (20 times per second): change the world here. `intents` say
    // what the player wants (move, interact, menu); games never see keys or buttons.
    virtual void update(const Intents& intents) = 0;

    // Called once per frame. `alpha` (0..1) says how far we are towards the next tick,
    // so movement can be drawn smoothly between ticks.
    virtual void render(Renderer& renderer, double alpha) = 0;

    // A change of the window the game asks for (US-081): full screen or windowed, and the size of the window. Luna polls this
    // after every tick and applies what it finds once.
    struct WindowChange {
        bool fullscreen = false;
        int width = 0;   // 0: keep the size
        int height = 0;
    };
    virtual std::optional<WindowChange> takeWindowChange() { return std::nullopt; }
};

} // namespace luna::engine
