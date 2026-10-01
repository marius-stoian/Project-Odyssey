#pragma once

#include "boundary.h"

#include "renderer.h"

#include <vector>

namespace luna::engine {

class Camera;

// Which tiles overlap a view: columns firstX..lastX and rows firstY..lastY, inclusive.
// Empty when lastX < firstX.
struct TileRange {
    int firstX = 0;
    int firstY = 0;
    int lastX = -1;
    int lastY = -1;

    int count() const { return lastX < firstX || lastY < firstY ? 0 : (lastX - firstX + 1) * (lastY - firstY + 1); }
};

// A rectangular world of square tiles. Each cell holds a tile number: the column of that
// tile in the tile sheet. The grid is stored row after row in ONE vector: cell (x, y)
// lives at index y * width + x. One allocation, and neighbours are close in memory.
class TileMap {
public:
    TileMap(int width, int height, int tileSize, int fillTile = 0);

    int width() const { return width_; }
    int height() const { return height_; }
    int tileSize() const { return tileSize_; }
    int pixelWidth() const { return width_ * tileSize_; }
    int pixelHeight() const { return height_ * tileSize_; }

    bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < width_ && y < height_; }
    int at(int x, int y) const;
    void set(int x, int y, int tile);

    // Solid tiles block walking (rocks, water). Outside the map counts as solid.
    void setSolid(int tile, bool solid);
    bool isSolid(int x, int y) const;

    // Things standing on a cell that block it like a solid tile (a tree, US-136), and how tall they are in
    // metres (0: nothing). The Game sets and clears them as things appear and go; they are not tiles.
    void setObstacle(int x, int y, double heightMetres);
    double obstacleHeight(int x, int y) const;

    // The tiles a view (in world pixels) overlaps, clamped to the map.
    TileRange visibleTiles(const Rect& view) const;

    // Draws only the tiles the camera sees; everything else is skipped.
    void draw(Renderer& renderer, const Texture& tileSheet, const Camera& camera, double alpha) const;

private:
    int width_;
    int height_;
    int tileSize_;
    std::vector<int> tiles_;       // width_ * height_ cells
    std::vector<bool> solidTiles_; // index = tile number
    std::vector<float> obstacles_; // height of the thing standing on each cell; empty until one is set
};

} // namespace luna::engine
