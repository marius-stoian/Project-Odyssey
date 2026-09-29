#include "luna/engine/collision.h"

#include <doctest/doctest.h>

using luna::engine::Box;
using luna::engine::moveAndCollide;
using luna::engine::TileMap;

TEST_CASE("Collision stops boxes flush against solid tiles") {
    TileMap map(10, 10, 32, 0);
    map.setSolid(1, true);
    map.set(5, 3, 1); // a rock at column 5, row 3: x 160..192, y 96..128
    const Box feet{100.0, 110.0, 20.0, 10.0};

    SUBCASE("moving right into the rock stops at its left edge") {
        const Box moved = moveAndCollide(map, feet, 200.0, 0.0);
        CHECK(moved.x + moved.width == doctest::Approx(160.0));
        CHECK(moved.y == doctest::Approx(110.0));
    }
    SUBCASE("free movement is exact") {
        const Box moved = moveAndCollide(map, feet, 4.8, 0.0);
        CHECK(moved.x == doctest::Approx(104.8));
    }
    SUBCASE("moving left from the right side stops at its right edge") {
        const Box right{200.0, 110.0, 20.0, 10.0};
        const Box moved = moveAndCollide(map, right, -100.0, 0.0);
        CHECK(moved.x == doctest::Approx(192.0));
    }
    SUBCASE("rows the rock does not reach are free") {
        const Box above{100.0, 80.0, 20.0, 10.0}; // y 80..90, above the rock
        const Box moved = moveAndCollide(map, above, 200.0, 0.0);
        CHECK(moved.x == doctest::Approx(300.0));
    }
    SUBCASE("the map edge is a wall") {
        const Box moved = moveAndCollide(map, feet, 0.0, 10000.0);
        CHECK(moved.y + moved.height == doctest::Approx(320.0));
        const Box left = moveAndCollide(map, feet, -10000.0, 0.0);
        CHECK(left.x == doctest::Approx(0.0));
    }
    SUBCASE("diagonal into a wall slides along it") {
        const Box atWall{140.0, 110.0, 20.0, 10.0}; // touching the rock's left edge
        const Box moved = moveAndCollide(map, atWall, 5.0, 5.0);
        CHECK(moved.x == doctest::Approx(140.0)); // blocked sideways
        CHECK(moved.y == doctest::Approx(115.0)); // still slides down
    }
}
