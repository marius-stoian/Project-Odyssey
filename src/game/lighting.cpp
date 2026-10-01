#include "game/lighting.h"

#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
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
        data.kinds.push_back(std::move(kind));
    }
    return data;
}

std::string lightingToText(const LightingData& data) {
    json root;
    root["version"] = 1;
    root["ambient"] = {{"color", {data.ambientRed, data.ambientGreen, data.ambientBlue}}, {"strength", data.ambientStrength}};
    root["lights"] = json::array();
    for (const LightKindDef& kind : data.kinds) {
        root["lights"].push_back({{"name", kind.name},
                                  {"color", {kind.red, kind.green, kind.blue}},
                                  {"radiusTiles", kind.radiusTiles},
                                  {"strength", kind.strength},
                                  {"height", kind.height}});
    }
    return root.dump(2);
}

} // namespace odysseus::game
