// US-241 Generated normal maps: the maths on a picture we can reason about, a filled disc.
#include "luna/engine/image.h"
#include "luna/engine/image_ops.h"

#include <doctest/doctest.h>

#include <cstdlib>

using luna::engine::Color;
using luna::engine::Image;

namespace {

Image disc(int size) {
    Image image(size, size);
    const double middle = size / 2.0;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const double dx = x + 0.5 - middle;
            const double dy = y + 0.5 - middle;
            if (dx * dx + dy * dy <= (middle - 2) * (middle - 2)) image.set(x, y, Color{180, 180, 180, 255});
        }
    }
    return image;
}

} // namespace

TEST_CASE("US-241 A disc leans outward: left edge left, right edge right, top up, bottom down, middle flat") {
    const Image normals = luna::engine::normalAtlas(disc(32), 32, 32, 2.0);
    const Color left = normals.get(5, 16);
    const Color right = normals.get(26, 16);
    const Color top = normals.get(16, 5);
    const Color bottom = normals.get(16, 26);
    const Color middle = normals.get(16, 16);
    CHECK(left.red < 100);     // pointing left (x negative)
    CHECK(right.red > 156);    // pointing right
    CHECK(top.green < 100);    // pointing up the screen (y negative)
    CHECK(bottom.green > 156); // pointing down
    CHECK(std::abs(middle.red - 128) < 10);
    CHECK(std::abs(middle.green - 128) < 10);
    CHECK(middle.blue > 240); // facing the viewer
    CHECK(normals.get(0, 0).red == 128); // see-through pixels are flat
    CHECK(normals.get(0, 0).blue == 255);
    CHECK(normals.get(0, 0).alpha == 255);
}

TEST_CASE("US-241 Cells do not leak into each other") {
    Image atlas(64, 32); // two cells: a disc, then nothing
    const Image one = disc(32);
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) atlas.set(x, y, one.get(x, y));
    }
    const Image normals = luna::engine::normalAtlas(atlas, 32, 32, 2.0);
    for (int y = 0; y < 32; ++y) {
        for (int x = 32; x < 64; ++x) CHECK(normals.get(x, y).red == 128);
    }
}

TEST_CASE("US-241 A mirrored picture is lit from the other side") {
    const Image normals = luna::engine::normalAtlas(disc(32), 32, 32, 2.0);
    const Image flipped = luna::engine::mirroredNormals(normals);
    // The picture is turned around, and so is the lean: the left edge of the new picture leans left again, as a disc's edge should.
    CHECK(flipped.get(5, 16).red < 100);
    CHECK(flipped.get(26, 16).red > 156);
    CHECK(std::abs(flipped.get(16, 5).green - normals.get(16, 5).green) <= 1); // up and down are unchanged
}
