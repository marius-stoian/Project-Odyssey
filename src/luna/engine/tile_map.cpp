#include "luna/engine/tile_map.h"

#include "core/assertions.h"
#include "luna/engine/camera.h"

#include <algorithm>
#include <cstddef>

namespace luna::engine {

namespace {

// Rounds towards minus infinity: -1 / 32 must be -1, not 0 as C++ integer division gives.
int floorDivide(int value, int divisor) {
    return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
}

} // namespace

TileMap::TileMap(int width, int height, int tileSize, int fillTile)
    : width_(width), height_(height), tileSize_(tileSize),
      tiles_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), fillTile) {
    ODYSSEUS_ASSERT(width > 0 && height > 0 && tileSize > 0, "a map needs a size");
}

int TileMap::at(int x, int y) const {
    ODYSSEUS_ASSERT(inside(x, y), "tile outside the map");
    return tiles_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)];
}

void TileMap::set(int x, int y, int tile) {
    if (inside(x, y)) {
        tiles_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)] = tile;
    }
}

void TileMap::setSolid(int tile, bool solid) {
    if (tile >= static_cast<int>(solidTiles_.size())) {
        solidTiles_.resize(static_cast<std::size_t>(tile) + 1, false);
    }
    solidTiles_[static_cast<std::size_t>(tile)] = solid;
}

bool TileMap::isSolid(int x, int y) const {
    if (!inside(x, y)) {
        return true; // the edge of the world is a wall
    }
    const int tile = at(x, y);
    return tile < static_cast<int>(solidTiles_.size()) && solidTiles_[static_cast<std::size_t>(tile)];
}

TileRange TileMap::visibleTiles(const Rect& view) const {
    TileRange range;
    range.firstX = std::max(0, floorDivide(view.x, tileSize_));
    range.firstY = std::max(0, floorDivide(view.y, tileSize_));
    range.lastX = std::min(width_ - 1, floorDivide(view.x + view.width - 1, tileSize_));
    range.lastY = std::min(height_ - 1, floorDivide(view.y + view.height - 1, tileSize_));
    return range;
}

void TileMap::draw(Renderer& renderer, const Texture& tileSheet, const Camera& camera, double alpha) const {
    const Rect view = camera.view(alpha);
    const TileRange range = visibleTiles(view);
    for (int y = range.firstY; y <= range.lastY; ++y) {
        for (int x = range.firstX; x <= range.lastX; ++x) {
            const Rect source{at(x, y) * tileSize_, 0, tileSize_, tileSize_};
            renderer.draw(tileSheet, source, {x * tileSize_ - view.x, y * tileSize_ - view.y});
        }
    }
}

} // namespace luna::engine
