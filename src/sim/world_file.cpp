#include "world_file.h"

#include "json_data.h"
#include "json_patch.h"
#include "save.h"
#include "world_places.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <map>

namespace odysseus::sim {

namespace {

using nlohmann::json;

struct Setting {
    const char* key;
    int RegionConfig::*member;
};

// Every setting of region.json a world may change, in the file's order. The size of the region and of a chunk are not among them (D-62 Q2).
const Setting kSettings[] = {
    {"lakeLevel", &RegionConfig::lakeLevel},           {"mountainLevel", &RegionConfig::mountainLevel}, {"riverBand", &RegionConfig::riverBand},
    {"forestMoisture", &RegionConfig::forestMoisture}, {"caveNoise", &RegionConfig::caveNoise},         {"edgeWall", &RegionConfig::edgeWall},
    {"flintPerMille", &RegionConfig::flintPerMille},   {"woodPerMille", &RegionConfig::woodPerMille},   {"berriesPerMille", &RegionConfig::berriesPerMille},
    {"herdPerMille", &RegionConfig::herdPerMille},     {"herdMinimum", &RegionConfig::herdMinimum},     {"herdMaximum", &RegionConfig::herdMaximum},
    {"berryRegrowDays", &RegionConfig::berryRegrowDays}, {"startNeedWithin", &RegionConfig::startNeedWithin},
};

struct Group {
    EditGroup group;
    const char* key;
};
const Group kGroups[] = {{EditGroup::Thing, "things"}, {EditGroup::Person, "people"}, {EditGroup::Place, "places"}, {EditGroup::Camp, "camps"}, {EditGroup::Resource, "resources"}};

const char* const kBiomeNames[] = {"Steppe", "Forest", "Water", "Mountain", "Cave"};

std::optional<Biome> biomeFromName(const std::string& name) {
    for (int i = 0; i < 5; ++i) {
        if (name == kBiomeNames[i]) return static_cast<Biome>(i);
    }
    return std::nullopt;
}

// "3,4" gives (3, 4); nothing for anything else.
std::optional<std::pair<int, int>> parseChunkKey(const std::string& text) {
    const std::size_t comma = text.find(',');
    if (comma == std::string::npos) return std::nullopt;
    int cx = 0;
    int cy = 0;
    const char* end = text.data() + text.size();
    const auto first = std::from_chars(text.data(), text.data() + comma, cx);
    const auto second = std::from_chars(text.data() + comma + 1, end, cy);
    if (first.ec != std::errc{} || first.ptr != text.data() + comma || second.ec != std::errc{} || second.ptr != end) return std::nullopt;
    return std::make_pair(cx, cy);
}

OrderedJson toJson(const WorldFile& world, const RegionConfig& base) {
    const int n = base.chunkSize;
    OrderedJson generator = OrderedJson::object();
    for (const auto& [key, value] : world.generator) generator[key] = value;

    // Tile edits per chunk, chunks in row order, tiles in tile order inside a chunk.
    std::map<std::pair<int, int>, std::vector<TileEdit>> perChunk;
    for (const TileEdit& edit : world.edits.tiles) perChunk[{edit.y / n, edit.x / n}].push_back(edit);
    OrderedJson chunks = OrderedJson::object();
    for (auto& [where, edits] : perChunk) {
        std::sort(edits.begin(), edits.end(), [](const TileEdit& a, const TileEdit& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
        OrderedJson list = OrderedJson::array();
        for (const TileEdit& edit : edits) {
            list.push_back({{"x", edit.x % n}, {"y", edit.y % n}, {"biome", kBiomeNames[static_cast<int>(edit.biome)]}});
        }
        chunks[std::format("{},{}", where.second, where.first)] = std::move(list);
    }

    OrderedJson overrides = OrderedJson::object();
    overrides["chunks"] = std::move(chunks);
    for (const Group& group : kGroups) {
        OrderedJson list = OrderedJson::array();
        for (const PlacedEdit& entry : world.edits.placed) {
            if (entry.group != group.group) continue;
            OrderedJson item = {{"id", entry.id}, {"kind", entry.kind}, {"at", OrderedJson::array({entry.x, entry.y})}};
            if (entry.removal) item["remove"] = true;
            if (entry.forced) item["forced"] = true;
            if (!entry.name.empty()) item["name"] = entry.name;
            if (!entry.npcClass.empty()) item["class"] = entry.npcClass;
            if (!entry.properties.empty()) {
                OrderedJson properties = OrderedJson::object();
                for (const auto& [key, value] : entry.properties) properties[key] = value;
                item["properties"] = std::move(properties);
            }
            list.push_back(std::move(item));
        }
        overrides[group.key] = std::move(list);
    }

    OrderedJson root = OrderedJson::object();
    root["worldVersion"] = world.version;
    root["seed"] = world.seed;
    root["generator"] = std::move(generator);
    root["overrides"] = std::move(overrides);
    return root;
}

} // namespace

RegionConfig worldConfig(const WorldFile& world, RegionConfig base) {
    for (const auto& [key, value] : world.generator) {
        const auto found = std::find_if(std::begin(kSettings), std::end(kSettings), [&key](const Setting& setting) { return key == setting.key; });
        if (found == std::end(kSettings)) throw DataError("(world)", "generator." + key, "is not a generator setting");
        base.*found->member = value;
    }
    return base;
}

std::vector<std::pair<std::string, int>> generatorDifferences(const RegionConfig& config, const RegionConfig& base) {
    std::vector<std::pair<std::string, int>> found;
    for (const Setting& setting : kSettings) {
        if (config.*setting.member != base.*setting.member) found.push_back({setting.key, config.*setting.member});
    }
    return found;
}

Region makeWorldRegion(const WorldFile& world, const RegionConfig& base) {
    Region region(world.seed, worldConfig(world, base));
    for (const TileEdit& edit : world.edits.tiles) region.setTileEdit(edit.x, edit.y, edit.biome);
    return region;
}

std::string worldText(const WorldFile& world, const RegionConfig& base) {
    return writeJsonText(toJson(world, base), 100);
}

void saveWorld(const WorldFile& world, const std::filesystem::path& file, const RegionConfig& base) {
    if (world.edits.tiles.size() + world.edits.placed.size() > kMaxWorldEntries) {
        throw DataError(file, "overrides", std::format("holds {} edits; a world file keeps at most {}", world.edits.tiles.size() + world.edits.placed.size(), kMaxWorldEntries));
    }
    writeSaveText(file, worldText(world, base));
}

WorldFile loadWorld(const std::filesystem::path& file, const RegionConfig& base) {
    const json data = readJsonFile(file);
    try {
        WorldFile world;
        world.version = data.at("worldVersion").get<int>();
        if (world.version > kWorldVersion) throw DataError(file, "worldVersion", std::format("{} is newer than this game reads ({})", world.version, kWorldVersion));
        if (world.version < 1) throw DataError(file, "worldVersion", "must be 1 or more");
        world.seed = data.at("seed").get<std::uint64_t>();
        for (const auto& [key, value] : data.at("generator").items()) {
            if (!value.is_number_integer()) throw DataError(file, "generator." + key, "must be a whole number");
            world.generator.push_back({key, value.get<int>()});
        }
        for (const auto& [key, value] : world.generator) {
            const bool known = std::any_of(std::begin(kSettings), std::end(kSettings), [&key](const Setting& setting) { return key == setting.key; });
            if (!known) throw DataError(file, "generator." + key, "is not a generator setting");
        }
        const int n = base.chunkSize;
        const int chunksPerSide = base.size / n;
        const json& overrides = data.at("overrides");
        for (const auto& [name, list] : overrides.at("chunks").items()) {
            const std::optional<std::pair<int, int>> where = parseChunkKey(name);
            const int cx = where ? where->first : -1;
            const int cy = where ? where->second : -1;
            if (!where || cx < 0 || cy < 0 || cx >= chunksPerSide || cy >= chunksPerSide) {
                throw DataError(file, "overrides.chunks." + name, "is not a chunk of the region (write \"cx,cy\")");
            }
            for (const json& item : list) {
                const int x = item.at("x").get<int>();
                const int y = item.at("y").get<int>();
                if (x < 0 || y < 0 || x >= n || y >= n) throw DataError(file, "overrides.chunks." + name, std::format("tile {},{} is outside its chunk (0 to {})", x, y, n - 1));
                const auto biome = biomeFromName(item.at("biome").get<std::string>());
                if (!biome) throw DataError(file, "overrides.chunks." + name, "biome must be Steppe, Forest, Water, Mountain or Cave");
                world.edits.tiles.push_back({cx * n + x, cy * n + y, *biome});
            }
        }
        std::sort(world.edits.tiles.begin(), world.edits.tiles.end(), [](const TileEdit& a, const TileEdit& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
        for (const Group& group : kGroups) {
            if (!overrides.contains(group.key)) continue;
            for (const json& item : overrides.at(group.key)) {
                PlacedEdit entry;
                entry.group = group.group;
                entry.id = item.at("id").get<std::string>();
                entry.kind = item.at("kind").get<std::string>();
                if (entry.id.empty() || entry.kind.empty()) throw DataError(file, std::string("overrides.") + group.key, "an entry needs an id and a kind");
                entry.x = item.at("at").at(0).get<int>();
                entry.y = item.at("at").at(1).get<int>();
                entry.removal = item.value("remove", false);
                entry.forced = item.value("forced", false);
                entry.name = item.value("name", std::string());
                entry.npcClass = item.value("class", std::string());
                if (item.contains("properties")) {
                    const std::string where = std::string("overrides.") + group.key + "." + entry.id + ".properties";
                    if (!item.at("properties").is_object()) throw DataError(file, where, "must be an object of property: value");
                    for (const auto& [key, value] : item.at("properties").items()) {
                        const std::string text = value.is_string() ? value.get<std::string>() : value.dump(); // a number written by hand is as good as a string
                        if (const std::string problem = placedPropertyProblem(entry.group, key, text); !problem.empty()) throw DataError(file, where + "." + key, problem);
                        entry.properties.push_back({key, text});
                    }
                }
                const bool repeated = std::any_of(world.edits.placed.begin(), world.edits.placed.end(), [&entry](const PlacedEdit& other) { return other.id == entry.id; });
                if (repeated) throw DataError(file, std::string("overrides.") + group.key, "the id '" + entry.id + "' is used twice");
                world.edits.placed.push_back(std::move(entry));
            }
        }
        if (world.edits.tiles.size() + world.edits.placed.size() > kMaxWorldEntries) throw DataError(file, "overrides", std::format("holds more than {} edits", kMaxWorldEntries));
        return world;
    } catch (const json::exception& error) {
        throw DataError(file, "(contents)", std::string("is damaged or incomplete: ") + error.what());
    }
}

} // namespace odysseus::sim
