#include "luna/engine/chunk_streamer.h"

#include <algorithm>
#include <cstdlib>

namespace luna::engine {

ChunkStreamer::ChunkStreamer(int chunkPixels, int chunksPerSide, int marginChunks)
    : chunkPixels_(std::max(1, chunkPixels)), chunksPerSide_(std::max(1, chunksPerSide)), margin_(std::max(0, marginChunks)) {}

bool ChunkStreamer::covers(const odysseus::core::Rect& view) const {
    const int firstX = std::max(0, view.x / chunkPixels_);
    const int firstY = std::max(0, view.y / chunkPixels_);
    const int lastX = std::min(chunksPerSide_ - 1, (view.x + view.width - 1) / chunkPixels_);
    const int lastY = std::min(chunksPerSide_ - 1, (view.y + view.height - 1) / chunkPixels_);
    for (int cy = firstY; cy <= lastY; ++cy)
        for (int cx = firstX; cx <= lastX; ++cx)
            if (!loaded(cx, cy)) return false;
    return true;
}

int ChunkStreamer::update(const odysseus::core::Rect& view, const Action& load, const Action& unload, int maxLoads) {
    // The chunks to keep: the view's, widened by the margin.
    const int firstX = std::max(0, view.x / chunkPixels_ - margin_);
    const int firstY = std::max(0, view.y / chunkPixels_ - margin_);
    const int lastX = std::min(chunksPerSide_ - 1, (view.x + view.width - 1) / chunkPixels_ + margin_);
    const int lastY = std::min(chunksPerSide_ - 1, (view.y + view.height - 1) / chunkPixels_ + margin_);
    const int viewFirstX = std::max(0, view.x / chunkPixels_);
    const int viewFirstY = std::max(0, view.y / chunkPixels_);
    const int viewLastX = std::min(chunksPerSide_ - 1, (view.x + view.width - 1) / chunkPixels_);
    const int viewLastY = std::min(chunksPerSide_ - 1, (view.y + view.height - 1) / chunkPixels_);

    // Wanted and not yet loaded; those in view first, then by distance from the view's middle.
    struct Want {
        int cx;
        int cy;
        bool inView;
        int distance;
    };
    std::vector<Want> wanted;
    const int middleX = (view.x + view.width / 2) / chunkPixels_;
    const int middleY = (view.y + view.height / 2) / chunkPixels_;
    for (int cy = firstY; cy <= lastY; ++cy) {
        for (int cx = firstX; cx <= lastX; ++cx) {
            if (loaded(cx, cy)) continue;
            const bool inView = cx >= viewFirstX && cx <= viewLastX && cy >= viewFirstY && cy <= viewLastY;
            wanted.push_back({cx, cy, inView, std::abs(cx - middleX) + std::abs(cy - middleY)});
        }
    }
    std::sort(wanted.begin(), wanted.end(), [](const Want& a, const Want& b) {
        if (a.inView != b.inView) return a.inView;
        if (a.distance != b.distance) return a.distance < b.distance;
        return a.cy != b.cy ? a.cy < b.cy : a.cx < b.cx;
    });
    int loadedNow = 0;
    for (const Want& want : wanted) {
        // What is in view is loaded at once, whatever the budget: popping in is worse than one slow tick.
        if (!want.inView && loadedNow >= maxLoads) break;
        load(want.cx, want.cy);
        loaded_.insert({want.cx, want.cy});
        ++loadedNow;
    }
    // Let go of what is well behind.
    std::vector<std::pair<int, int>> far;
    for (const auto& [cx, cy] : loaded_) {
        if (cx < firstX - 1 || cx > lastX + 1 || cy < firstY - 1 || cy > lastY + 1) far.push_back({cx, cy});
    }
    for (const auto& [cx, cy] : far) {
        unload(cx, cy);
        loaded_.erase({cx, cy});
    }
    return loadedNow;
}

} // namespace luna::engine
