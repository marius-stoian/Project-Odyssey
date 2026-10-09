#include "rivals.h"

#include "core/random.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace odysseus::sim {

Tier tierForDistance(int tiles) {
    if (tiles <= kActiveTiles) return Tier::Active;
    return tiles <= kNearbyTiles ? Tier::Nearby : Tier::Distant;
}

int ticksBetweenUpdates(Tier tier) {
    switch (tier) {
    case Tier::Active: return 1;
    case Tier::Nearby: return 20;
    default: return 200;
    }
}

int Rivals::distanceTiles(Tile a, Tile b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return static_cast<int>(std::lround(std::sqrt(dx * dx + dy * dy)));
}

Rivals::Rivals(Region& region, Tile playerCamp, std::uint64_t seed, const SimConfig& baseConfig, const std::vector<CampSite>& placed) : region_(region) {
    std::vector<CampSite> given;
    for (const CampSite& camp : placed) {
        if (!camp.player) given.push_back(camp);
    }
    core::Pcg32 random(seed, 41); // the stream "rivals" (Charter rule 6)
    const int margin = region.config().edgeWall + 6;
    const int span = region.size() - 2 * margin;
    static const char* const kNames[] = {"the River Clan", "the Stone Clan", "the Wind Clan", "the Ash Clan"};
    const int count = std::max(kClans, static_cast<int>(given.size()));
    for (int index = 0; index < count; ++index) {
        // Looking for a fair place far from everybody; after 400 tries, any walkable place far enough.
        Tile site{};
        bool found = false;
        const bool fixed = index < static_cast<int>(given.size()); // the owner put this clan's camp
        if (fixed) {
            site = given[static_cast<std::size_t>(index)].at;
            found = true;
        }
        for (int attempt = 0; attempt < 800 && !found; ++attempt) {
            const Tile candidate{margin + static_cast<int>(random.below(static_cast<std::uint32_t>(span))), margin + static_cast<int>(random.below(static_cast<std::uint32_t>(span)))};
            if (distanceTiles(candidate, playerCamp) < kMinimumDistanceTiles) continue;
            bool clear = true;
            for (const RivalClan& other : clans_) clear = clear && distanceTiles(candidate, other.camp) >= kMinimumBetweenClansTiles;
            if (!clear) continue;
            const Biome biome = region.biomeAt(candidate.x, candidate.y);
            if (biome != Biome::Steppe && biome != Biome::Forest) continue;
            if (attempt < 400 && !region.goodSite(candidate)) continue;
            site = candidate;
            found = true;
        }
        SimConfig config = baseConfig;
        config.clan.startingPeople = 10 + static_cast<int>(random.below(11)); // 10 to 20 people
        RivalClan clan;
        clan.name = kNames[index % 4];
        if (fixed) {
            if (!given[static_cast<std::size_t>(index)].clan.empty()) clan.name = given[static_cast<std::size_t>(index)].clan;
            if (given[static_cast<std::size_t>(index)].people > 0) config.clan.startingPeople = given[static_cast<std::size_t>(index)].people;
        }
        clan.world = std::make_unique<World>(seed ^ (0xA5A5A5A5ULL * static_cast<std::uint64_t>(index + 1)), config);
        clan.camp = found ? site : Tile{region.size() - margin - index * 10, region.size() - margin};
        clan.lastSeason = clan.world->date().season;
        clans_.push_back(std::move(clan));
    }
}

Tier Rivals::tierOf(const RivalClan& clan, Tile playerCamp) const { return tierForDistance(distanceTiles(clan.camp, playerCamp)); }

void Rivals::tick(Tile playerCamp) {
    ++ticks_;
    for (RivalClan& clan : clans_) {
        if (ticks_ % static_cast<std::uint64_t>(ticksBetweenUpdates(tierOf(clan, playerCamp))) != 0) continue;
        clan.world->tick();
        const Season season = clan.world->date().season;
        if (season != clan.lastSeason) {
            clan.lastSeason = season;
            considerMoving(clan);
        }
    }
}

// Food and water near a site: berries and herds count most.
int Rivals::siteScore(Tile site) {
    if (!walkable(region_.biomeAt(site.x, site.y)) || region_.biomeAt(site.x, site.y) == Biome::Cave) return -1000;
    int score = 0;
    for (const Resource& resource : region_.resourcesNear(site, 10)) {
        if (resource.takenDay >= 0) continue;
        score += resource.kind == ResourceKind::Berries ? 3 : (resource.kind == ResourceKind::Herd ? 5 : (resource.kind == ResourceKind::Wood ? 1 : 0));
    }
    for (int dy = -8; dy <= 8; dy += 2)
        for (int dx = -8; dx <= 8; dx += 2)
            if (region_.biomeAt(site.x + dx, site.y + dy) == Biome::Water) {
                score += 2;
            }
    return score;
}

// A new season: try the eight directions 14 tiles away, and move where the food is clearly better.
void Rivals::considerMoving(RivalClan& clan) {
    const int here = siteScore(clan.camp);
    Tile best = clan.camp;
    int bestScore = here + 2; // not worth moving for almost nothing
    constexpr int kDirections[8][2] = {{1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}};
    for (const auto& direction : kDirections) {
        const Tile candidate{clan.camp.x + direction[0] * 14, clan.camp.y + direction[1] * 14};
        if (candidate.x < 14 || candidate.y < 14 || candidate.x >= region_.size() - 14 || candidate.y >= region_.size() - 14) continue;
        const int score = siteScore(candidate);
        if (score > bestScore) {
            bestScore = score;
            best = candidate;
        }
    }
    if (!(best == clan.camp)) {
        clan.camp = best;
        ++clan.moves;
    }
}

} // namespace odysseus::sim
