#pragma once

#include "boundary.h"

#include "calendar.h"

#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim {

// The region (US-040, US-041, US-043, D-31): a square piece of land made from a seed, tile by tile and chunk by chunk,
// with whole numbers only (no floating point, Charter rule 6), so the same seed gives the same land on every machine.
// Every tile and every resource is a pure function of the seed and its place, so a chunk can be made whenever
// somebody walks near it and always comes out the same; a saved game then needs only the seed and what changed.

enum class Biome : std::uint8_t { Steppe, Forest, Water, Mountain, Cave };
// People walk on steppe, in forest and into cave mouths; water and mountains stop them.
bool walkable(Biome biome);

enum class ResourceKind : std::uint8_t { Flint, Wood, Berries, Herd };

struct Tile {
    int x = 0;
    int y = 0;
    friend bool operator==(const Tile&, const Tile&) = default;
};

struct Resource {
    ResourceKind kind = ResourceKind::Flint;
    int x = 0;
    int y = 0;
    int amount = 1;              // a herd: how many animals
    std::int64_t takenDay = -1;  // the game day it was harvested, -1 = never
};

// From assets/data/sim/region.json: the land and what lies on it.
struct RegionConfig {
    int size = 256;                // tiles on each side (a multiple of chunkSize)
    int chunkSize = 32;
    int lakeLevel = 140;           // elevation (0..1023) below this is water
    int mountainLevel = 800;       // above this, mountains
    int riverBand = 14;            // how wide the ridge of the river noise is, thousandths of the noise range
    int forestMoisture = 560;      // wetter than this, forest (at middle heights)
    int caveNoise = 900;           // a mountain tile whose cave noise is above this is a cave mouth
    int edgeWall = 8;              // the outer tiles rise into mountains: a closed world
    int flintPerMille = 25;        // of the tiles near water or caves
    int woodPerMille = 160;        // of forest tiles
    int berriesPerMille = 220;     // of forest-edge tiles
    int herdPerMille = 3;          // of steppe tiles
    int herdMinimum = 5;
    int herdMaximum = 12;
    int berryRegrowDays = 14;      // a harvested berry bush bears again after this many days, in summer or autumn
    int startNeedWithin = 20;      // tiles: the start has water and food this close
};

RegionConfig loadRegionConfig(const std::filesystem::path& file);
// Every rule loadRegionConfig enforces, for a configuration made in the Editor (US-201): one plain sentence for each setting out of its range, none when it is fine.
// A test keeps the two in step.
std::vector<std::string> regionConfigProblems(const RegionConfig& config);

struct Chunk {
    int cx = 0;
    int cy = 0;
    std::vector<Biome> biomes;       // chunkSize x chunkSize, row after row
    std::vector<Resource> resources; // in tile order
    bool changed = false;            // something in it was harvested since the region was made
};

class Region {
public:
    Region(std::uint64_t seed, RegionConfig config);

    std::uint64_t seed() const { return seed_; }
    const RegionConfig& config() const { return config_; }
    int size() const { return config_.size; }
    int chunksPerSide() const { return config_.size / config_.chunkSize; }

    // The land at a tile; outside the region is mountain. Pure: it never loads a chunk.
    Biome biomeAt(int x, int y) const;
    // The resource that lies on a tile, if any, in its untouched state. Pure.
    std::optional<Resource> resourceAt(int x, int y) const;

    // Chunks are made when first asked for and kept. `chunk` throws std::out_of_range outside the region.
    const Chunk& chunk(int cx, int cy);
    std::size_t loadedChunks() const { return chunks_.size(); }
    // The chunk a tile is in, as chunk coordinates.
    Tile chunkOf(int x, int y) const { return {x / config_.chunkSize, y / config_.chunkSize}; }

    // Where the player's clan starts: walkable, water and food within startNeedWithin tiles, all of it reachable on foot.
    Tile start() const { return start_; }
    // The same test for any camp: water and food within reach and a walk to both (rival clans look for such places too).
    bool goodSite(Tile tile) { return startIsGood(tile); }

    // Resources within `radius` tiles of a tile (loads the chunks it needs), nearest first.
    std::vector<Resource> resourcesNear(Tile around, int radius);

    // Is the resource there now? A harvested berry bush bears again after berryRegrowDays, in summer or autumn;
    // wood, flint and herds do not come back.
    bool available(const Resource& resource, const Date& today) const;
    // Takes what lies at a tile on `date` (game day number); false when nothing is there to take. Marks the chunk changed.
    bool harvest(int x, int y, const Date& today);

    // The chunks that changed, in chunk order: what a save must keep besides the seed.
    std::vector<const Chunk*> changedChunks() const;
    // Restores a harvest when loading (no date check).
    void restoreTaken(int x, int y, std::int64_t takenDay);

    // One number for the whole region as generated (every chunk), for tests that compare two regions tile for tile.
    std::uint64_t fingerprint();

private:
    static std::uint64_t key(int cx, int cy) { return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cx)) << 32) | static_cast<std::uint32_t>(cy); }
    std::uint32_t hash(int x, int y, std::uint32_t salt) const;
    int noise(int x, int y, int cell, std::uint32_t salt) const;
    int elevationAt(int x, int y) const;
    int moistureAt(int x, int y) const;
    Biome generatedBiome(int x, int y) const;
    Tile findStart();
    bool startIsGood(Tile tile);
    void carveStartArea(Tile centre);

    std::uint64_t seed_;
    RegionConfig config_;
    std::map<std::uint64_t, Chunk> chunks_;
    std::map<std::uint64_t, Biome> overrides_;     // the start area carved when the land offered no good start
    std::vector<Resource> extraResources_;         // food and flint put near a carved start
    Tile start_;
};

} // namespace odysseus::sim
