#pragma once

#include "boundary.h"

#include "luna/engine/application.h"
#include "luna/engine/camera.h"
#include "luna/engine/game.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"

#include "game/animals.h"
#include "game/arc_shots.h"
#include "game/art.h"
#include "game/catalogs.h"
#include "game/content_art.h"
#include "luna/engine/effects.h"
#include "game/editor.h"
#include "game/enemy.h"
#include "game/hero.h"
#include "game/pickups.h"
#include "game/plants.h"
#include "game/level.h"
#include "game/spear_range.h"
#include "game/sword.h"
#include "game/weapons.h"

#include "core/random.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <vector>

namespace odysseus::game {

enum class WeaponType { Sword, Bow };

// Game mode plays the level; Editor mode changes it (M2c, US-123).
enum class Mode { Game, Editor };

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
    // Effects playing now (US-132): hit sparks, smoke, trails.
    const luna::engine::EffectPlayer& effects() const { return effects_; }
    const Catalogs& catalogs() const { return catalogs_; }
    // What the hero carries (US-134): a hotbar of 9 slots, empty at the start. Walking over a
    // pickup puts its weapon in the first free slot; keys 1-9 hold a slot, Shift the next filled one.
    static constexpr int kHotbarSlots = 9;
    using Hotbar = std::array<std::string, kHotbarSlots>; // "" is a free slot
    const Hotbar& hotbar() const { return hotbar_; }
    int heldSlot() const { return heldSlot_; }
    std::size_t carriedCount() const; // filled slots
    // Puts a weapon (a name of weapons.json, or a demo weapon) in the first free slot; false when full.
    bool pickUp(const std::string& weapon);
    void selectSlot(int slot);
    // The held weapon's name ("Spear throw", "Sword", "iron sword", ...; "Empty hands" when the slot
    // is free), and the catalog weapon when it is one.
    std::string heldName() const;
    const WeaponDef* heldWeapon() const;
    // Pickups still lying in the level (those not yet picked up).
    std::size_t pickupsLeft() const;
    // "Hotbar full" is shown for a moment when a pickup is touched with no free slot.
    bool hotbarFullShown() const { return fullTicks_ > 0; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }
    // Plants (US-136): those of the level as they are now, growing or waiting to grow back.
    const std::vector<WorldPlant>& plants() const { return plants_; }
    std::size_t plantsGrowing() const;
    // What Interact (with empty hands) or Inspect (the right mouse button) showed for the plant
    // next to the hero, for 3 seconds; empty when nothing is shown.
    bool inspecting() const { return inspection_.ticks > 0; }
    const std::string& inspectedName() const { return inspection_.name; }
    const std::string& inspectedText() const { return inspection_.text; }
    const PlantArt& plantArt() const { return plantArt_; }
    const AnimalArt& animalArt() const { return animalArt_; }
    // Arrows, bolts and thrown weapons in flight or stuck in the ground (US-140): physics arcs with height.
    const std::vector<ArcShot>& arcShots() const { return arcShots_; }
    // Mouse aiming (US-139): while a catalog weapon is held and the pointer is over the picture,
    // the hero faces the pointer and Attack (the left button) goes toward it, at any angle.
    // The part of the world on screen now, world pixels (the camera stops at the map's edge, so the hero is not always centred).
    luna::engine::Rect cameraView() const { return camera_.view(); }
    bool aiming() const { return aiming_; }
    double aimDirectionX() const { return aimDx_; } // unit vector from the hero's feet to the pointer
    double aimDirectionY() const { return aimDy_; }
    double aimTargetX() const { return aimTargetX_; } // where the pointer is in the world, pixels
    double aimTargetY() const { return aimTargetY_; }
    // Starts the named effect (effects.json) centred on a world point, `size` pixels across.
    // Does nothing when the content atlas or the effect is missing.
    void playEffect(const std::string& name, double x, double y, int size = 0);
    // The hero's health (US-131): 100, lost to enemies striking back; at 0 a short fade, then
    // the hero starts again at the hero start with full health.
    int heroHp() const { return heroHp_; }
    bool heroRespawning() const { return respawnTicks_ > 0; }
    static constexpr int kHeroMaxHp = 100;
    const std::vector<PlacedCharacter>& bystanders() const { return bystanders_; }
    const std::filesystem::path& levelFile() const { return editor_.levelFile(); } // the Editor may open another
    Mode mode() const { return mode_; }
    // F1 and F2 do this; `--editor` starts in the Editor. Back in Game, the play state is
    // rebuilt from the level as edited (the hero at the hero start).
    void switchMode(Mode mode);
    Editor& editor() { return editor_; }

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
    std::vector<PlacedCharacter> bystanders_; // placed characters the sword does not fight: they stand and are seen
    Editor editor_;
    Mode mode_ = Mode::Game;
    luna::engine::Texture uiSheet_;

    void populate();  // the level's targets and enemies join the play state
    void resetPlay(); // the whole play state again, from the level
    void drawModeLabel(luna::engine::Renderer& renderer) const;
    Catalogs catalogs_;
    ContentAtlas content_;
    bool contentLoaded_ = false;
    luna::engine::Texture effectsTexture_;
    luna::engine::EffectPlayer effects_;
    std::vector<const WeaponDef*> starters_; // catalog weapons marked "starter"
    Hotbar hotbar_;
    int heldSlot_ = 0;
    struct WorldPickup {
        PlacedPickup pickup;
        bool taken = false;
    };
    std::vector<WorldPickup> pickups_; // the level's pickups, until the level restarts
    int fullTicks_ = 0;
    WeaponArt weaponArt_;
    void collectPickups();
    void cycleSlot();
    int attackCooldown_ = 0;                 // ticks until the held catalog weapon can attack again
    int swingTicks_ = 0;                     // a melee swing being drawn
    std::vector<Projectile> projectiles_;
    luna::engine::Texture iconsTexture_;
    luna::engine::Texture iconsMirrored_;    // the same icons facing the other way (west)
    // Along a unit vector. distancePixels is how far the hero aims (the pointer); chestHeight: aimed with keys, no pointer.
    void attackWith(const WeaponDef& weapon, double dirX, double dirY, double distancePixels, bool chestHeight);
    std::vector<ArcShot> arcShots_;

    // Plants (US-136).
    PlantArt plantArt_;
    AnimalArt animalArt_;
    std::vector<WorldPlant> plants_;
    core::Pcg32 plantRng_{1, 5}; // the stream "plants": where destroyed plants grow back (Charter rule 6)
    struct Inspection {
        std::string name;
        std::string text;
        int plantId = 0;
        int ticks = 0;
    } inspection_;
    void populatePlants();
    void destroyPlant(std::size_t index);
    void tickPlants();
    bool inspectNearestPlant();
    bool plantSpotFree(int cellX, int cellY) const;
    // Everything a shot can hit: the enemies, then the big plants still growing (`plantOf` maps a target's
    // index past the enemies to the plant).
    std::vector<Target> shotTargets(std::vector<std::size_t>& plantOf) const;
    void drawPlants(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha, bool behindHero) const;
    void drawInspection(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const;
    // acingBefore: how the hero faced before this tick's walking turned it (the pointer rules the facing, steadily).
    void updateAim(const luna::engine::Pointer& pointer, bool fallen, Facing facingBefore);
    void drawAim(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const;
    bool aiming_ = false;
    double aimDx_ = 0.0;
    double aimDy_ = 1.0;
    double aimTargetX_ = 0.0;
    double aimTargetY_ = 0.0;
    int pointerX_ = -1; // the pointer on screen, virtual pixels, for the crosshair
    int pointerY_ = -1;
    void strike(Enemy& enemy, int damage, const WeaponDef* weapon = nullptr); // a hit: damage, spark, then death smoke or strike back; the weapon's element follows
    void applyElement(Enemy& target, const WeaponDef& weapon, int dealt); // US-135: burn, slow, poison, chain, drain
    void tickStatus(Enemy& enemy);           // burning and poison hurt, effects show while they last
    void drawHeld(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const;
    int heroHp_ = kHeroMaxHp;
    int respawnTicks_ = 0; // counting down the fade after the hero falls
    static constexpr int kRespawnTicks = 20; // one second of fade
    void hurtHero(int damage, const Enemy& by);
    void drawHud(luna::engine::Renderer& renderer) const;
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
