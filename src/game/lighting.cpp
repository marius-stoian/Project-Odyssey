#include "game/lighting.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <set>

namespace odysseus::game {

using nlohmann::json;

namespace {

int colourPart(const json& list, const std::filesystem::path& file, const std::string& where, std::size_t index) {
    const json& part = list.at(index);
    if (!part.is_number_integer() || part.get<int>() < 0 || part.get<int>() > 255) {
        throw sim::DataError(file, std::format("{}[{}]", where, index), "must be a whole number from 0 to 255");
    }
    return part.get<int>();
}

void readColour(const json& object, const std::filesystem::path& file, const std::string& where, int& red, int& green, int& blue) {
    if (!object.contains("color") || !object.at("color").is_array() || object.at("color").size() != 3) {
        throw sim::DataError(file, where + ".color", "must be three numbers: [red, green, blue], each 0 to 255");
    }
    red = colourPart(object.at("color"), file, where + ".color", 0);
    green = colourPart(object.at("color"), file, where + ".color", 1);
    blue = colourPart(object.at("color"), file, where + ".color", 2);
}

double numberIn(const json& object, const std::filesystem::path& file, const std::string& where, const std::string& field, double low, double high) {
    if (!object.contains(field) || !object.at(field).is_number() || object.at(field).get<double>() < low || object.at(field).get<double>() > high) {
        throw sim::DataError(file, where + "." + field, std::format("must be a number from {} to {}", low, high));
    }
    return object.at(field).get<double>();
}

} // namespace

const LightKindDef* LightingData::kind(const std::string& name) const {
    const auto it = std::find_if(kinds.begin(), kinds.end(), [&](const LightKindDef& def) { return def.name == name; });
    return it == kinds.end() ? nullptr : &*it;
}

luna::engine::LightFrame LightingData::ambientFrame() const {
    luna::engine::LightFrame frame;
    frame.ambientR = static_cast<float>(ambientRed / 255.0 * ambientStrength);
    frame.ambientG = static_cast<float>(ambientGreen / 255.0 * ambientStrength);
    frame.ambientB = static_cast<float>(ambientBlue / 255.0 * ambientStrength);
    return frame;
}

double flickerNoise(std::uint64_t seed, double seconds, double period) {
    const auto value = [seed](std::int64_t step) {
        std::uint64_t x = seed * 0x9E3779B97F4A7C15ULL + static_cast<std::uint64_t>(step) * 0xBF58476D1CE4E5B9ULL;
        x ^= x >> 30; x *= 0xBF58476D1CE4E5B9ULL;
        x ^= x >> 27; x *= 0x94D049BB133111EBULL;
        x ^= x >> 31;
        return static_cast<double>(x >> 11) / static_cast<double>(1ULL << 53); // 0..1
    };
    const double position = seconds / period;
    const auto step = static_cast<std::int64_t>(std::floor(position));
    double t = position - static_cast<double>(step);
    t = t * t * (3.0 - 2.0 * t); // smoothstep: no kinks where the values meet
    return value(step) + (value(step + 1) - value(step)) * t;
}

LightingData loadLighting(const std::filesystem::path& file) {
    LightingData data;
    if (!std::filesystem::exists(file)) {
        return data;
    }
    const json root = sim::readJsonFile(file);
    if (!root.is_object() || !root.contains("version") || !root.at("version").is_number_integer() || root.at("version").get<int>() != 1) {
        throw sim::DataError(file, "version", "must be 1");
    }
    if (!root.contains("ambient") || !root.at("ambient").is_object()) {
        throw sim::DataError(file, "ambient", "must be an object with color and strength");
    }
    readColour(root.at("ambient"), file, "ambient", data.ambientRed, data.ambientGreen, data.ambientBlue);
    data.ambientStrength = numberIn(root.at("ambient"), file, "ambient", "strength", 0.0, 2.0);
    if (!root.contains("lights") || !root.at("lights").is_array()) {
        throw sim::DataError(file, "lights", "must be a list");
    }
    std::set<std::string> names;
    for (std::size_t i = 0; i < root.at("lights").size(); ++i) {
        const json& entry = root.at("lights").at(i);
        const std::string where = std::format("lights[{}]", i);
        if (!entry.is_object() || !entry.contains("name") || !entry.at("name").is_string() || entry.at("name").get<std::string>().empty()) {
            throw sim::DataError(file, where + ".name", "must be a name in quotes");
        }
        LightKindDef kind;
        kind.name = entry.at("name").get<std::string>();
        if (!names.insert(kind.name).second) {
            throw sim::DataError(file, where + ".name", "\"" + kind.name + "\" is listed twice");
        }
        readColour(entry, file, where, kind.red, kind.green, kind.blue);
        kind.radiusTiles = numberIn(entry, file, where, "radiusTiles", 0.5, 30.0);
        kind.strength = numberIn(entry, file, where, "strength", 0.0, 4.0);
        kind.height = numberIn(entry, file, where, "height", 1.0, 200.0);
        kind.flicker = root.at("lights").at(i).contains("flicker") ? numberIn(entry, file, where, "flicker", 0.0, 1.0) : 0.0;
        if (entry.contains("shadows")) {
            if (!entry.at("shadows").is_boolean()) throw sim::DataError(file, where + ".shadows", "must be true or false");
            kind.shadows = entry.at("shadows").get<bool>();
        }
        data.kinds.push_back(std::move(kind));
    }
    if (root.contains("fireShadows")) {
        const json& fire = root.at("fireShadows");
        if (!fire.is_object()) throw sim::DataError(file, "fireShadows", "must be an object with maxPerObject and strength");
        const double perObject = numberIn(fire, file, "fireShadows", "maxPerObject", 0.0, 8.0);
        if (perObject != std::floor(perObject)) throw sim::DataError(file, "fireShadows.maxPerObject", "must be a whole number from 0 to 8");
        data.shadowLightsPerObject = static_cast<int>(perObject);
        data.fireShadowStrength = numberIn(fire, file, "fireShadows", "strength", 0.0, 1.0);
    }
    if (root.contains("clanTorch")) {
        if (!root.at("clanTorch").is_string()) throw sim::DataError(file, "clanTorch", "must be the name of a kind of light");
        data.clanTorch = root.at("clanTorch").get<std::string>();
        if (!data.clanTorch.empty() && data.kind(data.clanTorch) == nullptr) {
            throw sim::DataError(file, "clanTorch", "\"" + data.clanTorch + "\" is not a kind of light in lights");
        }
    }
    return data;
}

std::string lightingToText(const LightingData& data) {
    json root;
    root["version"] = 1;
    root["ambient"] = {{"color", {data.ambientRed, data.ambientGreen, data.ambientBlue}}, {"strength", data.ambientStrength}};
    if (!data.clanTorch.empty()) root["clanTorch"] = data.clanTorch;
    root["fireShadows"] = {{"maxPerObject", data.shadowLightsPerObject}, {"strength", data.fireShadowStrength}};
    root["lights"] = json::array();
    for (const LightKindDef& kind : data.kinds) {
        root["lights"].push_back({{"name", kind.name},
                                  {"color", {kind.red, kind.green, kind.blue}},
                                  {"radiusTiles", kind.radiusTiles},
                                  {"strength", kind.strength},
                                  {"height", kind.height},
                                  {"flicker", kind.flicker},
                                  {"shadows", kind.shadows}});
    }
    return root.dump(2);
}

} // namespace odysseus::game
