// US-200 Minimap: the cached overview picture of a big map (Luna, game-agnostic).
#include "luna/engine/minimap.h"

#include <doctest/doctest.h>

namespace eng = luna::engine;

TEST_CASE("US-200 Minimap paints in slices, uploads once and maps a click to a cell") {
    eng::Minimap minimap(64, 64);
    int calls = 0;
    const auto colour = [&calls](int x, int y) {
        ++calls;
        return eng::Color{static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), 7, 255};
    };
    CHECK_FALSE(minimap.build(10, colour));
    CHECK(minimap.rowsDone() == 10);
    CHECK(calls == 640);
    CHECK_FALSE(minimap.complete());
    while (!minimap.build(20, colour)) {}
    CHECK(minimap.complete());
    CHECK(calls == 64 * 64);
    CHECK(minimap.image().get(5, 9) == eng::Color{5, 9, 7, 255});

    eng::RecordingRenderer renderer;
    const int first = minimap.texture(renderer).id;
    CHECK(first >= 0);
    CHECK(minimap.texture(renderer).id == first); // uploaded once

    const eng::Rect area{100, 50, 128, 128}; // the 64 x 64 map stretched twice
    CHECK(minimap.cellAt(area, 100, 50) == eng::Point{0, 0});
    CHECK(minimap.cellAt(area, 227, 177) == eng::Point{63, 63});
    CHECK(minimap.cellAt(area, 164, 114) == eng::Point{32, 32});
    CHECK_FALSE(minimap.cellAt(area, 99, 50).has_value());
    CHECK_FALSE(minimap.cellAt(area, 228, 50).has_value());
}
