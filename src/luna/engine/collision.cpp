#include "luna/engine/collision.h"

#include <cmath>

namespace luna::engine {

namespace {

// Tiny inset so a box resting exactly on a tile edge does not count as inside that tile.
constexpr double kEdge = 1e-6;

int tileOf(double worldPixel, int tileSize) {
    return static_cast<int>(std::floor(worldPixel / tileSize));
}

// Is any tile solid in columns [firstX, lastX] of rows [firstY, lastY]?
bool anySolid(const TileMap& map, int firstX, int lastX, int firstY, int lastY) {
    for (int y = firstY; y <= lastY; ++y) {
        for (int x = firstX; x <= lastX; ++x) {
            if (map.isSolid(x, y)) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

Box moveAndCollide(const TileMap& map, Box box, double dx, double dy) {
    const int size = map.tileSize();

    if (dx != 0.0) {
        const int firstRow = tileOf(box.y + kEdge, size);
        const int lastRow = tileOf(box.y + box.height - kEdge, size);
        if (dx > 0.0) {
            // Walk column by column from the box's right edge to where it wants to go.
            const int from = tileOf(box.x + box.width - kEdge, size) + 1;
            const int to = tileOf(box.x + box.width + dx - kEdge, size);
            double x = box.x + dx;
            for (int column = from; column <= to; ++column) {
                if (anySolid(map, column, column, firstRow, lastRow)) {
                    x = column * size - box.width; // flush against the tile's left edge
                    break;
                }
            }
            box.x = x;
        } else {
            const int from = tileOf(box.x + kEdge, size) - 1;
            const int to = tileOf(box.x + dx + kEdge, size);
            double x = box.x + dx;
            for (int column = from; column >= to; --column) {
                if (anySolid(map, column, column, firstRow, lastRow)) {
                    x = (column + 1) * size; // flush against the tile's right edge
                    break;
                }
            }
            box.x = x;
        }
    }

    if (dy != 0.0) {
        const int firstColumn = tileOf(box.x + kEdge, size);
        const int lastColumn = tileOf(box.x + box.width - kEdge, size);
        if (dy > 0.0) {
            const int from = tileOf(box.y + box.height - kEdge, size) + 1;
            const int to = tileOf(box.y + box.height + dy - kEdge, size);
            double y = box.y + dy;
            for (int row = from; row <= to; ++row) {
                if (anySolid(map, firstColumn, lastColumn, row, row)) {
                    y = row * size - box.height;
                    break;
                }
            }
            box.y = y;
        } else {
            const int from = tileOf(box.y + kEdge, size) - 1;
            const int to = tileOf(box.y + dy + kEdge, size);
            double y = box.y + dy;
            for (int row = from; row >= to; --row) {
                if (anySolid(map, firstColumn, lastColumn, row, row)) {
                    y = (row + 1) * size;
                    break;
                }
            }
            box.y = y;
        }
    }
    return box;
}

} // namespace luna::engine
