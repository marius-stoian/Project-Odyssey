#include "game/spear_range.h"

#include <algorithm>
#include <utility>

namespace odysseus::game {

namespace {

using luna::physics::Box;
using luna::physics::Fixed;
using luna::physics::Vec3;

// Cd x area of a thrown spear, m^2: a little more than its tip, because it wobbles.
const Fixed kSpearDragArea = Fixed::fromRatio(1, 100);
// A spear stuck in something stays visible; the oldest disappear when there are more.
constexpr std::size_t kMaxSpears = 12;

Vec3 flat(Vec3 v) {
    return {v.x, v.y, luna::physics::kFixedZero};
}

} // namespace

const char* hitKindName(HitKind kind) {
    switch (kind) {
    case HitKind::Target: return "the straw target";
    case HitKind::Rock: return "a rock";
    default: return "the ground";
    }
}

Vec3 facingDirection(Facing facing) {
    // Diagonals are normalised so every facing has length 1.
    const Fixed one = luna::physics::kFixedOne;
    const Fixed zero = luna::physics::kFixedZero;
    const Fixed diagonal = Fixed::fromRaw(3'037'000'500); // 1 / sqrt(2)
    switch (facing) {
    case Facing::South: return {zero, one, zero};
    case Facing::SouthWest: return {-diagonal, diagonal, zero};
    case Facing::West: return {-one, zero, zero};
    case Facing::NorthWest: return {-diagonal, -diagonal, zero};
    case Facing::North: return {zero, -one, zero};
    case Facing::NorthEast: return {diagonal, -diagonal, zero};
    case Facing::East: return {one, zero, zero};
    case Facing::SouthEast: return {diagonal, diagonal, zero};
    default: return {zero, one, zero};
    }
}

SpearRange::SpearRange(const luna::engine::TileMap& map, MaterialsConfig materials, RangeConfig config)
    : materials_(std::move(materials)), config_(config),
      ground_(Box{{luna::physics::kFixedZero, luna::physics::kFixedZero, -luna::physics::kFixedOne},
                  {Fixed::fromInt(map.width()), Fixed::fromInt(map.height()), luna::physics::kFixedZero}}) {
    // Every rock tile becomes a 1 x 1 m boulder standing on the ground (1 tile = 1 m).
    const int rock = static_cast<int>(TileKind::Rock);
    for (int y = 0; y < map.height(); ++y) {
        for (int x = 0; x < map.width(); ++x) {
            if (map.at(x, y) == rock) {
                rocks_.push_back(Box{{Fixed::fromInt(x), Fixed::fromInt(y), luna::physics::kFixedZero},
                                     {Fixed::fromInt(x + 1), Fixed::fromInt(y + 1), config_.rockHeight}});
            }
        }
    }
}

std::size_t SpearRange::addTarget(Vec3 base) {
    targets_.push_back({base, luna::physics::kFixedZero, 0});
    return targets_.size() - 1;
}

Box SpearRange::targetShape(const StrawTarget& target) const {
    const Vec3 half{config_.targetHalfWidth, config_.targetHalfWidth, luna::physics::kFixedZero};
    return {target.base - half, target.base + half + Vec3{luna::physics::kFixedZero, luna::physics::kFixedZero, config_.targetHeight}};
}

std::optional<std::size_t> SpearRange::targetInFront(Vec3 hand, Facing facing) const {
    const Vec3 ahead = facingDirection(facing);
    const Fixed cos45 = Fixed::fromRaw(3'037'000'500);
    std::optional<std::size_t> best;
    Fixed bestDistance;
    for (std::size_t i = 0; i < targets_.size(); ++i) {
        const Vec3 toTarget = flat(targets_[i].base - hand);
        const Fixed distance = luna::physics::length(toTarget);
        // In front: within 45 degrees of the facing, so dot >= cos 45 x distance.
        if (distance > luna::physics::kFixedZero && distance <= config_.maxAimDistance &&
            luna::physics::dot(toTarget, ahead) >= cos45 * distance && (!best || distance < bestDistance)) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

void SpearRange::throwSpear(Vec3 heroFeet, Facing facing, const SpearKind& kind) {
    const Vec3 ahead = facingDirection(facing);
    // The hand is at shoulder height, a little in front of the body.
    const Vec3 hand = heroFeet + Vec3{luna::physics::kFixedZero, luna::physics::kFixedZero, config_.handHeight} +
                      ahead * Fixed::fromRatio(3, 10);
    luna::physics::Projectile body{hand, {}, kind.mass, kSpearDragArea};
    if (const auto target = targetInFront(hand, facing)) {
        const Vec3 aimPoint = targets_[*target].base + Vec3{luna::physics::kFixedZero, luna::physics::kFixedZero, config_.targetAimHeight};
        const auto angle = luna::physics::aimLaunchAngle(body, air_, aimPoint, kind.throwSpeed);
        body.velocity = luna::physics::launchVelocity(hand, aimPoint, kind.throwSpeed, angle ? *angle : luna::physics::degrees(30));
    } else {
        body.velocity = luna::physics::launchVelocity(hand, hand + ahead, kind.throwSpeed, luna::physics::degrees(15));
    }
    if (spears_.size() >= kMaxSpears) {
        spears_.erase(spears_.begin()); // the oldest spear is picked up
    }
    spears_.push_back({kind, body, hand, luna::physics::normalized(body.velocity), SpearState::Flying});
}

std::vector<SpearHit> SpearRange::update() {
    // Everything a spear can strike this tick: boulders, the ground, then the targets.
    std::vector<luna::physics::Shape> obstacles(rocks_);
    obstacles.push_back(ground_);
    const std::size_t firstTarget = obstacles.size();
    for (const StrawTarget& target : targets_) {
        obstacles.push_back(targetShape(target));
    }

    std::vector<SpearHit> hits;
    for (std::size_t i = 0; i < spears_.size(); ++i) {
        FlyingSpear& spear = spears_[i];
        spear.previousPosition = spear.body.position;
        if (spear.state != SpearState::Flying) {
            continue;
        }
        const auto hit = luna::physics::flyTick(spear.body, air_, config_.tipRadius, obstacles);
        if (luna::physics::lengthSquared(spear.body.velocity) > luna::physics::kFixedZero) {
            spear.heading = luna::physics::normalized(spear.body.velocity);
        }
        if (!hit) {
            continue;
        }
        SpearHit report;
        report.spear = i;
        report.point = hit->hit.point;
        report.velocity = spear.body.velocity;
        if (hit->obstacle >= firstTarget) {
            report.kind = HitKind::Target;
            report.target = hit->obstacle - firstTarget;
            report.damage = impactDamage(materials_, spear.kind, spear.body.velocity);
            targets_[report.target].damageTaken += report.damage;
            ++targets_[report.target].hits;
        } else {
            report.kind = hit->obstacle == firstTarget - 1 ? HitKind::Ground : HitKind::Rock;
        }
        // It sticks where it struck.
        spear.state = SpearState::Stuck;
        spear.body.velocity = {};
        hits.push_back(report);
        allHits_.push_back(report);
    }
    return hits;
}

} // namespace odysseus::game
