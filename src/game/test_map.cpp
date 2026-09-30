#include "game/test_map.h"

#include "game/placeholder_art.h"

#include <cstdlib>

namespace odysseus::game {

luna::engine::TileMap makeTestMap() {
    const int grass = static_cast<int>(TileKind::Grass);
    const int path = static_cast<int>(TileKind::Path);
    const int rock = static_cast<int>(TileKind::Rock);
    const int water = static_cast<int>(TileKind::Water);

    luna::engine::TileMap map(kTestMapSize, kTestMapSize, kTileSize, grass);
    map.setSolid(rock, true);
    map.setSolid(water, true);

    const int centre = kTestMapSize / 2;
    for (int i = 0; i < kTestMapSize; ++i) {
        map.set(i, centre, path); // east-west path
        map.set(centre, i, path); // north-south path
    }
    // A pond north-east of the crossing.
    for (int y = 20; y <= 26; ++y) {
        for (int x = 38; x <= 46; ++x) {
            if ((x - 42) * (x - 42) + (y - 23) * (y - 23) * 2 <= 20) {
                map.set(x, y, water);
            }
        }
    }
    // Rocks: a fixed scattering, but never on the paths or near the crossing.
    for (int y = 1; y < kTestMapSize - 1; ++y) {
        for (int x = 1; x < kTestMapSize - 1; ++x) {
            const bool nearCentre = std::abs(x - centre) <= 3 && std::abs(y - centre) <= 3;
            const unsigned hash = static_cast<unsigned>(x * 73856093) ^ static_cast<unsigned>(y * 19349663);
            if (!nearCentre && map.at(x, y) == grass && hash % 23U == 0) {
                map.set(x, y, rock);
            }
        }
    }
    // A boulder on the east path, 4 tiles from the start: walking right runs into it (US-024).
    map.set(centre + 4, centre, rock);
    // A boulder on the north path, right in front of the second straw target (US-029 Blocked).
    map.set(centre, centre - 7, rock);
    return map;
}

} // namespace odysseus::game
