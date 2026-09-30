#pragma once

#include "boundary.h"

#include "game/catalogs.h"
#include "game/weapons.h"
#include "luna/engine/tile_map.h"
#include "luna/physics/ballistics.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace odysseus::game {

// Shots that fly real arcs (US-140, D-25): arrows, crossbow bolts and thrown weapons are Luna
// Physics projectiles with height, gravity and a launch angle solved to land where the hero
// aims. Positions are metres (one 32-pixel tile is one metre); z is height above the ground.
// Staff bolts and bullets stay on the flat path of weapons.h.

// How tall a rock is to a shot: lower arcs stop at it, higher ones clear it. (Water and other tiles that only stop
// walking do not stop a shot; taller things such as trees get their own heights when plants arrive, US-136.)
inline constexpr double kSolidHeightMetres = 1.0;
// A character is hit from his feet up to here, and within this half-width (metres).
inline constexpr double kBodyHeightMetres = 1.5;
inline constexpr double kBodyHalfWidthMetres = 0.3;
// The hand the shot leaves from: shoulder height, a little ahead of the body.
inline constexpr double kHandHeightMetres = 1.3;
inline constexpr double kHandAheadMetres = 0.3;
// A shot that has come down stays stuck in the ground this long (ticks), then is gone.
inline constexpr int kStuckTicks = 40;

enum class ArcState { Flying, Stuck };

struct ArcShot {
    const WeaponDef* weapon = nullptr;
    luna::physics::Projectile body;
    luna::physics::Vec3 previousPosition; // for smooth drawing between ticks
    luna::physics::Vec3 heading;          // unit vector of the flight, for drawing
    luna::physics::Vec3 aimPoint;         // where it was aimed (after the range clamp)
    ArcState state = ArcState::Flying;
    int stuckTicks = 0;
    int age = 0;                          // ticks in flight
};

// What ended a flight this tick.
enum class ArcEnd { Enemy, Solid, Ground, Lost };

struct ArcEvent {
    std::size_t shot = 0;   // index in the list passed to `stepArcShots` (before removals)
    const WeaponDef* weapon = nullptr;
    ArcEnd end = ArcEnd::Ground;
    std::size_t enemy = 0;  // with ArcEnd::Enemy: the index in `targets`
    luna::physics::Vec3 point;
};

// Where a shot from `heroFeetX/Y` (world pixels) aimed along (dirX, dirY) lands: on the ground
// `distancePixels` away (a mouse aim), or, for `chestHeight`, at chest height `distancePixels`
// away (a key aim, which has no pointer: a shallow arrow that hits what stands in front).
// The distance is clamped to the weapon's range and to what its launch speed can reach.
// Returns nothing for a weapon that does not arc (staffs, guns).
std::optional<ArcShot> launchArcShot(const WeaponDef& weapon, double heroFeetX, double heroFeetY, double dirX, double dirY, double distancePixels,
                                     bool chestHeight);

// The launch speed of a class, metres per second (bows 16, thrown 10).
double arcLaunchSpeed(WeaponClass weaponClass);
// Whether a class flies in an arc (bows and crossbows, thrown) rather than flat.
bool flysInArc(WeaponClass weaponClass);

// One tick of every shot: flying ones move (and stop at the first enemy, solid tile or the
// ground), stuck ones count down and are removed. `targets` are the characters that can be hit.
std::vector<ArcEvent> stepArcShots(std::vector<ArcShot>& shots, const luna::engine::TileMap& map, const std::vector<Target>& targets);

} // namespace odysseus::game
