// US-135 Elements: fire and poison over time, ice slows, lightning chains, void drains.
#include "game/level.h"
#include "game/odyssey_game.h"
#include "game/status.h"
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

namespace {

Intents holding(Intent intent) {
    Intents intents;
    intents.set(intent, true, false);
    return intents;
}

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// The demo level with goblins where the test wants them: the first 2 m east of the hero, the
// second 2 m south of the first (within the 3 m jump), the third far away (out of the jump).
fs::path levelWith(const std::string& name, int goblins) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us135" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    const game::PlacedCharacter first = level.characters.at(0);
    level.characters.clear();
    level.pickups.clear(); // the demo's pickups would share the goblins' ids
    const game::PixelPoint spots[] = {{first.feet.x, first.feet.y}, {first.feet.x, first.feet.y + 64}, {first.feet.x + 320, first.feet.y}};
    for (int i = 0; i < goblins; ++i) {
        game::PlacedCharacter goblin = first;
        goblin.id = i + 1;
        goblin.feet = spots[i];
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

    const game::Enemy& goblin(std::size_t index = 0) const { return odyssey.enemies().at(index); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
    // Takes the weapon, walks up to the first goblin, and is ready to strike with it.
    void arm(const std::string& weapon) {
        REQUIRE(odyssey.pickUp(weapon));
        odyssey.selectSlot(static_cast<int>(std::find(odyssey.hotbar().begin(), odyssey.hotbar().end(), weapon) - odyssey.hotbar().begin()));
        while (goblin().feetX() - odyssey.hero().feetX() > 40.0) tick(1, holding(Intent::MoveRight));
    }
    void attack() { tick(1, pressing(Intent::Interact)); }
    int damage(const std::string& weapon) const { return odyssey.catalogs().weapon(weapon)->damage; }
};

} // namespace

TEST_CASE("US-135 Numbers") {
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    CHECK(catalogs.element(game::Element::Fire).perSecond == 2.0);
    CHECK(catalogs.element(game::Element::Fire).seconds == 3.0);
    CHECK(catalogs.element(game::Element::Ice).slowTo == 0.5);
    CHECK(catalogs.element(game::Element::Lightning).chainMetres == 3.0);
    CHECK(catalogs.element(game::Element::Lightning).chainFraction == 0.5);
    CHECK(catalogs.element(game::Element::Poison).perSecond == 1.0);
    CHECK(catalogs.element(game::Element::Poison).seconds == 5.0);
    CHECK(catalogs.element(game::Element::Void).drainFraction == 0.25);
}

TEST_CASE("US-135 Status effects") {
    const game::Catalogs catalogs = game::loadCatalogs(ODYSSEUS_DATA_DIR);
    SUBCASE("burning loses 2 HP a second for 3 seconds, then stops") {
        game::StatusEffects status;
        status.apply(game::Element::Fire, catalogs.element(game::Element::Fire));
        CHECK(status.burning());
        int lost = 0;
        for (int i = 0; i < 60; ++i) lost += status.tick();
        CHECK(lost == 6);
        CHECK_FALSE(status.any());
        CHECK(status.tick() == 0);
    }
    SUBCASE("poison loses 1 HP a second for 5 seconds") {
        game::StatusEffects status;
        status.apply(game::Element::Poison, catalogs.element(game::Element::Poison));
        int lost = 0;
        for (int i = 0; i < 100; ++i) lost += status.tick();
        CHECK(lost == 5);
        CHECK_FALSE(status.poisoned());
    }
    SUBCASE("ice halves the speed for 2 seconds") {
        game::StatusEffects status;
        CHECK(status.speed() == 1.0);
        status.apply(game::Element::Ice, catalogs.element(game::Element::Ice));
        CHECK(status.speed() == 0.5);
        for (int i = 0; i < 39; ++i) status.tick();
        CHECK(status.slowed());
        status.tick();
        CHECK_FALSE(status.slowed());
        CHECK(status.speed() == 1.0);
    }
    SUBCASE("a new hit of the same element starts the timer again, without stacking") {
        game::StatusEffects status;
        status.apply(game::Element::Fire, catalogs.element(game::Element::Fire));
        for (int i = 0; i < 40; ++i) status.tick();
        status.apply(game::Element::Fire, catalogs.element(game::Element::Fire));
        int lost = 0;
        for (int i = 0; i < 60; ++i) lost += status.tick();
        CHECK(lost == 6);
    }
}

TEST_CASE("US-135 Fire") {
    Play play(levelWith("fire", 1));
    play.arm("flame sword");
    play.attack();
    const int afterHit = play.goblin().hp();
    CHECK(afterHit == 100 - play.damage("flame sword"));
    CHECK(play.goblin().status.burning());
    play.tick(20);
    CHECK(play.goblin().hp() == afterHit - 2);
    play.tick(40);
    CHECK(play.goblin().hp() == afterHit - 6); // 3 seconds, 2 HP each
    CHECK_FALSE(play.goblin().status.burning());
    play.tick(20);
    CHECK(play.goblin().hp() == afterHit - 6);
}

TEST_CASE("US-135 Fire shows") {
    Play play(levelWith("fire-shows", 1));
    play.arm("flame sword");
    play.attack();
    play.tick(12); // the hit's own effects are over; the burning one is drawn again every 6 ticks
    CHECK(play.odyssey.effects().count() > 0);
    play.tick(80);
    CHECK(play.odyssey.effects().count() == 0); // burning ended: nothing left to show
}

TEST_CASE("US-135 Poison") {
    Play play(levelWith("poison", 1));
    play.arm("venom sword");
    play.attack();
    const int afterHit = play.goblin().hp();
    CHECK(play.goblin().status.poisoned());
    play.tick(20);
    CHECK(play.goblin().hp() == afterHit - 1);
    play.tick(80);
    CHECK(play.goblin().hp() == afterHit - 5);
    CHECK_FALSE(play.goblin().status.poisoned());
}

TEST_CASE("US-135 Ice") {
    Play play(levelWith("ice", 1));
    play.arm("frost sword");
    play.attack();
    REQUIRE(play.goblin().status.slowed());
    CHECK(play.goblin().isWindingUp());
    play.tick(10);
    CHECK(play.goblin().isWindingUp()); // slowed: the wind-up takes 20 ticks, not 10
    CHECK(play.odyssey.heroHp() == 100);
    play.tick(10);
    CHECK_FALSE(play.goblin().isWindingUp());
    CHECK(play.odyssey.heroHp() == 100 - play.goblin().swordDamage);
}

TEST_CASE("US-135 Lightning") {
    SUBCASE("jumps to the nearest enemy within 3 m for half the damage") {
        Play play(levelWith("lightning", 3));
        play.arm("lightning spear");
        const int damage = play.damage("lightning spear");
        play.attack();
        CHECK(play.goblin(0).hp() == 100 - damage);
        CHECK(play.goblin(1).hp() == 100 - (damage + 1) / 2);
        CHECK(play.goblin(2).hp() == 100); // 10 m away: out of the jump
    }
    SUBCASE("with nobody near, nothing jumps") {
        Play play(levelWith("lightning-alone", 1));
        play.arm("lightning spear");
        play.attack();
        CHECK(play.goblin(0).hp() == 100 - play.damage("lightning spear"));
    }
}

TEST_CASE("US-135 Void") {
    Play play(levelWith("void", 1));
    play.arm("void katana");
    const int damage = play.damage("void katana");
    play.attack();
    CHECK(play.odyssey.heroHp() == 100); // already full: nothing to heal yet
    play.tick(10);                       // the goblin strikes back
    const int hurt = 100 - play.goblin().swordDamage;
    REQUIRE(play.odyssey.heroHp() == hurt);
    play.tick(10); // the blade rests between swings
    play.attack();
    const int drained = std::max(1, static_cast<int>(std::lround(damage * 0.25)));
    CHECK(play.odyssey.heroHp() == hurt + drained);
    CHECK(play.goblin().hp() == 100 - 2 * damage);
}

TEST_CASE("US-135 Bad numbers") {
    const fs::path data = fs::temp_directory_path() / "odysseus-us135" / "data";
    fs::remove_all(data);
    fs::create_directories(data);
    for (const auto& entry : fs::directory_iterator(ODYSSEUS_DATA_DIR)) fs::copy(entry.path(), data / entry.path().filename());
    std::string text;
    {
        std::ifstream in(data / "weapons.json");
        text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    const std::string good = "\"effect\":\"ember\"";
    const std::size_t at = text.find(good);
    REQUIRE(at != std::string::npos);
    text.replace(at, good.size(), "\"effect\":\"no such effect\"");
    std::ofstream(data / "weapons.json", std::ios::trunc) << text;
    CHECK_THROWS_WITH_AS(game::loadCatalogs(data), doctest::Contains("elements.fire.effect"), odysseus::sim::DataError);
}
