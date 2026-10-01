// M3 Living clan on screen: US-030 layers, US-032 the clan in the game, US-031 emotes and details.
#include "game/clan_art.h"
#include "game/clan_view.h"
#include "game/level.h"
#include "game/odyssey_game.h"
#include "luna/engine/renderer.h"
#include "luna/engine/sprite_layers.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;
namespace game = odysseus::game;
namespace sim = odysseus::sim;
using luna::engine::Color;
using luna::engine::Image;

namespace {

int opaquePixels(const Image& image) {
    int count = 0;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x) count += image.get(x, y).alpha > 0 ? 1 : 0;
    return count;
}

const game::LayerSheets& sheets() {
    static const game::LayerSheets made = game::makeLayerSheets();
    return made;
}

fs::path campLevel() { return fs::path(ODYSSEUS_DEMO_LEVEL).parent_path() / "camp.json"; }

} // namespace

TEST_CASE("US-030 Compose") {
    const game::LayerSheets& s = sheets();
    // Every layer is a 4 x 8 grid of 32 x 48 cells, so the cells line up in every frame and direction.
    const int width = game::kCharacterWidth * game::kWalkFrames;
    const int height = game::kCharacterHeight * 8;
    CHECK(s.body.width() == width);
    CHECK(s.body.height() == height);
    for (const Image& hair : s.hair) CHECK((hair.width() == width && hair.height() == height));
    for (const Image& outfit : s.outfit) CHECK((outfit.width() == width && outfit.height() == height));
    CHECK(s.item.width() == width);
    // A look with all four layers draws more than the body alone, in every cell.
    game::LookSpec look;
    look.spear = true;
    const Image composed = game::composeLook(s, look);
    CHECK(composed.width() == width);
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < game::kWalkFrames; ++column) {
            int layered = 0;
            int bodyOnly = 0;
            for (int y = 0; y < game::kCharacterHeight; ++y) {
                for (int x = 0; x < game::kCharacterWidth; ++x) {
                    const int px = column * game::kCharacterWidth + x;
                    const int py = row * game::kCharacterHeight + y;
                    layered += composed.get(px, py).alpha > 0 ? 1 : 0;
                    bodyOnly += s.body.get(px, py).alpha > 0 ? 1 : 0;
                }
            }
            CAPTURE(row);
            CAPTURE(column);
            CHECK(layered > bodyOnly);
        }
    }
}

TEST_CASE("US-030 Swap") {
    // Two people who differ only in their outfit: every pixel outside the two outfit layers is the same.
    game::LookSpec a;
    a.outfitStyle = 0;
    game::LookSpec b = a;
    b.outfitStyle = 2;
    b.outfitColour = 3;
    const Image pictureA = game::composeLook(sheets(), a);
    const Image pictureB = game::composeLook(sheets(), b);
    const Image outfitA = game::outfitLayer(sheets(), a.outfitStyle, a.outfitColour);
    const Image outfitB = game::outfitLayer(sheets(), b.outfitStyle, b.outfitColour);
    int differing = 0;
    for (int y = 0; y < pictureA.height(); ++y) {
        for (int x = 0; x < pictureA.width(); ++x) {
            if (pictureA.get(x, y) == pictureB.get(x, y)) continue;
            ++differing;
            CHECK((outfitA.get(x, y).alpha > 0 || outfitB.get(x, y).alpha > 0)); // only the outfit changed
        }
    }
    CHECK(differing > 100);
}

TEST_CASE("US-030 Palette") {
    // One hair picture, three palettes: three distinct colours at the same pixel, without new art.
    std::set<std::uint32_t> colours;
    int shape = -1;
    for (const int colour : {0, 2, 4}) {
        const Image hair = game::hairLayer(sheets(), 0, colour);
        if (shape < 0) shape = opaquePixels(hair);
        CHECK(opaquePixels(hair) == shape); // the same picture
        // The first opaque pixel.
        for (int y = 0; y < hair.height(); ++y) {
            bool found = false;
            for (int x = 0; x < hair.width(); ++x) {
                const Color c = hair.get(x, y);
                if (c.alpha > 0) {
                    colours.insert((std::uint32_t{c.red} << 16) | (std::uint32_t{c.green} << 8) | c.blue);
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
    }
    CHECK(colours.size() == 3);
    // The recolouring works on any layer.
    const Image red = luna::engine::recoloured(sheets().body, {{Color{224, 172, 128}, Color{255, 0, 0}}});
    CHECK(opaquePixels(red) == opaquePixels(sheets().body));
    CHECK(red.get(game::kCharacterWidth * 0 + 16, 8).red == 255);
}

TEST_CASE("US-030 Looks of people") {
    // Looks are the same for the same person and differ across a clan.
    sim::SimConfig config = sim::loadSimConfig(ODYSSEUS_DATA_DIR);
    sim::World world(7, config);
    std::set<game::LookSpec> looks;
    for (const sim::Person& person : world.people()) {
        const game::LookSpec first = game::lookOf(person, 30);
        CHECK(first == game::lookOf(person, 30));
        looks.insert(first);
        CHECK_FALSE(game::lookOf(person, 8).spear); // children carry no spear
    }
    CHECK(looks.size() >= 10);
}

TEST_CASE("US-032 Mirror and smooth") {
    const luna::engine::TileMap map(64, 64, 32);
    sim::World world(7, sim::loadSimConfig(ODYSSEUS_DATA_DIR));
    game::ClanView view({32 * 20, 32 * 20});
    view.update(world, map);
    int present = 0;
    for (const game::Figure& figure : view.figures()) present += figure.present ? 1 : 0;
    CHECK(present == 20); // one figure for each of the 20 people
    // Run two game days: nobody ever moves faster than walking pace, and the picture between two ticks is in between.
    for (int tick = 0; tick < 4800; ++tick) {
        world.tick();
        view.update(world, map);
        for (const game::Figure& figure : view.figures()) {
            if (!figure.present) continue;
            const double step = std::hypot(figure.x - figure.previousX, figure.y - figure.previousY);
            if (step > game::kClanWalkPixelsPerTick + 0.01) {
                CHECK(step <= game::kClanWalkPixelsPerTick + 0.01); // teleports only when wedged (not on an open map)
            }
            const double half = figure.feetX(0.5);
            CHECK(half >= std::min(figure.previousX, figure.x) - 1e-9);
            CHECK(half <= std::max(figure.previousX, figure.x) + 1e-9);
        }
    }
    // Everybody is somewhere on the map.
    for (const game::Figure& figure : view.figures()) {
        if (!figure.present) continue;
        CHECK(figure.x > 0);
        CHECK(figure.y > 0);
        CHECK(figure.x < 64 * 32);
        CHECK(figure.y < 64 * 32);
    }
}

TEST_CASE("US-032 The clan in the game") {
    REQUIRE(fs::exists(campLevel()));
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR, campLevel());
    luna::engine::RecordingRenderer renderer;
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);
    REQUIRE(odyssey.clanOn()); // the level says "clan": true
    for (int i = 0; i < 40; ++i) odyssey.update({});
    int present = 0;
    for (const game::Figure& figure : odyssey.clanView().figures()) present += figure.present ? 1 : 0;
    CHECK(present == 20);
    CHECK(odyssey.clan()->population() == 20);
    CHECK(odyssey.clan()->ticks() == 40); // one simulation tick for each game tick
    // The 20 people are drawn: more draws than the same level without the clan.
    renderer.clear();
    odyssey.render(renderer, 0.5);
    const std::size_t with = renderer.draws().size();
    odyssey.setClan(false);
    renderer.clear();
    odyssey.render(renderer, 0.5);
    CHECK(with >= renderer.draws().size() + 20);
}

TEST_CASE("US-031 Emotes") {
    sim::Person person;
    person.alive = true;
    CHECK(game::emoteOf(person) == game::Emote::None);
    person.needs[sim::Need::Warmth] = 10;
    CHECK(game::emoteOf(person) == game::Emote::Cold);
    person.needs[sim::Need::Hunger] = 5;
    CHECK(game::emoteOf(person) == game::Emote::Cold); // the cold comes first
    person.needs[sim::Need::Warmth] = 90;
    CHECK(game::emoteOf(person) == game::Emote::Hungry);
    person.needs[sim::Need::Hunger] = 90;
    person.health = sim::Health::Sick;
    CHECK(game::emoteOf(person) == game::Emote::Sick);
    person.health = sim::Health::Well;
    person.needs[sim::Need::Energy] = 10;
    CHECK(game::emoteOf(person) == game::Emote::Tired);
    person.needs[sim::Need::Energy] = 90;
    person.needs[sim::Need::Social] = 10;
    CHECK(game::emoteOf(person) == game::Emote::Lonely);
    person.alive = false;
    CHECK(game::emoteOf(person) == game::Emote::None);
}

TEST_CASE("US-031 Details on demand") {
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR, campLevel());
    luna::engine::RecordingRenderer renderer;
    odyssey.setViewScales(1, 1);
    odyssey.start(renderer);
    for (int i = 0; i < 40; ++i) odyssey.update({});
    // Find a person on screen and hover over their middle.
    const auto view = odyssey.cameraView();
    const game::Figure* target = nullptr;
    for (const game::Figure& figure : odyssey.clanView().figures()) {
        const int x = static_cast<int>(figure.x) - view.x;
        const int y = static_cast<int>(figure.y) - view.y;
        if (figure.present && x > 20 && x < 460 && y > 60 && y < 250) target = &figure;
    }
    REQUIRE(target != nullptr);
    renderer.clear();
    odyssey.render(renderer, 1.0);
    const std::size_t plain = renderer.draws().size();
    luna::engine::Intents hover;
    luna::engine::Pointer pointer;
    pointer.x = static_cast<int>(target->x) - view.x;
    pointer.y = static_cast<int>(target->y) - view.y - 20;
    hover.setPointer(pointer);
    odyssey.update(hover);
    renderer.clear();
    odyssey.render(renderer, 1.0);
    CHECK(renderer.draws().size() > plain); // the panel with the needs and the action
}
