#include "game/odyssey_game.h"

#include "core/version.h"
#include "game/placeholder_art.h"

#include <string>

namespace odysseus::game {

void OdysseyGame::update(const luna::engine::Intents& /*intents*/) {
    ++ticks_; // US-024 moves the character with the intents
}

void OdysseyGame::start(luna::engine::Renderer& renderer) {
    characters_ = renderer.createTexture(makeCharacterSheet());
    tiles_ = renderer.createTexture(makeTileSheet());
}

void OdysseyGame::render(luna::engine::Renderer& renderer, double /*alpha*/) {
    // US-022: the hero, idle and facing the player, in the middle of the 480x270 screen.
    // US-023 adds the map around them, US-024 makes them walk.
    const luna::engine::Rect idleSouth{0, 0, kCharacterWidth, kCharacterHeight};
    renderer.draw(characters_, idleSouth, {(480 - kCharacterWidth) / 2, (270 - kCharacterHeight) / 2});
}

std::uint64_t OdysseyGame::ticks() const {
    return ticks_;
}

luna::engine::AppConfig odysseyAppConfig() {
    luna::engine::AppConfig config;
    config.title = "Project Odyssey " + std::string(core::versionString());
    config.windowWidth = 1280; // US-020: a 1280 x 720 window
    config.windowHeight = 720;
    config.virtualWidth = 480; // US-022: pixel art drawn at 480 x 270, scaled up
    config.virtualHeight = 270;
    config.ticksPerSecond = 20; // ADR-006
    config.clearRed = 34;       // a deep green-blue, like dusk over the valley
    config.clearGreen = 52;
    config.clearBlue = 60;
    return config;
}

} // namespace odysseus::game
