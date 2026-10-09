// US-203 Water and mountains: the shapes of the tools, and the warnings about the land as it is.
#include "sim/region.h"
#include "sim/region_edits.h"
#include "sim/region_shapes.h"

#include <doctest/doctest.h>

#include <cstdlib>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

sim::RegionConfig config() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }

std::set<std::pair<int, int>> asSet(const std::vector<sim::Tile>& tiles) {
    std::set<std::pair<int, int>> set;
    for (const sim::Tile& tile : tiles) set.insert({tile.x, tile.y});
    return set;
}

} // namespace

TEST_CASE("US-203 A line is four-connected, has both ends, and is the same both ways") {
    const std::vector<std::pair<sim::Tile, sim::Tile>> lines = {{{10, 10}, {40, 12}}, {{40, 12}, {10, 10}}, {{5, 5}, {5, 30}}, {{20, 20}, {3, 3}}, {{7, 9}, {7, 9}}, {{0, 50}, {31, 0}}};
    for (const auto& [from, to] : lines) {
        const std::vector<sim::Tile> line = sim::line4(from, to);
        REQUIRE_FALSE(line.empty());
        CHECK(line.front() == from);
        CHECK(line.back() == to);
        CHECK(line.size() == static_cast<std::size_t>(std::abs(to.x - from.x) + std::abs(to.y - from.y) + 1)); // one step per tile, never diagonal
        for (std::size_t i = 1; i < line.size(); ++i) CHECK(std::abs(line[i].x - line[i - 1].x) + std::abs(line[i].y - line[i - 1].y) == 1);
    }
    // The same line whichever end it is drawn from (so a river does not change shape when dragged backwards).
    CHECK(asSet(sim::line4({10, 10}, {40, 12})) == asSet(sim::line4({40, 12}, {10, 10})));
}

TEST_CASE("US-203 Thickening uses the round stamp of the brush, and a disc is a disc") {
    CHECK(sim::thicken({{50, 50}}, 1, 256).size() == 1);
    CHECK(sim::thicken({{50, 50}}, 2, 256).size() == 5);  // the tile and its four neighbours
    CHECK(sim::thicken({{50, 50}}, 3, 256).size() == 9);  // a 3 x 3 block
    CHECK(sim::thicken({{0, 0}}, 3, 256).size() == 4);    // clipped by the region
    const auto line = sim::thicken(sim::line4({20, 20}, {20, 30}), 3, 256);
    CHECK(line.size() == 3 * 13); // a strip 3 wide, a tile longer at each end (the stamp is a 3 x 3 block)
    CHECK(sim::disc({100, 100}, 0, 256).size() == 1);
    CHECK(sim::disc({100, 100}, 1, 256).size() == 5);
    CHECK(sim::disc({100, 100}, 2, 256).size() == 13);
    CHECK(sim::disc({100, 100}, 5, 256).size() == 81);
    // In tile order: row after row.
    const auto strip = sim::thicken({{5, 5}, {6, 5}}, 1, 256);
    CHECK(strip.front() == sim::Tile{5, 5});
}

TEST_CASE("US-203 Warnings: the start without water, and a cave mouth nobody can reach") {
    sim::Region land(12, config()); // a seed with plenty of mountains
    CHECK(sim::landWarnings(land).empty());

    // A cave mouth in the middle of a cliff, with only mountain around it, is sealed; one at the edge of the cliff is not.
    sim::Tile sealed{-1, -1};
    sim::Tile edge{-1, -1};
    for (int y = 20; y < 236 && (sealed.x < 0 || edge.x < 0); ++y) {
        for (int x = 20; x < 236; ++x) {
            if (land.biomeAt(x, y) != sim::Biome::Mountain) continue;
            int walk = 0;
            for (const sim::Tile n : {sim::Tile{x + 1, y}, sim::Tile{x - 1, y}, sim::Tile{x, y + 1}, sim::Tile{x, y - 1}}) walk += sim::walkable(land.biomeAt(n.x, n.y)) ? 1 : 0;
            if (walk == 0 && sealed.x < 0) sealed = {x, y};
            if (walk > 0 && edge.x < 0) edge = {x, y};
        }
    }
    REQUIRE(sealed.x >= 0);
    REQUIRE(edge.x >= 0);
    land.setTileEdit(edge.x, edge.y, sim::Biome::Cave);
    CHECK(sim::landWarnings(land).empty());
    land.setTileEdit(sealed.x, sealed.y, sim::Biome::Cave);
    const auto warnings = sim::landWarnings(land);
    REQUIRE(warnings.size() == 1);
    CHECK(warnings.front().find("cave mouth") != std::string::npos);

    // Water over the start.
    const sim::Tile start = land.start();
    land.setTileEdit(start.x, start.y, sim::Biome::Water);
    bool saidStart = false;
    for (const std::string& warning : sim::landWarnings(land)) saidStart = saidStart || warning.find("start") != std::string::npos;
    CHECK(saidStart);
}
