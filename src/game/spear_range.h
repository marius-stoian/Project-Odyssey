#pragma once

#include "boundary.h"

#include "game/materials.h"
#include "game/placeholder_art.h"
#include "luna/engine/tile_map.h"
#include "luna/physics/ballistics.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// Sizes of things in the world, in metres (ADR-017: one tile is one metre).
struct RangeConfig {
    luna::physics::Fixed rockHeight = luna::physics::Fixed::fromRatio(3, 2);   // boulders are 1.5 m tall
    luna::physics::Fixed handHeight = luna::physics::Fixed::fromRatio(13, 10); // a 1.5 m hero throws from 1.3 m
    luna::physics::Fixed targetHalfWidth = luna::physics::Fixed::fromRatio(3, 10);
    luna::physics::Fixed targetHeight = luna::physics::Fixed::fromRatio(8, 5); // a straw bale on a post, 1.6 m
    luna::physics::Fixed targetAimHeight = luna::physics::kFixedOne;           // aim at the bale's middle
    luna::physics::Fixed tipRadius = luna::physics::Fixed::fromRatio(2, 100);  // the spear tip, 2 cm
    luna::physics::Fixed maxAimDistance = luna::physics::Fixed::fromInt(20);
};

struct StrawTarget {
    luna::physics::Vec3 base; // centre of the post at ground level
    luna::physics::Fixed damageTaken;
    int hits = 0;
};

enum class SpearState { Flying, Stuck };
enum class HitKind { Target, Rock, Ground };

struct FlyingSpear {
    SpearKind kind;
    luna::physics::Projectile body;
    luna::physics::Vec3 previousPosition; // for smooth drawing between ticks
    luna::physics::Vec3 heading;          // the direction it points (its last velocity)
    SpearState state = SpearState::Flying;
};

// What happened when a spear struck something.
struct SpearHit {
    std::size_t spear = 0;
    HitKind kind = HitKind::Ground;
    std::size_t target = 0; // which target, when kind is Target
    luna::physics::Vec3 point;
    luna::physics::Vec3 velocity; // at impact
    luna::physics::Fixed damage;  // dealt to the target (0 for rocks and ground)
};

// The spear-throwing demo (US-029): Luna Physics proven end to end in the game. Rock tiles are
// 3D boulders, straw targets stand on posts, spears fly with gravity and drag, and every
// tick's path is swept so a spear stops at the first thing it meets.
class SpearRange {
public:
    SpearRange(const luna::engine::TileMap& map, MaterialsConfig materials, RangeConfig config = {});

    std::size_t addTarget(luna::physics::Vec3 base);

    // Throws a spear from the hero's hand. It aims at the nearest target the hero faces
    // (within 45 degrees and maxAimDistance), solving the launch angle with drag; with no
    // target in front it throws straight ahead at 15 degrees.
    void throwSpear(luna::physics::Vec3 heroFeet, Facing facing, const SpearKind& kind);

    // One simulation tick (1/20 s): every flying spear moves; hits are reported.
    std::vector<SpearHit> update();

    const std::vector<FlyingSpear>& spears() const { return spears_; }
    const std::vector<StrawTarget>& targets() const { return targets_; }
    const MaterialsConfig& materials() const { return materials_; }
    const RangeConfig& config() const { return config_; }
    const std::vector<SpearHit>& allHits() const { return allHits_; }

    // The obstacle shape of a target, for tests and drawing.
    luna::physics::Box targetShape(const StrawTarget& target) const;

private:
    std::optional<std::size_t> targetInFront(luna::physics::Vec3 hand, Facing facing) const;

    MaterialsConfig materials_;
    RangeConfig config_;
    luna::physics::Air air_;
    std::vector<luna::physics::Shape> rocks_;
    luna::physics::Shape ground_;
    std::vector<StrawTarget> targets_;
    std::vector<FlyingSpear> spears_;
    std::vector<SpearHit> allHits_;
};

const char* hitKindName(HitKind kind);

// The unit direction on the ground for a facing: East is (1, 0), South is (0, 1).
luna::physics::Vec3 facingDirection(Facing facing);

} // namespace odysseus::game
