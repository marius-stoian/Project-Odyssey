// US-139 Mouse aiming: the hero faces the pointer, Attack goes toward it, Interact goes along the facing.
#include "game/level.h"
#include "game/odyssey_game.h"
#include "game/weapons.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
namespace game = odysseus::game;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Pointer;
using luna::engine::PointerButton;

namespace {

// The camera centres on the hero at the start, so the hero stands at the middle of the 480x270 picture.
constexpr int kHeroScreenX = 240;
constexpr int kHeroScreenY = 135;

Intents pointingAt(int x, int y, bool attack = false) {
    Intents intents;
    Pointer pointer;
    pointer.x = x;
    pointer.y = y;
    if (attack) {
        pointer.held[static_cast<std::size_t>(PointerButton::Left)] = true;
        pointer.pressed[static_cast<std::size_t>(PointerButton::Left)] = true;
        intents.set(Intent::Attack, true, true);
    }
    intents.setPointer(pointer);
    return intents;
}

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// The demo level with one goblin placed at an offset from the hero start.
fs::path levelWithGoblinAt(const std::string& name, int offsetX, int offsetY) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us139" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const game::Definitions definitions = game::loadDefinitions(ODYSSEUS_DATA_DIR);
    game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
    game::PlacedCharacter goblin = level.characters.at(0);
    goblin.feet = {level.heroStart.x + offsetX, level.heroStart.y + offsetY};
    goblin.hp = 100;
    level.characters = {goblin};
    level.pickups.clear();
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
    const game::Enemy& goblin() const { return odyssey.enemies().at(0); }
};

} // namespace

TEST_CASE("US-139 Facing from a direction") {
    CHECK(game::facingToward(1, 0) == game::Facing::East);
    CHECK(game::facingToward(1, 1) == game::Facing::SouthEast);
    CHECK(game::facingToward(0, 1) == game::Facing::South);
    CHECK(game::facingToward(-1, 1) == game::Facing::SouthWest);
    CHECK(game::facingToward(-1, 0) == game::Facing::West);
    CHECK(game::facingToward(-1, -1) == game::Facing::NorthWest);
    CHECK(game::facingToward(0, -1) == game::Facing::North);
    CHECK(game::facingToward(1, -1) == game::Facing::NorthEast);
    CHECK(game::facingToward(1.0, 0.3) == game::Facing::East);   // 17 degrees: still east
    CHECK(game::facingToward(1.0, 0.5) == game::Facing::SouthEast); // 27 degrees: past the 22.5-degree edge
    CHECK(game::facingToward(1.0, -0.3) == game::Facing::East);
}

TEST_CASE("US-139 Face the cursor") {
    Play play(levelWithGoblinAt("face", 200, 0));
    play.hold("iron sword");
    play.tick(1, pointingAt(kHeroScreenX - 60, kHeroScreenY));
    CHECK(play.odyssey.aiming());
    CHECK(play.odyssey.hero().facing() == game::Facing::West);
    CHECK(play.odyssey.aimDirectionX() == doctest::Approx(-1.0));
    play.tick(1, pointingAt(kHeroScreenX + 50, kHeroScreenY - 50));
    CHECK(play.odyssey.hero().facing() == game::Facing::NorthEast);
    CHECK(play.odyssey.aimDirectionX() == doctest::Approx(std::sqrt(0.5)));
    CHECK(play.odyssey.aimDirectionY() == doctest::Approx(-std::sqrt(0.5)));
    // The aim line and crosshair are drawn while aiming.
    play.renderer.clear();
    play.odyssey.render(play.renderer, 1.0);
    const std::size_t aimed = play.renderer.draws().size();
    // Off the picture nothing aims, and the line and crosshair are gone.
    play.tick(1, Intents{});
    CHECK_FALSE(play.odyssey.aiming());
    play.renderer.clear();
    play.odyssey.render(play.renderer, 1.0);
    CHECK(aimed > play.renderer.draws().size());
}

TEST_CASE("US-139 Swing toward the cursor") {
    // A goblin 30 px east and 30 px north of the hero (42 px away, inside the sword's 48 px reach).
    SUBCASE("pointer north-east: hit") {
        Play play(levelWithGoblinAt("ne-hit", 30, -30));
        play.hold("iron sword");
        play.tick(1, pointingAt(kHeroScreenX + 60, kHeroScreenY - 60, true));
        CHECK(play.goblin().hp() == 100 - play.odyssey.catalogs().weapon("iron sword")->damage);
    }
    SUBCASE("pointer south-west: miss") {
        Play play(levelWithGoblinAt("ne-miss", 30, -30));
        play.hold("iron sword");
        play.tick(1, pointingAt(kHeroScreenX - 60, kHeroScreenY + 60, true));
        CHECK(play.goblin().hp() == 100);
    }
    SUBCASE("a swing at an exact angle, not the nearest facing") {
        // 20 degrees south of east is still "east" for the picture, but the arc is centred on 20 degrees:
        // a goblin 40 px away at 20 degrees south of east is in the 90-degree arc either way; one at 60 degrees off is not.
        Play play(levelWithGoblinAt("angle", 0, -40)); // due north of the hero
        play.hold("iron sword");
        play.tick(1, pointingAt(kHeroScreenX + 60, kHeroScreenY + 22, true)); // aimed east-south-east, 90 degrees from the goblin
        CHECK(play.goblin().hp() == 100);
    }
}

TEST_CASE("US-139 Keys still work") {
    // Interact with the pointer off the picture goes along the facing (south at the start), as before.
    Play play(levelWithGoblinAt("keys", 0, 40));
    play.hold("iron sword");
    play.tick(1, pressing(Intent::Interact));
    CHECK_FALSE(play.odyssey.aiming());
    CHECK(play.goblin().hp() == 100 - play.odyssey.catalogs().weapon("iron sword")->damage);
}

TEST_CASE("US-139 Interact goes along the facing even with the pointer elsewhere") {
    Play play(levelWithGoblinAt("keys-pointer", 0, 40));
    play.hold("iron sword");
    Intents intents = pointingAt(kHeroScreenX, kHeroScreenY + 60); // the hero faces south toward the pointer
    intents.set(Intent::Interact, true, true);
    play.tick(1, intents);
    CHECK(play.odyssey.hero().facing() == game::Facing::South);
    CHECK(play.goblin().hp() == 100 - play.odyssey.catalogs().weapon("iron sword")->damage);
}
