#pragma once

#include "boundary.h"

#include "luna/engine/application.h"
#include "luna/engine/camera.h"
#include "luna/engine/game.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"

#include "game/hero.h"
#include "game/spear_range.h"

#include <cstdint>
#include <filesystem>

namespace odysseus::game {

// Project Odyssey as Luna sees it. It grows story by story: a window now, a walking
// character by the end of M1, the living clan in M3.
class OdysseyGame final : public luna::engine::Game {
public:
    // Content (materials.json) is read from `dataDirectory`.
    explicit OdysseyGame(const std::filesystem::path& dataDirectory);

    void start(luna::engine::Renderer& renderer) override;
    void update(const luna::engine::Intents& intents) override;
    void render(luna::engine::Renderer& renderer, double alpha) override;

    std::uint64_t ticks() const;
    const Hero& hero() const { return hero_; }
    const SpearRange& range() const { return range_; }

    // Where the demo's straw targets stand (US-029), in metres: one 8 tiles south of the
    // hero's start, one 8 tiles west behind a boulder.
    static luna::physics::Vec3 openTargetBase();
    static luna::physics::Vec3 blockedTargetBase();

private:
    std::uint64_t ticks_ = 0;
    luna::engine::Texture characters_;
    luna::engine::Texture tiles_;
    luna::engine::Texture props_;
    luna::engine::TileMap map_;
    luna::engine::Camera camera_;
    Hero hero_;
    SpearRange range_;
    bool nextSpearIsFlint_ = true; // Interact alternates flint and wooden spears
};

// Window title, sizes and colours for Luna.
luna::engine::AppConfig odysseyAppConfig();

} // namespace odysseus::game
