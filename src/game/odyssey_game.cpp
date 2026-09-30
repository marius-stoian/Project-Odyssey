#include "game/odyssey_game.h"

#include "core/log.h"
#include "core/version.h"
#include "game/placeholder_art.h"
#include "game/test_map.h"
#include "luna/engine/physics_view.h"

#include <cmath>
#include <format>
#include <string>

namespace odysseus::game {

namespace {
constexpr int kVirtualWidth = 480;
constexpr int kVirtualHeight = 270;
} // namespace

using luna::physics::Fixed;
using luna::physics::Vec3;

namespace {

// Where a spear is drawn between two ticks (alpha 0..1), like the hero and the camera.
Vec3 blended(const FlyingSpear& spear, double alpha) {
    const Fixed part = Fixed::fromRatio(static_cast<std::int64_t>(std::lround(alpha * 1000.0)), 1000);
    return spear.previousPosition + (spear.body.position - spear.previousPosition) * part;
}

} // namespace

// The hero starts at the crossing, feet at (32.5, 32.75) m.
Vec3 OdysseyGame::openTargetBase() {
    return {Fixed::fromRatio(49, 2), Fixed::fromRatio(131, 4), luna::physics::kFixedZero}; // (24.5, 32.75)
}

Vec3 OdysseyGame::blockedTargetBase() {
    return {Fixed::fromRatio(65, 2), Fixed::fromRatio(49, 2), luna::physics::kFixedZero}; // (32.5, 24.5)
}

OdysseyGame::OdysseyGame(const std::filesystem::path& dataDirectory)
    : map_(makeTestMap()), camera_(kVirtualWidth, kVirtualHeight, map_.pixelWidth(), map_.pixelHeight()),
      hero_(map_.pixelWidth() / 2.0 + kTileSize / 2.0, map_.pixelHeight() / 2.0 + kTileSize * 0.75),
      range_(map_, loadMaterials(dataDirectory)),
      enemy_(hero_.feetX() + 2 * kTileSize, hero_.feetY(), 100) { // two tiles east of the hero
    camera_.centreOn(hero_.feetX(), hero_.feetY());
    range_.addTarget(openTargetBase());
    range_.addTarget(blockedTargetBase());
}

void OdysseyGame::update(const luna::engine::Intents& intents) {
    ++ticks_;
    hero_.update(intents, map_);

    // Handle weapon switching (Shift key).
    if (intents.pressed(luna::engine::Intent::SwitchWeapon)) {
        currentWeapon_ = currentWeapon_ == WeaponType::Sword ? WeaponType::Bow : WeaponType::Sword;
        core::logInfo(std::format("Switched to {}", currentWeapon_ == WeaponType::Sword ? "Sword" : "Bow"));
    }

    // Handle attacks based on current weapon.
    if (intents.pressed(luna::engine::Intent::Interact)) {
        if (currentWeapon_ == WeaponType::Sword) {
            // Perform sword slash in the direction the hero is facing.
            sword_.slash(hero_.facing());
            core::logInfo(std::format("Sword slash facing {}", facingName(hero_.facing())));
        } else {
            // Throw spear (bow).
            const Vec3 feet{luna::engine::metresFromPixels(hero_.feetX()), luna::engine::metresFromPixels(hero_.feetY()),
                            luna::physics::kFixedZero};
            const SpearKind& kind = range_.materials().spear(nextSpearIsFlint_ ? "flint" : "wooden");
            const auto target = range_.throwSpear(feet, hero_.facing(), kind);
            nextSpearIsFlint_ = !nextSpearIsFlint_;
            // Frame the throw: the camera moves to halfway between the hero and where the spear
            // goes, so the whole arc is on screen. 2 seconds, then it follows the hero again.
            const Vec3 goal = target ? range_.targets()[*target].base : feet + facingDirection(hero_.facing()) * Fixed::fromInt(6);
            const luna::engine::ScreenPoint goalPixels = luna::engine::groundShadow(goal);
            framingX_ = (hero_.feetX() + goalPixels.x) / 2.0;
            framingY_ = (hero_.feetY() + goalPixels.y) / 2.0;
            framingTicks_ = 2 * 20;
            core::logInfo(std::format("Threw a {} spear facing {}", kind.name, facingName(hero_.facing())));
        }
    }

    // Update sword state every tick.
    sword_.update();

    // Update enemy state.
    enemy_.update();

    // One hit per swing, if the enemy is within reach. Range is in metres; one tile is one metre.
    if (sword_.isAttacking() && !sword_.hasHitInThisSlash() && enemy_.isAlive()) {
        const double dx = enemy_.feetX() - hero_.feetX();
        const double dy = enemy_.feetY() - hero_.feetY();
        if (std::sqrt(dx * dx + dy * dy) <= sword_.config().slashRangeMetres * kTileSize) {
            enemy_.takeDamage(sword_.config().damagePerHit);
            sword_.markHit();
        }
    }

    // Update spear physics.
    for (const SpearHit& hit : range_.update()) {
        const FlyingSpear& spear = range_.spears()[hit.spear];
        core::logInfo(std::format("Spear ({}) hit {} at ({:.2f}, {:.2f}, {:.2f}) m, {:.1f} m/s, {:.1f} damage", spear.kind.name,
                                  hitKindName(hit.kind), luna::engine::toDouble(hit.point.x),
                                  luna::engine::toDouble(hit.point.y), luna::engine::toDouble(hit.point.z),
                                  luna::engine::toDouble(luna::physics::length(hit.velocity)),
                                  luna::engine::toDouble(hit.damage)));
    }

    if (framingTicks_ > 0) {
        --framingTicks_;
        camera_.follow(framingX_, framingY_);
    } else {
        camera_.follow(hero_.feetX(), hero_.feetY());
    }
}

void OdysseyGame::start(luna::engine::Renderer& renderer) {
    characters_ = renderer.createTexture(makeCharacterSheet());
    tiles_ = renderer.createTexture(makeTileSheet());
    props_ = renderer.createTexture(makePropSheet());
}

void OdysseyGame::render(luna::engine::Renderer& renderer, double alpha) {
    map_.draw(renderer, tiles_, camera_, alpha);
    const luna::engine::Rect view = camera_.view(alpha);
    auto screen = [&view](double worldX, double worldY) {
        return luna::engine::Point{static_cast<int>(std::lround(worldX)) - view.x, static_cast<int>(std::lround(worldY)) - view.y};
    };

    // Shadows first: they lie on the ground, under everything that stands or flies.
    for (const FlyingSpear& spear : range_.spears()) {
        const luna::engine::ScreenPoint shadow = luna::engine::groundShadow(blended(spear, alpha));
        renderer.draw(props_, kShadowFrame, screen(shadow.x - kShadowFrame.width / 2.0, shadow.y - kShadowFrame.height / 2.0));
    }
    // Targets, bottom-centre on their base.
    for (const StrawTarget& target : range_.targets()) {
        const luna::engine::ScreenPoint base = luna::engine::topDownPosition(target.base);
        renderer.draw(props_, target.hits > 0 ? kTargetHitFrame : kTargetFrame,
                      screen(base.x - kTargetFrame.width / 2.0, base.y - kTargetFrame.height));
    }
    // The hero, blended between ticks like the camera, so walking looks smooth at 60 FPS.
    renderer.draw(characters_, hero_.spriteFrame(),
                  screen(hero_.feetX(alpha) - kCharacterWidth / 2.0, hero_.feetY(alpha) - kCharacterHeight));

    // The enemy: red while the hit flash lasts, with a health bar and "HP/max" above its head.
    if (enemy_.isAlive()) {
        const double left = enemy_.feetX() - kCharacterWidth / 2.0;
        const double top = enemy_.feetY() - kCharacterHeight;
        if (enemy_.isFlashing()) {
            renderer.draw(props_, kEnemyHitFrame, screen(left, top));
        } else {
            renderer.draw(characters_, luna::engine::Rect{0, 0, kCharacterWidth, kCharacterHeight}, screen(left, top));
        }

        const double barLeft = enemy_.feetX() - kHealthBarEmpty.width / 2.0;
        const double barTop = top - 6;
        renderer.draw(props_, kHealthBarEmpty, screen(barLeft, barTop));
        const int filled = kHealthBarFull.width * enemy_.hp() / enemy_.maxHp();
        renderer.draw(props_, luna::engine::Rect{kHealthBarFull.x, kHealthBarFull.y, filled, kHealthBarFull.height},
                      screen(barLeft, barTop));

        const std::string label = std::format("{}/{}", enemy_.hp(), enemy_.maxHp());
        const double labelWidth = static_cast<double>(label.size()) * kGlyphAdvance + 1;
        double x = enemy_.feetX() - labelWidth / 2.0;
        for (const char c : label) {
            renderer.draw(props_, glyphFrame(c), screen(x, barTop - 8));
            x += kGlyphAdvance;
        }
    }

    // Draw sword if it's the current weapon and actively attacking.
    if (currentWeapon_ == WeaponType::Sword && sword_.isAttacking()) {
        const int frame = sword_.animationFrame();
        const luna::engine::Rect swordSpriteFrame = swordFrame(frame);
        // Position sword at character's right hand. Character center is at feetX, feetY.
        // Right hand is roughly at character_center_x + 8, character_top_y + 20.
        // Sword sprite hilt is at x=16 (middle of 32-wide sprite), y=36 (lower part of 48-tall sprite).
        // So position sprite so its hilt aligns with hand position.
        const double heroWorldX = hero_.feetX(alpha) + 8 - 16;      // Center hilt at right hand x
        const double heroWorldY = hero_.feetY(alpha) - 48 + 20 - 36; // Align hilt at hand y
        renderer.draw(props_, swordSpriteFrame, screen(heroWorldX, heroWorldY));
    }

    // Spears: height lifts the sprite up the screen, while the shadow stays on the ground.
    for (const FlyingSpear& spear : range_.spears()) {
        const luna::engine::ScreenPoint p = luna::engine::topDownPosition(blended(spear, alpha));
        const luna::engine::ScreenPoint pointing = luna::engine::topDownDirection(spear.heading);
        renderer.draw(props_, spearFrame(facingForVector(pointing.x, pointing.y), spear.kind.tip == "flint"),
                      screen(p.x - kSpearFrameSize / 2.0, p.y - kSpearFrameSize / 2.0));
    }
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
