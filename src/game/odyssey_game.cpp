#include "game/odyssey_game.h"

#include "sim/data.h"

#include "luna/engine/scaled_renderer.h"

#include "game/game_rules.h"

#include "luna/engine/image_ops.h"

#include "core/log.h"
#include "core/version.h"
#include "game/art.h"
#include "game/object_art.h"
#include "luna/engine/image_ops.h"
#include "game/placeholder_art.h"
#include "game/region_level.h"
#include "luna/engine/physics_view.h"
#include "luna/engine/ui.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <numbers>
#include <string>

namespace odysseus::game {

namespace {
constexpr int kVirtualWidth = core::kVirtualWidth;
constexpr int kVirtualHeight = core::kVirtualHeight;
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

// The partner types of assets/data/sim/partner-types.json (US-293) are told to the data readers before anything that names one is read (the NPC classes, the kinds, the level).
Definitions loadDefinitionsAndPartnerTypes(const std::filesystem::path& dataDirectory) {
    sim::rules::setPartnerTypes(sim::rules::loadPartnerTypes(dataDirectory / "sim" / "partner-types.json"));
    return loadDefinitions(dataDirectory);
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
    : definitions_(loadDefinitionsAndPartnerTypes(dataDirectory)), levelFile_(chosenLevel(dataDirectory, levelFile)),
      level_(loadAndReport(levelFile_, definitions_)), map_(buildTileMap(level_, definitions_)),
      camera_(kVirtualWidth / GameSettings{}.cameraZoom, kVirtualHeight / GameSettings{}.cameraZoom, map_.pixelWidth(), map_.pixelHeight()),
      hero_(static_cast<double>(level_.heroStart.x), static_cast<double>(level_.heroStart.y)),
      range_(map_, loadMaterials(dataDirectory)), spritesDirectory_(dataDirectory.parent_path() / "sprites"),
      editor_(level_, definitions_, levelFile_, kVirtualWidth, kVirtualHeight), npcClasses_(dataDirectory / "npc-classes", dataDirectory / "npcs") {
    editor_.setNpcClasses(&npcClasses_);
    editor_.setGraphFolders(dataDirectory / "dialogue", dataDirectory / "interactions", [this] { reloadInteractions(); }); // Save in the graph editor reads the data again, like F5 (M9)
    // A placed plant may carry its own values for an interaction (US-173): the runner asks, when an action starts and when it ends.
    actions_.setAdjuster([this](const sim::rules::Interaction& base, const sim::rules::ThingRef& target) -> std::optional<sim::rules::Interaction> {
        if (target.kind != static_cast<int>(Subject::Kind::Plant)) return std::nullopt;
        const int index = plantIndexById(target.id);
        if (index < 0) return std::nullopt;
        std::optional<sim::rules::Interaction> changed;
        for (const ThingOverride& change : plants_[static_cast<std::size_t>(index)].overrides) {
            if (change.interaction != base.id) continue;
            if (!changed) changed = base;
            sim::rules::applyPatch(*changed, {change.field, change.valueMilli});
        }
        return changed;
    });
    // What the hero finishes counts for quests (US-181); other people's interactions do not.
    actions_.setFinishObserver([this](const std::string& interaction, int actor, const sim::rules::ThingRef&) {
        if (actor == kHeroActor) questEvent(sim::rules::QuestObjective::Kind::Interact, interaction);
    });
    npcCalendar_ = sim::loadCalendarConfig(dataDirectory / "sim" / "calendar.json");
    npcNeeds_ = sim::loadNeedsConfig(dataDirectory / "sim" / "needs.json");
    npcOpinions_ = sim::loadOpinionConfig(dataDirectory / "sim" / "opinions.json");
    tradeConfig_ = sim::loadPriceConfig(dataDirectory / "sim" / "trade.json");
    scheduleConfig_ = sim::rules::loadScheduleConfig(dataDirectory / "sim" / "schedule.json");
    eventCatalog_ = sim::loadEventCatalog(dataDirectory / "sim" / "events.json");
    dataDirectory_ = dataDirectory; // (set again below; the defaults of the partner types need it now)
    reloadPartnerDefaults();
    editor_.setLightPreview([this](double hour, const luna::engine::Rect& view) { return editorLightFrame(hour, view); });
    sky_ = loadSky(dataDirectory / "light" / "sky.json", dataDirectory / "sim" / "calendar.json");
    lighting_ = loadLighting(dataDirectory / "light" / "lights.json"); // a bad file stops the game with its name, like the other content
    catalogs_ = loadCatalogs(dataDirectory); // M2d content (US-130): weapons, plants, animals, effects, weather
    // Every `light` of the catalogs must name a kind of light of lights.json (US-243).
    {
        const auto check = [&](const std::string& light, const std::string& file, const std::string& where) {
            if (!light.empty() && lighting_.kind(light) == nullptr) {
                throw sim::DataError(dataDirectory / file, where + ".light", "\"" + light + "\" is not a kind of light in light/lights.json");
            }
        };
        for (const EffectDef& def : catalogs_.effects) check(def.light, "effects.json", "effect \"" + def.name + "\"");
        for (const WeaponDef& def : catalogs_.weapons) check(def.light, "weapons.json", "weapon \"" + def.name + "\"");
        for (const PlantDef& def : catalogs_.plants) check(def.light, def.object ? "objects.json" : "plants.json", "object \"" + def.name + "\"");
    }
    // The sun and moon (US-248): each names a kind of light of lights.json. A mistake in one of them, or in the eclipse file, does not stop the game:
    // the bad entry is left out, the level keeps its default pair, and the problem (file and field) is shown as a message.
    std::vector<std::string> celestialNotes = catalogs_.notes;
    std::erase_if(catalogs_.plants, [&](const PlantDef& def) {
        if (!def.celestial || lighting_.kind(def.sky.lightKind) != nullptr) return false;
        celestialNotes.push_back(sim::DataError(dataDirectory / "objects.json", "object \"" + def.name + "\".celestial.light",
                                                "\"" + def.sky.lightKind + "\" is not a kind of light in light/lights.json").what());
        return true;
    });
    for (const char* body : {"sun", "moon"}) {
        const bool has = std::any_of(catalogs_.plants.begin(), catalogs_.plants.end(), [&](const PlantDef& def) { return def.celestial && def.sky.followsClock && def.sky.body == body; });
        if (has) continue;
        PlantDef fallback; // the built-in default: a plain sun or moon on the usual orbit
        fallback.name = body;
        fallback.frame = body;
        fallback.object = true;
        fallback.celestial = true;
        fallback.tags = {"object", "celestial"};
        fallback.sky.body = body;
        catalogs_.plants.push_back(fallback);
    }
    for (const PlantDef& def : catalogs_.plants) {
        if (def.celestial && def.sky.followsClock) defaultBodies_.push_back(&def);
    }
    try {
        celestialEvents_ = loadCelestialEvents(dataDirectory / "light" / "celestial-events.json");
    } catch (const sim::DataError& problem) {
        celestialNotes.push_back(problem.what()); // no eclipses, the sky goes on
    }
    if (!celestialNotes.empty()) message_ = celestialNotes.front();
    dataDirectory_ = dataDirectory;
    saveDirectory_ = dataDirectory.parent_path() / "saves"; // next to the data and sprites; --save-dir chooses another folder
    if (std::filesystem::exists(dataDirectory / "hero")) heroData_ = sim::loadHeroData(dataDirectory); // a bad file stops the game with its name (US-060)
    {
        // The buildings of the data folder (US-250): a cost may only name an item the hero's data knows. A mistake leaves that file out and is shown.
        // Loaded before the interactions: the kinds give places their `blueprint-...` tags.
        std::set<std::string> items;
        if (heroData_) {
            for (const sim::Item& item : heroData_->items) items.insert(item.id);
        }
        buildings_.loadData(dataDirectory, items);
    }
    loadInteractions(); // after the hero data: the professions give people their `teaches-...` tags
    {
        BuildingEditor::Host host;
        host.data = &buildings_.dataMutable();
        host.prefabFolder = dataDirectory / "buildings" / "prefabs";
        host.saved = [this](const sim::buildings::KindDef& kind) {
            if (std::find(definitions_.buildingKinds.begin(), definitions_.buildingKinds.end(), kind.id) == definitions_.buildingKinds.end()) definitions_.buildingKinds.push_back(kind.id);
            if (kind.known) buildings_.learn(kind.id); // a prefab marked known is buildable at once
        };
        editor_.buildings().setHost(std::move(host));
        if (!buildings_.notes().empty()) {
            message_ = buildings_.notes().front();
            for (const std::string& note : buildings_.notes()) core::logWarning(note);
        }
    }
    {
        std::string note;
        settings_ = loadSettings(saveDirectory_ / "settings.json", &note);
        stats_.enable(settings_.statistics == 1);
        if (settings_.resolution != core::Resolution{}) pendingWindow_ = WindowChange{settings_.resolution};
        if (!note.empty()) message_ = note;
    }
    setViewScales(settings_.cameraZoom, settings_.uiScale);
    clanEnabled_ = level_.clan;
    weatherSeed_ = WeatherCycle::seedFromText(level_.name); // a level plays under the same weathers every time, unless --seed says otherwise
    weather_ = WeatherCycle(catalogs_.weather, weatherSeed_);
    std::vector<std::string> palette;
    for (const WeaponDef& weapon : catalogs_.weapons) {
        if (weapon.starter) {
            starters_.push_back(&weapon);
            palette.push_back(weapon.name);
        }
    }
    // The Editor offers the starters, then the two demo weapons (D-23).
    palette.push_back(kSpearThrowName);
    palette.push_back(kSwordSlashName);
    editor_.setWeaponPalette(std::move(palette));
    camera_.centreOn(hero_.feetX(), hero_.feetY());
    populate();
    buildNpcPopulation(); // the placed people are persons from the first frame, not only after a restart (X-M9a)
    if (clanEnabled_) startClan();
}

void OdysseyGame::resetPlay() {
    map_ = buildTileMap(level_, definitions_);
    camera_ = luna::engine::Camera(viewWidth(), viewHeight(), map_.pixelWidth(), map_.pixelHeight());
    hero_ = Hero(static_cast<double>(level_.heroStart.x), static_cast<double>(level_.heroStart.y));
    const MaterialsConfig materials = range_.materials();
    range_ = SpearRange(map_, materials);
    sword_ = Sword();
    framingTicks_ = 0;
    nextSpearIsFlint_ = true;
    heroHp_ = kHeroMaxHp;
    respawnTicks_ = 0;
    effects_.clear();
    weather_ = WeatherCycle(catalogs_.weather, weatherSeed_);
    if (clanEnabled_) startClan();
    if (region_) {
        rivals_ = std::make_unique<sim::Rivals>(*region_, region_->start(), region_->seed() ^ 0x5151ULL, sim::loadSimConfig(dataDirectory_));
    }
    projectiles_.clear();
    arcShots_.clear();
    attackCooldown_ = 0;
    swingTicks_ = 0;
    hotbar_ = {};     // a restart: empty hands, and every pickup lies in the level again
    heldSlot_ = 0;
    fullTicks_ = 0;
    actions_.clear(); // a restart drops what was under way and what was waiting
    actionClock_ = 0;
    bubbles_.clear(); // a restart starts the run's talk afresh
    greetingCooldowns_.clear();
    smalltalk_.clear();
    flags_.clear();
    quests_.clearStates();
    dialogueRng_ = core::Pcg32(1, 8);
    npcLife_.reset();
    camera_.centreOn(hero_.feetX(), hero_.feetY());
    populate();
    buildNpcPopulation(); // the placed people are persons of the simulation (US-262)
}

void OdysseyGame::switchMode(Mode mode) {
    if (mode == Mode::Editor && region_) {
        say("The Editor is for hand-made levels; a generated region cannot be edited");
        return;
    }
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
    if (mode == Mode::Editor) playHere_ = false;
    if (mode == Mode::Editor) {
        const luna::engine::Rect view = camera_.view();
        editor_.enter(view.x + view.width / 2.0, view.y + view.height / 2.0); // looking where the game looked
        core::logInfo("Mode: Editor");
    } else {
        resetPlay();
        core::logInfo(std::format("Mode: Game (level \"{}\", hero at ({}, {}))", level_.name, level_.heroStart.x, level_.heroStart.y));
    }
}

void OdysseyGame::drawModeLabel(luna::engine::Renderer& renderer) const {
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const char* label = mode_ == Mode::Game ? "GAME  F2: EDIT" : "EDITOR  F1: PLAY";
    const int width = luna::engine::UiPainter::textWidth(label) + 6;
    // Top right in the game; bottom right in the Editor, where the toolbar needs the top.
    const int top = mode_ == Mode::Game ? 2 : uiHeight() - luna::engine::kGlyphHeight - 6;
    const luna::engine::Rect box{uiWidth() - width - 2, top, width, luna::engine::kGlyphHeight + 6};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.text(box.x + 3, box.y + 3, label, mode_ == Mode::Game ? luna::engine::UiColor::Text : luna::engine::UiColor::Gold);
}

void OdysseyGame::populate() {
    populatePlants();
    buildings_.start(*this); // after the plants, so a wall never stands where a tree does
    startPlacedEffects();
    enemies_.clear();
    bystanders_.clear();
    pickups_.clear();
    for (const PlacedPickup& pickup : level_.pickups) pickups_.push_back({pickup, false});
    for (const PixelPoint& target : level_.targets) {
        range_.addTarget({luna::engine::metresFromPixels(target.x), luna::engine::metresFromPixels(target.y), luna::physics::kFixedZero});
    }
    // Every placed character the sword can hit stands in the world (M2c: they stand still, D-19).
    for (const PlacedCharacter& placed : level_.characters) {
        const CharacterKindDef* kind = definitions_.character(placed.kind);
        const bool fights = kind != nullptr && fightsHero(placed); // a kind file's attitude decides (US-264), else the old enemy switch
        if (kind != nullptr && !fights) {
            bystanders_.push_back(placed);
        }
        if (kind == nullptr || !fights) {
            continue;
        }
        Enemy enemy(placed.feet.x, placed.feet.y, placed.hp);
        enemy.id = placed.id;
        enemy.name = placed.name;
        enemy.frames = kind->frames;
        enemy.directions = kind->directions;
        enemy.animal = kind->animal;
        enemy.kindName = kind->name;
        enemy.facing = placed.facing;
        enemy.swordDamage = placed.swordDamage;
        enemy.reachMetres = kind->reach;
        enemies_.push_back(enemy);
    }
}

void OdysseyGame::playEffect(const std::string& name, double x, double y, int size) {
    const EffectDef* def = catalogs_.effect(name);
    if (!contentLoaded_ || def == nullptr) {
        return;
    }
    luna::engine::EffectSpec spec;
    spec.texture = effectsTexture_;
    spec.ticksPerFrame = def->ticksPerFrame;
    spec.loop = def->loop;
    for (int i = 0; i < def->frames; ++i) {
        if (const auto frame = content_.rect(content_.frameName(name, i))) spec.frames.push_back(*frame);
    }
    effects_.start(spec, x, y, size);
}

std::size_t OdysseyGame::carriedCount() const {
    return static_cast<std::size_t>(std::count_if(hotbar_.begin(), hotbar_.end(), [](const std::string& slot) { return !slot.empty(); }));
}

bool OdysseyGame::pickUp(const std::string& weapon) {
    for (std::string& slot : hotbar_) {
        if (slot.empty()) {
            slot = weapon;
            return true;
        }
    }
    return false;
}

void OdysseyGame::selectSlot(int slot) {
    if (slot < 0 || slot >= kHotbarSlots || slot == heldSlot_) return;
    heldSlot_ = slot;
    attackCooldown_ = 0;
    core::logInfo(std::format("Switched to {} (slot {})", heldName(), slot + 1));
}

// Shift: the next filled slot after the held one, wrapping round; nothing when none is filled.
void OdysseyGame::cycleSlot() {
    for (int step = 1; step <= kHotbarSlots; ++step) {
        const int slot = (heldSlot_ + step) % kHotbarSlots;
        if (!hotbar_[static_cast<std::size_t>(slot)].empty()) {
            selectSlot(slot);
            return;
        }
    }
}

std::string OdysseyGame::heldName() const {
    const std::string& name = hotbar_[static_cast<std::size_t>(heldSlot_)];
    return name.empty() ? "Empty hands" : name;
}

const WeaponDef* OdysseyGame::heldWeapon() const {
    return catalogs_.weapon(hotbar_[static_cast<std::size_t>(heldSlot_)]);
}

std::size_t OdysseyGame::pickupsLeft() const {
    return static_cast<std::size_t>(std::count_if(pickups_.begin(), pickups_.end(), [](const WorldPickup& p) { return !p.taken; }));
}

// Walking over a pickup puts its weapon in the first free slot and removes it until the level
// restarts; with no free slot it stays, and the player is told.
void OdysseyGame::collectPickups() {
    for (WorldPickup& lying : pickups_) {
        if (lying.taken) continue;
        const double distance = std::hypot(lying.pickup.at.x - hero_.feetX(), lying.pickup.at.y - hero_.feetY());
        if (distance > kPickupReach) continue;
        if (pickUp(lying.pickup.weapon)) {
            lying.taken = true;
            playEffect("spark", lying.pickup.at.x, lying.pickup.at.y, 20);
            core::logInfo(std::format("Picked up {} (slot {} of {})", lying.pickup.weapon,
                                      carriedCount(), kHotbarSlots));
        } else {
            if (fullTicks_ == 0) core::logInfo("Hotbar full: " + lying.pickup.weapon + " stays where it is");
            fullTicks_ = 40;
        }
    }
}

void OdysseyGame::strike(Enemy& enemy, int damage, const WeaponDef* weapon) {
    const int dealt = std::min(damage, enemy.hp());
    const bool defeated = enemy.takeDamage(damage);
    playEffect("spark", enemy.feetX(), enemy.feetY() - kCharacterHeight / 2.0, 24); // US-132: where the blow lands
    if (defeated) {
        if (!enemy.kindName.empty()) questEvent(sim::rules::QuestObjective::Kind::Defeat, questWord(enemy.kindName)); // the hero struck the blow (US-181)
        playEffect("smoke puff", enemy.feetX(), enemy.feetY() - kCharacterHeight / 3.0, 40);
    } else {
        enemy.provoke();
    }
    if (weapon != nullptr && weapon->element != Element::None) {
        applyElement(enemy, *weapon, dealt);
    }
}

// US-135: what the weapon's element does at the hit. The numbers are in weapons.json.
void OdysseyGame::applyElement(Enemy& target, const WeaponDef& weapon, int dealt) {
    const ElementDef& numbers = catalogs_.element(weapon.element);
    const double centreY = target.feetY() - kCharacterHeight / 2.0;
    switch (weapon.element) {
    case Element::Fire:
    case Element::Ice:
    case Element::Poison:
        if (target.isAlive()) {
            target.status.apply(weapon.element, numbers);
            playEffect(numbers.hitEffect, target.feetX(), centreY, 32);
            core::logInfo(std::format("{} is {} for {} s", target.name, weapon.element == Element::Fire ? "burning" : weapon.element == Element::Ice ? "slowed" : "poisoned", numbers.seconds));
        }
        break;
    case Element::Lightning: {
        // The jump: the nearest other living enemy within reach takes part of the damage (no second jump).
        Enemy* next = nullptr;
        double nearest = numbers.chainMetres * kTileSize;
        for (Enemy& other : enemies_) {
            if (&other == &target || !other.isAlive()) continue;
            const double distance = std::hypot(other.feetX() - target.feetX(), other.feetY() - target.feetY());
            if (distance < nearest || (distance == nearest && next == nullptr)) {
                next = &other;
                nearest = distance;
            }
        }
        if (next != nullptr) {
            const int jump = std::max(1, static_cast<int>(std::lround(weapon.damage * numbers.chainFraction)));
            playEffect(numbers.hitEffect, (target.feetX() + next->feetX()) / 2.0, (centreY + next->feetY() - kCharacterHeight / 2.0) / 2.0, 48);
            core::logInfo(std::format("Lightning jumped from {} to {}", target.name, next->name));
            strike(*next, jump);
        }
        break;
    }
    case Element::Void:
        if (dealt > 0 && respawnTicks_ == 0) {
            const int heal = std::max(1, static_cast<int>(std::lround(dealt * numbers.drainFraction)));
            heroHp_ = std::min(kHeroMaxHp, heroHp_ + heal);
            playEffect(numbers.hitEffect, target.feetX(), centreY, 32);
            playEffect(numbers.healEffect, hero_.feetX(), hero_.feetY() - kCharacterHeight / 2.0, 32);
            core::logInfo(std::format("The void drained {} HP from {}: the hero has {} / {}", heal, target.name, heroHp_, kHeroMaxHp));
        }
        break;
    case Element::None:
        break;
    }
}

// One tick of an enemy's statuses: burning and poison cost HP (no red flash each time), and their
// effects are started again each time the last one ends, so they read as continuous while they last.
void OdysseyGame::tickStatus(Enemy& enemy) {
    if (!enemy.isAlive() || !enemy.status.any()) {
        return;
    }
    const double centreY = enemy.feetY() - kCharacterHeight / 2.0;
    const auto show = [&](Element element) {
        const std::string& name = catalogs_.element(element).effect;
        const EffectDef* def = catalogs_.effect(name);
        if (def != nullptr && ticks_ % static_cast<std::uint64_t>(def->frames * def->ticksPerFrame) == 0) {
            playEffect(name, enemy.feetX(), centreY, 24);
        }
    };
    if (enemy.status.burning()) show(Element::Fire);
    if (enemy.status.poisoned()) show(Element::Poison);
    if (enemy.status.slowed()) show(Element::Ice);
    const int lost = enemy.status.tick();
    if (lost > 0 && enemy.takeDamage(lost, false)) {
        playEffect("smoke puff", enemy.feetX(), enemy.feetY() - kCharacterHeight / 3.0, 40);
    }
}

// The clan's simulation: the same world for the same level every time (its seed is the hash of the level's name).
void OdysseyGame::startClan() {
    sim::SimConfig simConfig = sim::loadSimConfig(dataDirectory_);
    if (startingPeople_ > 0) simConfig.clan.startingPeople = startingPeople_; // a performance run (US-234), past the file's 200 limit
    clan_ = std::make_unique<sim::World>(WeatherCycle::seedFromText(level_.name) ^ 0x9E3779B97F4A7C15ULL, simConfig);
    exchanges_.reset(clan_->chronicle().entries().size());
    // The camp is where the fire burns: the first flame placed in the level, else where the hero starts.
    PixelPoint camp = level_.heroStart;
    for (const PlacedEffect& placed : level_.effects) {
        if (placed.name == "flame" || placed.name == "big fire") {
            camp = placed.at;
            break;
        }
    }
    clanView_ = ClanView(camp);
    clanView_.update(*clan_, map_); // everybody appears at the fire at once
    if (!layerSheets_) {
        layerSheets_ = makeLayerSheets();
    }
}

void OdysseyGame::setClan(bool on) {
    clanEnabled_ = on;
    if (on) {
        startClan();
    } else {
        clan_.reset();
    }
}

// People drawn from their layers: each distinct look is composed once into a sheet; the figure picks its frame by
// facing and walking step. Figures above the hero's feet are drawn before him, the others after.
void OdysseyGame::drawClan(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha, bool behindHero) {
    if (!clan_) return;
    const double heroY = hero_.feetY(alpha);
    std::vector<const Figure*> order;
    for (const Figure& figure : clanView_.figures()) {
        if (figure.present && (figure.feetY(alpha) <= heroY) == behindHero) order.push_back(&figure);
    }
    std::sort(order.begin(), order.end(), [alpha](const Figure* a, const Figure* b) { return a->feetY(alpha) < b->feetY(alpha); });
    for (const Figure* figure : order) {
        const luna::engine::Texture lookSheet = lookTexture(renderer, figure->look);
        const int width = figure->child ? kCharacterWidth * 3 / 4 : kCharacterWidth;
        const int height = figure->child ? kCharacterHeight * 3 / 4 : kCharacterHeight;
        const int shiver = figure->emote == Emote::Cold ? ((ticks_ / 2) % 2 == 0 ? -1 : 1) : 0; // the cold makes them shake
        const int feetX = static_cast<int>(std::lround(figure->feetX(alpha))) - view.x + shiver;
        const int feetY = static_cast<int>(std::lround(figure->feetY(alpha))) - view.y;
        const luna::engine::Rect source{figure->animationFrame() * kCharacterWidth, static_cast<int>(figure->facing) * kCharacterHeight, kCharacterWidth, kCharacterHeight};
        renderer.drawStyled(lookSheet, source, {feetX - width / 2, feetY - height, width, height}, {});
        if (figure->emote != Emote::None) drawEmote(renderer, figure->emote, feetX, feetY - height - 14);
        if (const Bubble* said = bubbles_.of(static_cast<int>(figure - clanView_.figures().data()))) {
            // Words over the head, above the face of a need if there is one.
            luna::engine::UiPainter painter(renderer, uiSheet_);
            const int w = luna::engine::UiPainter::textWidth(said->text) + 8;
            const luna::engine::Rect box{feetX - w / 2, feetY - height - 14 - (figure->emote != Emote::None ? 14 : 0) - 2, w, 14};
            painter.fill(box, luna::engine::UiColor::Shade);
            painter.outline(box, luna::engine::UiColor::Border);
            painter.text(box.x + 4, box.y + 4, said->text, luna::engine::UiColor::Text);
        }
    }
}

// A small speech bubble over the head: a snowflake when cold, food when hungry, + when unwell, z when tired, ? when lonely.
void OdysseyGame::drawEmote(luna::engine::Renderer& renderer, Emote emote, int x, int y) const {
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const luna::engine::Rect bubble{x - 7, y, 14, 12};
    painter.fill(bubble, luna::engine::UiColor::Shade);
    painter.outline(bubble, luna::engine::UiColor::Border);
    const luna::engine::Rect inside{bubble.x + 1, bubble.y + 1, bubble.width - 2, bubble.height - 2};
    switch (emote) {
    case Emote::Cold:
        drawEffectPicture(renderer, effectArt_, "ice shards", inside);
        break;
    case Emote::Hungry:
        drawPlantIcon(renderer, plantArt_, "carrot top", inside);
        break;
    case Emote::Sick: painter.text(bubble.x + 4, bubble.y + 3, "+", luna::engine::UiColor::Red); break;
    case Emote::Tired: painter.text(bubble.x + 4, bubble.y + 3, "z", luna::engine::UiColor::Gold); break;
    case Emote::Lonely: painter.text(bubble.x + 4, bubble.y + 3, "?", luna::engine::UiColor::Dim); break;
    default: break;
    }
}

// Hover a person: their exact needs and what they are doing now.
void OdysseyGame::drawClanDetails(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const {
    if (!clan_ || pointerX_ < 0) return;
    const Figure* hovered = nullptr;
    std::size_t index = 0;
    for (std::size_t i = 0; i < clanView_.figures().size(); ++i) {
        const Figure& figure = clanView_.figures()[i];
        if (!figure.present) continue;
        const int feetX = static_cast<int>(std::lround(figure.feetX(alpha))) - view.x;
        const int feetY = static_cast<int>(std::lround(figure.feetY(alpha))) - view.y;
        const int height = figure.child ? kCharacterHeight * 3 / 4 : kCharacterHeight;
        const int halfWidth = (figure.child ? kCharacterWidth * 3 / 4 : kCharacterWidth) / 2;
        if (pointerX_ >= feetX - halfWidth && pointerX_ < feetX + halfWidth && pointerY_ >= feetY - height && pointerY_ <= feetY &&
            (hovered == nullptr || figure.feetY(alpha) > hovered->feetY(alpha))) {
            hovered = &figure;
            index = i;
        }
    }
    if (hovered == nullptr) return;
    const sim::Person& person = clan_->people().at(index);
    const int daysPerYear = clan_->calendar().daysPerYear();
    std::vector<std::pair<std::string, luna::engine::UiColor>> lines;
    lines.push_back({person.name, luna::engine::UiColor::Gold});
    lines.push_back({std::format("{}, age {}", person.sex == sim::Sex::Female ? "woman" : "man", person.ageYears(daysPerYear)), luna::engine::UiColor::Text});
    lines.push_back({std::format("now: {}", sim::actionName(person.action)), luna::engine::UiColor::Text});
    lines.push_back({std::format("Hunger {}  Energy {}", person.needs[sim::Need::Hunger], person.needs[sim::Need::Energy]), luna::engine::UiColor::Text});
    lines.push_back({std::format("Warmth {}  Social {}", person.needs[sim::Need::Warmth], person.needs[sim::Need::Social]), luna::engine::UiColor::Text});
    if (hovered->emote != Emote::None) lines.push_back({std::string("feels ") + emoteName(hovered->emote), luna::engine::UiColor::Red});
    luna::engine::UiPainter painter(renderer, uiSheet_);
    int width = 0;
    for (const auto& line : lines) width = std::max(width, luna::engine::UiPainter::textWidth(line.first));
    width += 8;
    const int height = static_cast<int>(lines.size()) * luna::engine::kLineHeight + 4;
    luna::engine::Rect box{pointerX_ + 10, pointerY_ + 10, width, height};
    box.x = std::min(box.x, viewWidth() - width - 2);
    box.y = std::min(box.y, viewHeight() - height - 2);
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.outline(box, luna::engine::UiColor::Gold);
    int y = box.y + 3;
    for (const auto& line : lines) {
        painter.text(box.x + 4, y, line.first, line.second);
        y += luna::engine::kLineHeight;
    }
}

// The date, the weather and the clan's numbers, top centre.
void OdysseyGame::drawClanHud(luna::engine::Renderer& renderer) const {
    if (!clan_) return;
    const int hour = static_cast<int>((clan_->ticks() % static_cast<std::uint64_t>(clan_->calendar().ticksPerDay())) / static_cast<std::uint64_t>(clan_->calendar().ticksPerHour()));
    const std::string line = std::format("{} {}  {:02}:00  {}C  pop {}  food {}", sim::seasonName(clan_->date().season), clan_->date().dayOfSeason, hour, clan_->temperature(),
                                         clan_->population(), clan_->food());
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const int width = luna::engine::UiPainter::textWidth(line) + 8;
    const luna::engine::Rect box{(uiWidth() - width) / 2, 20, width, luna::engine::kGlyphHeight + 6};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.text(box.x + 4, box.y + 3, line, luna::engine::UiColor::Text);
    if (!message_.empty()) {
        const int messageWidth = luna::engine::UiPainter::textWidth(message_) + 8;
        const luna::engine::Rect messageBox{std::max(2, (uiWidth() - messageWidth) / 2), 48, std::min(messageWidth, uiWidth() - 4), luna::engine::kGlyphHeight + 6};
        painter.fill(messageBox, luna::engine::UiColor::Shade);
        painter.outline(messageBox, luna::engine::UiColor::Gold);
        painter.text(messageBox.x + 4, messageBox.y + 3, message_.substr(0, static_cast<std::size_t>((messageBox.width - 8) / luna::engine::kTextAdvance)), luna::engine::UiColor::Gold);
    }
}

// Starts a run (US-050): the region of the seed, a clan made for the Comfort level, a hero of it at the preset's age.
void OdysseyGame::startNewRun(const sim::NewGame& game, bool useRegion, bool tutorial) {
    if (!heroData_) return;
    if (useRegion) loadRegion(game.seed);
    clan_ = std::make_unique<sim::World>(game.seed, sim::configForComfort(*heroData_, sim::loadSimConfig(dataDirectory_), game.comfort));
    life_ = std::make_unique<sim::HeroLife>(*heroData_, *clan_, game);
    watchHeroItems();
    clanEnabled_ = true;
    clanView_ = ClanView(clanView_.camp());
    clanView_.setHidden(life_->personId());
    clanView_.update(*clan_, map_);
    lastSavedDay_ = -1;
    buildings_.forgetAll(); // a new run knows only the starting blueprints (D-55 Q2)
    // A new clan is a new story: what was said, remembered and flagged in the last one does not carry over (US-162..US-164).
    flags_.clear();
    quests_.clearStates();
    bubbles_.clear();
    exchanges_.reset(clan_->chronicle().entries().size());
    greetingCooldowns_.clear();
    smalltalk_.clear();
    stats_.record("run-started", ticks_);
    if (tutorial) {
        if (tutorialScript_.steps.empty()) tutorialScript_ = loadTutorial(dataDirectory_ / "hero" / "tutorial.json");
        tutorial_.start(tutorialScript_);
    } else {
        tutorial_.stop();
    }
    if (life_->phase() == sim::Phase::Growing) {
        runFlow_.openFocus();
    } else {
        runFlow_.openMantle();
    }
    core::logInfo(std::format("New run: seed {}, {} preset, {} comfort; the hero is {}, age {}, {}", game.seed, life_->preset().name, heroData_->config.comforts.at(static_cast<std::size_t>(life_->game().comfort)).name,
                              life_->name(), life_->ageYears(), life_->origin()));
}

void OdysseyGame::setStatistics(bool agreed) {
    settings_.statistics = agreed ? 1 : 2;
    stats_.enable(agreed);
    saveSettings(settings_, saveDirectory_ / "settings.json");
}

std::filesystem::path OdysseyGame::finishSession() {
    stats_.record("session-ended", ticks_);
    return stats_.finish(saveDirectory_ / "sessions", ticks_, sessionStamp());
}

// The elder speaks in a box at the bottom while the first day is taught (US-090).
void OdysseyGame::drawTutorial(luna::engine::Renderer& renderer) const {
    if (!tutorial_.active() || runFlow_.modal()) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const std::string line = tutorial_.elder() + ": " + tutorial_.text();
    const int width = std::min(uiWidth() - 8, luna::engine::UiPainter::textWidth(line) + 8);
    const int chars = (width - 8) / luna::engine::kTextAdvance;
    const luna::engine::Rect box{(uiWidth() - width) / 2, uiHeight() - 2 * luna::engine::kGlyphHeight - 30, width, luna::engine::kGlyphHeight + 6};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.outline(box, tutorial_.hinting() ? luna::engine::UiColor::Gold : luna::engine::UiColor::Text);
    painter.text(box.x + 4, box.y + 3, line.substr(0, static_cast<std::size_t>(chars)), luna::engine::UiColor::Text);
}

void OdysseyGame::afterYear() {
    stats_.record("year-lived", ticks_);
    if (!clan_) return;
    clanView_.update(*clan_, map_);
    lastSavedDay_ = clan_->date().day;
}

// Gathering takes a plant without the healing and the leaves of hitting it; it grows back as any plant does.
bool OdysseyGame::harvestPlant(std::size_t index) {
    if (index >= plants_.size() || !plants_[index].alive) return false;
    destroyPlant(index, false);
    return true;
}

int OdysseyGame::plantAtWorld(double x, double y) const {
    int found = -1;
    int bestY = -1;
    for (std::size_t i = 0; i < plants_.size(); ++i) {
        const WorldPlant& plant = plants_[i];
        if (!plant.present()) continue;
        const luna::engine::Rect extent = plantExtent(plantArt_, plant.kind);
        if (x >= plant.feet.x - extent.width / 2.0 && x < plant.feet.x + extent.width / 2.0 && y >= plant.feet.y - extent.height && y <= plant.feet.y && plant.feet.y > bestY) {
            found = static_cast<int>(i);
            bestY = plant.feet.y;
        }
    }
    return found;
}

int OdysseyGame::personAtWorld(double x, double y) const {
    int found = -1;
    double bestY = -1.0;
    for (std::size_t i = 0; i < clanView_.figures().size(); ++i) {
        const Figure& figure = clanView_.figures()[i];
        if (!figure.present) continue;
        const double halfWidth = (figure.child ? kCharacterWidth * 3 / 4 : kCharacterWidth) / 2.0;
        const double height = figure.child ? kCharacterHeight * 3 / 4 : kCharacterHeight;
        if (x >= figure.x - halfWidth && x < figure.x + halfWidth && y >= figure.y - height && y <= figure.y && figure.y > bestY) {
            found = static_cast<int>(i);
            bestY = figure.y;
        }
    }
    return found;
}

std::vector<int> OdysseyGame::attendeesAt(double x, double y, int radiusTiles) const {
    std::vector<int> near;
    for (std::size_t i = 0; i < clanView_.figures().size(); ++i) {
        const Figure& figure = clanView_.figures()[i];
        if (figure.present && std::hypot(figure.x - x, figure.y - y) <= radiusTiles * kTileSize) near.push_back(static_cast<int>(i));
    }
    return near;
}

// assets/data/dialogue/smalltalk.json, when there is one (the game plays without small talk when it is not there). Mistakes go in `report`.
sim::rules::SmallTalk OdysseyGame::loadSmalltalk(sim::rules::LoadReport& report) const {
    const std::filesystem::path file = dataDirectory_ / "dialogue" / "smalltalk.json";
    std::error_code ec;
    if (!std::filesystem::exists(file, ec)) return {};
    ++report.filesRead;
    if (auto data = sim::rules::SmalltalkData::load(file, "dialogue/smalltalk.json", report)) return sim::rules::SmallTalk(std::move(*data));
    return {};
}

// The Editor's action checkboxes are the interactions of the registry (US-268).
void OdysseyGame::syncEditorActions() {
    std::vector<std::string> ids;
    for (const sim::rules::Interaction& interaction : interactions_.all()) ids.push_back(interaction.id);
    std::sort(ids.begin(), ids.end());
    editor_.setActionIds(std::move(ids));
}

// What the graph editor's check may name (US-175): the game's items, built-in actions, conversations, interactions and tags.
void OdysseyGame::syncGraphCatalog() {
    sim::rules::GraphCatalog catalog;
    if (heroData_) {
        for (const sim::Item& item : heroData_->items) catalog.items.insert(item.id);
    }
    catalog.builtins.insert(builtInActionNames().begin(), builtInActionNames().end());
    for (const sim::rules::DlgScript& script : dialogues_.all()) catalog.dialogues.insert(script.name);
    for (const sim::rules::Interaction& interaction : interactions_.all()) catalog.interactions.insert(interaction.id);
    catalog.tags = knownTags();
    editor_.graphs().setCatalog(std::move(catalog));
}

void OdysseyGame::loadInteractions() {
    // At start every file that reads cleanly loads; one with mistakes is left out and named in the log and the panel.
    sim::rules::LoadOptions options;
    options.knownTags = knownTags(); // an interaction aimed at a tag nothing carries gets a warning naming file and tag
    options.knownBuiltins.insert(builtInActionNames().begin(), builtInActionNames().end());
    interactionReport_ = {};
    interactions_ = sim::rules::InteractionRegistry::load(dataDirectory_ / "interactions", interactionReport_, options);
    sim::rules::LoadReport dialogueReport;
    dialogues_ = sim::rules::DialogueLibrary::load(dataDirectory_ / "dialogue", dialogueReport);
    smalltalk_ = loadSmalltalk(dialogueReport);
    sim::rules::LoadReport questReport;
    quests_.replaceQuests(sim::rules::loadQuests(dataDirectory_ / "quests", questReport));
    syncEditorActions();
    syncGraphCatalog();
    interactionReport_.errors.insert(interactionReport_.errors.end(), dialogueReport.errors.begin(), dialogueReport.errors.end());
    interactionReport_.warnings.insert(interactionReport_.warnings.end(), dialogueReport.warnings.begin(), dialogueReport.warnings.end());
    interactionReport_.filesRead += dialogueReport.filesRead;
    interactionReport_.errors.insert(interactionReport_.errors.end(), questReport.errors.begin(), questReport.errors.end());
    interactionReport_.filesRead += questReport.filesRead;
    core::logInfo(std::format("Quests: {} loaded from {} file(s), {} error(s)", questReport.loaded, questReport.filesRead, questReport.errors.size()));
    core::logInfo(std::format("Dialogue: {} conversation(s) loaded from {} file(s)", dialogueReport.loaded, dialogueReport.filesRead));
    for (const sim::rules::Diagnostic& d : interactionReport_.errors) core::logWarning("Interactions: " + d.text());
    for (const sim::rules::Diagnostic& d : interactionReport_.warnings) core::logWarning("Interactions: " + d.text());
    core::logInfo(std::format("Interactions: {} loaded from {} file(s), {} error(s)", interactionReport_.loaded, interactionReport_.filesRead, interactionReport_.errors.size()));
}

// F5 (US-156): all or nothing. The files are read into a registry on the side; only a clean one replaces the data in use, so a typo
// never leaves the game half-reloaded.
bool OdysseyGame::reloadInteractions() {
    const auto started = std::chrono::steady_clock::now();
    sim::rules::LoadOptions options;
    options.knownTags = knownTags();
    options.knownBuiltins.insert(builtInActionNames().begin(), builtInActionNames().end());
    sim::rules::LoadReport report;
    sim::rules::InteractionRegistry fresh = sim::rules::InteractionRegistry::load(dataDirectory_ / "interactions", report, options);
    sim::rules::LoadReport dialogueReport;
    sim::rules::DialogueLibrary freshDialogue = sim::rules::DialogueLibrary::load(dataDirectory_ / "dialogue", dialogueReport);
    sim::rules::SmallTalk freshSmalltalk = loadSmalltalk(dialogueReport);
    sim::rules::LoadReport questReport;
    std::vector<sim::rules::Quest> freshQuests = sim::rules::loadQuests(dataDirectory_ / "quests", questReport);
    report.errors.insert(report.errors.end(), questReport.errors.begin(), questReport.errors.end());
    report.filesRead += questReport.filesRead;
    report.errors.insert(report.errors.end(), dialogueReport.errors.begin(), dialogueReport.errors.end());
    report.warnings.insert(report.warnings.end(), dialogueReport.warnings.begin(), dialogueReport.warnings.end());
    report.filesRead += dialogueReport.filesRead;
    lastInteractionReloadMs_ = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    interactionReport_ = report; // the panel always shows the latest result
    for (const sim::rules::Diagnostic& d : report.errors) core::logWarning("Interactions: " + d.text());
    for (const sim::rules::Diagnostic& d : report.warnings) core::logWarning("Interactions: " + d.text());
    if (!report.errors.empty()) {
        core::logWarning(std::format("Interactions: reload found {} mistake(s); the last good data stays in use", report.errors.size()));
        return false;
    }
    interactions_ = std::move(fresh);
    syncEditorActions();
    dialogues_ = std::move(freshDialogue);
    smalltalk_ = std::move(freshSmalltalk);
    quests_.replaceQuests(std::move(freshQuests));
    syncGraphCatalog();
    core::logInfo(std::format("Interactions reloaded: {} from {} file(s) in {:.1f} ms", report.loaded, report.filesRead, lastInteractionReloadMs_));
    return true;
}

// The mistakes, top of the screen, until the files are fixed and F5 is pressed again.
void OdysseyGame::drawInteractionPanel(luna::engine::Renderer& renderer) const {
    if (interactionReport_.errors.empty()) return;
    constexpr std::size_t kMaxLines = 8;
    const int width = uiWidth() - 20;
    const int maxChars = (width - 8) / luna::engine::kTextAdvance;
    const std::size_t shown = std::min(kMaxLines, interactionReport_.errors.size());
    const luna::engine::Rect box{10, 34, width, static_cast<int>(shown + 2) * luna::engine::kLineHeight + 6};
    luna::engine::UiPainter painter(renderer, uiSheet_);
    painter.fill(box, luna::engine::UiColor::Panel);
    painter.outline(box, luna::engine::UiColor::Red);
    painter.text(box.x + 4, box.y + 4, std::format("Interaction files: {} mistake(s). Fix them, then press F5.", interactionReport_.errors.size()), luna::engine::UiColor::Gold);
    int y = box.y + 4 + luna::engine::kLineHeight;
    for (std::size_t i = 0; i < shown; ++i) {
        std::string line = interactionReport_.errors[i].text();
        if (static_cast<int>(line.size()) > maxChars) line = line.substr(0, static_cast<std::size_t>(maxChars - 3)) + "...";
        painter.text(box.x + 4, y, line, luna::engine::UiColor::Red);
        y += luna::engine::kLineHeight;
    }
    if (interactionReport_.errors.size() > shown) {
        painter.text(box.x + 4, y, std::format("...and {} more (see the log)", interactionReport_.errors.size() - shown), luna::engine::UiColor::Dim);
    }
}
std::set<std::string> OdysseyGame::knownTags() const {
    std::set<std::string> tags = catalogs_.knownTags();
    for (const CharacterKindDef& kind : definitions_.characters) tags.insert(kind.tags.begin(), kind.tags.end());
    for (const std::string& tag : builtInThingTags(*this)) tags.insert(tag); // the game's own things: people, fires, the stone, camps
    return tags;
}

sim::rules::ThingInfo OdysseyGame::plantThing(std::size_t index) const {
    const WorldPlant& plant = plants_.at(index);
    return {plant.kind, plant.def != nullptr ? plant.def->tags : std::vector<std::string>{}};
}

std::vector<sim::rules::Offer> OdysseyGame::plantOffers(std::size_t index) const {
    return offersFor(plantSubject(*this, index));
}

std::vector<sim::rules::Offer> OdysseyGame::offersFor(const Subject& subject) const {
    const double pixels = std::hypot(subject.x - hero_.feetX(), subject.y - hero_.feetY());
    const sim::rules::ThingInfo hero{"hero", {"hero", "person"}};
    const GameRuleContext context(*this, subject);
    return interactions_.offered(hero, subject.info, static_cast<long long>(pixels * 1000.0 / kTileSize), context);
}

void OdysseyGame::setPlantState(std::size_t index, const std::string& state) {
    if (index >= plants_.size()) return;
    plants_[index].state = state;
    if (state == "burning") postWorldEvent("fire", plants_[index].feet.x, plants_[index].feet.y); // a fire has started: the people near it may go to help (US-291)
}

bool OdysseyGame::helpPerson(int personId, sim::Need need, int amount) {
    if (!clan_ || personId < 0 || static_cast<std::size_t>(personId) >= clan_->people().size() || !clan_->people()[static_cast<std::size_t>(personId)].alive) return false;
    clan_->satisfyPersonNeed(personId, need, amount);
    return true;
}

bool OdysseyGame::rememberConversation(int holder, int other, const std::string& text, int feeling) {
    return clan_ && clan_->rememberConversation(holder, other, text, feeling);
}

void OdysseyGame::chronicleLine(const std::string& text, int who, int other) {
    if (clan_) clan_->note(text, sim::kImportanceConversation, sim::EventKind::Note, who, other);
}

void OdysseyGame::changeOpinion(int who, int about, int delta) {
    const auto count = clan_ ? static_cast<int>(clan_->people().size()) : 0;
    if (who < 0 || about < 0 || who >= count || about >= count) return;
    clan_->adjustOpinion(who, about, delta);
}

bool OdysseyGame::harmPerson(int personId, sim::Need need, int amount) {
    if (!clan_ || personId < 0 || static_cast<std::size_t>(personId) >= clan_->people().size() || !clan_->people()[static_cast<std::size_t>(personId)].alive) return false;
    clan_->drainPersonNeed(personId, need, amount);
    return true;
}

int OdysseyGame::plantIndexById(int id) const {
    for (std::size_t i = 0; i < plants_.size(); ++i) {
        if (plants_[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

// One play tick of the timed actions (US-153). The hero's action stops, with nothing given, when the player moves or attacks.
void OdysseyGame::tickActions(const luna::engine::Intents& intents) {
    ++actionClock_;
    if (actions_.running(0) != nullptr) {
        using luna::engine::Intent;
        const bool moving = intents.held(Intent::MoveUp) || intents.held(Intent::MoveDown) || intents.held(Intent::MoveLeft) || intents.held(Intent::MoveRight);
        if (moving || intents.held(Intent::Attack)) {
            actions_.cancel(0);
            core::logInfo("The hero stopped what they were doing");
        }
    }
    tickInteractions(*this);
}

// A ring of twelve dots over the target, filling as the action's time runs out (D-36).
void OdysseyGame::drawActionRing(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const {
    const sim::rules::RunningAction* action = actions_.running(0);
    if (action == nullptr) return;
    const std::optional<Subject> subject = subjectFor(*this, action->target);
    if (!subject) return;
    static constexpr int kDots[12][2] = {{0, -8}, {4, -7}, {7, -4}, {8, 0}, {7, 4}, {4, 7}, {0, 8}, {-4, 7}, {-7, 4}, {-8, 0}, {-7, -4}, {-4, -7}};
    const int filled = actions_.progress(0, actionClock_) * 12 / 100;
    const int centreX = static_cast<int>(std::lround(subject->x)) - view.x;
    const int centreY = static_cast<int>(std::lround(subject->y)) - view.y - 36; // above the target's head
    luna::engine::UiPainter painter(renderer, uiSheet_);
    for (int i = 0; i < 12; ++i) {
        painter.fill({centreX + kDots[i][0] - 1, centreY + kDots[i][1] - 1, 3, 3}, i < filled ? luna::engine::UiColor::Gold : luna::engine::UiColor::Dark);
    }
}

// What is saved besides the clan and the hero (US-153): the plants that are not in their starting state, and the effects waiting for
// their time (the berries ripening again). Running actions are not saved; their effects had not happened yet.
std::string OdysseyGame::thingsText() const {
    nlohmann::json plants = nlohmann::json::object();
    for (const WorldPlant& plant : plants_) {
        if (plant.def != nullptr && !plant.def->states.empty() && plant.state != plant.def->states.front()) plants[std::to_string(plant.id)] = plant.state;
    }
    return nlohmann::json{{"version", 2}, {"plants", plants}, {"timers", nlohmann::json::parse(actions_.savePending(actionClock_))}, {"flags", nlohmann::json::parse(flags_.save())}, {"quests", nlohmann::json::parse(quests_.save(actionClock_))}}.dump(1);
}

std::vector<std::string> OdysseyGame::restoreThings(const std::string& text) {
    std::vector<std::string> notes;
    try {
        const nlohmann::json data = nlohmann::json::parse(text);
        if (const int version = data.at("version").get<int>(); version != 1 && version != 2) return {"things.json was saved by another version and was not loaded"};
        for (const auto& [id, state] : data.at("plants").items()) {
            const int index = plantIndexById(std::stoi(id));
            if (index >= 0) plants_[static_cast<std::size_t>(index)].state = state.get<std::string>();
        }
        notes = actions_.loadPending(data.at("timers").dump(), actionClock_);
        if (data.contains("flags")) { // saves from before US-164 have none
            const std::vector<std::string> flagNotes = flags_.load(data.at("flags").dump());
            notes.insert(notes.end(), flagNotes.begin(), flagNotes.end());
        }
        if (data.contains("quests")) { // saves from before US-180 (version 1) have none: every quest starts locked
            const std::vector<std::string> questNotes = quests_.load(data.at("quests").dump(), actionClock_);
            notes.insert(notes.end(), questNotes.begin(), questNotes.end());
        }
    } catch (const std::exception& error) {
        notes.push_back(std::string("things.json could not be read: ") + error.what());
    }
    return notes;
}

void OdysseyGame::applySettings(const GameSettings& settings) {
    settings_ = settings;
    saveSettings(settings_, saveDirectory_ / "settings.json");
    pendingWindow_ = WindowChange{settings_.resolution};
    setViewScales(settings_.cameraZoom, settings_.uiScale);
}

GameClock OdysseyGame::gameClock() const {
    GameClock clock; // no clan, no clock: noon of a spring day
    if (!clan_) return clock;
    const std::uint64_t ticks = clan_->ticks();
    const auto perDay = static_cast<std::uint64_t>(clan_->calendar().ticksPerDay());
    clock.hour = static_cast<double>(ticks % perDay) * 24.0 / static_cast<double>(perDay);
    clock.day = static_cast<int>(ticks / perDay);
    clock.season = static_cast<int>(clan_->calendar().dateAt(ticks).season);
    return clock;
}

SkyState OdysseyGame::sky() const {
    const GameClock clock = gameClock();
    return skyAt(sky_, clock.hour, clock.season);
}

std::vector<CelestialBody> OdysseyGame::celestialBodies() const {
    std::vector<CelestialBody> placed;
    for (const WorldPlant& plant : plants_) {
        if (plant.alive && plant.def != nullptr && plant.def->celestial) placed.push_back({plant.def, static_cast<double>(plant.feet.x), static_cast<double>(plant.feet.y)});
    }
    return bodiesOf(defaultBodies_, placed);
}

CelestialLight OdysseyGame::celestialLight(double alpha) const {
    return currentLight(celestialBodies(), sky_, lighting_, celestialEvents_, gameClock(), hero_.feetX(alpha), hero_.feetY(alpha));
}

// The sun and the moon in the sky band at the top of the picture, moving with the clock (US-248). Drawn unlit: they are the light.
void OdysseyGame::drawSkyBodies(luna::engine::Renderer& renderer, double alpha) const {
    const int width = viewWidth();
    const int band = viewHeight() / 4;
    for (const SkySprite& sprite : skySprites(celestialBodies(), sky_, celestialEvents_, gameClock(), hero_.feetX(alpha), hero_.feetY(alpha))) {
        const int x = 20 + static_cast<int>(sprite.x * (width - 40));
        const int y = 8 + kObjectPictureSize + static_cast<int>(sprite.y * (band - kObjectPictureSize));
        drawPlant(renderer, plantArt_, sprite.name, x, y);
    }
}

void OdysseyGame::setViewScales(int cameraZoom, int uiScale) {
    settings_.cameraZoom = std::clamp(cameraZoom, 1, 2);
    settings_.uiScale = std::clamp(uiScale, 1, 2);
    camera_.setViewSize(viewWidth(), viewHeight()); // zooming keeps the hero where he is on the picture
}

// The zoom keys and the mouse wheel in play (US-232): one step in or out, kept in the settings file.
void OdysseyGame::updateZoom(const luna::engine::Intents& intents) {
    int step = 0;
    if (intents.pressed(luna::engine::Intent::ZoomIn)) ++step;
    if (intents.pressed(luna::engine::Intent::ZoomOut)) --step;
    if (intents.pointer().inside()) step += intents.pointer().wheel > 0 ? 1 : (intents.pointer().wheel < 0 ? -1 : 0);
    if (step == 0) return;
    const int zoom = std::clamp(settings_.cameraZoom + step, 1, 2); // zoom in = bigger pixels = the higher number
    if (zoom == settings_.cameraZoom) return;
    GameSettings changed = settings_;
    changed.cameraZoom = zoom;
    applySettings(changed);
}

std::optional<luna::engine::Game::WindowChange> OdysseyGame::takeWindowChange() {
    const auto change = pendingWindow_;
    pendingWindow_.reset();
    return change;
}

// The hero's name, age and standing, under the date line.
void OdysseyGame::drawRunHud(luna::engine::Renderer& renderer) const {
    if (!life_ || runFlow_.modal()) return;
    std::string specialty;
    for (const std::string& n : life_->specialtyNames()) specialty += (specialty.empty() ? "" : "/") + n;
    const std::string line = std::format("{}, {}{}  Trade {}%  Religion {}%", life_->name(), life_->ageYears(), specialty.empty() ? "" : " " + specialty, life_->tradePercent(), life_->religionPercent());
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const int width = luna::engine::UiPainter::textWidth(line) + 8;
    const luna::engine::Rect box{(uiWidth() - width) / 2, 34, width, luna::engine::kGlyphHeight + 6};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.text(box.x + 4, box.y + 3, line, luna::engine::UiColor::Gold);
}

// Things of the run standing in the world: the knapping stone at the camp and the sacred fire.
void OdysseyGame::drawRunWorld(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const {
    if (!life_) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const PixelPoint stone = knappingStone();
    const int sx = stone.x - view.x;
    const int sy = stone.y - view.y;
    if (sx > -40 && sy > -40 && sx < viewWidth() + 40 && sy < viewHeight() + 40) {
        renderer.draw(tiles_, {static_cast<int>(TileKind::Rock) * kTileSize, 0, kTileSize, kTileSize}, {sx - 16, sy - 28});
        painter.text(sx - luna::engine::UiPainter::textWidth("knapping stone") / 2, sy - 38, "knapping stone", luna::engine::UiColor::Dim);
    }
    const sim::SacredFire& fire = life_->fire();
    if (fire.founded) {
        const int fx = fire.tileX * kTileSize + 16 - view.x;
        const int fy = fire.tileY * kTileSize + 16 - view.y;
        if (fx > -60 && fy > -40 && fx < viewWidth() + 60 && fy < viewHeight() + 40) {
            if (fire.lit) drawEffectPicture(renderer, effectArt_, "flame", {fx - 14, fy - 28, 28, 28});
            painter.text(fx - luna::engine::UiPainter::textWidth(fire.name) / 2, fy - 38, fire.name, fire.lit ? luna::engine::UiColor::Gold : luna::engine::UiColor::Dim);
        }
    }
}

double OdysseyGame::frameMilliseconds() const {
    if (frameTimesFilled_ == 0) return 0.0;
    double total = 0.0;
    for (std::size_t i = 0; i < frameTimesFilled_; ++i) total += frameTimes_[i];
    return total / static_cast<double>(frameTimesFilled_);
}

double OdysseyGame::framesPerSecond() const {
    const double ms = frameMilliseconds();
    return ms > 0.0 ? 1000.0 / ms : 0.0;
}

double OdysseyGame::drawMilliseconds() const {
    if (drawTimesFilled_ == 0) return 0.0;
    double total = 0.0;
    for (std::size_t i = 0; i < drawTimesFilled_; ++i) total += drawTimes_[i];
    return total / static_cast<double>(drawTimesFilled_);
}

// One drawn frame: the draw time, the card's time (when measured) and, in a performance run, the totals and a line a minute.
void OdysseyGame::recordFrame(double drawMs, double gpuMs) {
    drawTimes_[drawTimeAt_] = drawMs;
    drawTimeAt_ = (drawTimeAt_ + 1) % drawTimes_.size();
    drawTimesFilled_ = std::min(drawTimesFilled_ + 1, drawTimes_.size());
    gpuMs_ = gpuMs;
    if (!perfLog_ || frameTimesFilled_ == 0) return;
    const double frameMs = frameTimes_[(frameTimeAt_ + frameTimes_.size() - 1) % frameTimes_.size()];
    ++perf_.frames;
    if (frameMs > 20.0) ++perf_.over20;
    perf_.frameSum += frameMs;
    perf_.frameMax = std::max(perf_.frameMax, frameMs);
    perf_.drawSum += drawMs;
    perf_.drawMax = std::max(perf_.drawMax, drawMs);
    if (gpuMs >= 0.0) {
        perf_.gpuSum += gpuMs;
        perf_.gpuMax = std::max(perf_.gpuMax, gpuMs);
        ++perf_.gpuFrames;
    }
    perf_.tickWorst = std::max(perf_.tickWorst, worstTickMilliseconds());
    const auto now = std::chrono::steady_clock::now();
    if (perfLastLog_.time_since_epoch().count() == 0) perfLastLog_ = now;
    if (now - perfLastLog_ >= std::chrono::seconds(60)) {
        perfLastLog_ = now;
        const double frames = static_cast<double>(perf_.frames);
        core::logInfo(std::format("Perf: {} frames, {:.1f} FPS average, frame {:.2f} ms (max {:.1f}), {} frames over 20 ms, draw {:.2f} ms (max {:.1f}), "
                                  "gpu {:.2f} ms (max {:.1f}), tick {:.2f} ms (worst {:.1f}), people {}",
                                  perf_.frames, 1000.0 * frames / perf_.frameSum, perf_.frameSum / frames, perf_.frameMax, perf_.over20,
                                  perf_.drawSum / frames, perf_.drawMax, perf_.gpuFrames > 0 ? perf_.gpuSum / static_cast<double>(perf_.gpuFrames) : -1.0,
                                  perf_.gpuMax, tickMilliseconds(), perf_.tickWorst, clan_ ? clan_->population() : 0));
    }
}

double OdysseyGame::tickMilliseconds() const {
    if (tickTimesFilled_ == 0) return 0.0;
    double total = 0.0;
    for (std::size_t i = 0; i < tickTimesFilled_; ++i) total += tickTimes_[i];
    return total / static_cast<double>(tickTimesFilled_);
}

double OdysseyGame::worstTickMilliseconds() const {
    double worst = 0.0;
    for (std::size_t i = 0; i < tickTimesFilled_; ++i) worst = std::max(worst, tickTimes_[i]);
    return worst;
}

// F3: the numbers a slow game shows first (US-082).
void OdysseyGame::drawOverlay(luna::engine::Renderer& renderer) const {
    if (!overlayOn_) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    // CPU: the simulation tick and the drawing. GPU: the card's time for the frame (the fallback renderer cannot measure it).
    const std::string cpu = std::format("FPS {:.0f}  frame {:.1f} ms  CPU: tick {:.2f} ms (worst {:.2f}), draw {:.2f} ms", framesPerSecond(), frameMilliseconds(), tickMilliseconds(),
                                        worstTickMilliseconds(), drawMilliseconds());
    const std::string gpu = std::format("GPU {}  people {}", gpuMs_ >= 0.0 ? std::format("{:.2f} ms", gpuMs_) : std::string("n/a"), clan_ ? clan_->population() : 0);
    const int width = std::max(luna::engine::UiPainter::textWidth(cpu), luna::engine::UiPainter::textWidth(gpu)) + 8;
    const int height = 2 * luna::engine::kLineHeight + 4;
    const luna::engine::Rect box{2, uiHeight() - height - 4, width, height};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.text(box.x + 4, box.y + 3, cpu, luna::engine::UiColor::Text);
    painter.text(box.x + 4, box.y + 3 + luna::engine::kLineHeight, gpu, luna::engine::UiColor::Text);
}

void OdysseyGame::say(const std::string& text) {
    message_ = text;
    messageTicks_ = 6 * 20;
    core::logInfo(text);
}

// A generated region (US-040): the land, the level made from it, the clan at its start and two rivals far away.
void OdysseyGame::loadRegion(std::uint64_t seed) {
    region_ = std::make_unique<sim::Region>(seed, sim::loadRegionConfig(dataDirectory_ / "sim" / "region.json"));
    level_ = levelFromRegion(*region_, definitions_, catalogs_);
    clanEnabled_ = true;
    weatherSeed_ = WeatherCycle::seedFromText(level_.name);
    editor_.levelChanged();
    switchMode(Mode::Game);
    resetPlay();
    lastSavedDay_ = -1;
    core::logInfo(std::format("Region {}: start at tile ({}, {}), {} chunks made for the start", seed, region_->start().x, region_->start().y, region_->loadedChunks()));
}

// Saves the clan's world, and the region's changes when there is a region: safely, with backups, and quickly (US-080 wants
// under 200 ms; the time is logged and kept).
bool OdysseyGame::autosave() {
    if (!clan_) return false;
    const auto started = std::chrono::steady_clock::now();
    try {
        sim::saveWorld(*clan_, saveDirectory_ / "clan.json");
        if (region_) sim::saveRegion(*region_, saveDirectory_ / "region.json");
        if (life_) life_->save(saveDirectory_ / "hero.json");
        sim::writeSaveText(saveDirectory_ / "things.json", thingsText());
        sim::writeSaveText(saveDirectory_ / "buildings.json", buildings_.saveText()); // US-257
        if (!saveNpcPopulation()) say("The placed people could not be saved");
    } catch (const std::exception& error) {
        say(std::string("Autosave failed: ") + error.what());
        return false;
    }
    lastAutosaveMs_ = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    ++autosaves_;
    core::logInfo(std::format("Autosaved day {} in {:.1f} ms", clan_->date().day, lastAutosaveMs_));
    if (lastAutosaveMs_ > 200.0) core::logWarning(std::format("The autosave took {:.0f} ms (more than the 200 ms it should take)", lastAutosaveMs_));
    return true;
}

// Brings the autosave back. A damaged file is skipped for the newest intact backup, and a message says what happened.
bool OdysseyGame::loadAutosave() {
    const std::filesystem::path clanFile = saveDirectory_ / "clan.json";
    const std::filesystem::path regionFile = saveDirectory_ / "region.json";
    std::string notes;
    try {
        if (std::filesystem::exists(regionFile) || std::filesystem::exists(regionFile.string() + ".bak1")) {
            sim::LoadedRegion loaded = sim::loadRegion(regionFile, sim::loadRegionConfig(dataDirectory_ / "sim" / "region.json"));
            for (const std::string& note : loaded.notes) notes += (notes.empty() ? "" : "; ") + note;
            region_ = std::make_unique<sim::Region>(std::move(loaded.region));
            level_ = levelFromRegion(*region_, definitions_, catalogs_);
            clanEnabled_ = true;
            weatherSeed_ = WeatherCycle::seedFromText(level_.name);
            editor_.levelChanged();
            switchMode(Mode::Game);
            resetPlay();
        }
        if (!std::filesystem::exists(clanFile) && !std::filesystem::exists(clanFile.string() + ".bak1")) {
            if (!notes.empty()) say(notes);
            return region_ != nullptr;
        }
        sim::LoadedWorld loaded = sim::loadWorld(clanFile, sim::loadSimConfig(dataDirectory_));
        for (const std::string& note : loaded.notes) notes += (notes.empty() ? "" : "; ") + note;
        if (loaded.loadedFrom != clanFile) notes = std::format("The latest save was damaged: loaded the newest backup ({}). {}", loaded.loadedFrom.filename().string(), notes);
        clan_ = std::make_unique<sim::World>(std::move(loaded.world));
        exchanges_.reset(clan_->chronicle().entries().size()); // what happened before the save is not shown again
        if (!layerSheets_) layerSheets_ = makeLayerSheets();
        PixelPoint camp = level_.heroStart;
        for (const PlacedEffect& placed : level_.effects) {
            if (placed.name == "flame") {
                camp = placed.at;
                break;
            }
        }
        clanEnabled_ = true;
        clanView_ = ClanView(camp);
        clanView_.update(*clan_, map_);
        lastSavedDay_ = clan_->date().day;
        if (heroData_ && std::filesystem::exists(saveDirectory_ / "hero.json")) {
            life_ = std::make_unique<sim::HeroLife>(sim::HeroLife::load(*heroData_, *clan_, saveDirectory_ / "hero.json"));
            watchHeroItems();
            clanView_.setHidden(life_->personId());
            clanView_.update(*clan_, map_);
        }
        if (std::ifstream things(saveDirectory_ / "things.json", std::ios::binary); things) {
            const std::string text((std::istreambuf_iterator<char>(things)), std::istreambuf_iterator<char>());
            for (const std::string& note : restoreThings(text)) notes += (notes.empty() ? "" : "; ") + note;
        }
        if (std::ifstream built(saveDirectory_ / "buildings.json", std::ios::binary); built) {
            const std::string text((std::istreambuf_iterator<char>(built)), std::istreambuf_iterator<char>());
            for (const std::string& note : buildings_.loadText(text)) notes += (notes.empty() ? "" : "; ") + note;
            buildings_.syncObstacles(*this);
        }
        if (const std::string problem = loadNpcPopulation(); !problem.empty()) notes += (notes.empty() ? "" : "; ") + problem;
        say(notes.empty() ? std::format("Loaded {}: day {}", clanFile.filename().string(), clan_->date().day) : notes);
        return true;
    } catch (const std::exception& error) {
        say(std::string("Could not load the save: ") + error.what());
        return false;
    }
}

void OdysseyGame::skipDay() {
    if (!clan_) return;
    const std::uint64_t perDay = static_cast<std::uint64_t>(clan_->calendar().ticksPerDay());
    clan_->runTicks(perDay - clan_->ticks() % perDay);
    clanView_.update(*clan_, map_);
}

// The person under the pointer (by index into the clan's people), or -1.
int OdysseyGame::figureAt(const luna::engine::Rect& view, double alpha) const {
    if (!clan_ || pointerX_ < 0) return -1;
    int found = -1;
    double bestY = -1.0;
    for (std::size_t i = 0; i < clanView_.figures().size(); ++i) {
        const Figure& figure = clanView_.figures()[i];
        if (!figure.present) continue;
        const int feetX = static_cast<int>(std::lround(figure.feetX(alpha))) - view.x;
        const int feetY = static_cast<int>(std::lround(figure.feetY(alpha))) - view.y;
        const int height = figure.child ? kCharacterHeight * 3 / 4 : kCharacterHeight;
        const int halfWidth = (figure.child ? kCharacterWidth * 3 / 4 : kCharacterWidth) / 2;
        if (pointerX_ >= feetX - halfWidth && pointerX_ < feetX + halfWidth && pointerY_ >= feetY - height && pointerY_ <= feetY && figure.feetY(alpha) > bestY) {
            found = static_cast<int>(i);
            bestY = figure.feetY(alpha);
        }
    }
    return found;
}

namespace {
struct ToolButton {
    const char* label;
    int speed; // 0: skip a day
};
constexpr ToolButton kToolButtons[] = {{"1x", 1}, {"2x", 2}, {"4x", 4}, {"16x", 16}, {"Day", 0}};
luna::engine::Rect toolButtonRect(int index) { return {6 + index * 34, 52, 32, 12}; }
} // namespace

// F12 (Debug builds only): open or close the panel; click a button for speed, or a person to inspect them.
void OdysseyGame::updateDevTools(const luna::engine::Intents& intents) {
#ifndef NDEBUG
    if (intents.pressed(luna::engine::Intent::DevTools)) devToolsOpen_ = !devToolsOpen_;
    if (!devToolsOpen_ || !clan_) return;
    const luna::engine::Pointer pointer = luna::engine::scaledPointer(intents.pointer(), settings_.uiScale); // the panel is interface
    if (!pointer.wasPressed(luna::engine::PointerButton::Left) || !pointer.inside()) return;
    for (int i = 0; i < 5; ++i) {
        const luna::engine::Rect button = toolButtonRect(i);
        if (pointer.x >= button.x && pointer.x < button.x + button.width && pointer.y >= button.y && pointer.y < button.y + button.height) {
            if (kToolButtons[i].speed == 0) {
                skipDay();
            } else {
                setClanSpeed(kToolButtons[i].speed);
            }
            return;
        }
    }
    const luna::engine::Pointer inWorld = luna::engine::scaledPointer(intents.pointer(), settings_.cameraZoom);
    pointerX_ = inWorld.x; // this tick's pointer, not the one the aiming code has not read yet
    pointerY_ = inWorld.y;
    const int person = figureAt(camera_.view(), 1.0);
    if (person >= 0) selectedPerson_ = person;
#else
    (void)intents; // a Release build has no developer tools: the key does nothing
#endif
}

void OdysseyGame::drawDevTools(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const {
#ifndef NDEBUG
    if (!devToolsOpen_ || !clan_) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const bool somebody = selectedPerson_ >= 0 && selectedPerson_ < static_cast<int>(clan_->people().size());
    const luna::engine::Rect panel{2, 40, 236, somebody ? 224 : 96};
    painter.fill(panel, luna::engine::UiColor::Shade);
    painter.outline(panel, luna::engine::UiColor::Gold);
    painter.text(panel.x + 4, panel.y + 3, "DEV TOOLS  F12 closes", luna::engine::UiColor::Gold);
    for (int i = 0; i < 5; ++i) {
        const luna::engine::Rect button = toolButtonRect(i);
        painter.fill(button, kToolButtons[i].speed == clanSpeed_ ? luna::engine::UiColor::Selected : luna::engine::UiColor::Panel);
        painter.outline(button, luna::engine::UiColor::Border);
        painter.text(button.x + 4, button.y + 3, kToolButtons[i].label, luna::engine::UiColor::Text);
    }
    int y = panel.y + 28;
    const auto line = [&](const std::string& text, luna::engine::UiColor colour = luna::engine::UiColor::Text) {
        painter.text(panel.x + 4, y, text, colour);
        y += luna::engine::kLineHeight;
    };
    line(std::format("speed {}x   day {}   people {}", clanSpeed_, clan_->date().day, clan_->population()));
    if (rivals_) {
        for (const sim::RivalClan& rival : rivals_->clans()) {
            const sim::Tile here{static_cast<int>(hero_.feetX()) / kTileSize, static_cast<int>(hero_.feetY()) / kTileSize};
            const char* tier[] = {"active", "nearby", "distant"};
            line(std::format("{}: {} people, {} tiles, {}", rival.name, rival.world->population(), sim::Rivals::distanceTiles(rival.camp, here),
                             tier[static_cast<int>(rivals_->tierOf(rival, here))]), luna::engine::UiColor::Dim);
        }
    }
    y += 2;
    if (selectedPerson_ >= 0 && selectedPerson_ < static_cast<int>(clan_->people().size())) {
        const sim::Person& person = clan_->people()[static_cast<std::size_t>(selectedPerson_)];
        line(std::format("{}  #{}  age {}  {}", person.name, person.id, person.ageYears(clan_->calendar().daysPerYear()), person.alive ? "alive" : "dead"), luna::engine::UiColor::Gold);
        line(std::format("needs: hunger {} energy {} warmth {} social {}", person.needs[sim::Need::Hunger], person.needs[sim::Need::Energy], person.needs[sim::Need::Warmth], person.needs[sim::Need::Social]));
        line(std::format("doing: {}   health {}", sim::actionName(person.action), person.health == sim::Health::Well ? "well" : (person.health == sim::Health::Sick ? "sick" : "injured")));
        // What the AI scored last hour, best first.
        std::vector<std::pair<int, int>> scores;
        for (std::size_t a = 0; a < sim::kActionCount; ++a) scores.push_back({person.lastDecision.scores[a], static_cast<int>(a)});
        std::sort(scores.begin(), scores.end(), [](const auto& l, const auto& r) { return l.first != r.first ? l.first > r.first : l.second < r.second; });
        std::string ai = "AI:";
        for (int i = 0; i < 4; ++i) ai += std::format(" {} {}", sim::actionName(static_cast<sim::Action>(scores[static_cast<std::size_t>(i)].second)), scores[static_cast<std::size_t>(i)].first);
        line(ai);
        line(std::format("memories ({}):", person.memories.size()), luna::engine::UiColor::Dim);
        const std::size_t first = person.memories.size() > 4 ? person.memories.size() - 4 : 0;
        for (std::size_t m = first; m < person.memories.size(); ++m) {
            const sim::Memory& memory = person.memories[m];
            const std::string other = memory.subject == person.id ? "someone" : clan_->people().at(static_cast<std::size_t>(memory.subject)).name;
            line(std::format(" {} {:+d} by {} (day {})", sim::memoryKindName(memory.kind), memory.feeling, other, memory.day));
        }
        // Who they like most and least.
        std::vector<std::pair<int, int>> opinions;
        for (std::size_t o = 0; o < person.opinions.size(); ++o) {
            if (static_cast<int>(o) != person.id && clan_->people()[o].alive) opinions.push_back({person.opinions[o], static_cast<int>(o)});
        }
        std::sort(opinions.begin(), opinions.end());
        line("relationships:", luna::engine::UiColor::Dim);
        if (!opinions.empty()) {
            line(std::format(" likes {} ({:+d})", clan_->people()[static_cast<std::size_t>(opinions.back().second)].name, opinions.back().first));
            line(std::format(" dislikes {} ({:+d})", clan_->people()[static_cast<std::size_t>(opinions.front().second)].name, opinions.front().first));
        }
        // The chosen person is framed in the world.
        const Figure& figure = clanView_.figures()[static_cast<std::size_t>(selectedPerson_)];
        if (figure.present) {
            const int feetX = static_cast<int>(std::lround(figure.feetX(alpha))) - view.x;
            const int feetY = static_cast<int>(std::lround(figure.feetY(alpha))) - view.y;
            painter.outline({feetX - 17, feetY - kCharacterHeight - 1, kCharacterWidth + 2, kCharacterHeight + 2}, luna::engine::UiColor::Gold);
        }
    } else {
        line("click a person to inspect them", luna::engine::UiColor::Dim);
    }
#else
    (void)renderer;
    (void)view;
    (void)alpha;
#endif
}

// The rival clans' camps: a small fire and the clan's name and size.
void OdysseyGame::drawRivals(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const {
    if (!rivals_) return;
    luna::engine::UiPainter painter(renderer, uiSheet_);
    for (const sim::RivalClan& rival : rivals_->clans()) {
        const int x = rival.camp.x * kTileSize + kTileSize / 2 - view.x;
        const int y = rival.camp.y * kTileSize + kTileSize / 2 - view.y;
        if (x < -80 || y < -40 || x > viewWidth() + 80 || y > viewHeight() + 40) continue;
        drawEffectPicture(renderer, effectArt_, "flame", {x - 12, y - 24, 24, 24});
        const std::string label = std::format("{} ({})", rival.name, rival.world->population());
        painter.fill({x - luna::engine::UiPainter::textWidth(label) / 2 - 2, y - 36, luna::engine::UiPainter::textWidth(label) + 4, 10}, luna::engine::UiColor::Shade);
        painter.text(x - luna::engine::UiPainter::textWidth(label) / 2, y - 35, label, luna::engine::UiColor::Gold);
    }
}

void OdysseyGame::setWeatherSeed(std::uint64_t seed) {
    weatherSeed_ = seed;
    weather_ = WeatherCycle(catalogs_.weather, weatherSeed_);
}

bool OdysseyGame::setWeatherNamed(const std::string& name) {
    for (std::size_t i = 0; i < catalogs_.weather.size(); ++i) {
        if (catalogs_.weather[i].name == name) {
            weather_.force(static_cast<int>(i));
            return true;
        }
    }
    return false;
}

// The effects placed in the level play in a loop from the start of the level (US-138).
void OdysseyGame::startPlacedEffects() {
    if (!contentLoaded_) return;
    for (const PlacedEffect& placed : level_.effects) playEffect(placed.name, placed.at.x, placed.at.y);
}

// The weather: the page's 32 x 64 pictures tiled over the whole screen (rain, snow, sparks; fog and clouds stretched), animated; the old weather fades out as the
// new one fades in (alpha), light weathers added to the picture, fog and clouds laid over it.
void OdysseyGame::drawWeather(luna::engine::Renderer& renderer) const {
    if (weatherFrames_.empty() || catalogs_.weather.empty()) return;
    constexpr double kStrength = 0.8; // the most opaque a weather gets
    auto layer = [&](int index, double amount) {
        if (amount <= 0.0 || index < 0 || index >= static_cast<int>(catalogs_.weather.size())) return;
        const WeatherDef& def = catalogs_.weather[static_cast<std::size_t>(index)];
        const auto frames = weatherFrames_.find(def.name);
        if (def.frames == 0 || frames == weatherFrames_.end() || frames->second.empty()) return; // clear sky: nothing to draw
        const luna::engine::Rect& source = frames->second[static_cast<std::size_t>((ticks_ / static_cast<std::uint64_t>(std::max(1, def.ticksPerFrame))) % frames->second.size())];
        const luna::engine::DrawStyle style{static_cast<std::uint8_t>(std::clamp(amount * kStrength, 0.0, 1.0) * 255.0), def.additive ? luna::engine::Blend::Add : luna::engine::Blend::Normal};
        if (!def.additive) {
            // Fog and clouds: the one picture stretched over the whole screen, soft (tiled it would show its edges).
            renderer.drawStyled(weatherTexture_, source, {0, 0, uiWidth(), uiHeight()}, style);
            return;
        }
        for (int y = 0; y < uiHeight(); y += source.height) {
            for (int x = 0; x < uiWidth(); x += source.width) renderer.drawStyled(weatherTexture_, source, {x, y, source.width, source.height}, style);
        }
    };
    if (weather_.fading()) {
        layer(weather_.previous(), 1.0 - weather_.fade());
        layer(weather_.current(), weather_.fade());
    } else {
        layer(weather_.current(), 1.0);
    }
}

std::size_t OdysseyGame::plantsGrowing() const {
    return static_cast<std::size_t>(std::count_if(plants_.begin(), plants_.end(), [](const WorldPlant& plant) { return plant.alive; }));
}

// US-136: the level's plants stand in the play state; big ones block their foot cell like a rock.
void OdysseyGame::populatePlants() {
    plants_.clear();
    inspection_ = {};
    plantRng_ = core::Pcg32(1, 5); // the same seed every start, so a replay grows the same plants back in the same places
    for (const PlacedPlant& placed : level_.plants) {
        const PlantDef* def = catalogs_.plant(placed.kind);
        if (def == nullptr) continue; // the level names a plant the catalog lost: skipped
        plants_.push_back({placed.id, placed.kind, def, placed.feet, true, 0, def->states.empty() ? std::string() : def->states.front()});
        plants_.back().overrides = placed.overrides; // this plant's own interaction values (US-173)
        if (const double height = plantObstacleHeight(*def); height > 0.0) {
            const PixelPoint cell = plantCell(placed.feet);
            map_.setObstacle(cell.x, cell.y, height);
        }
    }
}

// A weapon hit destroys a plant: leaves fly, an edible one heals the hero, and 15 s later it grows back elsewhere.
void OdysseyGame::destroyPlant(std::size_t index, bool heal) {
    WorldPlant& plant = plants_.at(index);
    if (!plant.alive) return;
    plant.alive = false;
    plant.regrowTicks = kRegrowTicks;
    if (plant.def != nullptr && plant.def->blocks) {
        const PixelPoint cell = plantCell(plant.feet);
        map_.setObstacle(cell.x, cell.y, 0.0);
    }
    const luna::engine::Rect extent = plantExtent(plantArt_, plant.kind);
    playEffect("nature leaf", plant.feet.x, plant.feet.y - extent.height / 2.0, 40);
    core::logInfo(std::format("The {} was destroyed", plant.kind));
    if (heal && plant.def != nullptr && plant.def->edible && respawnTicks_ == 0) {
        const int before = heroHp_;
        heroHp_ = std::min(kHeroMaxHp, heroHp_ + kPlantHeal);
        playEffect("healing glow", hero_.feetX(), hero_.feetY() - kCharacterHeight / 2.0, 32);
        core::logInfo(std::format("Eating the {} healed the hero: {} to {} HP", plant.kind, before, heroHp_));
    }
    if (inspection_.plantId == plant.id) inspection_.ticks = 0;
}

bool OdysseyGame::plantSpotFree(int cellX, int cellY) const {
    if (!map_.inside(cellX, cellY) || map_.isSolid(cellX, cellY)) return false;
    const double centreX = cellX * kTileSize + kTileSize / 2.0;
    const double centreY = cellY * kTileSize + kTileSize / 2.0;
    if (std::hypot(hero_.feetX() - centreX, hero_.feetY() - centreY) < 40.0) return false; // not under the hero
    for (const WorldPlant& plant : plants_) {
        if (!plant.alive) continue;
        const PixelPoint cell = plantCell(plant.feet);
        if (cell.x == cellX && cell.y == cellY) return false;
    }
    for (const Enemy& enemy : enemies_) {
        if (enemy.isAlive() && static_cast<int>(enemy.feetX()) / kTileSize == cellX && static_cast<int>(enemy.feetY()) / kTileSize == cellY) return false;
    }
    for (const PlacedCharacter& other : bystanders_) {
        if (other.feet.x / kTileSize == cellX && other.feet.y / kTileSize == cellY) return false;
    }
    return true;
}

// Destroyed plants count down; at zero the same plant grows back at a random free spot inside the camera view.
void OdysseyGame::tickPlants() {
    for (WorldPlant& plant : plants_) {
        if (plant.alive || --plant.regrowTicks > 0) continue;
        const luna::engine::Rect view = camera_.view();
        const int firstX = std::max(0, view.x / kTileSize);
        const int firstY = std::max(0, view.y / kTileSize);
        const int cellsX = std::max(1, std::min(map_.width() - 1, (view.x + view.width - 1) / kTileSize) - firstX + 1);
        const int cellsY = std::max(1, std::min(map_.height() - 1, (view.y + view.height - 1) / kTileSize) - firstY + 1);
        bool grown = false;
        for (int attempt = 0; attempt < kRegrowTries && !grown; ++attempt) {
            const int cellX = firstX + static_cast<int>(plantRng_.below(static_cast<std::uint32_t>(cellsX)));
            const int cellY = firstY + static_cast<int>(plantRng_.below(static_cast<std::uint32_t>(cellsY)));
            if (!plantSpotFree(cellX, cellY)) continue;
            plant.feet = {cellX * kTileSize + kTileSize / 2, cellY * kTileSize + kTileSize - 4};
            plant.alive = true;
            if (plant.def != nullptr) plant.state = plant.def->states.empty() ? std::string() : plant.def->states.front(); // grows back ripe
            if (plant.def != nullptr) {
                if (const double height = plantObstacleHeight(*plant.def); height > 0.0) map_.setObstacle(cellX, cellY, height);
            }
            const luna::engine::Rect extent = plantExtent(plantArt_, plant.kind);
            playEffect("tree growth", plant.feet.x, plant.feet.y - extent.height / 2.0, 48);
            core::logInfo(std::format("A {} grew back at tile ({}, {})", plant.kind, cellX, cellY));
            grown = true;
        }
        if (!grown) plant.regrowTicks = kRegrowRetryTicks; // no free spot in view: look again in a second
    }
}

// Interact and Inspect: the nearest plant within 1.5 m tells its name and a line about itself for 3 seconds.
bool OdysseyGame::inspectNearestPlant() {
    const WorldPlant* nearest = nullptr;
    double best = kInspectReachPixels;
    for (const WorldPlant& plant : plants_) {
        if (!plant.present()) continue;
        const double distance = std::hypot(plant.feet.x - hero_.feetX(), plant.feet.y - hero_.feetY());
        if (distance <= best) {
            best = distance;
            nearest = &plant;
        }
    }
    if (nearest == nullptr || nearest->def == nullptr) return false;
    inspection_ = {nearest->def->name, nearest->def->inspect, nearest->id, kInspectTicks};
    core::logInfo(std::format("{}: {}", nearest->def->name, nearest->def->inspect));
    return true;
}

std::vector<Target> OdysseyGame::shotTargets(std::vector<std::size_t>& plantOf) const {
    std::vector<Target> targets;
    for (const Enemy& enemy : enemies_) targets.push_back({enemy.feetX(), enemy.feetY(), enemy.isAlive()});
    plantOf.clear();
    for (std::size_t i = 0; i < plants_.size(); ++i) {
        if (!plants_[i].alive || plants_[i].def == nullptr || !plants_[i].def->blocks) continue; // shots fly through grass and flowers
        targets.push_back({static_cast<double>(plants_[i].feet.x), static_cast<double>(plants_[i].feet.y), true});
        plantOf.push_back(i);
    }
    return targets;
}

void OdysseyGame::drawPlants(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha, bool behindHero) const {
    std::vector<const WorldPlant*> order;
    const double heroY = hero_.feetY(alpha);
    for (const WorldPlant& plant : plants_) {
        if (plant.present() && (plant.feet.y <= heroY) == behindHero) order.push_back(&plant);
    }
    std::sort(order.begin(), order.end(), [](const WorldPlant* a, const WorldPlant* b) { return a->feet.y != b->feet.y ? a->feet.y < b->feet.y : a->id < b->id; });
    for (const WorldPlant* plant : order) {
        drawPlant(renderer, plantArt_, plant->kind, plant->feet.x - view.x, plant->feet.y - view.y);
    }
}

// The plant's name and its line in a small box above it.
void OdysseyGame::drawInspection(luna::engine::Renderer& renderer, const luna::engine::Rect& view) const {
    if (inspection_.ticks <= 0) return;
    for (const WorldPlant& plant : plants_) {
        if (plant.id != inspection_.plantId || !plant.alive) continue;
        luna::engine::UiPainter painter(renderer, uiSheet_);
        const int width = std::max(luna::engine::UiPainter::textWidth(inspection_.name), luna::engine::UiPainter::textWidth(inspection_.text)) + 8;
        const int height = 2 * luna::engine::kLineHeight + 4;
        const luna::engine::Rect extent = plantExtent(plantArt_, plant.kind);
        luna::engine::Rect box{plant.feet.x - view.x - width / 2, plant.feet.y - view.y - extent.height - height - 2, width, height};
        box.x = std::clamp(box.x, 2, viewWidth() - width - 2);
        box.y = std::max(box.y, 2);
        painter.fill(box, luna::engine::UiColor::Shade);
        painter.outline(box, luna::engine::UiColor::Gold);
        painter.text(box.x + 4, box.y + 3, inspection_.name, luna::engine::UiColor::Gold);
        painter.text(box.x + 4, box.y + 3 + luna::engine::kLineHeight, inspection_.text, luna::engine::UiColor::Text);
        return;
    }
}

void OdysseyGame::attackWith(const WeaponDef& weapon, double dirX, double dirY, double distancePixels, bool chestHeight) {
    if (attackCooldown_ > 0) {
        return;
    }
    attackCooldown_ = cooldownTicks(weapon);
    const WeaponBehaviour& behaviour = behaviourOf(weapon.weaponClass);
    if (behaviour.melee()) {
        std::vector<Target> targets;
        for (const Enemy& enemy : enemies_) targets.push_back({enemy.feetX(), enemy.feetY(), enemy.isAlive()});
        swingTicks_ = 6;
        const auto hits = behaviour.swingToward(weapon, hero_.feetX(), hero_.feetY(), dirX, dirY, targets);
        core::logInfo(std::format("{} ({}) swung toward {:.0f} degrees, facing {}: {} hit", weapon.name, weaponClassName(weapon.weaponClass), std::atan2(dirY, dirX) * 180.0 / std::numbers::pi, facingName(hero_.facing()), hits.size()));
        for (const std::size_t index : hits) strike(enemies_[index], weapon.damage, &weapon);
        // The same swing also cuts the plants in its arc (the nearest one for a sword or spear, all for an axe or whip).
        std::vector<Target> plantTargets;
        std::vector<std::size_t> plantOf;
        for (std::size_t i = 0; i < plants_.size(); ++i) {
            if (!plants_[i].present()) continue;
            plantTargets.push_back({static_cast<double>(plants_[i].feet.x), static_cast<double>(plants_[i].feet.y), true});
            plantOf.push_back(i);
        }
        for (const std::size_t index : behaviour.swingToward(weapon, hero_.feetX(), hero_.feetY(), dirX, dirY, plantTargets)) destroyPlant(plantOf[index]);
    } else if (auto arc = launchArcShot(weapon, catalogs_.weaponClass(weapon.weaponClass).launchSpeed, hero_.feetX(), hero_.feetY(), dirX, dirY, distancePixels, chestHeight)) {
        arcShots_.push_back(*arc);
        core::logInfo(std::format("{} ({}) arced toward {:.0f} degrees, {:.1f} m away", weapon.name, weaponClassName(weapon.weaponClass),
                                  std::atan2(dirY, dirX) * 180.0 / std::numbers::pi,
                                  luna::engine::toDouble(luna::physics::length({arc->aimPoint.x - arc->body.position.x, arc->aimPoint.y - arc->body.position.y, luna::physics::kFixedZero}))));
    } else if (auto shot = behaviour.launchToward(weapon, hero_.feetX(), hero_.feetY(), dirX, dirY)) {
        shot->pixelsPerTick = catalogs_.weaponClass(weapon.weaponClass).launchSpeed * kTileSize / StatusEffects::kTicksPerSecond; // weapons.json sets the speed
        projectiles_.push_back(*shot);
        core::logInfo(std::format("{} ({}) shot toward {:.0f} degrees, facing {}", weapon.name, weaponClassName(weapon.weaponClass), std::atan2(dirY, dirX) * 180.0 / std::numbers::pi, facingName(hero_.facing())));
    }
}

// US-139: where the pointer points in the world, and the hero turned to it. The hero always faces the
// pointer while it is over the picture (walking does not turn him); only a held catalog weapon also
// aims its attacks there and shows the aim line. Near the hero, and near the edge between two facings,
// he keeps his facing, so walking past the pointer never makes the sprite flicker from side to side.
namespace {
constexpr double kChestHeightPixels = 20.0;       // the middle of the hero's body above his feet
constexpr double kFacingDeadZonePixels = 16.0;    // a pointer this close to his chest does not turn him
constexpr double kFacingHysteresisDegrees = 10.0; // how far past a sector's edge before he turns
} // namespace

void OdysseyGame::updateAim(const luna::engine::Pointer& pointer, bool fallen, Facing facingBefore) {
    aiming_ = false;
    pointerX_ = pointer.x;
    pointerY_ = pointer.y;
    if (fallen || !pointer.inside()) {
        return; // no pointer: the hero faces the way he walks
    }
    const luna::engine::Rect view = camera_.view();
    aimTargetX_ = view.x + pointer.x;
    aimTargetY_ = view.y + pointer.y;

    // Facing: seen from the hero's chest, so a pointer level with the sprite means "sideways".
    const double chestDx = aimTargetX_ - hero_.feetX();
    const double chestDy = aimTargetY_ - (hero_.feetY() - kChestHeightPixels);
    if (std::hypot(chestDx, chestDy) < kFacingDeadZonePixels) {
        hero_.face(facingBefore); // the pointer is on him: keep facing the same way
    } else {
        hero_.face(facingToward(chestDx, chestDy, facingBefore, kFacingHysteresisDegrees));
    }

    // Aim of the attacks: from the feet, as the weapons measure (only with a catalog weapon in hand).
    if (heldWeapon() == nullptr) {
        return;
    }
    const double dx = aimTargetX_ - hero_.feetX();
    const double dy = aimTargetY_ - hero_.feetY();
    const double length = std::hypot(dx, dy);
    if (length < 1.0) {
        return; // right on the hero: keep the last direction
    }
    aimDx_ = dx / length;
    aimDy_ = dy / length;
    aiming_ = true;
}
// The aim line (dots from the hand toward the pointer, up to the weapon's range) and a crosshair.
void OdysseyGame::drawAim(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const {
    const WeaponDef* weapon = heldWeapon();
    if (!aiming_ || weapon == nullptr || respawnTicks_ > 0) {
        return;
    }
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const double startX = hero_.feetX(alpha) - view.x;
    const double startY = hero_.feetY(alpha) - 20.0 - view.y;
    const double endX = pointerX_;
    const double endY = pointerY_;
    const double length = std::hypot(endX - startX, endY - startY);
    const double reach = weapon->range * kTileSize;
    for (double d = 12.0; d < std::min(length, reach); d += 6.0) {
        const int x = static_cast<int>(std::lround(startX + (endX - startX) * d / length));
        const int y = static_cast<int>(std::lround(startY + (endY - startY) * d / length));
        painter.fill({x, y, 2, 2}, luna::engine::UiColor::Gold);
    }
    const luna::engine::UiColor colour = length <= reach ? luna::engine::UiColor::Gold : luna::engine::UiColor::Red; // red: out of range
    const int px = pointerX_;
    const int py = pointerY_;
    painter.fill({px - 5, py, 4, 1}, colour);
    painter.fill({px + 2, py, 4, 1}, colour);
    painter.fill({px, py - 5, 1, 4}, colour);
    painter.fill({px, py + 2, 1, 4}, colour);
}

void OdysseyGame::drawHeld(luna::engine::Renderer& renderer, const luna::engine::Rect& view, double alpha) const {
    if (!contentLoaded_) {
        return;
    }
    const int iconsWidth = content_.pictures.at("icons").width();
    // The held weapon in the hero's hand (US-133): its icon, turned to the side the hero faces,
    // raised a little during a swing.
    if (const WeaponDef* weapon = heldWeapon(); weapon != nullptr && respawnTicks_ == 0) {
        if (const auto icon = content_.rect(weapon->frame)) {
            const Facing facing = hero_.facing();
            const bool west = facing == Facing::West || facing == Facing::SouthWest || facing == Facing::NorthWest;
            const luna::engine::Rect source = west ? luna::engine::Rect{iconsWidth - icon->x - icon->width, icon->y, icon->width, icon->height} : *icon;
            const double handX = hero_.feetX(alpha) + (west ? -9.0 : 9.0);
            const double handY = hero_.feetY(alpha) - 22.0 - (swingTicks_ > 0 ? 4.0 : 0.0);
            const int size = 20;
            renderer.drawStyled(west ? iconsMirrored_ : iconsTexture_, source,
                                {static_cast<int>(std::lround(handX)) - size / 2 - view.x, static_cast<int>(std::lround(handY)) - size / 2 - view.y, size, size}, {});
        }
    }
    // Shots in flight: a thrown weapon is its own icon; arrows, bolts and bullets are light.
    for (const Projectile& shot : projectiles_) {
        std::string frame = "light arrow";
        luna::engine::Texture texture = effectsTexture_;
        luna::engine::DrawStyle style{255, luna::engine::Blend::Add};
        int size = 16;
        switch (shot.weapon->weaponClass) {
        case WeaponClass::Thrown: frame = shot.weapon->frame; texture = iconsTexture_; style = {}; size = 14; break;
        case WeaponClass::Staff: frame = content_.frameName("arcane orb", 0); break;
        case WeaponClass::Gun: frame = "gold spark"; size = 10; break;
        default: break;
        }
        if (const auto source = content_.rect(frame)) {
            renderer.drawStyled(texture, *source,
                                {static_cast<int>(std::lround(shot.x)) - size / 2 - view.x, static_cast<int>(std::lround(shot.y)) - size / 2 - view.y, size, size}, style);
        }
    }
    // Arcs (US-140): the sprite is lifted by its height, and its shadow moves along the ground
    // underneath, so the player can judge where it will come down.
    luna::engine::UiPainter painter(renderer, uiSheet_);
    for (const ArcShot& shot : arcShots_) {
        const auto between = [&](luna::physics::Fixed before, luna::physics::Fixed after) {
            return luna::engine::toDouble(before) + (luna::engine::toDouble(after) - luna::engine::toDouble(before)) * (shot.state == ArcState::Flying ? alpha : 1.0);
        };
        const luna::physics::Vec3 at{luna::engine::metresFromPixels(between(shot.previousPosition.x, shot.body.position.x) * luna::engine::kPixelsPerMetre),
                                     luna::engine::metresFromPixels(between(shot.previousPosition.y, shot.body.position.y) * luna::engine::kPixelsPerMetre),
                                     luna::engine::metresFromPixels(between(shot.previousPosition.z, shot.body.position.z) * luna::engine::kPixelsPerMetre)};
        const luna::engine::ScreenPoint lifted = luna::engine::topDownPosition(at);
        const luna::engine::ScreenPoint shadow = luna::engine::groundShadow(at);
        if (shot.state == ArcState::Flying) {
            painter.fill({static_cast<int>(std::lround(shadow.x)) - 3 - view.x, static_cast<int>(std::lround(shadow.y)) - 1 - view.y, 6, 2}, luna::engine::UiColor::Shade);
        }
        const bool thrown = shot.weapon->weaponClass == WeaponClass::Thrown;
        const std::string frame = thrown ? shot.weapon->frame : "light arrow";
        if (const auto source = content_.rect(frame)) {
            const int size = thrown ? 14 : 16;
            renderer.drawStyled(thrown ? iconsTexture_ : effectsTexture_, *source,
                                {static_cast<int>(std::lround(lifted.x)) - size / 2 - view.x, static_cast<int>(std::lround(lifted.y)) - size / 2 - view.y, size, size},
                                thrown ? luna::engine::DrawStyle{} : luna::engine::DrawStyle{255, luna::engine::Blend::Add});
        }
    }
}

void OdysseyGame::hurtHero(int damage, const Enemy& by) {
    heroHp_ = std::max(0, heroHp_ - damage);
    core::logInfo(std::format("{} struck the hero for {}, HP {} / {}", by.name, damage, heroHp_, kHeroMaxHp));
    if (heroHp_ == 0) {
        respawnTicks_ = kRespawnTicks;
        core::logInfo("The hero fell");
    }
}

void OdysseyGame::drawHud(luna::engine::Renderer& renderer) const {
    luna::engine::UiPainter painter(renderer, uiSheet_);
    const luna::engine::Rect view = camera_.view();
    // A warning mark over every enemy winding up to strike: the moment to step away.
    for (const Enemy& enemy : enemies_) {
        if (enemy.isAlive() && enemy.isWindingUp()) {
            const int x = static_cast<int>(std::lround(enemy.feetX())) - view.x - 2;
            const int y = static_cast<int>(std::lround(enemy.feetY())) - kCharacterHeight - view.y - 26;
            painter.fill({x - 2, y - 2, luna::engine::kGlyphWidth + 4, luna::engine::kGlyphHeight + 4}, luna::engine::UiColor::Red);
            painter.text(x, y, "!", luna::engine::UiColor::Text);
        }
    }
    // The hero's health, top left.
    const std::string label = std::format("HP {}/{}", heroHp_, kHeroMaxHp);
    const luna::engine::Rect box{2, 2, luna::engine::UiPainter::textWidth(label) + 6, luna::engine::kGlyphHeight + 6};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.text(box.x + 3, box.y + 3, label, heroHp_ * 4 <= kHeroMaxHp ? luna::engine::UiColor::Red : luna::engine::UiColor::Text);
    // What the hero holds, under the health.
    const std::string held = heldName();
    const luna::engine::Rect heldBox{2, box.y + box.height + 1, luna::engine::UiPainter::textWidth(held) + 6, luna::engine::kGlyphHeight + 6};
    painter.fill(heldBox, luna::engine::UiColor::Shade);
    painter.text(heldBox.x + 3, heldBox.y + 3, held, luna::engine::UiColor::Gold);
    // The hotbar (US-134), bottom centre: nine slots, the number in the corner, the held slot framed in gold.
    constexpr int kSlot = 22;
    constexpr int kGap = 2;
    const int left = (uiWidth() - (kHotbarSlots * kSlot + (kHotbarSlots - 1) * kGap)) / 2;
    const int top = uiHeight() - kSlot - 4;
    for (int slot = 0; slot < kHotbarSlots; ++slot) {
        const luna::engine::Rect cell{left + slot * (kSlot + kGap), top, kSlot, kSlot};
        painter.fill(cell, luna::engine::UiColor::Shade);
        const std::string& weapon = hotbar_[static_cast<std::size_t>(slot)];
        if (!weapon.empty()) {
            drawWeaponIcon(renderer, painter, weaponArt_, weapon, {cell.x + 2, cell.y + 2, kSlot - 4, kSlot - 4});
        }
        painter.outline(cell, slot == heldSlot_ ? luna::engine::UiColor::Gold : luna::engine::UiColor::Grid);
        if (slot == heldSlot_) painter.outline({cell.x + 1, cell.y + 1, cell.width - 2, cell.height - 2}, luna::engine::UiColor::Gold);
        painter.text(cell.x + 2, cell.y + 2, std::to_string(slot + 1), slot == heldSlot_ ? luna::engine::UiColor::Gold : luna::engine::UiColor::Text);
    }
    if (fullTicks_ > 0) {
        const char* message = "Hotbar full";
        painter.text((uiWidth() - luna::engine::UiPainter::textWidth(message)) / 2, top - luna::engine::kGlyphHeight - 3, message, luna::engine::UiColor::Red);
    }
    // After a fall the screen darkens, more with every step of the fade.
    for (int layer = 0; layer < (kRespawnTicks - respawnTicks_) / 4 + 1 && respawnTicks_ > 0; ++layer) {
        painter.fill({0, 0, uiWidth(), uiHeight()}, luna::engine::UiColor::Shade);
    }
}

void OdysseyGame::update(const luna::engine::Intents& intents) {
    if (intents.pressed(luna::engine::Intent::Reload)) {
        reloadInteractions();
        if (npcClasses_.reload()) {
            editor_.classesChanged(); // F5 also reads the NPC Classes again (US-260)
            refreshTraders();         // and the trade profiles that came with them (US-281)
            reloadPartnerDefaults();  // and the default actions of the partner types (US-293)
            refreshLife();            // and the schedules (US-290)
        }
    }
    if (intents.pressed(luna::engine::Intent::ModeEditor)) {
        switchMode(Mode::Editor);
    } else if (intents.pressed(luna::engine::Intent::ModeGame)) {
        switchMode(Mode::Game);
    }
    if (mode_ == Mode::Editor) {
        if (intents.pressed(luna::engine::Intent::PlayHere) && intents.pointer().inside()) { // Play here (US-186): the hero starts at the cursor; F2 returns to the Editor, which has not changed
            const auto [wx, wy] = editor_.worldUnder(intents.pointer());
            switchMode(Mode::Game);
            hero_ = Hero(wx, wy);
            playHere_ = true;
            camera_.centreOn(hero_.feetX(), hero_.feetY());
            return;
        }
        editor_.update(intents); // the world stands still
        return;
    }
    // The screens of the run (New Game, Focus, menu, crafting...) stop the world while they are open.
    if (intents.pressed(luna::engine::Intent::Overlay)) overlayOn_ = !overlayOn_;
    // The first time the New Game screen comes up and the player has not chosen yet: ask about statistics first (US-092).
    if (!privacyAsked_ && runFlow_.screen() == Screen::NewGame) {
        privacyAsked_ = true;
        if (settings_.statistics == 0) runFlow_.openPrivacy();
    }
    if (playHere_ && !runFlow_.modal() && intents.pressed(luna::engine::Intent::OpenMenu)) { // Esc ends a Play here session (US-186)
        playHere_ = false;
        switchMode(Mode::Editor);
        return;
    }
    if (!runFlow_.modal() && intents.pressed(luna::engine::Intent::Journal)) runFlow_.openJournal();
    if (!runFlow_.modal() && intents.pressed(luna::engine::Intent::OpenMenu) && !buildings_.escape()) runFlow_.openMenu(); // Esc first leaves placing and the Build menu
    // Two pictures, two pointers (US-232): the world is drawn zoomed and the interface scaled, so the pointer is
    // turned into the pixels of each one before anything reads it.
    luna::engine::Intents worldIntents = intents;
    worldIntents.setPointer(luna::engine::scaledPointer(intents.pointer(), settings_.cameraZoom));
    luna::engine::Intents uiIntents = intents;
    uiIntents.setPointer(luna::engine::scaledPointer(intents.pointer(), settings_.uiScale));
    updateQuestDebug(uiIntents);
    if (runFlow_.modal()) {
        updateAim(worldIntents.pointer(), true, hero_.facing()); // keeps the pointer for drawing
        runFlow_.update(*this, uiIntents);
        return;
    }
    if (!devToolsOpen_) updateZoom(intents);
    ++ticks_;
    tickActions(intents);
    if (ticks_ % 10 == 0) tickQuests(*this);
    tutorial_.tick();
    const auto tickStarted = std::chrono::steady_clock::now();
    weather_.update();
    if (!insideBuilding()) tickNpcPopulation();
    if (clan_) {
        for (int i = 0; i < clanSpeed_; ++i) clan_->tick();
        if (!insideBuilding()) clanView_.update(*clan_, map_);
        // An in-game day ended: save (US-080).
        const std::int64_t day = clan_->date().day;
        if (day != lastSavedDay_) {
            if (lastSavedDay_ >= 0 && !insideBuilding()) autosave();
            lastSavedDay_ = day;
        }
        bubbles_.tick();
        if (!insideBuilding()) {
            updateGreetings(*this);
            exchanges_.update(*this);
            npcLife_.tick(*this); // clan members and animals look around, walk and do things with the same interactions as the hero (US-154)
        }
    }
    if (rivals_) rivals_->tick({static_cast<int>(hero_.feetX()) / kTileSize, static_cast<int>(hero_.feetY()) / kTileSize});
    if (life_ && clan_) {
        std::vector<sim::RivalSummary> summaries;
        if (rivals_) {
            for (const sim::RivalClan& clan : rivals_->clans()) summaries.push_back({clan.name, clan.world->population()});
        }
        life_->setRivals(std::move(summaries));
        life_->update();
        if (life_->phase() == sim::Phase::Ended && runFlow_.screen() != Screen::Ended) runFlow_.openEnded();
    }
    tickTimes_[tickTimeAt_] = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tickStarted).count();
    tickTimeAt_ = (tickTimeAt_ + 1) % tickTimes_.size();
    tickTimesFilled_ = std::min(tickTimesFilled_ + 1, tickTimes_.size());
    if (messageTicks_ > 0 && --messageTicks_ == 0) message_.clear();
    updateDevTools(intents);
    // After a fall the hero waits out a short fade, then starts again at the hero start.
    const bool fallen = respawnTicks_ > 0;
    if (fallen && --respawnTicks_ == 0) {
        hero_ = Hero(static_cast<double>(level_.heroStart.x), static_cast<double>(level_.heroStart.y));
        heroHp_ = kHeroMaxHp;
        camera_.centreOn(hero_.feetX(), hero_.feetY());
        core::logInfo(std::format("The hero is back at the start with {} HP", heroHp_));
    }
    const Facing facingBefore = hero_.facing();
    if (!fallen) {
        hero_.update(intents, map_);
    }
    updateAim(worldIntents.pointer(), fallen, facingBefore);
    // Pressing left or right always turns him that way, whatever the pointer says; the aim of the
    // attacks still follows the pointer. Up and down alone leave the pointer in charge of his facing.
    if (!fallen && intents.moveX() != 0) {
        hero_.face(facingFor(intents.moveX(), intents.moveY(), hero_.facing()));
    }

    // Pickups first (US-134), so a weapon picked up this tick can be chosen and used this tick.
    if (fullTicks_ > 0) --fullTicks_;
    if (!fallen) collectPickups();

    // Hotbar: keys 1-9 hold a slot, Shift the next filled one.
    if (!fallen) {
        for (int slot = 0; slot < kHotbarSlots; ++slot) {
            if (intents.pressed(static_cast<luna::engine::Intent>(static_cast<int>(luna::engine::Intent::Slot1) + slot))) selectSlot(slot);
        }
        if (intents.pressed(luna::engine::Intent::SwitchWeapon)) cycleSlot();
    }
    const std::string& heldSlotName = hotbar_[static_cast<std::size_t>(heldSlot_)];
    currentWeapon_ = heldSlotName == kSwordSlashName ? WeaponType::Sword : WeaponType::Bow; // the demos' own two

    // Handle attacks based on the held weapon.
    // Plants (US-136): Interact with empty hands, or the right mouse button, looks at the plant next to the hero.
    if (!fallen && intents.pressed(luna::engine::Intent::Confront)) confrontKey(worldIntents.pointer());
    if (!fallen && intents.pressed(luna::engine::Intent::Actions)) actionsKey(worldIntents.pointer());
    if (!fallen && intents.pressed(luna::engine::Intent::Inspect) && buildings_.placing()) {
        buildings_.stopPlacing(); // a right click while placing a blueprint stops placing (US-251)
    } else if (!fallen && intents.pressed(luna::engine::Intent::Inspect)) {
        // A right click on something in the world offers its actions (US-061); on nothing it looks at the nearest plant.
        const luna::engine::Pointer& p = worldIntents.pointer();
        const luna::engine::Rect view = camera_.view();
        if (!(p.inside() && runFlow_.openContext(*this, view.x + p.x, view.y + p.y))) inspectNearestPlant(); // outside a run only placed people have a menu
    } else if (!fallen && intents.pressed(luna::engine::Intent::Interact) && heldSlotName.empty()) {
        inspectNearestPlant();
    }
    if (inspection_.ticks > 0) --inspection_.ticks;
    tickPlants();
    buildings_.tick(*this, worldIntents, uiIntents);

    // Attack (the left button) goes toward the pointer; Interact goes along the facing, as before.
    const bool mouseAttack = heldWeapon() != nullptr && aiming_ && intents.held(luna::engine::Intent::Attack) && !devToolsOpen_ && !buildings_.capturesPointer(uiIntents.pointer());
    if (!fallen && heldWeapon() != nullptr && (mouseAttack || intents.pressed(luna::engine::Intent::Interact))) {
        double dirX = aimDx_;
        double dirY = aimDy_;
        double distance = std::hypot(aimTargetX_ - hero_.feetX(), aimTargetY_ - hero_.feetY());
        if (!mouseAttack) {
            facingVector(hero_.facing(), dirX, dirY);
            distance = heldWeapon()->range * kTileSize; // keys have no pointer: out to the weapon's range
        }
        attackWith(*heldWeapon(), dirX, dirY, distance, !mouseAttack);
    } else if (!fallen && intents.pressed(luna::engine::Intent::Interact) && !heldSlotName.empty()) {
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

    // Enemies strike back (US-131): the strike lands when the wind-up ends, if the hero is
    // still within reach; stepping away in time is how the hero dodges.
    for (Enemy& enemy : enemies_) tickStatus(enemy); // burning and poison, before anyone strikes (US-135)
    for (Enemy& enemy : enemies_) {
        if (enemy.update() && respawnTicks_ == 0) {
            const double distance = std::hypot(enemy.feetX() - hero_.feetX(), enemy.feetY() - hero_.feetY());
            if (distance <= enemy.reachMetres * kTileSize) {
                hurtHero(enemy.swordDamage, enemy);
            } else {
                core::logInfo(enemy.name + " struck back and missed");
            }
        }
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
            strike(*nearest, sword_.config().damagePerHit);
            sword_.markHit();
        }
    }

    // Catalog weapons (US-133): the cooldown between attacks, and shots in flight.
    if (attackCooldown_ > 0) --attackCooldown_;
    if (swingTicks_ > 0) --swingTicks_;
    if (!projectiles_.empty()) {
        std::vector<std::size_t> plantOf;
        std::vector<Target> targets = shotTargets(plantOf);
        for (Projectile& shot : projectiles_) {
            bool done = false;
            if (const auto hit = stepProjectile(shot, map_, targets, done)) {
                if (*hit < enemies_.size()) {
                    strike(enemies_[*hit], shot.weapon->damage, shot.weapon);
                    targets[*hit].alive = enemies_[*hit].isAlive();
                } else {
                    destroyPlant(plantOf[*hit - enemies_.size()]);
                    targets[*hit].alive = false;
                }
            } else if (done) {
                // Stopped by something solid: if a big plant stands there, the shot cuts it down.
                const int cellX = static_cast<int>(std::floor(shot.x / kTileSize));
                const int cellY = static_cast<int>(std::floor((shot.y + 20.0) / kTileSize));
                if (shot.weapon != nullptr && shot.weapon->element == Element::Fire) { // a fire shot stops against a standing piece of a building (US-255)
                    const int ahead = static_cast<int>(kTileSize / 2);
                    for (const auto& [wx, wy] : {std::pair{shot.x, shot.y}, std::pair{shot.x + shot.dx * ahead, shot.y + shot.dy * ahead}}) {
                        if (buildings_.fireHit(*this, static_cast<int>(std::floor(wx / kTileSize)), static_cast<int>(std::floor(wy / kTileSize)))) break;
                    }
                }
                for (std::size_t i = 0; i < plants_.size(); ++i) {
                    const PixelPoint cell = plantCell(plants_[i].feet);
                    if (plants_[i].alive && plants_[i].def != nullptr && plants_[i].def->blocks && cell.x == cellX && cell.y == cellY) {
                        destroyPlant(i);
                        break;
                    }
                }
            }
            if (done) shot.maxDistance = -1.0; // spent: removed below
        }
        std::erase_if(projectiles_, [](const Projectile& shot) { return shot.maxDistance < 0.0; });
    }

    // Arcs (US-140): each shot flies one physics tick and stops at the first enemy, solid tile or the ground.
    if (!arcShots_.empty()) {
        std::vector<std::size_t> plantOf;
        std::vector<Target> targets = shotTargets(plantOf);
        for (const ArcEvent& event : stepArcShots(arcShots_, map_, targets)) {
            const luna::engine::ScreenPoint ground = luna::engine::groundShadow(event.point);
            static constexpr const char* kEnds[] = {"hit an enemy", "stopped at a solid", "came down", "was lost"};
            core::logInfo(std::format("{} {} at ({:.1f}, {:.1f}) m, {:.2f} m up", event.weapon->name, kEnds[static_cast<int>(event.end)], luna::engine::toDouble(event.point.x), luna::engine::toDouble(event.point.y), luna::engine::toDouble(event.point.z)));
            if (event.end == ArcEnd::Enemy && event.enemy < enemies_.size()) {
                strike(enemies_[event.enemy], event.weapon->damage, event.weapon);
                targets[event.enemy].alive = enemies_[event.enemy].isAlive();
            } else if (event.end == ArcEnd::Enemy) {
                destroyPlant(plantOf[event.enemy - enemies_.size()]);
                targets[event.enemy].alive = false;
            } else if (event.end == ArcEnd::Solid && event.weapon != nullptr && event.weapon->element == Element::Fire &&
                       buildings_.fireHit(*this, static_cast<int>(std::floor(luna::engine::toDouble(event.point.x))), static_cast<int>(std::floor(luna::engine::toDouble(event.point.y))))) {
                // the building caught fire
            } else if (event.end != ArcEnd::Lost) {
                playEffect("dust", ground.x, ground.y, 16); // a miss sticks in the ground or against a rock
            }
        }
    }

    // A puff of dust behind every spear in the air, every third tick: its trail (US-132).
    if (ticks_ % 3 == 0) {
        for (const FlyingSpear& spear : range_.spears()) {
            if (spear.state != SpearState::Flying) continue;
            const luna::engine::ScreenPoint p = luna::engine::topDownPosition(spear.body.position);
            playEffect("dust", p.x, p.y, 16);
        }
    }
    effects_.update();

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
    // A texture and, when its atlas has a normal map, that map (US-241): lights then shade the sprite; without one it is lit flat.
    // The normal textures are made last, so the numbers of all the other textures stay as they were (tests find props by number).
    std::vector<std::pair<luna::engine::Texture, const luna::engine::Image*>> pendingNormals;
    static const luna::engine::Image kNone(0, 0); // "no normal map": the conditionals below must give lvalues, the pointers outlive them
    std::vector<luna::engine::Image> mirroredNormalPictures;
    mirroredNormalPictures.reserve(1);
    // Black copies of the sprite pages shadows are cut from (US-244); like the normal maps they are uploaded last.
    std::vector<std::pair<luna::engine::Texture, luna::engine::Image>> pendingShadows;
    const auto withNormals = [&renderer, &pendingNormals, &pendingShadows](const luna::engine::Image& picture, const luna::engine::Image& normals) {
        const luna::engine::Texture texture = renderer.createTexture(picture);
        pendingShadows.emplace_back(texture, luna::engine::silhouette(picture));
        if (normals.width() > 0) pendingNormals.emplace_back(texture, &normals);
        return texture;
    };
    characters_ = withNormals(art_.heroSheet, art_.heroNormals);
    tiles_ = withNormals(art_.tileStrip, art_.tileNormals);
    props_ = renderer.createTexture(makePropSheet()); // texture 2: tests find props by this number
    charactersAtlas_ = withNormals(art_.characters, art_.charactersNormals);
    charactersHitAtlas_ = withNormals(art_.charactersHit, art_.charactersNormals);
    uiSheet_ = renderer.createTexture(luna::engine::makeUiSheet());
    buildings_.loadArt(renderer); // the placeholder pieces of the buildings (US-250)
    editor_.setTextures({tiles_, characters_, charactersAtlas_, props_, uiSheet_, &art_, nullptr, nullptr, nullptr, nullptr, &buildings_});
    // The M2d content atlas (US-130); without it the game plays on, just without effects.
    std::string problem;
    if (auto content = loadContent(spritesDirectory_ / "atlas", problem)) {
        content_ = std::move(*content);
        contentLoaded_ = true;
        effectsTexture_ = renderer.createTexture(content_.pictures.at("effects"));
        iconsTexture_ = renderer.createTexture(content_.pictures.at("icons"));
        iconsMirrored_ = renderer.createTexture(luna::engine::mirrored(content_.pictures.at("icons")));
        weaponArt_.icons = iconsTexture_;
        if (const auto weather = content_.pictures.find("weather"); weather != content_.pictures.end()) {
            weatherTexture_ = renderer.createTexture(weather->second);
            for (const WeatherDef& def : catalogs_.weather) {
                for (int i = 0; i < def.frames; ++i) {
                    if (const auto rect = content_.rect(content_.frameName(def.name, i))) weatherFrames_[def.name].push_back(*rect);
                }
            }
        }
        effectArt_.page = effectsTexture_;
        for (const EffectDef& def : catalogs_.effects) {
            if (const auto rect = content_.rect(content_.frameName(def.name, 0))) effectArt_.firstFrame[def.name] = *rect;
        }
        for (const char* page : {"plants-small", "plants-tall", "trees"}) {
            if (const auto found = content_.pictures.find(page); found != content_.pictures.end()) {
                const auto normals = content_.normals.find(page);
                plantArt_.pages[page] = withNormals(found->second, normals == content_.normals.end() ? kNone : normals->second);
            }
        }
        if (const auto animals = content_.pictures.find("animals"); animals != content_.pictures.end()) {
            const auto animalNormals = content_.normals.find("animals");
            const bool hasNormals = animalNormals != content_.normals.end();
            animalArt_.page = withNormals(animals->second, hasNormals ? animalNormals->second : kNone);
            if (hasNormals) mirroredNormalPictures.push_back(luna::engine::mirroredNormals(animalNormals->second));
            animalArt_.mirrored = withNormals(luna::engine::mirrored(animals->second), hasNormals ? mirroredNormalPictures.back() : kNone);
            animalArt_.pageWidth = animals->second.width();
            for (const AnimalDef& animal : catalogs_.animals) {
                if (const auto rect = content_.rect(animal.frame)) animalArt_.sources[animal.name] = *rect;
            }
        }
        for (const PlantDef& plant : catalogs_.plants) {
            const auto frame = content_.frames.find(plant.frame);
            const auto rect = content_.rect(plant.frame);
            if (frame != content_.frames.end() && rect) plantArt_.sources[plant.name] = {frame->second.page, *rect};
        }
        {
            // The world objects (US-155) have no pictures in the atlas: the game draws them itself (programmer art).
            std::vector<const PlantDef*> objects;
            for (const PlantDef& plant : catalogs_.plants) {
                if (plant.object) objects.push_back(&plant);
            }
            if (!objects.empty()) {
                std::map<std::string, core::Rect> rects;
                const luna::engine::Image objectPage = makeObjectPage(objects, rects);
                plantArt_.pages["objects"] = renderer.createTexture(objectPage);
                pendingShadows.emplace_back(plantArt_.pages["objects"], luna::engine::silhouette(objectPage));
                for (const auto& [name, rect] : rects) plantArt_.sources[name] = {"objects", rect};
            }
        }
        for (const WeaponDef& weapon : catalogs_.weapons) {
            if (const auto icon = content_.rect(weapon.frame)) weaponArt_.sources[weapon.name] = *icon;
        }
        editor_.setTextures({tiles_, characters_, charactersAtlas_, props_, uiSheet_, &art_, &weaponArt_, &plantArt_, &animalArt_, &effectArt_, &buildings_});
        startPlacedEffects(); // the content is loaded now: the effects placed in the level start
    } else {
        core::logWarning("Content art missing, no effects: " + problem);
    }
    for (const auto& [texture, normals] : pendingNormals) renderer.setNormalMap(texture, renderer.createTexture(*normals));
    for (const auto& [texture, black] : pendingShadows) silhouettes_[texture.id] = renderer.createTexture(black);
}

void OdysseyGame::render(luna::engine::Renderer& output, double alpha) {
    // Draw time and GPU time (US-234): the card is only asked to measure while the overlay or a performance run needs it.
    struct DrawTimer {
        OdysseyGame& game;
        luna::engine::Renderer& output;
        std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
        ~DrawTimer() {
            game.recordFrame(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count(), output.gpuMilliseconds());
        }
    } drawTimer{*this, output};
    output.measureGpu(overlayOn_ || perfLog_);
    const auto renderStarted = std::chrono::steady_clock::now();
    if (lastRender_.time_since_epoch().count() != 0) {
        frameTimes_[frameTimeAt_] = std::chrono::duration<double, std::milli>(renderStarted - lastRender_).count();
        frameTimeAt_ = (frameTimeAt_ + 1) % frameTimes_.size();
        frameTimesFilled_ = std::min(frameTimesFilled_ + 1, frameTimes_.size());
    }
    lastRender_ = renderStarted;
    // The world is drawn zoomed and the interface scaled (US-232): each is laid out in its own small pixels.
    luna::engine::ScaledRenderer ui(output, settings_.uiScale);
    if (buildings_.artStale()) buildings_.loadArt(output); // the Editor saved a prefab: its icon is new
    if (mode_ == Mode::Editor) {
        editor_.render(output, alpha);
        drawInteractionPanel(ui); // F5 works in the Editor too, so its mistakes show there
        drawModeLabel(ui);
        return;
    }
    luna::engine::ScaledRenderer world(output, settings_.cameraZoom);
    luna::engine::Renderer& renderer = world;
    // The world is lit (US-240): the ambient colour tints everything drawn until the lighting is cleared below; the interface is never dimmed.
    luna::engine::LightFrame lightFrame = ambientLightFrame(alpha); // the time of day (US-242) and an eclipse (US-248) tint the ambient light
    lightFrame.normalMaps = settings_.lighting != "Low"; // the Low lighting quality shades sprites flat (US-247)
    // Fires and torches light the dark (US-243): the darker the ambient light, the stronger they shine; in full daylight they add nothing.
    if (const double darkness = darknessOf(lightFrame); darkness > 0.0) lightFrame.lights = worldLights(camera_.view(alpha), alpha, darkness);
    renderer.setLighting(&lightFrame);
    map_.draw(renderer, tiles_, camera_, alpha);
    const luna::engine::Rect view = camera_.view(alpha);
    drawShadows(renderer, view, alpha); // on the ground, under everything that stands or flies
    buildings_.drawGround(renderer, view);
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
    // Weapons lying in the level: a shadow on the ground and the icon, until picked up (US-134).
    {
        luna::engine::UiPainter painter(renderer, uiSheet_);
        for (const WorldPickup& lying : pickups_) {
            if (lying.taken) continue;
            renderer.draw(props_, kShadowFrame, screen(lying.pickup.at.x - kShadowFrame.width / 2.0, lying.pickup.at.y + 4.0));
            const luna::engine::Point corner = screen(lying.pickup.at.x - kPickupSize / 2.0, lying.pickup.at.y - kPickupSize / 2.0);
            drawWeaponIcon(renderer, painter, weaponArt_, lying.pickup.weapon, {corner.x, corner.y, kPickupSize, kPickupSize});
        }
    }
    drawPlants(renderer, view, alpha, true); // plants whose feet are above the hero's are behind him
    buildings_.drawStanding(renderer, view, hero_.feetY(alpha), true);
    drawClan(renderer, view, alpha, true);
    // The hero, blended between ticks like the camera, so walking looks smooth at 60 FPS.
    renderer.draw(characters_, hero_.spriteFrame(),
                  screen(hero_.feetX(alpha) - kCharacterWidth / 2.0, hero_.feetY(alpha) - kCharacterHeight));

    // Placed characters who are not enemies stand where they were put (M2c, D-19).
    for (const PlacedCharacter& placed : bystanders_) {
        if (const CharacterKindDef* kind = definitions_.character(placed.kind)) {
            if (kind->animal) {
                const luna::engine::Point feet = screen(placed.feet.x, placed.feet.y);
                drawAnimal(renderer, animalArt_, kind->name, feet.x, feet.y, placed.facing, false);
                continue;
            }
            renderer.draw(charactersAtlas_, art_.frame(kind->frames, kind->directions, placed.facing, 0),
                          screen(placed.feet.x - kCharacterWidth / 2.0, placed.feet.y - kCharacterHeight));
        }
    }

    // Words over the heads of the placed people who are talking to each other (US-292).
    for (const auto& [id, said] : npcBubbles_) {
        for (const PlacedCharacter& placed : bystanders_) {
            if (placed.id != id) continue;
            luna::engine::UiPainter painter(renderer, uiSheet_);
            const luna::engine::Point head = screen(placed.feet.x, placed.feet.y - kCharacterHeight);
            const int w = luna::engine::UiPainter::textWidth(said.first) + 8;
            const luna::engine::Rect box{static_cast<int>(head.x) - w / 2, static_cast<int>(head.y) - 16, w, 14};
            painter.fill(box, luna::engine::UiColor::Shade);
            painter.outline(box, luna::engine::UiColor::Border);
            painter.text(box.x + 4, box.y + 4, said.first, luna::engine::UiColor::Text);
        }
    }

    // Enemies: red while the hit flash lasts, with a health bar and "HP/max" above their heads.
    for (const Enemy& enemy : enemies_) {
        if (!enemy.isAlive()) {
            continue;
        }
        const double left = enemy.feetX() - kCharacterWidth / 2.0;
        const double top = enemy.feetY() - kCharacterHeight;
        if (enemy.animal) {
            const luna::engine::Point feet = screen(enemy.feetX(), enemy.feetY());
            drawAnimal(renderer, animalArt_, enemy.kindName, feet.x, feet.y, enemy.facing, enemy.isFlashing());
        } else {
            renderer.draw(enemy.isFlashing() ? charactersHitAtlas_ : charactersAtlas_, art_.frame(enemy.frames, enemy.directions, enemy.facing, 0),
                          screen(left, top));
        }

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

    drawPlants(renderer, view, alpha, false); // the plants in front of the hero and the enemies
    drawClan(renderer, view, alpha, false);
    buildings_.drawStanding(renderer, view, hero_.feetY(alpha), false);
    buildings_.drawRoofs(*this, renderer, view);
    buildings_.drawBlueprints(*this, renderer, uiSheet_, view);
    buildings_.drawGhost(*this, renderer, view);

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
    drawHeld(renderer, view, alpha);
    drawAim(renderer, view, alpha);
    effects_.draw(renderer, view);
    renderer.setLighting(nullptr);
    drawSkyBodies(renderer, alpha);
    drawWeather(ui);
    drawInspection(renderer, view);
    drawRivals(renderer, view);
    drawRunWorld(renderer, view);
    drawClanDetails(renderer, view, alpha);
    drawQuestSigns(renderer, view, alpha);
    drawQuestMarker(renderer, view, alpha);
    drawHud(ui);
    buildings_.drawMenu(*this, ui, uiSheet_);
    drawClanHud(ui);
    drawRunHud(ui);
    drawDevTools(ui, view, alpha);
    drawTutorial(ui);
    drawQuestTracker(ui);
    drawQuestDebug(ui);
    runFlow_.draw(ui, uiSheet_);
    drawActionRing(renderer, view);
    drawOverlay(ui);
    drawInteractionPanel(ui);
    drawModeLabel(ui);
}

std::uint64_t OdysseyGame::ticks() const {
    return ticks_;
}

luna::engine::AppConfig odysseyAppConfig() {
    luna::engine::AppConfig config;
    config.title = "Project Odyssey " + std::string(core::versionString());
    config.windowWidth = 1280; // US-020: a 1280 x 720 window
    config.windowHeight = 720;
    config.virtualWidth = core::kVirtualWidth;
    config.virtualHeight = core::kVirtualHeight;
    config.ticksPerSecond = 20; // ADR-006
    config.clearRed = 34;       // a deep green-blue, like dusk over the valley
    config.clearGreen = 52;
    config.clearBlue = 60;
    return config;
}


// ---- inside a building (US-254)

// The world outside, kept while the hero is in an interior map and put back when the hero leaves.
struct OdysseyGame::OutsideWorld {
    Level level;
    std::filesystem::path levelFile;
    std::optional<luna::engine::TileMap> map;
    std::vector<WorldPlant> plants;
    std::vector<Enemy> enemies;
    std::vector<PlacedCharacter> bystanders;
    std::vector<WorldPickup> pickups;
    std::string buildingsText;
    double heroX = 0.0;
    double heroY = 0.0;
    int buildingId = 0;
};

std::string OdysseyGame::enterBuilding(int buildingId) {
    if (insideBuilding()) return "You are already inside.";
    const sim::buildings::BuildingStore& store = buildings_.store();
    const sim::buildings::PlacedBuilding* building = store.find(buildingId);
    if (building == nullptr || building->state != sim::buildings::State::Finished) return "There is nothing to go into.";
    const std::string name = store.interiorLevelOf(*building);
    if (store.interiorMode(*building) != "map" || name.empty()) return "This building has no inside to see.";
    const std::filesystem::path file = dataDirectory_.parent_path() / "levels" / (name + ".json");
    LoadedLevel loaded;
    try {
        loaded = loadLevel(file, definitions_);
    } catch (const std::exception& error) {
        return std::string("The inside could not be opened: ") + error.what();
    }
    auto stash = std::make_shared<OutsideWorld>();
    stash->level = level_;
    stash->levelFile = levelFile_;
    stash->map = map_;
    stash->plants = plants_;
    stash->enemies = enemies_;
    stash->bystanders = bystanders_;
    stash->pickups = pickups_;
    stash->buildingsText = buildings_.saveText();
    stash->heroX = hero_.feetX();
    stash->heroY = hero_.feetY();
    stash->buildingId = buildingId;
    outside_ = stash;
    level_ = loaded.level;
    map_ = buildTileMap(level_, definitions_);
    const MaterialsConfig materials = range_.materials();
    range_ = SpearRange(map_, materials);
    camera_ = luna::engine::Camera(viewWidth(), viewHeight(), map_.pixelWidth(), map_.pixelHeight());
    hero_ = Hero(static_cast<double>(level_.heroStart.x), static_cast<double>(level_.heroStart.y));
    projectiles_.clear();
    arcShots_.clear();
    populate();
    clanView_.setAllHidden(true);
    if (clan_) clanView_.update(*clan_, map_);
    camera_.centreOn(hero_.feetX(), hero_.feetY());
    editor_.levelChanged();
    say("You go inside.");
    return {};
}

std::string OdysseyGame::leaveBuilding() {
    if (!insideBuilding()) return "You are not inside a building.";
    const std::shared_ptr<OutsideWorld> stash = outside_;
    outside_.reset();
    level_ = stash->level;
    levelFile_ = stash->levelFile;
    map_ = *stash->map;
    const MaterialsConfig materials = range_.materials();
    range_ = SpearRange(map_, materials);
    camera_ = luna::engine::Camera(viewWidth(), viewHeight(), map_.pixelWidth(), map_.pixelHeight());
    plants_ = stash->plants;
    enemies_ = stash->enemies;
    bystanders_ = stash->bystanders;
    pickups_ = stash->pickups;
    projectiles_.clear();
    arcShots_.clear();
    startPlacedEffects();
    buildings_.start(*this);
    buildings_.loadText(stash->buildingsText);
    buildings_.syncObstacles(*this);
    hero_ = Hero(stash->heroX, stash->heroY);
    clanView_.setAllHidden(false);
    if (clan_) clanView_.update(*clan_, map_);
    camera_.centreOn(hero_.feetX(), hero_.feetY());
    editor_.levelChanged();
    say("You step outside.");
    return {};
}

// What enters the hero bag is reported to the quests (US-181): crafted things count as crafting, everything else as gathering.
void OdysseyGame::watchHeroItems() {
    if (!life_) return;
    life_->setItemObserver([this](const std::string& item, int amount, bool crafted) {
        questEvent(crafted ? sim::rules::QuestObjective::Kind::Craft : sim::rules::QuestObjective::Kind::Gather, item, amount);
    });
}

} // namespace odysseus::game
