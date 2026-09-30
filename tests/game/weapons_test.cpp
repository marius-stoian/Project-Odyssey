// US-133 Weapon classes and the starter set.
#include "game/level.h"
#include "game/odyssey_game.h"
#include "game/weapons.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Intent;
using luna::engine::Intents;

namespace {

Intents holding(std::initializer_list<Intent> list) {
    Intents intents;
    for (const Intent intent : list) intents.set(intent, true, false);
    return intents;
}

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

game::WeaponDef weaponOf(game::WeaponClass weaponClass, double range) {
    game::WeaponDef weapon;
    weapon.name = "test";
    weapon.weaponClass = weaponClass;
    weapon.damage = 7;
    weapon.speed = 2.0;
    weapon.range = range;
    return weapon;
}

struct Play {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Play(const fs::path& data = ODYSSEUS_DATA_DIR) : odyssey(data, ODYSSEUS_DEMO_LEVEL) { odyssey.start(renderer); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
    void hold(const std::string& name) {
        for (std::size_t i = 0; i < odyssey.carriedCount() && odyssey.heldName() != name; ++i) tick(1, pressing(Intent::SwitchWeapon));
        REQUIRE(odyssey.heldName() == name);
    }
};

} // namespace

TEST_CASE("US-133 Classes") {
    const std::vector<game::Target> targets{{64, 0}, {40, 30}, {-40, 0}, {0, 40}}; // east, south-east, west, south
    SUBCASE("sword: the nearest in a 90-degree arc") {
        const auto hits = game::behaviourOf(game::WeaponClass::Sword).swing(weaponOf(game::WeaponClass::Sword, 2.5), 0, 0, game::Facing::East, targets);
        CHECK(hits == std::vector<std::size_t>{1});
    }
    SUBCASE("axe: everything in a 120-degree arc") {
        const auto hits = game::behaviourOf(game::WeaponClass::Axe).swing(weaponOf(game::WeaponClass::Axe, 2.5), 0, 0, game::Facing::East, targets);
        CHECK(hits == std::vector<std::size_t>{1, 0});
    }
    SUBCASE("spear: a narrow thrust, long") {
        const auto hits = game::behaviourOf(game::WeaponClass::Spear).swing(weaponOf(game::WeaponClass::Spear, 2.2), 0, 0, game::Facing::East, targets);
        CHECK(hits == std::vector<std::size_t>{0}); // the south-east one is 37 degrees off
    }
    SUBCASE("whip: 100 degrees, everything, out to its reach") {
        const auto hits = game::behaviourOf(game::WeaponClass::Whip).swing(weaponOf(game::WeaponClass::Whip, 1.5), 0, 0, game::Facing::East, targets);
        CHECK(hits == std::vector<std::size_t>{}); // 1.5 m = 48 px: the nearest stands 50 px away
        const auto longer = game::behaviourOf(game::WeaponClass::Whip).swing(weaponOf(game::WeaponClass::Whip, 2.8), 0, 0, game::Facing::East, targets);
        CHECK(longer == std::vector<std::size_t>{1, 0});
    }
    SUBCASE("the four ranged classes launch; faster ones fly further per tick") {
        double previous = 0.0;
        for (const auto weaponClass : {game::WeaponClass::Thrown, game::WeaponClass::Staff, game::WeaponClass::Bow, game::WeaponClass::Gun}) {
            const auto& behaviour = game::behaviourOf(weaponClass);
            CHECK_FALSE(behaviour.melee());
            const auto shot = behaviour.launch(weaponOf(weaponClass, 8.0), 0, 0, game::Facing::West);
            REQUIRE(shot);
            CHECK(shot->dx == doctest::Approx(-1.0));
            CHECK(shot->pixelsPerTick > previous);
            previous = shot->pixelsPerTick;
        }
    }
    SUBCASE("a shot hits the first target on its line, a wall, or falls at its range") {
        luna::engine::TileMap map(40, 10, 32);
        const game::WeaponDef bow = weaponOf(game::WeaponClass::Bow, 8.0);
        auto flyUntilDone = [&](game::Projectile shot, const std::vector<game::Target>& in) {
            std::optional<std::size_t> hit;
            bool done = false;
            for (int i = 0; i < 100 && !done; ++i) hit = game::stepProjectile(shot, map, in, done);
            return std::pair{hit, shot};
        };
        const auto shot = *game::behaviourOf(game::WeaponClass::Bow).launch(bow, 100, 200, game::Facing::East);
        const auto [hit, spent] = flyUntilDone(shot, {{300, 200}, {200, 200}});
        CHECK(hit == 1u); // the nearer one, on the line
        const auto [none, far] = flyUntilDone(shot, {{900, 200}});
        CHECK_FALSE(none.has_value());
        CHECK(far.travelled == doctest::Approx(8.0 * 32).epsilon(0.05)); // fell at 8 m
        map.set(8, 6, 1);
        map.setSolid(1, true);
        const auto [wall, stopped] = flyUntilDone(shot, {{900, 200}});
        CHECK_FALSE(wall.has_value());
        CHECK(stopped.x < 8 * 32 + 8); // stopped within one step of the wall tile at x = 256
    }
    CHECK(game::cooldownTicks(weaponOf(game::WeaponClass::Gun, 12.0)) == 10); // 2 attacks a second
}

TEST_CASE("US-133 Starters fight") {
    // Every starter hurts the demo goblin: melee from close, ranged from 2 m.
    Play probe;
    std::vector<std::string> starters;
    for (const auto& weapon : probe.odyssey.catalogs().weapons) {
        if (weapon.starter) starters.push_back(weapon.name);
    }
    REQUIRE(starters.size() == 16);
    for (const std::string& name : starters) {
        CAPTURE(name);
        Play play;
        play.hold(name);
        const game::WeaponDef* weapon = play.odyssey.heldWeapon();
        REQUIRE(weapon != nullptr);
        const auto& goblin = play.odyssey.enemies().at(0);
        const double stand = game::behaviourOf(weapon->weaponClass).melee() ? 40.0 : 64.0;
        while (goblin.feetX() - play.odyssey.hero().feetX() > stand) play.tick(1, holding({Intent::MoveRight}));
        play.tick(1, holding({Intent::MoveRight})); // face east
        play.tick(1, pressing(Intent::Interact));
        play.tick(12); // shots fly
        CHECK(goblin.hp() == goblin.maxHp() - weapon->damage);
    }
}

TEST_CASE("US-133 Starter set") {
    // Swapping a starter is a JSON edit: no code change.
    const fs::path data = fs::temp_directory_path() / "odysseus-us133" / "data";
    fs::remove_all(data);
    fs::create_directories(data);
    for (const auto& entry : fs::directory_iterator(ODYSSEUS_DATA_DIR)) fs::copy(entry.path(), data / entry.path().filename());
    std::stringstream text;
    text << std::ifstream(data / "weapons.json").rdbuf();
    std::string json = text.str();
    const auto swap = [&](const std::string& name, bool starter) {
        const std::size_t at = json.find("\"name\":\"" + name + "\"");
        REQUIRE(at != std::string::npos);
        const std::size_t flag = json.find("\"starter\":", at);
        json.replace(flag, json.find(',', flag) - flag, starter ? "\"starter\":true" : "\"starter\":false");
    };
    swap("iron sword", false);
    swap("katana", true);
    std::ofstream(data / "weapons.json", std::ios::trunc) << json;
    Play play(data);
    std::set<std::string> carried;
    for (std::size_t i = 0; i < play.odyssey.carriedCount(); ++i) {
        carried.insert(play.odyssey.heldName());
        play.tick(1, pressing(Intent::SwitchWeapon));
    }
    CHECK(carried.size() == 18); // spear throw, sword, 16 starters
    CHECK(carried.contains("katana"));
    CHECK_FALSE(carried.contains("iron sword"));
}

TEST_CASE("US-133 In hand") {
    Play play;
    play.hold("iron sword");
    // Walking in each of the 8 directions, the icon is drawn at the hand, 20 pixels across,
    // from the mirrored icons when the hero faces west.
    const std::vector<std::vector<Intent>> directions{{Intent::MoveDown}, {Intent::MoveDown, Intent::MoveLeft}, {Intent::MoveLeft}, {Intent::MoveUp, Intent::MoveLeft},
                                                      {Intent::MoveUp}, {Intent::MoveUp, Intent::MoveRight}, {Intent::MoveRight}, {Intent::MoveDown, Intent::MoveRight}};
    std::set<int> textures;
    for (std::size_t d = 0; d < directions.size(); ++d) {
        Intents intents;
        for (const Intent intent : directions[d]) intents.set(intent, true, false);
        play.tick(2, intents);
        CHECK(static_cast<int>(play.odyssey.hero().facing()) == static_cast<int>(d));
        play.renderer.clear();
        play.odyssey.render(play.renderer, 1.0);
        int icons = 0;
        for (const auto& draw : play.renderer.draws()) {
            if (draw.styled && draw.destination.width == 20) {
                ++icons;
                textures.insert(draw.texture);
            }
        }
        CHECK(icons == 1);
    }
    CHECK(textures.size() == 2); // the icons, and the mirrored icons for west
}
