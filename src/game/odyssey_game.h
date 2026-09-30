#pragma once

#include "boundary.h"

#include "luna/engine/application.h"
#include "luna/engine/camera.h"
#include "luna/engine/game.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"

#include "game/art.h"
#include "game/enemy.h"
#include "game/hero.h"
#include "game/level.h"
#include "game/spear_range.h"
#include "game/sword.h"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace odysseus::game {

enum class WeaponType { Sword, Bow };

// Project Odyssey as Luna sees it. It grows story by story: a window now, a walking
// character by the end of M1, the living clan in M3.
class OdysseyGame final : public luna::engine::Game {
public:
    // Content (materials.json, tiles.json, characters.json) is read from `dataDirectory`; the
    // level from `levelFile` (default: assets/levels/valley.json next to the data folder).
    explicit OdysseyGame(const std::filesystem::path& dataDirectory, const std::filesystem::path& levelFile = {});

    void start(luna::engine::Renderer& renderer) override;
    void update(const luna::engine::Intents& intents) override;
    void render(luna::engine::Renderer& renderer, double alpha) override;

    std::uint64_t ticks() const;
    const Hero& hero() const { return hero_; }
    const SpearRange& range() const { return range_; }
    const Level& level() const { return level_; }
    const Definitions& definitions() const { return definitions_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::filesystem::path& levelFile() const { return levelFile_; }

    // Where the demo's straw targets stand (US-029), in metres: one 8 tiles west of the
    // hero's start (the view is 15 tiles wide, so the whole throw fits on screen), one
    // 8 tiles north behind a boulder.
    static luna::physics::Vec3 openTargetBase();
    static luna::physics::Vec3 blockedTargetBase();

private:
    std::uint64_t ticks_ = 0;
    luna::engine::Texture characters_;
    luna::engine::Texture tiles_;
    luna::engine::Texture props_;
    luna::engine::Texture charactersAtlas_;
    luna::engine::Texture charactersHitAtlas_;
    ArtSet art_;
    // Declared in this order on purpose: each is built from the ones above it.
    Definitions definitions_;
    std::filesystem::path levelFile_;
    Level level_;
    luna::engine::TileMap map_;
    luna::engine::Camera camera_;
    Hero hero_;
    SpearRange range_;
    std::filesystem::path spritesDirectory_;
    Sword sword_;
    std::vector<Enemy> enemies_;
    WeaponType currentWeapon_ = WeaponType::Bow; // the spear demo (US-029) is the default; Shift switches to the sword
    bool nextSpearIsFlint_ = true; // Interact alternates flint and wooden spears
    // After a throw the camera frames the hero and the target together for a while.
    int framingTicks_ = 0;
    double framingX_ = 0.0;
    double framingY_ = 0.0;
};

// Window title, sizes and colours for Luna.
luna::engine::AppConfig odysseyAppConfig();

} // namespace odysseus::game
