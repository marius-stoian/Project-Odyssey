#pragma once

#include "boundary.h"

#include "game/catalogs.h"
#include "game/placeholder_art.h"
#include "luna/engine/tile_map.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The eight weapon classes (US-133, D-21): each attacks its own way; the numbers (damage,
// speed, range) come from the weapon in weapons.json. Positions are world pixels; one tile
// (32 pixels) is one metre.

// Something a weapon can hit: where it stands and whether it still does.
struct Target {
    double x = 0.0;
    double y = 0.0;
    bool alive = true;
};

// A shot in flight: an arrow, a thrown star, a bolt, a bullet.
struct Projectile {
    const WeaponDef* weapon = nullptr;
    double x = 0.0;
    double y = 0.0;
    double dx = 0.0;          // direction, length 1
    double dy = 0.0;
    double pixelsPerTick = 0.0;
    double travelled = 0.0;   // pixels so far
    double maxDistance = 0.0; // pixels: the weapon's range
};

// How a class attacks. Melee classes hit what stands in front of the hero; ranged classes
// launch a projectile.
class WeaponBehaviour {
public:
    virtual ~WeaponBehaviour() = default;
    virtual bool melee() const = 0;
    // Melee: the targets one swing hits (indexes into `targets`), nearest first. The swing points
    // along (dirX, dirY), a unit vector: toward the mouse pointer (US-139), or along a facing.
    virtual std::vector<std::size_t> swingToward(const WeaponDef& weapon, double heroX, double heroY, double dirX, double dirY,
                                                 const std::vector<Target>& targets) const;
    // Ranged: the projectile the hero lets go along (dirX, dirY).
    virtual std::optional<Projectile> launchToward(const WeaponDef& weapon, double heroX, double heroY, double dirX, double dirY) const;
    // The same along one of the 8 facings.
    std::vector<std::size_t> swing(const WeaponDef& weapon, double heroX, double heroY, Facing facing, const std::vector<Target>& targets) const;
    std::optional<Projectile> launch(const WeaponDef& weapon, double heroX, double heroY, Facing facing) const;
};

class MeleeBehaviour final : public WeaponBehaviour {
public:
    // `arcDegrees`: how wide the swing is, centred on the facing; `hitsAll`: every target in the
    // arc (axe, whip) or only the nearest (sword, spear).
    MeleeBehaviour(double arcDegrees, bool hitsAll) : arcDegrees_(arcDegrees), hitsAll_(hitsAll) {}
    bool melee() const override { return true; }
    std::vector<std::size_t> swingToward(const WeaponDef& weapon, double heroX, double heroY, double dirX, double dirY,
                                         const std::vector<Target>& targets) const override;

private:
    double arcDegrees_;
    bool hitsAll_;
};

class RangedBehaviour final : public WeaponBehaviour {
public:
    explicit RangedBehaviour(double metresPerSecond) : metresPerSecond_(metresPerSecond) {}
    bool melee() const override { return false; }
    std::optional<Projectile> launchToward(const WeaponDef& weapon, double heroX, double heroY, double dirX, double dirY) const override;

private:
    double metresPerSecond_;
};

// The behaviour of each class: sword (90-degree arc, nearest), axe (120 degrees, all), spear
// (30-degree thrust, nearest), whip (100 degrees, all), bow (arrow, 16 m/s), thrown
// (10 m/s), staff (bolt, 12 m/s), gun (bullet, 24 m/s).
const WeaponBehaviour& behaviourOf(WeaponClass weaponClass);

// Ticks between two attacks: 20 ticks per second divided by the weapon's attacks per second.
int cooldownTicks(const WeaponDef& weapon);

// One tick of flight. Returns what it hit: a target's index, or nothing. `done` becomes true
// when the projectile is spent (a hit, a solid tile, or the end of its range).
std::optional<std::size_t> stepProjectile(Projectile& projectile, const luna::engine::TileMap& map, const std::vector<Target>& targets,
                                          bool& done);

// The unit vector a facing points along, on the ground (x east, y south).
void facingVector(Facing facing, double& x, double& y);
// The nearest of the 8 facings to a direction (the hero faces the mouse pointer that way).
Facing facingToward(double dirX, double dirY);
// The same, but the hero keeps current until the direction is more than hysteresisDegrees beyond the edge of
// its 45-degree sector. Without this, a pointer close to a sector edge (or a hero walking past the pointer)
// makes the sprite flicker between two facings.
Facing facingToward(double dirX, double dirY, Facing current, double hysteresisDegrees);

} // namespace odysseus::game
