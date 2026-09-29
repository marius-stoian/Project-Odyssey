#include "game/odyssey_game.h"

#include "core/version.h"
#include "game/placeholder_art.h"
#include "game/test_map.h"

#include <cmath>
#include <string>

namespace odysseus::game {

namespace {
constexpr int kVirtualWidth = 480;
constexpr int kVirtualHeight = 270;
} // namespace

OdysseyGame::OdysseyGame()
    : map_(makeTestMap()), camera_(kVirtualWidth, kVirtualHeight, map_.pixelWidth(), map_.pixelHeight()),
      hero_(map_.pixelWidth() / 2.0 + kTileSize / 2.0, map_.pixelHeight() / 2.0 + kTileSize * 0.75) {
    camera_.centreOn(hero_.feetX(), hero_.feetY());
}

void OdysseyGame::update(const luna::engine::Intents& intents) {
    ++ticks_;
    hero_.update(intents, map_);
    camera_.follow(hero_.feetX(), hero_.feetY());
}

void OdysseyGame::start(luna::engine::Renderer& renderer) {
    characters_ = renderer.createTexture(makeCharacterSheet());
    tiles_ = renderer.createTexture(makeTileSheet());
}

void OdysseyGame::render(luna::engine::Renderer& renderer, double alpha) {
    map_.draw(renderer, tiles_, camera_, alpha);
    // The hero, blended between ticks like the camera, so walking looks smooth at 60 FPS.
    const luna::engine::Rect view = camera_.view(alpha);
    const int x = static_cast<int>(std::lround(hero_.feetX(alpha))) - kCharacterWidth / 2 - view.x;
    const int y = static_cast<int>(std::lround(hero_.feetY(alpha))) - kCharacterHeight - view.y;
    renderer.draw(characters_, hero_.spriteFrame(), {x, y});
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
