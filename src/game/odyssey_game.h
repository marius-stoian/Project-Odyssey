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
#include "game/clan_view.h"
#include "game/catalogs.h"
#include "game/content_art.h"
#include "luna/engine/effects.h"
#include "game/editor.h"
#include "game/enemy.h"
#include "game/game_rules.h"
#include "game/hero.h"
#include "game/pickups.h"
#include "game/effect_art.h"
#include "game/plants.h"
#include "game/run_flow.h"
#include "game/session_stats.h"
#include "game/settings.h"
#include "game/tutorial.h"
#include "game/weather.h"
#include "game/level.h"
#include "game/spear_range.h"
#include "game/sword.h"
#include "game/weapons.h"

#include "core/random.h"

#include "sim/hero_life.h"
#include "sim/interaction.h"
#include "sim/region.h"
#include "sim/region_save.h"
#include "sim/rivals.h"
#include "sim/save.h"
#include "sim/world.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
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
    // Interactions read from assets/data/interactions/ when the game starts (US-150); mistakes are in the report.
    const sim::rules::InteractionRegistry& interactions() const { return interactions_; }
    const sim::rules::LoadReport& interactionReport() const { return interactionReport_; }
    // Smart objects (US-151): what a plant is to the rules (its kind and tags), and what the hero may do to it now, in menu order,
    // with the reason when an item is disabled.
    sim::rules::ThingInfo plantThing(std::size_t index) const;
    std::vector<sim::rules::Offer> plantOffers(std::size_t index) const;
    // The same for any Subject (a clan member, a fire, the stone, a rival camp, a plant): what the hero may do to it now, in menu order,
    // with the reason for a disabled item (US-152).
    std::vector<sim::rules::Offer> offersFor(const Subject& subject) const;
    void setPlantState(std::size_t index, const std::string& state); // "picked", "ripe"...
    std::set<std::string> knownTags() const; // every tag a catalog or character kind carries
    // F5 (US-156): reads the interaction files again. With no mistakes the new data replaces the old and the panel closes; with mistakes the
    // last good data stays in use and the panel lists "file:line: message". Returns true when the new data was taken.
    bool reloadInteractions();
    bool interactionPanelOpen() const { return !interactionReport_.errors.empty(); }
    double lastInteractionReloadMilliseconds() const { return lastInteractionReloadMs_; }
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
    // The simulated clan (US-032, D-30): on in levels marked "clan": true, or with `--clan`. It is the M2 simulation running
    // inside the game, one simulation tick per game tick, and the view puts each person somewhere and walks them there.
    void setClan(bool on);
    bool clanOn() const { return clan_ != nullptr; }
    // Runs the clan's simulation this many ticks for each game tick (fast forward, for demos and screenshots; 1 is real time).
    void setClanSpeed(int ticksPerTick) { clanSpeed_ = ticksPerTick < 1 ? 1 : ticksPerTick; }
    const sim::World* clan() const { return clan_.get(); }
    const ClanView& clanView() const { return clanView_; }
    // The player's run (M5, D-32): a hero of the clan with a Growing Period, professions, trade and a sacred fire, played through the
    // screens of RunFlow. `--new-game` opens the New Game screen; Esc opens the menu.
    sim::HeroLife* life() { return life_.get(); }
    const sim::HeroLife* life() const { return life_.get(); }
    const sim::HeroData* heroData() const { return heroData_ ? &*heroData_ : nullptr; }
    RunFlow& run() { return runFlow_; }
    const RunFlow& run() const { return runFlow_; }
    // useRegion false starts the run in the level already loaded (a hand-made camp) instead of a generated region.
    void startNewRun(const sim::NewGame& game, bool useRegion = true, bool tutorial = false);
    // The elder's first-day guidance (US-090) and the opt-in session statistics (US-092).
    Tutorial& tutorial() { return tutorial_; }
    const Tutorial& tutorial() const { return tutorial_; }
    SessionStats& stats() { return stats_; }
    void setStatistics(bool agreed);
    // Writes the session's statistics file (only when the player agreed); called when the game ends. Returns the file or empty.
    std::filesystem::path finishSession();
    void afterYear();                                    // the clan has lived a year while the hero grew up
    bool harvestPlant(std::size_t index);                // gathering takes a plant (it regrows): no healing, no leaf burst
    int plantAtWorld(double x, double y) const;          // the growing plant whose picture covers a world point, or -1
    int personAtWorld(double x, double y) const;         // the clan member standing there, or -1
    std::vector<int> attendeesAt(double x, double y, int radiusTiles) const; // the clan members within reach of a point
    PixelPoint campPixels() const { return clanView_.camp(); }
    PixelPoint knappingStone() const { return {clanView_.camp().x + 4 * kTileSize, clanView_.camp().y + 2 * kTileSize}; }
    const GameSettings& settings() const { return settings_; }
    // The performance overlay (US-082): F3. Frame rate and frame time over the last second of frames, and the time one simulation
    // tick (the clan, the rivals and the run) takes: its average and its worst over the last 100 ticks.
    bool overlayOn() const { return overlayOn_; }
    void showOverlay(bool on) { overlayOn_ = on; }
    double framesPerSecond() const;
    double frameMilliseconds() const;
    double tickMilliseconds() const;
    double worstTickMilliseconds() const;
    void applySettings(const GameSettings& settings);    // saved, and asked of the window at once
    std::optional<WindowChange> takeWindowChange() override;
    // A generated region (US-040..US-042, D-31): the land is made from the seed and played as a 256-tile level, with the
    // clan at the start and two rival clans far away. The Editor is off in a region (it is for hand-made levels).
    void loadRegion(std::uint64_t seed);
    const sim::Region* region() const { return region_.get(); }
    const sim::Rivals* rivals() const { return rivals_.get(); }
    // Saves (US-080): the clan's world (and the region's changes) are written each time an in-game day ends, into this folder
    // (by default the user's save folder). `loadAutosave` brings them back; a damaged file falls back to the newest backup.
    void setSaveDirectory(const std::filesystem::path& directory) {
        saveDirectory_ = directory;
        settings_ = loadSettings(saveDirectory_ / "settings.json", nullptr);
        stats_.enable(settings_.statistics == 1);
    }
    const std::filesystem::path& saveDirectory() const { return saveDirectory_; }
    bool autosave();              // false when it could not write
    bool loadAutosave();          // false when there is nothing to load
    double lastAutosaveMilliseconds() const { return lastAutosaveMs_; }
    int autosaves() const { return autosaves_; }
    const std::string& message() const { return message_; }
    // Developer tools (US-083): F12 in Debug builds only. Click a person to inspect them; set the clan's speed or skip a day.
    bool devToolsOpen() const { return devToolsOpen_; }
    int selectedPerson() const { return selectedPerson_; }
    void setClanSpeedFromTools(int ticksPerTick) { setClanSpeed(ticksPerTick); }
    int clanSpeed() const { return clanSpeed_; }
    // Runs the clan to the start of the next day at once.
    void skipDay();
    // The weather (US-138): a seeded cycle, drawn over the world and under the interface. `--seed` (or this) fixes it.
    void setWeatherSeed(std::uint64_t seed);
    // Starts under the named weather (false when weather.json has none of that name).
    bool setWeatherNamed(const std::string& name);
    const WeatherCycle& weather() const { return weather_; }
    const EffectArt& effectArt() const { return effectArt_; }
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
    sim::rules::InteractionRegistry interactions_;
    sim::rules::LoadReport interactionReport_;
    void loadInteractions(); // at start: reads the interaction files; a file with mistakes is left out, the rest load
    double lastInteractionReloadMs_ = 0.0;
    void drawInteractionPanel(luna::engine::Renderer& renderer) const;
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
    std::filesystem::path dataDirectory_;
    std::optional<sim::HeroData> heroData_;
    std::unique_ptr<sim::HeroLife> life_;
    RunFlow runFlow_;
    Tutorial tutorial_;
    TutorialScript tutorialScript_;
    SessionStats stats_;
    bool privacyAsked_ = false;
    void drawTutorial(luna::engine::Renderer& renderer) const;
    bool overlayOn_ = false;
    std::array<double, 100> tickTimes_{};
    std::array<double, 60> frameTimes_{};
    std::size_t tickTimeAt_ = 0;
    std::size_t frameTimeAt_ = 0;
    std::size_t tickTimesFilled_ = 0;
    std::size_t frameTimesFilled_ = 0;
    std::chrono::steady_clock::time_point lastRender_{};
    void drawOverlay(luna::engine::Renderer& renderer) const;
    GameSettings settings_;
    std::optional<WindowChange> pendingWindow_;
    void drawRunHud(luna::engine::Renderer& renderer) const;
    void drawRunWorld(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const;
    bool clanEnabled_ = false;
    int clanSpeed_ = 1;
    std::unique_ptr<sim::Region> region_;
    std::unique_ptr<sim::Rivals> rivals_;
    std::filesystem::path saveDirectory_;
    std::int64_t lastSavedDay_ = -1;
    double lastAutosaveMs_ = 0.0;
    int autosaves_ = 0;
    std::string message_;
    int messageTicks_ = 0;
    bool devToolsOpen_ = false;
    int selectedPerson_ = -1;
    void say(const std::string& text);
    void updateDevTools(const luna::engine::Intents& intents);
    void drawDevTools(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const;
    void drawRivals(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const;
    int figureAt(const luna::engine::Rect& view, double alpha) const; // the person under the pointer, -1 for nobody
    std::unique_ptr<sim::World> clan_;
    ClanView clanView_;
    std::optional<LayerSheets> layerSheets_;
    std::map<LookSpec, luna::engine::Texture> lookTextures_; // one composed sheet per distinct look, made when first seen
    void startClan();
    void drawClan(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha, bool behindHero);
    void drawEmote(luna::engine::Renderer& renderer, Emote emote, int x, int y) const;
    void drawClanDetails(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const;
    void drawClanHud(luna::engine::Renderer& renderer) const;
    EffectArt effectArt_;
    WeatherCycle weather_;
    std::uint64_t weatherSeed_ = 0;
    luna::engine::Texture weatherTexture_;
    std::map<std::string, std::vector<luna::engine::Rect>> weatherFrames_; // weather name -> its frames in the page
    void startPlacedEffects();
    void drawWeather(luna::engine::Renderer& renderer) const;
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
    void destroyPlant(std::size_t index, bool heal = true);
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
