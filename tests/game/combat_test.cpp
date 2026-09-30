// US-131 Hero HP, fighting back and death.
#include "game/level.h"
#include "game/odyssey_game.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <filesystem>

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

// The demo level, with the goblin's strike changed when asked (US-131 tests a fall).
fs::path demoCopy(const std::string& name, int goblinStrike = -1) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us131" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    if (goblinStrike < 0) {
        fs::copy_file(ODYSSEUS_DEMO_LEVEL, folder / "level.json");
    } else {
        const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.at(0).swordDamage = goblinStrike;
        game::saveLevel(level, definitions, folder / "level.json");
    }
    return folder / "level.json";
}

struct Play {
    game::OdysseyGame odyssey;
    luna::engine::RecordingRenderer renderer;
    explicit Play(const fs::path& level) : odyssey(ODYSSEUS_DATA_DIR, level) { odyssey.start(renderer); }

    const game::Enemy& goblin() const { return odyssey.enemies().at(0); }
    double gap() const { return goblin().feetX() - odyssey.hero().feetX(); }
    void tick(int count = 1, Intents intents = {}) {
        for (int i = 0; i < count; ++i) odyssey.update(intents);
    }
    // Walk east until the goblin is within sword reach (1.5 m), then draw the sword.
    void closeIn() {
        while (gap() > 40.0) tick(1, holding(Intent::MoveRight));
        tick(1, pressing(Intent::SwitchWeapon)); // the spear is first; Shift draws the sword
    }
};

} // namespace

TEST_CASE("US-131 Strike back") {
    Play play(demoCopy("strike"));
    REQUIRE(play.odyssey.heroHp() == 100);
    play.closeIn();
    const int strike = play.goblin().swordDamage;
    play.tick(1, pressing(Intent::Interact));
    CHECK(play.goblin().hp() < play.goblin().maxHp());
    CHECK(play.goblin().isWindingUp()); // the warning: half a second to step away
    play.tick(9);
    CHECK(play.odyssey.heroHp() == 100);
    play.tick(1);
    CHECK_FALSE(play.goblin().isWindingUp());
    CHECK(play.odyssey.heroHp() == 100 - strike);
    // The next swing (the sword rests 25 ticks between swings) starts the next wind-up.
    play.tick(15);
    play.tick(1, pressing(Intent::Interact));
    play.tick(10);
    CHECK(play.odyssey.heroHp() == 100 - 2 * strike);
}

TEST_CASE("US-131 One strike per wind-up") {
    // A hit during the wind-up does not restart it: the strike still comes 10 ticks after the first.
    game::Enemy enemy(0.0, 0.0, 100);
    enemy.provoke();
    for (int i = 0; i < 5; ++i) CHECK_FALSE(enemy.update());
    enemy.provoke();
    for (int i = 0; i < 4; ++i) CHECK_FALSE(enemy.update());
    CHECK(enemy.update());
    CHECK_FALSE(enemy.isWindingUp());
}

TEST_CASE("US-131 Out of reach") {
    Play play(demoCopy("dodge"));
    play.closeIn();
    play.tick(1, pressing(Intent::Interact));
    REQUIRE(play.goblin().isWindingUp());
    // Step away during the wind-up: the strike falls short.
    play.tick(10, holding(Intent::MoveLeft));
    CHECK(play.gap() > 1.5 * game::kTileSize);
    CHECK(play.odyssey.heroHp() == 100);
    CHECK_FALSE(play.goblin().isWindingUp());
}

TEST_CASE("US-131 Death and respawn") {
    SUBCASE("the hero falls and starts again") {
        Play play(demoCopy("fall", 150));
        const auto start = play.odyssey.level().heroStart;
        play.closeIn();
        play.tick(1, pressing(Intent::Interact));
        play.tick(10);
        CHECK(play.odyssey.heroHp() == 0);
        CHECK(play.odyssey.heroRespawning());
        play.tick(5, holding(Intent::MoveRight)); // no walking while fallen
        CHECK(play.odyssey.hero().feetX() > start.x);
        play.tick(15);
        CHECK_FALSE(play.odyssey.heroRespawning());
        CHECK(play.odyssey.heroHp() == 100);
        CHECK(play.odyssey.hero().feetX() == doctest::Approx(start.x));
        CHECK(play.odyssey.hero().feetY() == doctest::Approx(start.y));
        CHECK(play.goblin().hp() < play.goblin().maxHp()); // enemies keep their HP
    }
    SUBCASE("the enemy dies and strikes no more") {
        Play play(demoCopy("win", 1));
        play.closeIn();
        while (play.goblin().isAlive()) {
            play.tick(1, pressing(Intent::Interact));
            play.tick(12); // the swing ends and any strike lands
        }
        const int hp = play.odyssey.heroHp();
        play.tick(1, pressing(Intent::Interact));
        play.tick(20);
        CHECK(play.odyssey.heroHp() == hp);
        CHECK_FALSE(play.goblin().isWindingUp());
    }
}

TEST_CASE("US-132 Combat effects") {
    Play play(demoCopy("effects", -1));
    REQUIRE(play.odyssey.catalogs().effect("spark") != nullptr);
    play.closeIn();
    CHECK(play.odyssey.effects().count() == 0);
    play.tick(1, pressing(Intent::Interact));
    CHECK(play.odyssey.effects().count() == 1); // a hit spark where the blade landed
    play.tick(20);
    CHECK(play.odyssey.effects().count() == 0); // one-shot: gone when played
    // A throw: dust puffs trail the spear while it flies.
    play.tick(1, pressing(Intent::SwitchWeapon));
    play.tick(1, pressing(Intent::Interact));
    int most = 0;
    for (int i = 0; i < 20; ++i) {
        play.tick();
        most = std::max(most, static_cast<int>(play.odyssey.effects().count()));
    }
    CHECK(most >= 2);
}

TEST_CASE("US-132 Death smoke") {
    Play play(demoCopy("smoke", 0)); // a goblin that strikes for nothing
    play.closeIn();
    bool smoke = false;
    while (play.goblin().isAlive()) {
        play.tick(1, pressing(Intent::Interact));
        if (!play.goblin().isAlive()) smoke = play.odyssey.effects().count() == 2; // spark and smoke
        play.tick(25);
    }
    CHECK(smoke);
}
