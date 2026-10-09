#include "region_shapes.h"

#include <algorithm>
#include <cstdlib>
#include <set>

namespace odysseus::sim {

namespace {

std::vector<Tile> inTileOrder(const std::set<std::pair<int, int>>& tiles) { // keyed (y, x)
    std::vector<Tile> ordered;
    ordered.reserve(tiles.size());
    for (const auto& [y, x] : tiles) ordered.push_back({x, y});
    return ordered;
}

} // namespace

std::vector<Tile> line4(Tile from, Tile to) {
    if (to.y < from.y || (to.y == from.y && to.x < from.x)) { // always drawn from the earlier tile, so a river is the same whichever way it is dragged
        std::vector<Tile> backwards = line4(to, from);
        std::reverse(backwards.begin(), backwards.end());
        return backwards;
    }
    std::vector<Tile> tiles{from};
    const int dx = std::abs(to.x - from.x);
    const int dy = std::abs(to.y - from.y);
    const int sx = from.x < to.x ? 1 : -1;
    const int sy = from.y < to.y ? 1 : -1;
    int err = dx - dy;
    int x = from.x;
    int y = from.y;
    while (x != to.x || y != to.y) {
        const int doubled = 2 * err;
        const bool stepX = doubled > -dy;
        const bool stepY = doubled < dx;
        if (stepX) {
            err -= dy;
            x += sx;
        }
        if (stepY) {
            err += dx;
            y += sy;
        }
        if (stepX && stepY) tiles.push_back({x, y - sy}); // the corner between the two tiles keeps the line four-connected
        tiles.push_back({x, y});
    }
    return tiles;
}

std::vector<Tile> thicken(const std::vector<Tile>& centre, int width, int regionSize) {
    const int radiusSquared = width * width / 4;
    const int reach = (width + 1) / 2;
    std::set<std::pair<int, int>> found;
    for (const Tile& at : centre) {
        for (int dy = -reach; dy <= reach; ++dy) {
            for (int dx = -reach; dx <= reach; ++dx) {
                const int x = at.x + dx;
                const int y = at.y + dy;
                if (dx * dx + dy * dy <= radiusSquared && x >= 0 && y >= 0 && x < regionSize && y < regionSize) found.insert({y, x});
            }
        }
    }
    return inTileOrder(found);
}

std::vector<Tile> disc(Tile centre, int radius, int regionSize) {
    std::set<std::pair<int, int>> found;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            const int x = centre.x + dx;
            const int y = centre.y + dy;
            if (dx * dx + dy * dy <= radius * radius && x >= 0 && y >= 0 && x < regionSize && y < regionSize) found.insert({y, x});
        }
    }
    return inTileOrder(found);
}

} // namespace odysseus::sim
