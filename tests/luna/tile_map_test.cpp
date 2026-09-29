#include "luna/engine/camera.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"

#include <doctest/doctest.h>

using luna::engine::Camera;
using luna::engine::Image;
using luna::engine::RecordingRenderer;
using luna::engine::Rect;
using luna::engine::TileMap;

namespace {
constexpr int kView = 480;
constexpr int kViewH = 270;
constexpr int kTile = 32;
} // namespace

TEST_CASE("US-023 Only visible tiles are drawn") {
    TileMap map(64, 64, kTile); // the 64 x 64 test map size
    RecordingRenderer renderer;
    const auto sheet = renderer.createTexture(Image(4 * kTile, kTile));

    SUBCASE("camera aligned to the tile grid: 15 x 9 tiles (270 / 32 = 8.4 rows, so 9)") {
        Camera camera(kView, kViewH, map.pixelWidth(), map.pixelHeight());
        camera.centreOn(0, 0); // clamped to the top-left corner
        map.draw(renderer, sheet, camera, 1.0);
        CHECK(renderer.draws().size() == 15 * 9);
    }
    SUBCASE("camera between tiles: one more column (16 x 9)") {
        Camera camera(kView, kViewH, map.pixelWidth(), map.pixelHeight());
        camera.centreOn(kView / 2 + 16, kViewH / 2 + 16); // view at (16, 16)
        CHECK(camera.view() == Rect{16, 16, kView, kViewH});
        map.draw(renderer, sheet, camera, 1.0);
        CHECK(renderer.draws().size() == 16 * 9);
    }
    SUBCASE("every drawn tile touches the screen; the other 4000+ tiles are skipped") {
        Camera camera(kView, kViewH, map.pixelWidth(), map.pixelHeight());
        camera.centreOn(map.pixelWidth() / 2.0, map.pixelHeight() / 2.0);
        map.draw(renderer, sheet, camera, 1.0);
        CHECK(renderer.draws().size() < 64 * 64 / 20);
        for (const auto& draw : renderer.draws()) {
            CHECK(draw.at.x > -kTile);
            CHECK(draw.at.y > -kTile);
            CHECK(draw.at.x < kView);
            CHECK(draw.at.y < kViewH);
        }
    }
}

TEST_CASE("US-023 Camera follows and stops at the map edges") {
    TileMap map(64, 64, kTile);
    const double centre = 64 * kTile / 2.0;

    SUBCASE("the camera follows smoothly: part of the way each tick, never a jump") {
        Camera camera(kView, kViewH, map.pixelWidth(), map.pixelHeight());
        camera.centreOn(centre, centre);
        const Rect start = camera.view();
        const double target = centre + 200; // the character walked 200 px right
        camera.follow(target, centre);
        const Rect oneTick = camera.view();
        CHECK(oneTick.x > start.x);                 // it moved
        CHECK(oneTick.x < start.x + 200);           // but not all the way at once
        int previous = oneTick.x;
        for (int tick = 0; tick < 60; ++tick) {     // 3 seconds
            camera.follow(target, centre);
            CHECK(camera.view().x >= previous);     // always forwards, never overshooting
            previous = camera.view().x;
        }
        CHECK(camera.view().x == start.x + 200);    // settled exactly on the character
    }
    SUBCASE("between ticks the view is blended, so the picture moves every frame") {
        Camera camera(kView, kViewH, map.pixelWidth(), map.pixelHeight());
        camera.centreOn(centre, centre);
        camera.follow(centre + 100, centre);
        const int before = camera.view(0.0).x;
        const int middle = camera.view(0.5).x;
        const int after = camera.view(1.0).x;
        CHECK(before < middle);
        CHECK(middle < after);
    }
    SUBCASE("at the map edges the camera stops and shows no void") {
        Camera camera(kView, kViewH, map.pixelWidth(), map.pixelHeight());
        camera.centreOn(centre, centre);
        for (int tick = 0; tick < 200; ++tick) {
            camera.follow(0, 0); // the character in the top-left corner
        }
        CHECK(camera.view() == Rect{0, 0, kView, kViewH});
        for (int tick = 0; tick < 200; ++tick) {
            camera.follow(map.pixelWidth(), map.pixelHeight()); // bottom-right corner
        }
        CHECK(camera.view() == Rect{map.pixelWidth() - kView, map.pixelHeight() - kViewH, kView, kViewH});
    }
}

TEST_CASE("TileMap stores a 2D grid in one vector") {
    TileMap map(5, 3, kTile, 0);
    map.set(4, 2, 7);
    map.setSolid(7, true);
    CHECK(map.at(4, 2) == 7);
    CHECK(map.at(0, 0) == 0);
    CHECK(map.isSolid(4, 2));
    CHECK_FALSE(map.isSolid(0, 0));
    CHECK(map.isSolid(-1, 0)); // outside the world is a wall
    CHECK(map.isSolid(5, 0));
    CHECK(map.visibleTiles(Rect{-100, -100, 50, 50}).count() == 0);
}
