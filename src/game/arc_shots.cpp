#include "game/arc_shots.h"

#include "game/placeholder_art.h"
#include "luna/engine/physics_view.h"

#include <algorithm>
#include <cmath>

namespace odysseus::game {

namespace {

using luna::physics::Box;
using luna::physics::Fixed;
using luna::physics::Vec3;

constexpr double kPixelsPerMetre = 32.0;   // one tile (D-16)
constexpr double kGravity = 9.81;          // m/s^2
constexpr double kReachMargin = 0.95;      // a shot aimed at the very edge of what the speed can reach would be too fragile
constexpr double kMinDistance = 0.5;       // metres: a shot at your own feet still goes somewhere
constexpr int kMaxFlightTicks = 200;       // ten seconds: anything longer is lost
constexpr double kTileSearchMetres = 3.0;  // solid tiles this close to the shot are tested each tick
constexpr double kChestMetres = 1.0;       // where a key-aimed shot is aimed, above the ground

// Metres as Fixed, rounded to 1/1024 m like every pixel position (deterministic).
Fixed metres(double value) { return luna::engine::metresFromPixels(value * kPixelsPerMetre); }

const luna::physics::Air& air() {
    static const luna::physics::Air still; // no wind
    return still;
}

} // namespace

double arcLaunchSpeed(WeaponClass weaponClass) {
    switch (weaponClass) {
    case WeaponClass::Bow: return 16.0;
    case WeaponClass::Thrown: return 10.0;
    default: return 0.0;
    }
}

bool flysInArc(WeaponClass weaponClass) { return weaponClass == WeaponClass::Bow || weaponClass == WeaponClass::Thrown; }

std::optional<ArcShot> launchArcShot(const WeaponDef& weapon, double heroFeetX, double heroFeetY, double dirX, double dirY, double distancePixels,
                                     bool chestHeight) {
    if (!flysInArc(weapon.weaponClass)) {
        return std::nullopt;
    }
    const double speed = arcLaunchSpeed(weapon.weaponClass);
    const double reach = speed * speed / kGravity * kReachMargin; // the far end of a 45-degree arc, with a margin
    const double distance = std::max(kMinDistance, std::min({distancePixels / kPixelsPerMetre, weapon.range, reach}));

    const double feetX = heroFeetX / kPixelsPerMetre;
    const double feetY = heroFeetY / kPixelsPerMetre;
    const Vec3 hand{metres(feetX + dirX * kHandAheadMetres), metres(feetY + dirY * kHandAheadMetres), metres(kHandHeightMetres)};
    const Vec3 aimPoint{metres(feetX + dirX * distance), metres(feetY + dirY * distance), metres(chestHeight ? kChestMetres : 0.0)};

    ArcShot shot;
    shot.weapon = &weapon;
    shot.body = luna::physics::Projectile{hand, {}, Fixed::fromRatio(1, 10), luna::physics::kFixedZero};
    const auto angle = luna::physics::aimLaunchAngle(shot.body, air(), aimPoint, metres(speed));
    shot.body.velocity = luna::physics::launchVelocity(hand, aimPoint, metres(speed), angle ? *angle : luna::physics::degrees(45));
    shot.previousPosition = hand;
    shot.heading = luna::physics::normalized(shot.body.velocity);
    shot.aimPoint = aimPoint;
    return shot;
}

std::vector<ArcEvent> stepArcShots(std::vector<ArcShot>& shots, const luna::engine::TileMap& map, const std::vector<Target>& targets) {
    std::vector<ArcEvent> events;
    std::vector<bool> remove(shots.size(), false);
    const Fixed bodyHeight = metres(kBodyHeightMetres);
    const Fixed halfWidth = metres(kBodyHalfWidthMetres);
    const Box ground{{luna::physics::kFixedZero, luna::physics::kFixedZero, -luna::physics::kFixedOne},
                     {Fixed::fromInt(map.width()), Fixed::fromInt(map.height()), luna::physics::kFixedZero}};

    for (std::size_t i = 0; i < shots.size(); ++i) {
        ArcShot& shot = shots[i];
        shot.previousPosition = shot.body.position;
        if (shot.state == ArcState::Stuck) {
            if (--shot.stuckTicks <= 0) remove[i] = true;
            continue;
        }
        ++shot.age;
        const double x = luna::engine::toDouble(shot.body.position.x);
        const double y = luna::engine::toDouble(shot.body.position.y);
        if (shot.age > kMaxFlightTicks || x < 0.0 || y < 0.0 || x > map.width() || y > map.height()) {
            remove[i] = true;
            events.push_back({i, shot.weapon, ArcEnd::Lost, 0, shot.body.position});
            continue;
        }

        // What it can hit: solid tiles near it, the ground, then the characters.
        std::vector<luna::physics::Shape> obstacles;
        const int firstTileX = static_cast<int>(std::floor(x - kTileSearchMetres));
        const int lastTileX = static_cast<int>(std::floor(x + kTileSearchMetres));
        const int firstTileY = static_cast<int>(std::floor(y - kTileSearchMetres));
        const int lastTileY = static_cast<int>(std::floor(y + kTileSearchMetres));
        for (int ty = std::max(0, firstTileY); ty <= std::min(map.height() - 1, lastTileY); ++ty) {
            for (int tx = std::max(0, firstTileX); tx <= std::min(map.width() - 1, lastTileX); ++tx) {
                if (map.at(tx, ty) == static_cast<int>(TileKind::Rock)) {
                    obstacles.push_back(Box{{Fixed::fromInt(tx), Fixed::fromInt(ty), luna::physics::kFixedZero},
                                            {Fixed::fromInt(tx + 1), Fixed::fromInt(ty + 1), metres(kSolidHeightMetres)}});
                }
            }
        }
        const std::size_t groundIndex = obstacles.size();
        obstacles.push_back(ground);
        const std::size_t firstTarget = obstacles.size();
        std::vector<std::size_t> targetOf; // obstacle offset -> index in `targets`
        for (std::size_t t = 0; t < targets.size(); ++t) {
            if (!targets[t].alive) continue;
            const Fixed cx = metres(targets[t].x / kPixelsPerMetre);
            const Fixed cy = metres(targets[t].y / kPixelsPerMetre);
            obstacles.push_back(Box{{cx - halfWidth, cy - halfWidth, luna::physics::kFixedZero}, {cx + halfWidth, cy + halfWidth, bodyHeight}});
            targetOf.push_back(t);
        }

        const auto hit = luna::physics::flyTick(shot.body, air(), Fixed::fromRatio(2, 100), obstacles);
        if (luna::physics::lengthSquared(shot.body.velocity) > luna::physics::kFixedZero) {
            shot.heading = luna::physics::normalized(shot.body.velocity);
        }
        if (!hit) {
            continue;
        }
        ArcEvent event{i, shot.weapon, ArcEnd::Solid, 0, hit->hit.point};
        if (hit->obstacle >= firstTarget) {
            event.end = ArcEnd::Enemy;
            event.enemy = targetOf.at(hit->obstacle - firstTarget);
            remove[i] = true; // it is spent on the enemy
        } else {
            event.end = hit->obstacle == groundIndex ? ArcEnd::Ground : ArcEnd::Solid;
            shot.state = ArcState::Stuck; // it sticks where it came down
            shot.stuckTicks = kStuckTicks;
            shot.body.velocity = {};
        }
        events.push_back(event);
    }

    std::size_t keep = 0;
    for (std::size_t i = 0; i < shots.size(); ++i) {
        if (!remove[i]) {
            if (keep != i) shots[keep] = std::move(shots[i]);
            ++keep;
        }
    }
    shots.resize(keep);
    return events;
}

} // namespace odysseus::game
