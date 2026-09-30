#pragma once

#include "boundary.h"

#include "core/geometry.h"

#include <cstdint>
#include <functional>
#include <set>
#include <utility>
#include <vector>

namespace luna::engine {

// Keeps the chunks near the camera loaded (US-043): a world too big to hold whole is cut into square chunks; each tick the
// streamer loads the ones the view will soon show, a few at a time so no tick stalls, and lets go of the ones far behind.
// It knows nothing of what a chunk holds: the game gives it functions that load and unload one.
class ChunkStreamer {
public:
    using Action = std::function<void(int chunkX, int chunkY)>;

    // `chunkPixels`: the side of a chunk in world pixels; `chunksPerSide`: how many chunks the world has each way;
    // `marginChunks`: how far beyond the view chunks are loaded ahead of need.
    ChunkStreamer(int chunkPixels, int chunksPerSide, int marginChunks = 1);

    // One tick with the view rectangle (world pixels). Chunks the view touches are loaded first, then the margin, nearest to
    // the view's middle first, at most `maxLoads` in all; chunks farther than the margin plus one are unloaded.
    // Returns how many were loaded this call.
    int update(const odysseus::core::Rect& view, const Action& load, const Action& unload, int maxLoads = 2);

    bool loaded(int chunkX, int chunkY) const { return loaded_.contains({chunkX, chunkY}); }
    std::size_t loadedCount() const { return loaded_.size(); }
    // Is every chunk the view touches loaded? (What the player would see as pop-in if not.)
    bool covers(const odysseus::core::Rect& view) const;

private:
    int chunkPixels_;
    int chunksPerSide_;
    int margin_;
    std::set<std::pair<int, int>> loaded_;
};

} // namespace luna::engine
