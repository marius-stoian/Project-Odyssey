#include "game/odyssey_game.h"

#include "luna/engine/image_ops.h"

#include "core/log.h"
#include "core/version.h"
#include "game/art.h"
#include "game/placeholder_art.h"
#include "luna/engine/physics_view.h"
#include "luna/engine/ui.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
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
      range_(map_, loadMaterials(dataDirectory)), spritesDirectory_(dataDirectory.parent_path() / "sprites"),
      editor_(level_, definitions_, levelFile_, kVirtualWidth, kVirtualHeight) {
    catalogs_ = loadCatalogs(dataDirectory); // M2d content (US-130): weapons, plants, animals, effects, weather
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
}

void OdysseyGame::resetPlay() {
    map_ = buildTileMap(level_, definitions_);
    camera_ = luna::engine::Camera(kVirtualWidth, kVirtualHeight, map_.pixelWidth(), map_.pixelHeight());
    hero_ = Hero(static_cast<double>(level_.heroStart.x), static_cast<double>(level_.heroStart.y));
    const MaterialsConfig materials = range_.materials();
    range_ = SpearRange(map_, materials);
    sword_ = Sword();
    framingTicks_ = 0;
    nextSpearIsFlint_ = true;
    heroHp_ = kHeroMaxHp;
    respawnTicks_ = 0;
    effects_.clear();
    projectiles_.clear();
    arcShots_.clear();
    attackCooldown_ = 0;
    swingTicks_ = 0;
    hotbar_ = {};     // a restart: empty hands, and every pickup lies in the level again
    heldSlot_ = 0;
    fullTicks_ = 0;
    camera_.centreOn(hero_.feetX(), hero_.feetY());
    populate();
}

void OdysseyGame::switchMode(Mode mode) {
    if (mode == mode_) {
        return;
    }
    mode_ = mode;
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
    const int top = mode_ == Mode::Game ? 2 : kVirtualHeight - luna::engine::kGlyphHeight - 6;
    const luna::engine::Rect box{kVirtualWidth - width - 2, top, width, luna::engine::kGlyphHeight + 6};
    painter.fill(box, luna::engine::UiColor::Shade);
    painter.text(box.x + 3, box.y + 3, label, mode_ == Mode::Game ? luna::engine::UiColor::Text : luna::engine::UiColor::Gold);
}

void OdysseyGame::populate() {
    populatePlants();
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
        if (kind != nullptr && !kind->enemy) {
            bystanders_.push_back(placed);
        }
        if (kind == nullptr || !kind->enemy) {
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
        plants_.push_back({placed.id, placed.kind, def, placed.feet, true, 0});
        if (const double height = plantObstacleHeight(*def); height > 0.0) {
            const PixelPoint cell = plantCell(placed.feet);
            map_.setObstacle(cell.x, cell.y, height);
        }
    }
}

// A weapon hit destroys a plant: leaves fly, an edible one heals the hero, and 15 s later it grows back elsewhere.
void OdysseyGame::destroyPlant(std::size_t index) {
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
    if (plant.def != nullptr && plant.def->edible && respawnTicks_ == 0) {
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
        if (!plant.alive) continue;
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
        if (plant.alive && (plant.feet.y <= heroY) == behindHero) order.push_back(&plant);
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
        box.x = std::clamp(box.x, 2, kVirtualWidth - width - 2);
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
            if (!plants_[i].alive) continue;
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
    const int left = (kVirtualWidth - (kHotbarSlots * kSlot + (kHotbarSlots - 1) * kGap)) / 2;
    const int top = kVirtualHeight - kSlot - 4;
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
        painter.text((kVirtualWidth - luna::engine::UiPainter::textWidth(message)) / 2, top - luna::engine::kGlyphHeight - 3, message, luna::engine::UiColor::Red);
    }
    // After a fall the screen darkens, more with every step of the fade.
    for (int layer = 0; layer < (kRespawnTicks - respawnTicks_) / 4 + 1 && respawnTicks_ > 0; ++layer) {
        painter.fill({0, 0, kVirtualWidth, kVirtualHeight}, luna::engine::UiColor::Shade);
    }
}

void OdysseyGame::update(const luna::engine::Intents& intents) {
    if (intents.pressed(luna::engine::Intent::ModeEditor)) {
        switchMode(Mode::Editor);
    } else if (intents.pressed(luna::engine::Intent::ModeGame)) {
        switchMode(Mode::Game);
    }
    if (mode_ == Mode::Editor) {
        editor_.update(intents); // the world stands still
        return;
    }
    ++ticks_;
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
    updateAim(intents.pointer(), fallen, facingBefore);

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
    if (!fallen && (intents.pressed(luna::engine::Intent::Inspect) || (intents.pressed(luna::engine::Intent::Interact) && heldSlotName.empty()))) {
        inspectNearestPlant();
    }
    if (inspection_.ticks > 0) --inspection_.ticks;
    tickPlants();

    // Attack (the left button) goes toward the pointer; Interact goes along the facing, as before.
    const bool mouseAttack = heldWeapon() != nullptr && aiming_ && intents.held(luna::engine::Intent::Attack);
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
    characters_ = renderer.createTexture(art_.heroSheet);
    tiles_ = renderer.createTexture(art_.tileStrip);
    props_ = renderer.createTexture(makePropSheet()); // texture 2: tests find props by this number
    charactersAtlas_ = renderer.createTexture(art_.characters);
    charactersHitAtlas_ = renderer.createTexture(art_.charactersHit);
    uiSheet_ = renderer.createTexture(luna::engine::makeUiSheet());
    editor_.setTextures({tiles_, characters_, charactersAtlas_, props_, uiSheet_, &art_});
    // The M2d content atlas (US-130); without it the game plays on, just without effects.
    std::string problem;
    if (auto content = loadContent(spritesDirectory_ / "atlas", problem)) {
        content_ = std::move(*content);
        contentLoaded_ = true;
        effectsTexture_ = renderer.createTexture(content_.pictures.at("effects"));
        iconsTexture_ = renderer.createTexture(content_.pictures.at("icons"));
        iconsMirrored_ = renderer.createTexture(luna::engine::mirrored(content_.pictures.at("icons")));
        weaponArt_.icons = iconsTexture_;
        for (const char* page : {"plants-small", "plants-tall", "trees"}) {
            if (const auto found = content_.pictures.find(page); found != content_.pictures.end()) plantArt_.pages[page] = renderer.createTexture(found->second);
        }
        if (const auto animals = content_.pictures.find("animals"); animals != content_.pictures.end()) {
            animalArt_.page = renderer.createTexture(animals->second);
            animalArt_.mirrored = renderer.createTexture(luna::engine::mirrored(animals->second));
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
        for (const WeaponDef& weapon : catalogs_.weapons) {
            if (const auto icon = content_.rect(weapon.frame)) weaponArt_.sources[weapon.name] = *icon;
        }
        editor_.setTextures({tiles_, characters_, charactersAtlas_, props_, uiSheet_, &art_, &weaponArt_, &plantArt_, &animalArt_});
    } else {
        core::logWarning("Content art missing, no effects: " + problem);
    }
}

void OdysseyGame::render(luna::engine::Renderer& renderer, double alpha) {
    if (mode_ == Mode::Editor) {
        editor_.render(renderer, alpha);
        drawModeLabel(renderer);
        return;
    }
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
    drawInspection(renderer, view);
    drawHud(renderer);
    drawModeLabel(renderer);
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
