// US-141 Bows, crossbows, thrown weapons and staff bolts: each class shoots its own way, elements apply, the range level plays.
#include "game/level.h"
#include "game/odyssey_game.h"
#include "sim/data.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Pointer;

namespace {

// The demo level with goblins placed relative to the hero start (first 4 m south, the others as given).
fs::path levelWithGoblins(const std::string& name, const std::vector<std::pair<int, int>>& offsetsInTiles) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us141" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    const game::PlacedCharacter first = level.characters.at(0);
    level.characters.clear();
    level.pickups.clear();
    int id = 1;
    for (const auto& [dx, dy] : offsetsInTiles) {
        game::PlacedCharacter goblin = first;
        goblin.id = id++;
        goblin.feet = {level.heroStart.x + dx * 32, level.heroStart.y + dy * 32};
        goblin.hp = 100;
        goblin.swordDamage = 4;
        level.characters.push_back(goblin);
    }
    game::saveLevel(level, definitions, folder / "level.json");
    return folder / "level.json";
}

struct Play {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Play(const fs::path& level) : odyssey(ODYSSEUS_DATA_DIR, level) { odyssey.start(renderer); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
    void hold(const std::string& weapon) {
        REQUIRE(odyssey.pickUp(weapon));
        odyssey.selectSlot(static_cast<int>(std::find(odyssey.hotbar().begin(), odyssey.hotbar().end(), weapon) - odyssey.hotbar().begin()));
    }
    // Fires at enemy `index` with the pointer on its feet.
    void shootAt(std::size_t index) {
        const game::Enemy& enemy = odyssey.enemies().at(index);
        Pointer pointer;
        const auto view = odyssey.cameraView();
        pointer.x = static_cast<int>(std::lround(enemy.feetX())) - view.x;
        pointer.y = static_cast<int>(std::lround(enemy.feetY())) - view.y;
        Intents intents;
        intents.set(Intent::Attack, true, true);
        intents.setPointer(pointer);
        tick(1, intents);
    }
    const game::Enemy& goblin(std::size_t index = 0) const { return odyssey.enemies().at(index); }
    int damage(const std::string& weapon) const { return odyssey.catalogs().weapon(weapon)->damage; }
};

} // namespace

TEST_CASE("US-141 Each ranged class shoots") {
    // A goblin 4 m south, out of its own reach (1.5 m), so nothing interferes.
    struct Case {
        const char* weapon;
        bool arcs;
    };
    for (const Case& shooter : {Case{"wooden longbow", true}, Case{"crossbow", true}, Case{"throwing knives", true}, Case{"nature staff", false}}) {
        CAPTURE(shooter.weapon);
        Play play(levelWithGoblins("class", {{0, 4}}));
        play.tick(30); // the camera settles on the hero
        play.hold(shooter.weapon);
        play.shootAt(0);
        // Arcs are physics shots with height; the staff's bolt is a flat projectile.
        CHECK(play.odyssey.arcShots().empty() == !shooter.arcs);
        CHECK(play.odyssey.projectiles().empty() == shooter.arcs);
        play.tick(30);
        const int damage = play.damage(shooter.weapon);
        CHECK(play.goblin().hp() <= 100 - damage);
        CHECK(play.goblin().hp() > 100 - damage - 6); // no more than the hit and a status
    }
}

TEST_CASE("US-141 Every ranged starter is shootable from the hotbar") {
    Play probe(levelWithGoblins("probe", {{0, 4}}));
    std::vector<std::string> starters;
    for (const auto& weapon : probe.odyssey.catalogs().weapons) {
        if (weapon.starter && (weapon.weaponClass == game::WeaponClass::Bow || weapon.weaponClass == game::WeaponClass::Thrown ||
                               weapon.weaponClass == game::WeaponClass::Staff)) {
            starters.push_back(weapon.name);
        }
    }
    CHECK(starters.size() == 6); // two bows, two thrown, two staffs
    for (const std::string& name : starters) {
        CAPTURE(name);
        Play play(levelWithGoblins("starter", {{0, 4}}));
        play.tick(30);
        play.hold(name);
        play.shootAt(0);
        play.tick(30);
        CHECK(play.goblin().hp() <= 100 - play.damage(name));
    }
}

TEST_CASE("US-141 Elements on shots") {
    SUBCASE("a poison arrow poisons the goblin") {
        Play play(levelWithGoblins("poison-arrow", {{0, 4}}));
        play.tick(30);
        play.hold("venom recurve");
        play.shootAt(0);
        for (int i = 0; i < 30 && play.odyssey.arcShots().size() > 0 && !play.goblin().status.poisoned(); ++i) play.tick();
        CHECK(play.goblin().status.poisoned());
    }
    SUBCASE("a frost arrow slows the goblin") {
        Play play(levelWithGoblins("frost-arrow", {{0, 4}}));
        play.tick(30);
        play.hold("frost bow");
        play.shootAt(0);
        for (int i = 0; i < 30 && !play.goblin().status.slowed(); ++i) play.tick();
        CHECK(play.goblin().status.slowed());
    }
    SUBCASE("a lightning bolt from a staff chains to a second goblin") {
        Play play(levelWithGoblins("storm", {{0, 4}, {2, 4}}));
        play.tick(30);
        play.hold("storm staff");
        play.shootAt(0);
        play.tick(30);
        CHECK(play.goblin(0).hp() < 100);
        CHECK(play.goblin(1).hp() < 100); // the jump, 2 m to the side
    }
}

TEST_CASE("US-141 Speeds come from weapons.json") {
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    CHECK(catalogs.weaponClass(game::WeaponClass::Bow).launchSpeed == 16.0);
    CHECK(catalogs.weaponClass(game::WeaponClass::Thrown).launchSpeed == 10.0);
    CHECK(catalogs.weaponClass(game::WeaponClass::Staff).launchSpeed == 12.0);
    CHECK(catalogs.weaponClass(game::WeaponClass::Sword).launchSpeed == 0.0);
    // Changing the number in the file changes the shot, and a missing entry is named.
    const fs::path data = fs::temp_directory_path() / "odysseus-us141" / "data";
    fs::remove_all(data);
    fs::create_directories(data);
    for (const auto& entry : fs::directory_iterator(ODYSSEUS_DATA_DIR)) fs::copy(entry.path(), data / entry.path().filename());
    std::string text;
    {
        std::ifstream in(data / "weapons.json");
        text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    const std::string bow = "\"bow\": {\"launchSpeed\":16}";
    REQUIRE(text.find(bow) != std::string::npos);
    std::string faster = text;
    faster.replace(faster.find(bow), bow.size(), "\"bow\": {\"launchSpeed\":20}");
    std::ofstream(data / "weapons.json", std::ios::trunc) << faster;
    CHECK(game::loadCatalogs(data).weaponClass(game::WeaponClass::Bow).launchSpeed == 20.0);
    std::string missing = text;
    missing.replace(missing.find(bow), bow.size(), "\"arrow\": {\"launchSpeed\":16}");
    std::ofstream(data / "weapons.json", std::ios::trunc) << missing;
    CHECK_THROWS_WITH_AS(game::loadCatalogs(data), doctest::Contains("classes.bow"), odysseus::sim::DataError);
}

TEST_CASE("US-141 Shooting range") {
    const fs::path range = fs::path(ODYSSEUS_DEMO_LEVEL).parent_path() / "range.json";
    REQUIRE(fs::exists(range));
    Play play(range);
    Intents walking;
    walking.set(Intent::MoveRight, true, false);
    play.tick(40, walking); // walking east along the row of pickups puts them in the hotbar
    play.tick(30);          // the camera settles
    CHECK(play.odyssey.carriedCount() == 7);
    CHECK(play.odyssey.enemies().size() >= 3); // goblins in the open and behind rocks
    // Shoot the goblin standing in the open south-east of the start (the second) with a bow.
    play.hold("wooden longbow");
    const game::Enemy& target = play.goblin(1);
    const int before = target.hp();
    play.shootAt(1);
    play.tick(40);
    CHECK(target.hp() < before);
}
