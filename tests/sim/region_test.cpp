// M4: US-040 regions, US-041 resources, US-042 rival clans, US-043 saving only the changes.
#include "sim/region.h"
#include "sim/region_save.h"
#include "sim/rivals.h"
#include "sim/save.h"

#include <doctest/doctest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <queue>
#include <set>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

sim::RegionConfig config() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }

sim::Date dateOf(std::int64_t day, sim::Season season) {
    sim::Date date;
    date.day = day;
    date.season = season;
    return date;
}

// Independent of the generator's own start check: can people walk from the start to water and to food within 20 tiles?
bool playableStart(sim::Region& region) {
    const sim::Tile start = region.start();
    const int reach = 20;
    if (!sim::walkable(region.biomeAt(start.x, start.y))) return false;
    std::set<std::pair<int, int>> seen{{start.x, start.y}};
    std::queue<sim::Tile> queue;
    queue.push(start);
    bool water = false;
    bool food = false;
    while (!queue.empty()) {
        const sim::Tile at = queue.front();
        queue.pop();
        if ((at.x - start.x) * (at.x - start.x) + (at.y - start.y) * (at.y - start.y) > reach * reach) continue;
        if (const auto resource = region.resourceAt(at.x, at.y)) {
            if (resource->kind == sim::ResourceKind::Berries || resource->kind == sim::ResourceKind::Herd) food = true;
        }
        for (const sim::Tile next : {sim::Tile{at.x + 1, at.y}, sim::Tile{at.x - 1, at.y}, sim::Tile{at.x, at.y + 1}, sim::Tile{at.x, at.y - 1}}) {
            const sim::Biome biome = region.biomeAt(next.x, next.y);
            if (biome == sim::Biome::Water) {
                water = true;
                continue;
            }
            if (sim::walkable(biome) && seen.insert({next.x, next.y}).second) queue.push(next);
        }
    }
    return water && food;
}

} // namespace

TEST_CASE("US-040 Variety") {
    sim::Region a(1, config());
    sim::Region b(2, config());
    CHECK(a.fingerprint() != b.fingerprint());
    // Different layout: many tiles differ in biome, and each biome takes a different share.
    int differing = 0;
    int counts[2][5] = {};
    for (int y = 0; y < a.size(); ++y) {
        for (int x = 0; x < a.size(); ++x) {
            const sim::Biome first = a.biomeAt(x, y);
            const sim::Biome second = b.biomeAt(x, y);
            differing += first != second ? 1 : 0;
            ++counts[0][static_cast<int>(first)];
            ++counts[1][static_cast<int>(second)];
        }
    }
    CHECK(differing > a.size() * a.size() / 5);
    // Steppe, forest, water (rivers and lakes), mountains and caves all exist in the region.
    for (int biome = 0; biome < 5; ++biome) {
        CAPTURE(biome);
        CHECK(counts[0][biome] > 0);
        CHECK(counts[1][biome] > 0);
    }
    CHECK(counts[0][static_cast<int>(sim::Biome::Steppe)] != counts[1][static_cast<int>(sim::Biome::Steppe)]);
}

TEST_CASE("US-040 Repeatable") {
    sim::Region first(1, config());
    sim::Region second(1, config());
    CHECK(first.fingerprint() == second.fingerprint());
    for (int y = 0; y < first.size(); ++y)
        for (int x = 0; x < first.size(); ++x) CHECK(first.biomeAt(x, y) == second.biomeAt(x, y));
    CHECK(first.start() == second.start());
    // A chunk made later, on its own, is the same as one made in a full pass.
    sim::Region lazy(1, config());
    const sim::Chunk& alone = lazy.chunk(3, 4);
    const sim::Chunk& inPass = first.chunk(3, 4);
    CHECK(alone.biomes == inPass.biomes);
    CHECK(alone.resources.size() == inPass.resources.size());
}

TEST_CASE("US-040 Playable") {
#ifdef NDEBUG
    const std::uint64_t seeds = 30;
#else
    const std::uint64_t seeds = 8; // Debug builds with their checks are several times slower; Release runs all 30
#endif
    for (std::uint64_t seed = 1; seed <= seeds; ++seed) {
        CAPTURE(seed);
        const auto began = std::chrono::steady_clock::now();
        sim::Region region(seed, config());
        region.fingerprint(); // every chunk made
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
#ifdef NDEBUG
        CHECK(seconds < 10.0);
#else
        CHECK(seconds < 60.0); // Debug builds with their checks are several times slower
#endif
        CHECK(playableStart(region));
    }
}

TEST_CASE("US-041 Biome rules") {
    sim::Region region(1, config());
    int flint = 0;
    int berries = 0;
    int herds = 0;
    int wood = 0;
    const auto near = [&](int x, int y, sim::Biome wanted, int reach) {
        for (int dy = -reach; dy <= reach; ++dy)
            for (int dx = -reach; dx <= reach; ++dx)
                if (region.biomeAt(x + dx, y + dy) == wanted) return true;
        return false;
    };
    for (int cy = 0; cy < region.chunksPerSide(); ++cy) {
        for (int cx = 0; cx < region.chunksPerSide(); ++cx) {
            for (const sim::Resource& resource : region.chunk(cx, cy).resources) {
                const sim::Biome here = region.biomeAt(resource.x, resource.y);
                switch (resource.kind) {
                case sim::ResourceKind::Flint:
                    ++flint;
                    CHECK((near(resource.x, resource.y, sim::Biome::Water, 2) || near(resource.x, resource.y, sim::Biome::Cave, 2))); // near rivers and caves
                    break;
                case sim::ResourceKind::Berries:
                    ++berries;
                    CHECK(here == sim::Biome::Forest);
                    CHECK(near(resource.x, resource.y, sim::Biome::Steppe, 1)); // on the forest's edge
                    break;
                case sim::ResourceKind::Herd:
                    ++herds;
                    CHECK(here == sim::Biome::Steppe);
                    CHECK(resource.amount >= 5);
                    CHECK(resource.amount <= 12);
                    break;
                case sim::ResourceKind::Wood:
                    ++wood;
                    CHECK(here == sim::Biome::Forest);
                    break;
                }
            }
        }
    }
    CHECK(flint > 10);
    CHECK(berries > 10);
    CHECK(herds > 10);
    CHECK(wood > 100);
}

TEST_CASE("US-041 Regrowth") {
    sim::Region region(1, config());
    sim::Resource berries;
    bool found = false;
    for (int cy = 0; cy < region.chunksPerSide() && !found; ++cy)
        for (int cx = 0; cx < region.chunksPerSide() && !found; ++cx)
            for (const sim::Resource& resource : region.chunk(cx, cy).resources)
                if (resource.kind == sim::ResourceKind::Berries) {
                    berries = resource;
                    found = true;
                    break;
                }
    REQUIRE(found);
    // Harvested in summer on day 100: empty, then bearing again after 14 days while it is summer or autumn.
    CHECK(region.harvest(berries.x, berries.y, dateOf(100, sim::Season::Summer)));
    CHECK_FALSE(region.harvest(berries.x, berries.y, dateOf(100, sim::Season::Summer))); // nothing left to take
    const sim::Resource taken = region.resourcesNear({berries.x, berries.y}, 0).front();
    CHECK_FALSE(region.available(taken, dateOf(110, sim::Season::Summer)));
    CHECK_FALSE(region.available(taken, dateOf(115, sim::Season::Winter))); // the right season only
    CHECK(region.available(taken, dateOf(114, sim::Season::Summer)));
    CHECK(region.available(taken, dateOf(120, sim::Season::Autumn)));
    CHECK(region.harvest(berries.x, berries.y, dateOf(120, sim::Season::Autumn))); // it bears again
    // Wood never comes back.
    sim::Resource tree;
    found = false;
    for (int cy = 0; cy < region.chunksPerSide() && !found; ++cy)
        for (int cx = 0; cx < region.chunksPerSide() && !found; ++cx)
            for (const sim::Resource& resource : region.chunk(cx, cy).resources)
                if (resource.kind == sim::ResourceKind::Wood) {
                    tree = resource;
                    found = true;
                    break;
                }
    REQUIRE(found);
    CHECK(region.harvest(tree.x, tree.y, dateOf(10, sim::Season::Spring)));
    CHECK_FALSE(region.harvest(tree.x, tree.y, dateOf(500, sim::Season::Summer)));
}

TEST_CASE("US-042 Spawn") {
    const sim::SimConfig simConfig = sim::loadSimConfig(ODYSSEUS_DATA_DIR);
    for (std::uint64_t seed = 1; seed <= 8; ++seed) {
        CAPTURE(seed);
        sim::Region region(seed, config());
        const sim::Rivals rivals(region, region.start(), seed, simConfig);
        REQUIRE(rivals.clans().size() == 2);
        for (const sim::RivalClan& clan : rivals.clans()) {
            CHECK(clan.world->population() >= 10);
            CHECK(clan.world->population() <= 20);
            CHECK(sim::Rivals::distanceTiles(clan.camp, region.start()) >= 60);
        }
        CHECK(sim::Rivals::distanceTiles(rivals.clans()[0].camp, rivals.clans()[1].camp) >= 40);
    }
}

TEST_CASE("US-042 LOD") {
    CHECK(sim::tierForDistance(10) == sim::Tier::Active);
    CHECK(sim::tierForDistance(60) == sim::Tier::Nearby);
    CHECK(sim::tierForDistance(120) == sim::Tier::Nearby);
    CHECK(sim::tierForDistance(200) == sim::Tier::Distant);
    CHECK(sim::ticksBetweenUpdates(sim::Tier::Nearby) == 20); // 1 tick a second at 20 ticks a second
    // A rival 60+ tiles away is simulated at that rate: after 2000 game ticks its world has run 100 ticks.
    sim::Region region(3, config());
    sim::Rivals rivals(region, region.start(), 3, sim::loadSimConfig(ODYSSEUS_DATA_DIR));
    const sim::Tile player = region.start();
    for (const sim::RivalClan& clan : rivals.clans()) {
        const sim::Tier tier = rivals.tierOf(clan, player);
        CHECK(tier != sim::Tier::Active);
    }
    for (int i = 0; i < 2000; ++i) rivals.tick(player);
    for (const sim::RivalClan& clan : rivals.clans()) {
        const int expected = 2000 / sim::ticksBetweenUpdates(rivals.tierOf(clan, player));
        CHECK(clan.world->ticks() >= static_cast<std::uint64_t>(expected) - 1);
        CHECK(clan.world->ticks() <= static_cast<std::uint64_t>(expected) + 1);
    }
}

TEST_CASE("US-042 Autonomy") {
    const sim::SimConfig simConfig = sim::loadSimConfig(ODYSSEUS_DATA_DIR);
    sim::Region region(4, config());
    sim::Rivals rivals(region, region.start(), 4, simConfig);
    std::vector<int> before;
    for (const sim::RivalClan& clan : rivals.clans()) before.push_back(clan.world->population());
    const sim::Tile player = region.start();
    const std::uint64_t yearTicks = rivals.clans()[0].world->calendar().ticksPerYear();
    // One game year of each rival's own time at the Nearby rate (one simulation tick per 20 game ticks).
    for (std::uint64_t i = 0; i < yearTicks * 20 + 40; ++i) rivals.tick(player);
    bool changed = false;
    for (std::size_t i = 0; i < rivals.clans().size(); ++i) {
        const sim::RivalClan& clan = rivals.clans()[i];
        CHECK(clan.world->date().year >= 2); // a year has passed for them
        changed = changed || clan.world->population() != before[i] || clan.moves > 0;
    }
    CHECK(changed); // they grew, shrank or moved camp on their own
}

TEST_CASE("US-043 Delta save") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-m4-region";
    fs::remove_all(folder);
    sim::Region region(5, config());
    // Change three chunks: take a resource in each of three different chunks.
    std::set<std::pair<int, int>> touched;
    for (int cy = 0; cy < region.chunksPerSide() && touched.size() < 3; ++cy) {
        for (int cx = 0; cx < region.chunksPerSide() && touched.size() < 3; ++cx) {
            const sim::Chunk& chunk = region.chunk(cx, cy);
            if (!chunk.resources.empty() && chunk.resources.front().kind != sim::ResourceKind::Herd) {
                const sim::Resource resource = chunk.resources.front();
                if (region.harvest(resource.x, resource.y, dateOf(10, sim::Season::Summer))) touched.insert({cx, cy});
            }
        }
    }
    REQUIRE(touched.size() == 3);
    CHECK(sim::savedChunkCount(region) == 3);
    sim::saveRegion(region, folder / "region.json");
    // Only the seed and the three chunks are stored: a few hundred bytes, however large the region is.
    CHECK(fs::file_size(folder / "region.json") < 2000);
    sim::LoadedRegion loaded = sim::loadRegion(folder / "region.json", config());
    CHECK(loaded.region.seed() == 5);
    CHECK(sim::savedChunkCount(loaded.region) == 3);
    CHECK(loaded.region.fingerprint() == region.fingerprint());
    for (const auto& [cx, cy] : touched) {
        const sim::Resource first = region.chunk(cx, cy).resources.front();
        CHECK(loaded.region.chunk(cx, cy).resources.front().takenDay == first.takenDay);
    }
    // Four saves: the latest and three backups exist.
    for (int i = 0; i < 4; ++i) sim::saveRegion(region, folder / "region.json");
    for (int number = 1; number <= 3; ++number) CHECK(fs::exists(sim::backupPath(folder / "region.json", number)));
    // A damaged latest save: the newest intact backup loads, and the notes say so.
    std::ofstream(folder / "region.json", std::ios::trunc) << "{ this is not json";
    sim::LoadedRegion fallback = sim::loadRegion(folder / "region.json", config());
    CHECK(fallback.loadedFrom == sim::backupPath(folder / "region.json", 1));
    CHECK_FALSE(fallback.notes.empty());
    CHECK(fallback.region.seed() == 5);
}
