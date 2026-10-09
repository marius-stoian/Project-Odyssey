// US-200 The region in the Editor: a zoomable map of the whole generated land, with layer switches, the same world as the game's.
#include "game/catalogs.h"
#include "game/level.h"
#include "game/region_level.h"
#include "game/region_view.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"
#include "sim/region.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;
namespace game = odysseus::game;
namespace sim = odysseus::sim;
namespace eng = luna::engine;

namespace {

sim::RegionConfig config() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }

struct Rig {
    game::RegionView view{960, 540, [](const std::string&) {}};
    eng::RecordingRenderer renderer;
    eng::Texture sheet;
    Rig(std::uint64_t seed = 1) {
        sheet = renderer.createTexture(eng::makeUiSheet());
        view.open(seed, config());
    }
    // One drawn frame; the draws are those of this frame only.
    void frame() {
        renderer.clear();
        eng::UiPainter painter(renderer, sheet);
        painter.setScreen({0, 0, 960, 540});
        view.render(renderer, painter);
    }
    // Frames until every chunk picture the view needs is made; returns how many frames that took.
    int settle() {
        int frames = 0;
        do {
            frame();
            ++frames;
            REQUIRE(view.chunkBuildsThisFrame() <= game::RegionView::kChunkBuildsPerFrame);
        } while (view.chunkBuildsThisFrame() > 0 && frames < 500);
        return frames;
    }
    // The draws of the chunk pictures of one layer.
    std::vector<eng::RecordingRenderer::Draw> drawsOf(game::RegionLayer layer) const {
        std::vector<eng::RecordingRenderer::Draw> found;
        for (const auto& draw : renderer.draws()) {
            const auto owner = view.layerOfTexture(draw.texture);
            if (owner && *owner == layer) found.push_back(draw);
        }
        return found;
    }
};

eng::Intents pointerAt(int x, int y, bool held = false, int wheel = 0) {
    eng::Intents intents;
    eng::Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    pointer.wheel = wheel;
    pointer.held[static_cast<std::size_t>(eng::PointerButton::Left)] = held;
    pointer.pressed[static_cast<std::size_t>(eng::PointerButton::Left)] = held;
    intents.setPointer(pointer);
    return intents;
}

long long area(const std::vector<eng::RecordingRenderer::Draw>& draws) {
    long long total = 0;
    for (const auto& draw : draws) total += static_cast<long long>(draw.destination.width) * draw.destination.height;
    return total;
}

} // namespace

TEST_CASE("US-200 Open: the region shows at every zoom, from the whole map to single tiles, a few pictures a frame") {
    Rig rig;
    CHECK(rig.view.region()->size() == 256);
    for (int zoom = game::RegionView::kMinZoom; zoom <= game::RegionView::kMaxZoom; ++zoom) {
        CAPTURE(zoom);
        rig.view.setZoom(zoom);
        rig.view.centreOn(rig.view.region()->start().x + 0.5, rig.view.region()->start().y + 0.5);
        if (zoom == game::RegionView::kMinZoom) rig.view.centreOn(128.0, 128.0);
        const int frames = rig.settle();
        CHECK(frames < 200); // never a long wait
        rig.frame();
        const auto range = rig.view.visibleTiles();
        REQUIRE_FALSE(range.empty());
        // The ground fills the screen: the terrain and water pictures together cover every visible tile twice over (each is drawn whole,
        // see-through where the other layer lies), so the terrain pictures alone cover exactly the visible tiles.
        const long long tile = rig.view.tilePixels();
        const long long visible = static_cast<long long>(range.x1 - range.x0 + 1) * (range.y1 - range.y0 + 1) * tile * tile;
        CHECK(area(rig.drawsOf(game::RegionLayer::Terrain)) == visible);
        CHECK(area(rig.drawsOf(game::RegionLayer::Water)) == visible);
    }
    rig.view.setZoom(game::RegionView::kMinZoom);
    rig.view.centreOn(128.0, 128.0);
    const auto whole = rig.view.visibleTiles();
    CHECK(whole.x0 == 0);
    CHECK(whole.y0 == 0);
    CHECK(whole.x1 == 255);
    CHECK(whole.y1 == 255); // the whole 256 x 256 map is on screen at the widest zoom
    rig.view.setZoom(game::RegionView::kMaxZoom);
    CHECK(rig.view.tilePixels() == 64); // single tiles
}

TEST_CASE("US-200 Open: panning across the whole map never makes more than a few pictures in a frame") {
    Rig rig;
    rig.view.setZoom(game::RegionView::kMaxZoom);
    int madeBefore = 0;
    for (int step = 0; step < 40; ++step) {
        rig.view.centreOn(4.0 + step * 6.0, 4.0 + step * 6.0);
        rig.frame();
        CHECK(rig.view.chunkBuildsThisFrame() <= game::RegionView::kChunkBuildsPerFrame);
        CHECK(rig.view.chunkPicturesMade() >= madeBefore);
        madeBefore = rig.view.chunkPicturesMade();
    }
    // Going back to where it has been costs nothing: the pictures are kept.
    rig.view.centreOn(4.0, 4.0);
    rig.settle();
    const int made = rig.view.chunkPicturesMade();
    rig.view.centreOn(4.0, 4.0);
    rig.frame();
    CHECK(rig.view.chunkBuildsThisFrame() == 0);
    CHECK(rig.view.chunkPicturesMade() == made);
}

TEST_CASE("US-200 Layers: hiding plants and people leaves only terrain") {
    Rig rig;
    rig.view.centreOn(rig.view.region()->start().x + 0.5, rig.view.region()->start().y + 0.5);
    rig.view.setZoom(2);
    rig.settle();
    rig.frame();
    CHECK_FALSE(rig.drawsOf(game::RegionLayer::Plants).empty()); // all layers on: there are trees and berries in sight
    CHECK_FALSE((rig.drawsOf(game::RegionLayer::People).empty() && rig.drawsOf(game::RegionLayer::Things).empty()));

    rig.view.setLayerShown(game::RegionLayer::Plants, false);
    rig.view.setLayerShown(game::RegionLayer::People, false);
    rig.view.setLayerShown(game::RegionLayer::Things, false);
    rig.settle();
    rig.frame();
    for (const auto& draw : rig.renderer.draws()) {
        const auto owner = rig.view.layerOfTexture(draw.texture);
        if (owner) CHECK((*owner == game::RegionLayer::Terrain || *owner == game::RegionLayer::Water));
    }
    CHECK_FALSE(rig.drawsOf(game::RegionLayer::Terrain).empty());
    CHECK(rig.drawsOf(game::RegionLayer::Plants).empty());
    CHECK(rig.drawsOf(game::RegionLayer::People).empty());

    rig.view.setLayerShown(game::RegionLayer::Terrain, false);
    rig.view.setLayerShown(game::RegionLayer::Water, false);
    rig.frame();
    for (const auto& draw : rig.renderer.draws()) CHECK_FALSE(rig.view.layerOfTexture(draw.texture).has_value());
}

TEST_CASE("US-200 Same world: the view holds exactly what the game's level is made from") {
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    for (const std::uint64_t seed : {1ULL, 7ULL, 42ULL}) {
        CAPTURE(seed);
        Rig rig(seed);
        sim::Region gameRegion(seed, config()); // the game makes its region from the same seed and the same file
        const game::Level level = game::levelFromRegion(gameRegion, definitions, catalogs);
        const int grass = definitions.tileNumber("grass");
        const int water = definitions.tileNumber("water");
        const int stone = definitions.tileNumber("stone");
        const int path = definitions.tileNumber("path");
        const int n = rig.view.region()->config().chunkSize;
        int mismatches = 0;
        long long plantPixels = 0;
        long long thingPixels = 0;
        long long peoplePixels = 0;
        for (int cy = 0; cy < rig.view.region()->chunksPerSide(); ++cy) {
            for (int cx = 0; cx < rig.view.region()->chunksPerSide(); ++cx) {
                const eng::Image terrain = rig.view.chunkImage(cx, cy, game::RegionLayer::Terrain);
                const eng::Image waterImage = rig.view.chunkImage(cx, cy, game::RegionLayer::Water);
                for (int ty = 0; ty < n; ++ty) {
                    for (int tx = 0; tx < n; ++tx) {
                        const int ground = level.at(cx * n + tx, cy * n + ty);
                        const bool isWater = waterImage.get(tx, ty).alpha != 0;
                        const bool isTerrain = terrain.get(tx, ty).alpha != 0;
                        if (isWater == isTerrain) ++mismatches; // exactly one of the two layers holds every tile
                        if (isWater != (ground == water)) ++mismatches;
                        const sim::Biome biome = rig.view.cellAt(cx * n + tx, cy * n + ty).biome;
                        const int expected = biome == sim::Biome::Water ? water : biome == sim::Biome::Mountain ? stone : biome == sim::Biome::Cave ? path : grass;
                        if (ground != expected) ++mismatches;
                    }
                }
                const auto count = [&](game::RegionLayer layer) {
                    const eng::Image image = rig.view.chunkImage(cx, cy, layer);
                    long long pixels = 0;
                    for (int y = 0; y < n; ++y) {
                        for (int x = 0; x < n; ++x) pixels += image.get(x, y).alpha != 0 ? 1 : 0;
                    }
                    return pixels;
                };
                plantPixels += count(game::RegionLayer::Plants);
                thingPixels += count(game::RegionLayer::Things);
                peoplePixels += count(game::RegionLayer::People);
            }
        }
        CHECK(mismatches == 0);
        // Every plant, flint patch and herd of the level is one pixel of a layer, and the other way round.
        CHECK(plantPixels + thingPixels == static_cast<long long>(level.plants.size()));
        CHECK(peoplePixels == static_cast<long long>(level.characters.size()));
        // The start is the same too.
        CHECK(level.heroStart.x == rig.view.region()->start().x * game::kTileSize + game::kTileSize / 2);
    }
}

TEST_CASE("US-200 The overview is painted in slices and a click on it moves the view") {
    Rig rig;
    CHECK(rig.view.overviewRowsDone() == 0);
    rig.view.update(pointerAt(-1, -1));
    CHECK(rig.view.overviewRowsDone() == game::RegionView::kOverviewRowsPerTick);
    for (int i = 0; i < 20 && !rig.view.overviewComplete(); ++i) rig.view.update(pointerAt(-1, -1));
    CHECK(rig.view.overviewComplete());

    const eng::Rect box = rig.view.overviewArea();
    rig.view.setZoom(3);
    rig.view.update(pointerAt(box.x + box.width / 4, box.y + box.height / 4, true)); // a quarter of the way across: tile 64 of 256
    CHECK(rig.view.centreX() == doctest::Approx(64.0).epsilon(0.02));
    CHECK(rig.view.centreY() == doctest::Approx(64.0).epsilon(0.02));
}

TEST_CASE("US-200 The wheel zooms around the pointer and a drag pans") {
    Rig rig;
    rig.view.centreOn(100.0, 100.0);
    rig.view.setZoom(2);
    const auto before = rig.view.tileAtScreen(700, 300);
    REQUIRE(before.has_value());
    rig.view.update(pointerAt(700, 300, false, 1));
    CHECK(rig.view.zoom() == 3);
    const auto after = rig.view.tileAtScreen(700, 300);
    REQUIRE(after.has_value());
    CHECK(std::abs(after->x - before->x) <= 1); // the tile under the pointer stays under it
    CHECK(std::abs(after->y - before->y) <= 1);

    rig.view.update(pointerAt(-1, -1));
    const double x = rig.view.centreX();
    rig.view.update(pointerAt(500, 300, true));
    rig.view.update(pointerAt(460, 300, true)); // dragged 40 pixels to the left: the land moves left, the view moves right
    CHECK(rig.view.centreX() > x);
    rig.view.update(pointerAt(-1, -1));
    for (int i = 0; i < 100; ++i) rig.view.update(pointerAt(480, 300, false, -1)); // zoom out as far as it goes
    CHECK(rig.view.zoom() == game::RegionView::kMinZoom);
}
