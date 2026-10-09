#include "region_save.h"

#include "json_data.h"
#include "save.h"

#include <format>
#include <functional>
#include <fstream>
#include <sstream>

namespace odysseus::sim {

namespace {

using nlohmann::json;

Region parse(const std::filesystem::path& file, const RegionConfig& config, const std::function<Region()>& makeBase) {
    const json data = readJsonFile(file);
    try {
        const int version = data.at("version").get<int>();
        if (version != kRegionSaveVersion) throw DataError(file, "version", std::format("{} is not a version this game reads", version));
        if (data.at("size").get<int>() != config.size || data.at("chunkSize").get<int>() != config.chunkSize) {
            throw DataError(file, "size", "the region was saved with a different size than assets/data/sim/region.json says");
        }
        Region region = makeBase ? makeBase() : Region(data.at("seed").get<std::uint64_t>(), config); // a run on a world file starts from the land with the owner's edits laid over it (US-207)
        if (region.seed() != data.at("seed").get<std::uint64_t>()) throw DataError(file, "seed", "the region was saved for another seed than the world it is loaded onto");
        for (const json& chunk : data.at("chunks")) {
            if (chunk.contains("amounts")) {
                for (const json& left : chunk.at("amounts")) region.restoreAmount(left.at(0).get<int>(), left.at(1).get<int>(), left.at(2).get<int>());
            }
            for (const json& taken : chunk.at("taken")) {
                region.restoreTaken(taken.at(0).get<int>(), taken.at(1).get<int>(), taken.at(2).get<std::int64_t>());
            }
        }
        return region;
    } catch (const json::exception& error) {
        throw DataError(file, "(contents)", std::string("is damaged: ") + error.what());
    }
}

} // namespace

std::size_t savedChunkCount(const Region& region) { return region.changedChunks().size(); }

void saveRegion(const Region& region, const std::filesystem::path& file) {
    json chunks = json::array();
    for (const Chunk* chunk : region.changedChunks()) {
        json taken = json::array();
        for (const Resource& resource : chunk->resources) {
            if (resource.takenDay >= 0) taken.push_back({resource.x, resource.y, resource.takenDay});
        }
        json amounts = json::array(); // what is left of a flint or wood spot of several (US-205)
        for (const Resource& resource : chunk->resources) {
            if (resource.kind != ResourceKind::Herd && resource.kind != ResourceKind::Berries && resource.amount > 1) amounts.push_back({resource.x, resource.y, resource.amount});
        }
        chunks.push_back({{"cx", chunk->cx}, {"cy", chunk->cy}, {"taken", taken}, {"amounts", amounts}});
    }
    const json data{{"version", kRegionSaveVersion},
                    {"seed", region.seed()},
                    {"size", region.config().size},
                    {"chunkSize", region.config().chunkSize},
                    {"chunks", chunks}};
    writeSaveText(file, data.dump(1));
}

LoadedRegion loadRegion(const std::filesystem::path& file, const RegionConfig& config, const std::function<Region()>& makeBase) {
    std::vector<std::string> notes;
    std::vector<std::filesystem::path> candidates{file};
    for (int number = 1; number <= kSaveBackups; ++number) candidates.push_back(backupPath(file, number));
    std::string firstProblem;
    for (const auto& candidate : candidates) {
        if (!std::filesystem::exists(candidate)) continue;
        try {
            Region region = parse(candidate, config, makeBase);
            if (candidate != file) notes.push_back(std::format("{} was damaged; the newest intact backup, {}, was loaded instead", file.filename().string(), candidate.filename().string()));
            return {std::move(region), candidate, notes};
        } catch (const DataError& error) {
            notes.push_back(std::string("skipped ") + candidate.filename().string() + ": " + error.what());
            if (firstProblem.empty()) firstProblem = error.what();
        }
    }
    throw DataError(file, "(file)", firstProblem.empty() ? "no save exists there" : "no save or backup could be read: " + firstProblem);
}

} // namespace odysseus::sim
