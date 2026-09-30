#include "game/odyssey_game.h"

#include "core/log.h"
#include "core/version.h"
#include "game/art.h"
#include "game/placeholder_art.h"
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

namespace {

std::filesystem::path chosenLevel(const std::filesystem::path& dataDirectory, const std::filesystem::path& levelFile) {
    return levelFile.empty() ? dataDirectory.parent_path() / "levels" / "valley.json" : levelFile;
}

Level loadAndReport(const std::filesystem::path& file, const Definitions& definitions) {
    LoadedLevel loaded = loadLevel(file, definitions);
    for (const std::string& note : loaded.notes) {
        core::logWarning("Level: " + note);
    }
    core::logInfo(std::format("Level \"{}\" ({}x{} tiles, {} characters) from {}", loaded.level.name, loaded.level.width, loaded.level.height,
                              loaded.level.characters.size(), loaded.loadedFrom.string()));
    return std::move(loaded.level);
}

} // namespace

OdysseyGame::OdysseyGame(const std::filesystem::path& dataDirectory, const std::filesystem::path& levelFile)
    : definitions_(loadDefinitions(dataDirectory)), levelFile_(chosenLevel(dataDirectory, levelFile)),
      level_(loadAndReport(levelFile_, definitions_)), map_(buildTileMap(level_, definitions_)),
      camera_(kVirtualWidth, kVirtualHeight, map_.pixelWidth(), map_.pixelHeight()),
      hero_(static_cast<double>(level_.heroStart.x), static_cast<double>(level_.heroStart.y)),
      range_(map_, loadMaterials(dataDirectory)), spritesDirectory_(dataDirectory.parent_path() / "sprites") {
    camera_.centreOn(hero_.feetX(), hero_.feetY());
    for (const PixelPoint& target : level_.targets) {
        range_.addTarget({luna::engine::metresFromPixels(target.x), luna::engine::metresFromPixels(target.y), luna::physics::kFixedZero});
    }
    // Every placed character the sword can hit stands in the world (M2c: they stand still, D-19).
    for (const PlacedCharacter& placed : level_.characters) {
        const CharacterKindDef* kind = definitions_.character(placed.kind);
        if (kind == nullptr || !kind->enemy) {
            continue;
        }
        Enemy enemy(placed.feet.x, placed.feet.y, placed.hp);
        enemy.id = placed.id;
        enemy.name = placed.name;
        enemy.frames = kind->frames;
        enemy.directions = kind->directions;
        enemy.facing = placed.facing;
        enemy.swordDamage = placed.swordDamage;
        enemies_.push_back(enemy);
    }
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

    for (Enemy& enemy : enemies_) {
        enemy.update();
    }

    // One hit per swing, on the nearest living enemy within reach. Range is in metres; one tile
    // is one metre. Ties go to the lower id, so the same play always hits the same enemy.
    if (sword_.isAttacking() && !sword_.hasHitInThisSlash()) {
        Enemy* nearest = nullptr;
        double nearestDistance = sword_.config().slashRangeMetres * kTileSize;
        for (Enemy& enemy : enemies_) {
            const double dx = enemy.feetX() - hero_.feetX();
            const double dy = enemy.feetY() - hero_.feetY();
            const double distance = std::sqrt(dx * dx + dy * dy);
            if (enemy.isAlive() && (distance < nearestDistance || (distance == nearestDistance && nearest == nullptr))) {
                nearest = &enemy;
                nearestDistance = distance;
            }
        }
        if (nearest != nullptr) {
            nearest->takeDamage(sword_.config().damagePerHit);
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
    // The owner's art when its atlas loads, else the programmer art (US-120).
    std::vector<std::string> groundFrames;
    for (const TileKindDef& kind : definitions_.tiles) {
        groundFrames.push_back(kind.atlas);
    }
    art_ = makeArtSet(spritesDirectory_, groundFrames);
    if (art_.ownArt) {
        core::logInfo("Art: the owner's atlas");
    } else {
        core::logWarning("Art: programmer art, because " + art_.problem);
    }
    characters_ = renderer.createTexture(art_.heroSheet);
    tiles_ = renderer.createTexture(art_.tileStrip);
    props_ = renderer.createTexture(makePropSheet()); // texture 2: tests find props by this number
    charactersAtlas_ = renderer.createTexture(art_.characters);
    charactersHitAtlas_ = renderer.createTexture(art_.charactersHit);
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

    // Enemies: red while the hit flash lasts, with a health bar and "HP/max" above their heads.
    for (const Enemy& enemy : enemies_) {
        if (!enemy.isAlive()) {
            continue;
        }
        const double left = enemy.feetX() - kCharacterWidth / 2.0;
        const double top = enemy.feetY() - kCharacterHeight;
        renderer.draw(enemy.isFlashing() ? charactersHitAtlas_ : charactersAtlas_, art_.frame(enemy.frames, enemy.directions, enemy.facing, 0),
                      screen(left, top));

        const double barLeft = enemy.feetX() - kHealthBarEmpty.width / 2.0;
        const double barTop = top - 6;
        renderer.draw(props_, kHealthBarEmpty, screen(barLeft, barTop));
        const int filled = kHealthBarFull.width * enemy.hp() / enemy.maxHp();
        renderer.draw(props_, luna::engine::Rect{kHealthBarFull.x, kHealthBarFull.y, filled, kHealthBarFull.height},
                      screen(barLeft, barTop));

        const std::string label = std::format("{}/{}", enemy.hp(), enemy.maxHp());
        const double labelWidth = static_cast<double>(label.size()) * kGlyphAdvance + 1;
        double x = enemy.feetX() - labelWidth / 2.0;
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
