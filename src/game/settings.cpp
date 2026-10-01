#include "game/settings.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace odysseus::game {

using nlohmann::json;

namespace {

bool allowedSize(int width, int height) {
    return (width == 1280 && height == 720) || (width == 1600 && height == 900) ||
           (width == 1920 && height == 1080) || (width == 2560 && height == 1440);
}

const char* modeName(core::WindowMode mode) {
    switch (mode) {
    case core::WindowMode::Windowed: return "Windowed";
    case core::WindowMode::Borderless: return "Borderless";
    case core::WindowMode::Exclusive: return "Exclusive";
    }
    return "Windowed";
}

const char* scalingName(core::ScalingMode scaling) {
    return scaling == core::ScalingMode::Whole ? "Whole" : "Fill";
}

int boundedInteger(const json& object, const char* key, int minimum, int maximum) {
    if (!object.contains(key) || !object.at(key).is_number_integer()) throw std::runtime_error(std::string(key) + " must be a whole number");
    const int value = object.at(key).get<int>();
    if (value < minimum || value > maximum) throw std::runtime_error(std::string(key) + " is out of range");
    return value;
}

} // namespace

void saveSettings(const GameSettings& settings, const std::filesystem::path& file) {
    if (file.has_parent_path()) std::filesystem::create_directories(file.parent_path());
    const json data{{"version", 2}, {"resolution", {{"width", settings.resolution.width}, {"height", settings.resolution.height},
                    {"mode", modeName(settings.resolution.mode)}, {"scaling", scalingName(settings.resolution.scaling)}}},
                    {"cameraZoom", settings.cameraZoom}, {"uiScale", settings.uiScale}, {"lighting", settings.lighting},
                    {"volume", settings.volume}, {"statistics", settings.statistics}};
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << data.dump(2) << '\n';
}

GameSettings loadSettings(const std::filesystem::path& file, std::string* note) {
    GameSettings defaults;
    if (!std::filesystem::exists(file)) {
        saveSettings(defaults, file);
        return defaults;
    }
    try {
        std::ifstream in(file, std::ios::binary);
        const json data = json::parse(in);
        if (!data.is_object()) throw std::runtime_error("settings must be an object");
        if (data.contains("version")) {
            if (!data.at("version").is_number_integer()) throw std::runtime_error("version must be a whole number");
            const int version = data.at("version").get<int>();
            if (version > 2) {
                if (note) *note = "settings.json has an unsupported future version; the file was kept";
                return defaults;
            }
            if (version != 2) throw std::runtime_error("version is not supported");
        }
        GameSettings read;
        if (data.contains("version")) {
            if (!data.contains("resolution") || !data.at("resolution").is_object()) throw std::runtime_error("resolution must be an object");
            const json& r = data.at("resolution");
            read.resolution.width = boundedInteger(r, "width", 1, kMaxWindowWidth);
            read.resolution.height = boundedInteger(r, "height", 1, kMaxWindowHeight);
            if (!allowedSize(read.resolution.width, read.resolution.height)) throw std::runtime_error("resolution is not a supported windowed size");
            if (!r.contains("mode") || !r.at("mode").is_string()) throw std::runtime_error("mode must be a name");
            const std::string mode = r.at("mode").get<std::string>();
            if (mode == "Windowed") read.resolution.mode = core::WindowMode::Windowed;
            else if (mode == "Borderless") read.resolution.mode = core::WindowMode::Borderless;
            else if (mode == "Exclusive") read.resolution.mode = core::WindowMode::Exclusive;
            else throw std::runtime_error("mode is not supported");
            if (!r.contains("scaling") || !r.at("scaling").is_string()) throw std::runtime_error("scaling must be a name");
            const std::string scaling = r.at("scaling").get<std::string>();
            if (scaling == "Whole") read.resolution.scaling = core::ScalingMode::Whole;
            else if (scaling == "Fill") read.resolution.scaling = core::ScalingMode::Fill;
            else throw std::runtime_error("scaling is not supported");
            read.cameraZoom = boundedInteger(data, "cameraZoom", 1, 2);
            read.uiScale = boundedInteger(data, "uiScale", 1, 2);
            if (!data.contains("lighting") || !data.at("lighting").is_string()) throw std::runtime_error("lighting must be a name");
            read.lighting = data.at("lighting").get<std::string>();
            if (read.lighting != "Low" && read.lighting != "Medium" && read.lighting != "High") throw std::runtime_error("lighting is not supported");
        } else {
            if (!data.contains("fullscreen") || !data.at("fullscreen").is_boolean()) throw std::runtime_error("fullscreen must be true or false");
            read.resolution.mode = data.at("fullscreen").get<bool>() ? core::WindowMode::Borderless : core::WindowMode::Windowed;
            const int width = boundedInteger(data, "width", 1, kMaxWindowWidth);
            const int height = boundedInteger(data, "height", 1, kMaxWindowHeight);
            if (allowedSize(width, height)) { read.resolution.width = width; read.resolution.height = height; }
        }
        read.volume = boundedInteger(data, "volume", 0, 100);
        if (data.contains("statistics")) read.statistics = boundedInteger(data, "statistics", 0, 2);
        if (!data.contains("version")) saveSettings(read, file);
        return read;
    } catch (const std::exception& error) {
        if (note) *note = std::string("settings.json had a problem (") + error.what() + "): the defaults are used and the file was rewritten";
        saveSettings(defaults, file);
        return defaults;
    }
}

} // namespace odysseus::game
