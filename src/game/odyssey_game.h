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
#include "game/npc_life.h"
#include "sim/action_runner.h"
#include "game/hero.h"
#include "game/pickups.h"
#include "game/effect_art.h"
#include "game/plants.h"
#include "game/run_flow.h"
#include "game/session_stats.h"
#include "game/lighting.h"
#include "game/settings.h"
#include "game/celestial.h"
#include "game/sky.h"
#include "game/tutorial.h"
#include "game/weather.h"
#include "game/level.h"
#include "game/spear_range.h"
#include "game/sword.h"
#include "game/weapons.h"

#include "core/random.h"

#include "sim/hero_life.h"
#include "game/bubbles.h"
#include "sim/dialogue_script.h"
#include "sim/flag_store.h"
#include "sim/npc_chooser.h"
#include "sim/smalltalk.h"
#include "game/npc_class_book.h"
#include "sim/npc_population.h"
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
#include <unordered_map>
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
    const luna::engine::Camera& camera() const { return camera_; }
    const Definitions& definitions() const { return definitions_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    std::vector<Enemy>& enemiesMutable() { return enemies_; } // for tests and the confrontations of NPCs (US-266)
    // A fight (US-266): the placed character with this id strikes back (an enemy winds up; a person who stood by becomes an enemy). False when there is no
    // such character.
    bool startFight(int placedId);
    // The fight ends for now: an enemy stops winding up. False when the id is no enemy.
    bool calmFight(int placedId);
    // The Confront key (US-266): the NPC under the pointer, else the nearest within 6 m, gets the menu of its confront actions.
    void confrontKey(const luna::engine::Pointer& pointer);
    // Effects playing now (US-132): hit sparks, smoke, trails.
    const luna::engine::EffectPlayer& effects() const { return effects_; }
    const Catalogs& catalogs() const { return catalogs_; }
    // Interactions read from assets/data/interactions/ when the game starts (US-150); mistakes are in the report.
    const sim::rules::InteractionRegistry& interactions() const { return interactions_; }
    const sim::rules::LoadReport& interactionReport() const { return interactionReport_; }
    // The placed people of the level as persons of the simulation (US-262). Animals and monsters are not in it.
    const sim::NpcPopulation& npcPopulation() const { return npcPopulation_; }
    sim::NpcPopulation& npcPopulationMutable() { return npcPopulation_; }
    bool isPersonKind(const PlacedCharacter& placed) const;
    // The placed character with this id, or nullptr.
    const PlacedCharacter* placedCharacter(int id) const;
    // The dialogue a placed person speaks to the player: the "player" dialogue of its classes, kind and own fields, found by name among the loaded scripts.
    // nullptr when there is none (US-265).
    const sim::rules::DlgScript* npcDialogueFor(int placedId) const;
    // Whether a placed character fights the hero (US-264): with a kind file it is its attitude being hostile, without one the old enemy switch of its kind.
    bool fightsHero(const PlacedCharacter& placed) const;
    // The attitude word of a placed character to the hero: the person's own opinion when they are a person of the population, else the attitude of its
    // kind file and its own fields.
    std::string attitudeWordOf(int placedId) const;
    const sim::OpinionConfig& npcOpinions() const { return npcOpinions_; }
    bool saveNpcPopulation() const;
    NpcClassBook& npcClasses() { return npcClasses_; }
    const NpcClassBook& npcClasses() const { return npcClasses_; }
    // The conversations of assets/data/dialogue/ (US-160), read and reloaded together with the interaction files; their mistakes are in the
    // same report and panel (as "dialogue/<name>.dlg:<line>: message").
    const sim::rules::DialogueLibrary& dialogues() const { return dialogues_; }
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
    // Timed actions (US-153): the runner, its clock (play ticks since the run began; it stops while a screen is open), and the plant
    // with a given id (-1 when there is none).
    sim::rules::ActionRunner& actions() { return actions_; }
    const sim::rules::ActionRunner& actions() const { return actions_; }
    std::int64_t actionClock() const { return actionClock_; }
    // Greetings (US-162): the bubbles over heads, the minute each NPC waits between greetings, and the seeded stream that chooses between
    // equally fitting scripts (the stream "dialogue", Charter rule 6).
    Bubbles& bubbles() { return bubbles_; }
    Exchanges& exchanges() { return exchanges_; }
    const Exchanges& exchanges() const { return exchanges_; }
    const Bubbles& bubbles() const { return bubbles_; }
    sim::rules::CooldownTable& greetingCooldowns() { return greetingCooldowns_; }
    core::Pcg32& dialogueRandom() { return dialogueRng_; }
    // Generated small talk (US-163): what people say when no script fits them, and for `{smalltalk.topic}`.
    sim::rules::SmallTalk& smalltalk() { return smalltalk_; }
    // Story notes set by conversations and interactions (`flag met-elder`), saved with the things (US-164).
    sim::rules::FlagStore& flags() { return flags_; }
    const sim::rules::FlagStore& flags() const { return flags_; }
    // What a conversation leaves behind (US-164): a memory in someone's mind (see World::rememberConversation) and a line in the clan's chronicle.
    bool rememberConversation(int holder, int other, const std::string& text, int feeling);
    void chronicleLine(const std::string& text, int who, int other);
    int plantIndexById(int id) const;
    // What clan members and animals do on their own (US-154), and what they need to do it: the ground, the animals and the people to move.
    NpcLife& npcs() { return npcLife_; }
    const NpcLife& npcs() const { return npcLife_; }
    Enemy& enemyAt(std::size_t index) { return enemies_.at(index); }
    // The harmless animals and people placed in the level (deer, rabbits): the ones that walk and graze (US-154) are moved here.
    std::vector<PlacedCharacter>& bystandersMutable() { return bystanders_; }
    ClanView& clanViewMutable() { return clanView_; }
    const luna::engine::TileMap& tileMap() const { return map_; }
    // Outside help for a clan member's need (a fire pit's warmth, a bed): false when there is no clan or the person is gone.
    bool helpPerson(int personId, sim::Need need, int amount);
    // What `who` thinks of `about` changes by `delta` (a conversation's choice, US-161); the world keeps it between -100 and 100.
    void changeOpinion(int who, int about, int delta);
    // And harm: a clan member's need falls (a hazard, or a test that wants someone hungry).
    bool harmPerson(int personId, sim::Need need, int amount);
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
    // A performance run (US-234, --perf): the overlay is on, GPU time is measured and the frame figures are written to the log
    // once a minute, with the totals so far. --people N starts the clan with N people instead of the usual twenty.
    void setPerformanceLog(bool on) { perfLog_ = on; overlayOn_ = on || overlayOn_; }
    void setStartingPeople(int people) { startingPeople_ = people; }
    double drawMilliseconds() const;
    double gpuMilliseconds() const { return gpuMs_; }
    // Runs the clan's simulation this many ticks for each game tick (fast forward, for demos and screenshots; 1 is real time).
    void setClanSpeed(int ticksPerTick) { clanSpeed_ = ticksPerTick < 1 ? 1 : ticksPerTick; }
    const sim::World* clan() const { return clan_.get(); }
    sim::World* clanMutable() { return clan_.get(); } // for the few things that read and empty a queue of the world (US-165)
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
    // Camera zoom and UI scale (US-232). Zoom 2x is the world as it always looked (15 x 8.4 tiles); 1x shows 30 x 17.
    // `setViewScales` only changes what is drawn (tests use it); `applySettings` also saves them.
    void setViewScales(int cameraZoom, int uiScale);
    // The sky now (US-242): the light follows the clan's game clock (a level without a clan has no clock and stays at noon).
    SkyState sky() const;
    // The clock the sky and the celestial bodies follow (a level without a clan has no clock and stays at noon of day 0).
    GameClock gameClock() const;
    // The sun and moon of this level (US-248): the ones placed in the Editor, and the default pair for any kind it places none of.
    std::vector<CelestialBody> celestialBodies() const;
    // The light the scene has now, from the sun by day and the moon by night, seen from the hero: the direction shadows fall (US-244 draws them).
    CelestialLight celestialLight(double alpha = 1.0) const;
    // Whether a texture is a black copy that shadows are cut from (US-244; tests tell shadow draws from the rest by it).
    bool isShadowTexture(int textureId) const {
        return std::any_of(silhouettes_.begin(), silhouettes_.end(), [textureId](const auto& entry) { return entry.second.id == textureId; });
    }
    // The point lights of the world now (US-243), in the pixels of the picture they light: placed effects, burning objects and the held weapon that
    // have a `light`, and the torches clan members carry at night. They shine in proportion to how dark it is (`darkness` 0 to 1).
    std::vector<luna::engine::PointLight> worldLights(const luna::engine::Rect& view, double alpha, double darkness) const;
    // The same lights in world pixels, with the kind they are and the spot on the ground below them (US-245 casts shadows away from them).
    struct LightSource {
        const LightKindDef* kind = nullptr;
        double x = 0.0, y = 0.0;   // where the light hangs
        double groundY = 0.0;      // the ground point below it, at the same x
        std::uint64_t id = 0;
    };
    std::vector<LightSource> lightSources(double alpha) const;
    std::vector<LightSource> levelLightSources() const; // the lights the level holds: effects with a light and the Light tool's lights (US-247)
    std::vector<luna::engine::PointLight> pointLights(const std::vector<LightSource>& sources, const luna::engine::Rect& view, double seconds, double darkness) const;
    void buildNpcPopulation();
    void registerCreatures();
    void tickNpcPopulation();
    std::string loadNpcPopulation(); // the problem, or empty
    luna::engine::LightFrame editorLightFrame(double hour, const luna::engine::Rect& view) const; // the Editor's time-of-day preview (US-247)
    luna::engine::LightFrame ambientLightFrame(double alpha, bool withWeather = true) const; // the ambient colour of the world now (no point lights)
    static double darknessOf(const luna::engine::LightFrame& frame);
    void updateZoom(const luna::engine::Intents& intents);
    int cameraZoom() const { return settings_.cameraZoom; }
    int uiScale() const { return settings_.uiScale; }
    // The world picture and the interface picture, in their own pixels: the 960 x 540 screen divided by the zoom or scale.
    int viewWidth() const { return core::kVirtualWidth / settings_.cameraZoom; }
    int viewHeight() const { return core::kVirtualHeight / settings_.cameraZoom; }
    int uiWidth() const { return core::kVirtualWidth / settings_.uiScale; }
    int uiHeight() const { return core::kVirtualHeight / settings_.uiScale; }
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
        pendingWindow_ = WindowChange{settings_.resolution};
        setViewScales(settings_.cameraZoom, settings_.uiScale);
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
    NpcClassBook npcClasses_; // US-260
    sim::CalendarConfig npcCalendar_;
    sim::NeedsConfig npcNeeds_;
    sim::OpinionConfig npcOpinions_;   // US-264
    sim::NpcPopulation npcPopulation_; // US-262
    std::unordered_map<int, std::int64_t> npcMetDay_; // person id -> the day they last met the hero
    Mode mode_ = Mode::Game;
    luna::engine::Texture uiSheet_;

    void populate();  // the level's targets and enemies join the play state
    void resetPlay(); // the whole play state again, from the level
    void drawModeLabel(luna::engine::Renderer& renderer) const;
    Catalogs catalogs_;
    sim::rules::InteractionRegistry interactions_;
    sim::rules::LoadReport interactionReport_;
    sim::rules::DialogueLibrary dialogues_;
    sim::rules::SmallTalk smalltalk_;
    sim::rules::FlagStore flags_;
    Bubbles bubbles_;
    Exchanges exchanges_;
    sim::rules::CooldownTable greetingCooldowns_;
    core::Pcg32 dialogueRng_{1, 8};
    void loadInteractions();
    sim::rules::SmallTalk loadSmalltalk(sim::rules::LoadReport& report) const; // at start: reads the interaction files; a file with mistakes is left out, the rest load
    double lastInteractionReloadMs_ = 0.0;
    sim::rules::ActionRunner actions_;
    std::int64_t actionClock_ = 0;
    NpcLife npcLife_;
    void tickActions(const luna::engine::Intents& intents);
    void drawActionRing(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const;
    std::string thingsText() const;                          // the plants' states and the waiting effects, as saved in things.json
    std::vector<std::string> restoreThings(const std::string& text);
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
    bool perfLog_ = false;
    int startingPeople_ = 0; // 0: the data's number
    std::array<double, 60> drawTimes_{};
    std::size_t drawTimeAt_ = 0;
    std::size_t drawTimesFilled_ = 0;
    double gpuMs_ = -1.0;
    struct PerfTotals {
        std::uint64_t frames = 0;
        std::uint64_t over20 = 0; // frames that took longer than 20 ms (the 60 FPS budget is 16.7)
        double frameSum = 0.0, frameMax = 0.0, drawSum = 0.0, drawMax = 0.0, gpuSum = 0.0, gpuMax = 0.0, tickWorst = 0.0;
        std::uint64_t gpuFrames = 0;
    } perf_;
    std::chrono::steady_clock::time_point perfLastLog_{};
    void recordFrame(double drawMs, double gpuMs);
    std::array<double, 100> tickTimes_{};
    std::array<double, 60> frameTimes_{};
    std::size_t tickTimeAt_ = 0;
    std::size_t frameTimeAt_ = 0;
    std::size_t tickTimesFilled_ = 0;
    std::size_t frameTimesFilled_ = 0;
    std::chrono::steady_clock::time_point lastRender_{};
    void drawOverlay(luna::engine::Renderer& renderer) const;
    GameSettings settings_;
    SkyData sky_;           // assets/data/light/sky.json and the daylight of calendar.json (US-242)
    LightingData lighting_; // assets/data/light/lights.json (US-240): the ambient colour and the kinds of light
    CelestialEvents celestialEvents_; // assets/data/light/celestial-events.json (US-248): scripted eclipses
    std::vector<const PlantDef*> defaultBodies_; // the sun and moon of objects.json that follow the clock: used when a level places none
    void drawSkyBodies(luna::engine::Renderer& renderer, double alpha) const;
    // Shadows (US-244): black copies of the sprite textures to cut shadows from (by texture number), and what a shadow is like now.
    struct ShadowCast {
        bool on = false;
        double dirX = 0.0, dirY = 0.0;  // the way a shadow falls on the picture
        double lengthPerHeight = 0.0;   // shadow length divided by the height of the thing
        std::uint8_t alpha = 0;         // how dark
    };
    std::map<int, luna::engine::Texture> silhouettes_;
    ShadowCast shadowCast(double alpha) const;
    // Shadows of fires (US-245): the lights that cast them and how dark the night is, gathered once per frame by drawShadows.
    struct FireShadows {
        std::vector<LightSource> lights; // only kinds with `shadows`, in world pixels
        double darkness = 0.0;
    } fireShadows_;
    // The sun or moon shadow of one thing, then a faint one away from each of the nearest fires (the thing's feet are at world (worldX, worldY)).
    void castShadow(luna::engine::Renderer& renderer, const ShadowCast& cast, const luna::engine::Texture& texture, const luna::engine::Rect& source,
                    int feetX, int feetY, double heightMetres, double worldX, double worldY) const;
    void drawShadows(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha); // before the lit things are drawn
    luna::engine::Texture lookTexture(luna::engine::Renderer& renderer, const LookSpec& look);        // the composed sheet of a look, with its shadow
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
