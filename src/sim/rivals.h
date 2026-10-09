#pragma once

#include "boundary.h"

#include "region.h"
#include "world_places.h"
#include "world.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace odysseus::sim {

// Levels of detail (ADR-005, US-042): what is near the player is simulated every tick, what is in the same region
// but far away once a second, and the rest far more rarely. A tick is 1/20 of a second.
enum class Tier { Active, Nearby, Distant };

inline constexpr int kActiveTiles = 30;  // within this many tiles of the player: every tick
inline constexpr int kNearbyTiles = 120; // up to here: once a second (every 20 ticks)

Tier tierForDistance(int tiles);
// How many game ticks pass between two simulation ticks of a clan in this tier: 1, 20 (1 tick/s), 200.
int ticksBetweenUpdates(Tier tier);

// A rival clan (US-042): its own small world, living elsewhere in the region.
struct RivalClan {
    std::string name;
    std::unique_ptr<World> world;
    Tile camp;       // where they live now
    Season lastSeason = Season::Spring;
    int moves = 0;   // how many times they have moved camp
};

// Two AI clans of 10 to 20 people, each at least `kMinimumDistanceTiles` from the player's camp and from each other.
class Rivals {
public:
    static constexpr int kClans = 2;
    static constexpr int kMinimumDistanceTiles = 60;
    static constexpr int kMinimumBetweenClansTiles = 40;

    // `placed` are the rival camps the owner put in the world file (US-205): the first clans live there from the start (named and sized as the entry says); the rest are
    // looked for as before, so there are always at least kClans.
    Rivals(Region& region, Tile playerCamp, std::uint64_t seed, const SimConfig& baseConfig, const std::vector<CampSite>& placed = {});

    // One game tick: each clan advances its world when its tier says so; when the season changes it looks for a better camp.
    void tick(Tile playerCamp);

    const std::vector<RivalClan>& clans() const { return clans_; }
    Tier tierOf(const RivalClan& clan, Tile playerCamp) const;
    std::uint64_t ticks() const { return ticks_; }

    static int distanceTiles(Tile a, Tile b);

private:
    void considerMoving(RivalClan& clan);
    int siteScore(Tile site);

    Region& region_;
    std::vector<RivalClan> clans_;
    std::uint64_t ticks_ = 0;
};

} // namespace odysseus::sim
