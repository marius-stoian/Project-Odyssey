// US-123 Game mode and Editor mode.
#include "game/odyssey_game.h"

#include "luna/engine/renderer.h"

#include <doctest/doctest.h>

namespace game = odysseus::game;
using luna::engine::Intent;
using luna::engine::Intents;

namespace {

Intents pressing(Intent intent) {
    Intents intents;
    intents.set(intent, true, true);
    return intents;
}

Intents holding(Intent intent) {
    Intents intents;
    intents.set(intent, true, false);
    return intents;
}

} // namespace

TEST_CASE("US-123 Switch") {
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR);
    luna::engine::RecordingRenderer renderer;
    odyssey.start(renderer);
    CHECK(odyssey.mode() == game::Mode::Game);
    odyssey.update(pressing(Intent::ModeEditor));
    REQUIRE(odyssey.mode() == game::Mode::Editor);

    // The world pauses: the hero does not walk, the game's clock does not run.
    const double heroX = odyssey.hero().feetX();
    const auto ticks = odyssey.ticks();
    const double cameraX = odyssey.editor().centreX();
    for (int i = 0; i < 10; ++i) odyssey.update(holding(Intent::MoveRight));
    CHECK(odyssey.hero().feetX() == doctest::Approx(heroX));
    CHECK(odyssey.ticks() == ticks);
    // The camera pans freely instead: ten ticks of the right key move it 80 pixels.
    CHECK(odyssey.editor().centreX() == doctest::Approx(cameraX + 10 * game::Editor::kPanPerTick));

    // A right-button drag slides the world with the pointer.
    Intents drag;
    drag.set(Intent::MoveUp, false, false);
    luna::engine::Pointer pointer;
    pointer.x = 200;
    pointer.y = 100;
    pointer.held[static_cast<std::size_t>(luna::engine::PointerButton::Right)] = true;
    drag.setPointer(pointer);
    odyssey.update(drag);
    const double before = odyssey.editor().centreX();
    pointer.x = 170; // the pointer moves 30 pixels left: the view moves 30 pixels right
    drag.setPointer(pointer);
    odyssey.update(drag);
    CHECK(odyssey.editor().centreX() == doctest::Approx(before + 30));

    // Drawn: the level, the start marker and the mode label.
    renderer.clear();
    odyssey.render(renderer, 1.0);
    CHECK(renderer.draws().size() > 100);
}

TEST_CASE("US-123 Back to play") {
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR);
    luna::engine::RecordingRenderer renderer;
    odyssey.start(renderer);
    odyssey.update(pressing(Intent::ModeEditor));
    // The owner changes the level: a new hero start, water across the path, the goblin moved.
    game::Level& level = odyssey.editor().level();
    level.heroStart = {1200, 1100};
    level.set(40, 32, odyssey.definitions().tileNumber("water"));
    level.characters.front().feet = {1300, 1100};
    level.characters.front().hp = 7;
    odyssey.editor().levelChanged();
    odyssey.update(pressing(Intent::ModeGame));
    REQUIRE(odyssey.mode() == game::Mode::Game);
    // The game plays the edited level from the hero start.
    CHECK(odyssey.hero().feetX() == doctest::Approx(1200.0));
    CHECK(odyssey.hero().feetY() == doctest::Approx(1100.0));
    REQUIRE(odyssey.enemies().size() == 1);
    CHECK(odyssey.enemies().front().feetX() == doctest::Approx(1300.0));
    CHECK(odyssey.enemies().front().hp() == 7);
    CHECK(odyssey.range().spears().empty());
    // And it plays: the hero walks again.
    odyssey.update(holding(Intent::MoveLeft));
    odyssey.update(holding(Intent::MoveLeft));
    CHECK(odyssey.hero().feetX() < 1200.0);
}

TEST_CASE("US-123 Game untouched") {
    game::OdysseyGame odyssey(ODYSSEUS_DATA_DIR);
    luna::engine::RecordingRenderer renderer;
    odyssey.start(renderer);
    // F1 in Game mode changes nothing; the hero walks; the sword and spear still work.
    odyssey.update(pressing(Intent::ModeGame));
    CHECK(odyssey.mode() == game::Mode::Game);
    const double x = odyssey.hero().feetX();
    odyssey.update(holding(Intent::MoveLeft));
    CHECK(odyssey.hero().feetX() < x);
    odyssey.update(pressing(Intent::Interact)); // a spear (the bow is the default weapon)
    CHECK(odyssey.range().spears().size() == 1);
    // A round trip through the Editor restores the level's play state exactly.
    odyssey.update(pressing(Intent::ModeEditor));
    odyssey.update(pressing(Intent::ModeGame));
    CHECK(odyssey.hero().feetX() == doctest::Approx(1040.0));
    CHECK(odyssey.range().spears().empty());
}
