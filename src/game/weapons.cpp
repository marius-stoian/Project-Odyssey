#include "game/weapons.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace odysseus::game {

namespace {

constexpr double kPixelsPerMetre = 32.0; // one tile (D-16)
constexpr double kTicksPerSecond = 20.0; // ADR-006
constexpr double kHitRadius = 14.0;      // how close a projectile passes to hit (pixels)

} // namespace

void facingVector(Facing facing, double& x, double& y) {
    // Facing order: S, SW, W, NW, N, NE, E, SE.
    constexpr double d = std::numbers::sqrt2 / 2.0;
    constexpr std::array<std::array<double, 2>, 8> kVectors{{{0, 1}, {-d, d}, {-1, 0}, {-d, -d}, {0, -1}, {d, -d}, {1, 0}, {d, d}}};
    const auto& v = kVectors.at(static_cast<std::size_t>(facing) % 8);
    x = v[0];
    y = v[1];
}

Facing facingToward(double dirX, double dirY) {
    // Facing order: S, SW, W, NW, N, NE, E, SE; each takes the 45 degrees around its direction.
    const double angle = std::atan2(dirY, dirX); // 0 = east, pi/2 = south
    const int sector = static_cast<int>(std::lround(angle / (std::numbers::pi / 4.0))); // -4..4, 0 = east
    constexpr std::array<Facing, 8> kBySector{Facing::East, Facing::SouthEast, Facing::South, Facing::SouthWest,
                                              Facing::West, Facing::NorthWest, Facing::North, Facing::NorthEast};
    return kBySector.at(static_cast<std::size_t>((sector + 8) % 8));
}

std::vector<std::size_t> WeaponBehaviour::swingToward(const WeaponDef&, double, double, double, double, const std::vector<Target>&) const {
    return {};
}

std::optional<Projectile> WeaponBehaviour::launchToward(const WeaponDef&, double, double, double, double) const {
    return std::nullopt;
}

std::vector<std::size_t> WeaponBehaviour::swing(const WeaponDef& weapon, double heroX, double heroY, Facing facing,
                                                const std::vector<Target>& targets) const {
    double fx = 0.0;
    double fy = 0.0;
    facingVector(facing, fx, fy);
    return swingToward(weapon, heroX, heroY, fx, fy, targets);
}

std::optional<Projectile> WeaponBehaviour::launch(const WeaponDef& weapon, double heroX, double heroY, Facing facing) const {
    double fx = 0.0;
    double fy = 0.0;
    facingVector(facing, fx, fy);
    return launchToward(weapon, heroX, heroY, fx, fy);
}

std::vector<std::size_t> MeleeBehaviour::swingToward(const WeaponDef& weapon, double heroX, double heroY, double fx, double fy,
                                                     const std::vector<Target>& targets) const {
    const double reach = weapon.range * kPixelsPerMetre;
    const double halfArc = arcDegrees_ / 2.0 * std::numbers::pi / 180.0;
    std::vector<std::pair<double, std::size_t>> inArc;
    for (std::size_t i = 0; i < targets.size(); ++i) {
        if (!targets[i].alive) continue;
        const double dx = targets[i].x - heroX;
        const double dy = targets[i].y - heroY;
        const double distance = std::hypot(dx, dy);
        if (distance > reach) continue;
        // Standing on the hero counts as in front; otherwise the angle to the facing decides.
        if (distance > 1.0) {
            const double cosine = std::clamp((dx * fx + dy * fy) / distance, -1.0, 1.0);
            if (std::acos(cosine) > halfArc) continue;
        }
        inArc.push_back({distance, i});
    }
    // Nearest first; equal distances by index, so the same play always hits the same target.
    std::sort(inArc.begin(), inArc.end());
    std::vector<std::size_t> hits;
    for (const auto& [distance, index] : inArc) {
        hits.push_back(index);
        if (!hitsAll_) break;
    }
    return hits;
}

std::optional<Projectile> RangedBehaviour::launchToward(const WeaponDef& weapon, double heroX, double heroY, double dirX, double dirY) const {
    Projectile shot;
    shot.weapon = &weapon;
    shot.dx = dirX;
    shot.dy = dirY;
    // It starts at the hero's middle, a little ahead, so it does not hit what stands behind.
    shot.x = heroX + shot.dx * 10.0;
    shot.y = heroY - 20.0 + shot.dy * 10.0;
    shot.pixelsPerTick = metresPerSecond_ * kPixelsPerMetre / kTicksPerSecond;
    shot.maxDistance = weapon.range * kPixelsPerMetre;
    return shot;
}

const WeaponBehaviour& behaviourOf(WeaponClass weaponClass) {
    static const MeleeBehaviour sword(90.0, false);
    static const MeleeBehaviour axe(120.0, true);
    static const MeleeBehaviour spear(30.0, false);
    static const MeleeBehaviour whip(100.0, true);
    static const RangedBehaviour bow(16.0);
    static const RangedBehaviour thrown(10.0);
    static const RangedBehaviour staff(12.0);
    static const RangedBehaviour gun(24.0);
    switch (weaponClass) {
    case WeaponClass::Sword: return sword;
    case WeaponClass::Axe: return axe;
    case WeaponClass::Spear: return spear;
    case WeaponClass::Whip: return whip;
    case WeaponClass::Bow: return bow;
    case WeaponClass::Thrown: return thrown;
    case WeaponClass::Staff: return staff;
    case WeaponClass::Gun: return gun;
    }
    return sword;
}

int cooldownTicks(const WeaponDef& weapon) {
    return std::max(1, static_cast<int>(std::lround(kTicksPerSecond / weapon.speed)));
}

std::optional<std::size_t> stepProjectile(Projectile& projectile, const luna::engine::TileMap& map, const std::vector<Target>& targets,
                                          bool& done) {
    done = false;
    // Small steps, so a fast bullet cannot jump over a goblin or through a wall corner.
    const int steps = std::max(1, static_cast<int>(std::ceil(projectile.pixelsPerTick / 8.0)));
    const double step = projectile.pixelsPerTick / steps;
    for (int s = 0; s < steps; ++s) {
        projectile.x += projectile.dx * step;
        projectile.y += projectile.dy * step;
        projectile.travelled += step;
        // Targets are hit around their middle (feet minus half a figure).
        for (std::size_t i = 0; i < targets.size(); ++i) {
            if (targets[i].alive && std::hypot(targets[i].x - projectile.x, targets[i].y - 20.0 - projectile.y) <= kHitRadius) {
                done = true;
                return i;
            }
        }
        const int tileX = static_cast<int>(std::floor(projectile.x / kPixelsPerMetre));
        const int tileY = static_cast<int>(std::floor((projectile.y + 20.0) / kPixelsPerMetre)); // the ground under it
        if (map.isSolid(tileX, tileY) || projectile.travelled >= projectile.maxDistance) {
            done = true;
            return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace odysseus::game
