#include "game/hero.h"

#include <doctest/doctest.h>

#include <cmath>
#include <set>

using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::TileMap;
using odysseus::game::Facing;
using odysseus::game::Hero;
using odysseus::game::HeroConfig;

namespace {

Intents holding(Intent intent) {
    Intents intents;
    intents.set(intent, true, false);
    return intents;
}

// An open field 20 x 20 tiles; tile 1 is a solid rock.
TileMap field() {
    TileMap map(20, 20, 32, 0);
    map.setSolid(1, true);
    return map;
}

} // namespace

TEST_CASE("US-024 Walk right") {
    const TileMap map = field();
    const HeroConfig config; // 96 px per second, 20 ticks per second
    Hero hero(160.0, 320.0, config);
    CHECK_FALSE(hero.walking());
    CHECK(hero.facing() == Facing::South); // idle, facing the player

    std::set<int> framesSeen;
    for (int tick = 0; tick < 20; ++tick) { // hold Move Right for one second
        hero.update(holding(Intent::MoveRight), map);
        framesSeen.insert(hero.animationFrame());
    }
    CHECK(hero.walking());
    CHECK(hero.facing() == Facing::East);
    CHECK(hero.feetX() == doctest::Approx(160.0 + config.speedPixelsPerSecond)); // exactly the configured speed
    CHECK(hero.feetY() == doctest::Approx(320.0));                                // straight right
    CHECK(framesSeen == std::set<int>{0, 1, 2, 3});                               // the whole walking cycle played
    CHECK(hero.spriteFrame().y == static_cast<int>(Facing::East) * odysseus::game::kCharacterHeight);
}

TEST_CASE("US-024 Stop at a rock") {
    TileMap map = field();
    map.set(8, 9, 1); // a rock at column 8, row 9: its left edge is x = 256
    Hero hero(160.0, 9 * 32 + 20.0);  // feet in row 9, left of the rock
    for (int tick = 0; tick < 60; ++tick) { // keep pushing right for 3 seconds
        hero.update(holding(Intent::MoveRight), map);
    }
    const auto feet = hero.feetBox();
    CHECK(feet.x + feet.width == doctest::Approx(256.0)); // flush against the rock's edge
    const double stopped = hero.feetX();
    hero.update(holding(Intent::MoveRight), map);
    CHECK(hero.feetX() == doctest::Approx(stopped)); // no creeping into the rock
}

TEST_CASE("US-024 Stop and face the last direction") {
    const TileMap map = field();
    Hero hero(320.0, 320.0);
    for (int tick = 0; tick < 10; ++tick) {
        hero.update(holding(Intent::MoveLeft), map);
    }
    CHECK(hero.walking());
    const double x = hero.feetX();
    hero.update(Intents{}, map); // release all movement input
    CHECK_FALSE(hero.walking());
    CHECK(hero.facing() == Facing::West);    // still facing where it went
    CHECK(hero.animationFrame() == 0);       // the idle pose
    CHECK(hero.spriteFrame().x == 0);
    CHECK(hero.feetX() == doctest::Approx(x)); // stopped at once
}

TEST_CASE("Diagonal walking keeps the same speed (D-17)") {
    const TileMap map = field();
    Hero hero(320.0, 320.0);
    Intents diagonal;
    diagonal.set(Intent::MoveRight, true, false);
    diagonal.set(Intent::MoveDown, true, false);
    for (int tick = 0; tick < 20; ++tick) {
        hero.update(diagonal, map);
    }
    const double dx = hero.feetX() - 320.0;
    const double dy = hero.feetY() - 320.0;
    CHECK(std::sqrt(dx * dx + dy * dy) == doctest::Approx(96.0));
    CHECK(hero.facing() == Facing::SouthEast);
}
