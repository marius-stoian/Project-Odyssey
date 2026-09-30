// US-140 Arc ballistics: shots land where aimed, hit in their path, rocks block low shots.
#include "game/arc_shots.h"
#include "game/catalogs.h"
#include "game/odyssey_game.h"

#include "luna/engine/physics_view.h"
#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::toDouble;

namespace {

const game::Catalogs& catalogs() {
    static const game::Catalogs loaded = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    return loaded;
}

const game::WeaponDef& bow() { return *catalogs().weapon("wooden longbow"); }
const game::WeaponDef& knives() { return *catalogs().weapon("throwing knives"); }

constexpr double kTile = 32.0;
constexpr double kHeroX = 10.0 * kTile; // the hero stands at tile (10, 20)
constexpr double kHeroY = 20.0 * kTile;

struct Flight {
    std::vector<game::ArcShot> shots;
    std::vector<game::ArcEvent> events; // everything that happened, in order
};

// Flies the shot until it ends (enemy, solid, ground or lost), or 400 ticks pass.
Flight fly(game::ArcShot shot, const luna::engine::TileMap& map, const std::vector<game::Target>& targets = {}) {
    Flight flight;
    flight.shots.push_back(std::move(shot));
    for (int tick = 0; tick < 400 && !flight.shots.empty(); ++tick) {
        for (const game::ArcEvent& event : game::stepArcShots(flight.shots, map, targets)) flight.events.push_back(event);
        if (!flight.events.empty() && flight.events.back().end != game::ArcEnd::Ground && flight.events.back().end != game::ArcEnd::Solid) break;
        if (!flight.events.empty()) break; // came down: stuck in the ground, that is the end of this test's interest
    }
    return flight;
}

double metresBetween(luna::physics::Vec3 a, luna::physics::Vec3 b) {
    return std::hypot(toDouble(a.x) - toDouble(b.x), toDouble(a.y) - toDouble(b.y));
}

} // namespace

TEST_CASE("US-140 Lands at the cursor") {
    const luna::engine::TileMap map(64, 64, 32);
    for (const double distance : {3.0, 6.0, 9.0}) {
        CAPTURE(distance);
        const auto shot = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, distance * kTile, false);
        REQUIRE(shot);
        const Flight flight = fly(*shot, map);
        REQUIRE(flight.events.size() == 1);
        CHECK(flight.events[0].end == game::ArcEnd::Ground);
        CHECK(metresBetween(flight.events[0].point, shot->aimPoint) < 0.5);
    }
    SUBCASE("a thrown weapon lands where it is aimed too, in another direction") {
        const double angle = 0.6; // radians, to the south-east
        const auto shot = game::launchArcShot(knives(), kHeroX, kHeroY, std::cos(angle), std::sin(angle), 5.0 * kTile, false);
        REQUIRE(shot);
        const Flight flight = fly(*shot, map);
        REQUIRE(flight.events.size() == 1);
        CHECK(metresBetween(flight.events[0].point, shot->aimPoint) < 0.5);
    }
    SUBCASE("the same inputs fly the same shot") {
        const auto a = game::launchArcShot(bow(), kHeroX, kHeroY, 0.6, 0.8, 7.0 * kTile, false);
        const auto b = game::launchArcShot(bow(), kHeroX, kHeroY, 0.6, 0.8, 7.0 * kTile, false);
        REQUIRE((a && b));
        const Flight fa = fly(*a, map);
        const Flight fb = fly(*b, map);
        REQUIRE((fa.events.size() == 1 && fb.events.size() == 1));
        CHECK(fa.events[0].point == fb.events[0].point);
    }
    SUBCASE("staffs and guns do not arc") {
        CHECK_FALSE(game::launchArcShot(*catalogs().weapon("iron sword"), kHeroX, kHeroY, 1.0, 0.0, 100.0, false));
        CHECK_FALSE(game::flysInArc(game::WeaponClass::Staff));
        CHECK_FALSE(game::flysInArc(game::WeaponClass::Gun));
        CHECK(game::flysInArc(game::WeaponClass::Bow));
        CHECK(game::flysInArc(game::WeaponClass::Thrown));
    }
}

TEST_CASE("US-140 Range and misses") {
    const luna::engine::TileMap map(64, 64, 32);
    // A pointer 30 m away: the shot lands at the weapon's range (10 m for a bow), not at the pointer.
    const auto shot = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, 30.0 * kTile, false);
    REQUIRE(shot);
    const Flight flight = fly(*shot, map);
    REQUIRE(flight.events.size() == 1);
    const double flown = toDouble(flight.events[0].point.x) - kHeroX / kTile;
    CHECK(flown == doctest::Approx(bow().range).epsilon(0.05));
    // A miss sticks in the ground for a while, then is gone.
    std::vector<game::ArcShot> shots{*shot};
    int ticks = 0;
    while (!shots.empty() && ticks < 400) {
        game::stepArcShots(shots, map, {});
        ++ticks;
        if (!shots.empty() && shots[0].state == game::ArcState::Stuck) {
            CHECK(toDouble(shots[0].body.position.z) <= 0.05);
        }
    }
    REQUIRE(shots.empty());
    CHECK(ticks > game::kStuckTicks); // it flew, then stayed stuck for kStuckTicks ticks
    // Nothing hit: the targets list is empty, so the only event was the ground.
}

TEST_CASE("US-140 Hits in its path") {
    luna::engine::TileMap map(64, 64, 32);
    const double enemyX = kHeroX + 5.0 * kTile;
    const std::vector<game::Target> enemy{{enemyX, kHeroY, true}};
    SUBCASE("a shot through the enemy's body height hits it") {
        const auto shot = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, 8.0 * kTile, true); // chest height, 8 m out
        REQUIRE(shot);
        const Flight flight = fly(*shot, map, enemy);
        REQUIRE(flight.events.size() == 1);
        CHECK(flight.events[0].end == game::ArcEnd::Enemy);
        CHECK(flight.events[0].enemy == 0);
    }
    SUBCASE("a dead enemy is not hit") {
        const auto shot = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, 8.0 * kTile, true);
        REQUIRE(shot);
        const Flight flight = fly(*shot, map, {{enemyX, kHeroY, false}});
        REQUIRE(flight.events.size() == 1);
        CHECK(flight.events[0].end != game::ArcEnd::Enemy);
    }
    SUBCASE("an enemy off to the side is missed") {
        const auto shot = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, 8.0 * kTile, true);
        REQUIRE(shot);
        const Flight flight = fly(*shot, map, {{enemyX, kHeroY + 2.0 * kTile, true}});
        REQUIRE(flight.events.size() == 1);
        CHECK(flight.events[0].end != game::ArcEnd::Enemy);
    }
    SUBCASE("a rock blocks a low shot, a high arc clears it") {
        const int rockX = static_cast<int>(kHeroX / kTile) + 3; // a rock 3 m ahead
        map.set(rockX, static_cast<int>(kHeroY / kTile), static_cast<int>(game::TileKind::Rock));
        // Aimed at the ground just behind the rock (4 m), the arrow is still low at the rock: it stops there.
        const auto low = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, 4.0 * kTile, false);
        REQUIRE(low);
        const Flight blocked = fly(*low, map);
        REQUIRE(blocked.events.size() == 1);
        CHECK(blocked.events[0].end == game::ArcEnd::Solid);
        CHECK(toDouble(blocked.events[0].point.x) < rockX + 0.05); // stopped at the rock's near face
        // Aimed 9 m out, the arrow is well above the rock as it passes and lands beyond it.
        const auto high = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, 9.0 * kTile, false);
        REQUIRE(high);
        const Flight over = fly(*high, map);
        REQUIRE(over.events.size() == 1);
        CHECK(over.events[0].end == game::ArcEnd::Ground);
        CHECK(toDouble(over.events[0].point.x) > rockX + 1.0);
    }
    SUBCASE("water does not stop a shot") {
        map.set(13, 20, static_cast<int>(game::TileKind::Water));
        map.setSolid(static_cast<int>(game::TileKind::Water), true);
        const auto shot = game::launchArcShot(bow(), kHeroX, kHeroY, 1.0, 0.0, 4.0 * kTile, false);
        REQUIRE(shot);
        const Flight flight = fly(*shot, map);
        REQUIRE(flight.events.size() == 1);
        CHECK(flight.events[0].end == game::ArcEnd::Ground);
    }
}

TEST_CASE("US-140 In the game") {
    // A goblin 4 m south (there is a rock 3 m east of the hero start); aimed with the pointer at its feet, a bow arrow arcs in and hurts it.
    const fs::path folder = fs::temp_directory_path() / "odysseus-us140";
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    game::PlacedCharacter goblin = level.characters.at(0);
    goblin.feet = {level.heroStart.x, level.heroStart.y + 4 * 32};
    goblin.hp = 100;
    level.characters = {goblin};
    level.pickups.clear();
    game::saveLevel(level, definitions, folder / "level.json");

    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR, folder / "level.json");
    luna::engine::RecordingRenderer renderer;
    odyssey.start(renderer);
    REQUIRE(odyssey.pickUp("wooden longbow"));
    odyssey.selectSlot(0);
    luna::engine::Intents aim;
    luna::engine::Pointer pointer;
    pointer.x = 240; // the hero is at the middle of the picture; the goblin 4 m to the south
    pointer.y = 135 + 4 * 32;
    aim.set(luna::engine::Intent::Attack, true, true);
    aim.setPointer(pointer);
    odyssey.update(aim);
    REQUIRE(odyssey.arcShots().size() == 1);
    CHECK(odyssey.arcShots()[0].body.position.z > luna::physics::kFixedZero); // it leaves from hand height
    // Flight, with the picture drawn along the way (the sprite lifted, its shadow on the ground).
    for (int i = 0; i < 40 && !odyssey.arcShots().empty(); ++i) {
        odyssey.update({});
        renderer.clear();
        odyssey.render(renderer, 0.5);
    }
    CHECK(odyssey.enemies().at(0).hp() == 100 - bow().damage);
    CHECK(odyssey.arcShots().empty()); // spent on the goblin
}
