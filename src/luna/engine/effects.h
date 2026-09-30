#pragma once

#include "boundary.h"

#include "luna/engine/renderer.h"

#include <cstddef>
#include <vector>

namespace luna::engine {

// A short animation drawn from a texture (US-132): its frames, how long each shows, and
// whether it starts over. Luna knows nothing of what it shows; the game names and picks them.
struct EffectSpec {
    Texture texture;
    std::vector<Rect> frames;     // regions of the texture, in order
    int ticksPerFrame = 3;
    bool loop = false;            // false: plays once, then is gone
    DrawStyle style{};            // see-through or additive (glows)
};

// Running effects, each centred on a point: in the world (moves with the camera) or on the
// screen. One tick of the fixed step advances them all.
class EffectPlayer {
public:
    // Starts an effect centred on (x, y), `size` pixels across (0: the frame's own size).
    // Returns its handle, to stop a looping one later. Effects without frames are ignored (-1).
    int start(const EffectSpec& spec, double x, double y, int size = 0, bool onScreen = false);
    void stop(int handle);
    void clear() { running_.clear(); }
    // One tick: every effect moves on; finished one-shot effects are removed.
    void update();
    // World effects are placed relative to `view` (the camera's rectangle); screen ones are not.
    void draw(Renderer& renderer, const Rect& view) const;

    std::size_t count() const { return running_.size(); }
    bool isRunning(int handle) const;

private:
    struct Running {
        int handle;
        EffectSpec spec;
        double x;
        double y;
        int size;
        bool onScreen;
        int age = 0; // ticks since it started
    };
    std::vector<Running> running_;
    int nextHandle_ = 1;
};

} // namespace luna::engine
