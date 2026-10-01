#include "game/settings.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace odysseus::game {

using nlohmann::json;

void saveSettings(const GameSettings& settings, const std::filesystem::path& file) {
    if (file.has_parent_path()) std::filesystem::create_directories(file.parent_path());
    const json data{{"fullscreen", settings.fullscreen}, {"width", settings.width}, {"height", settings.height}, {"volume", settings.volume}, {"statistics", settings.statistics}};
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    out << data.dump(1) << '\n';
}

GameSettings loadSettings(const std::filesystem::path& file, std::string* note) {
    GameSettings settings;
    std::string problem;
    if (std::filesystem::exists(file)) {
        try {
            std::ifstream in(file, std::ios::binary);
            std::stringstream text;
            text << in.rdbuf();
            const json data = json::parse(text.str());
            GameSettings read = settings;
            if (!data.contains("fullscreen") || !data.at("fullscreen").is_boolean()) problem = "fullscreen must be true or false";
            else read.fullscreen = data.at("fullscreen").get<bool>();
            if (problem.empty()) {
                if (!data.contains("width") || !data.at("width").is_number_integer() || data.at("width").get<int>() < kMinWindowWidth || data.at("width").get<int>() > kMaxWindowWidth) problem = "width must be a whole number from 640 to 7680";
                else read.width = data.at("width").get<int>();
            }
            if (problem.empty()) {
                if (!data.contains("height") || !data.at("height").is_number_integer() || data.at("height").get<int>() < kMinWindowHeight || data.at("height").get<int>() > kMaxWindowHeight) problem = "height must be a whole number from 360 to 4320";
                else read.height = data.at("height").get<int>();
            }
            if (problem.empty()) {
                if (!data.contains("volume") || !data.at("volume").is_number_integer() || data.at("volume").get<int>() < 0 || data.at("volume").get<int>() > 100) problem = "volume must be a whole number from 0 to 100";
                else read.volume = data.at("volume").get<int>();
            }
            if (problem.empty() && data.contains("statistics")) {
                if (!data.at("statistics").is_number_integer() || data.at("statistics").get<int>() < 0 || data.at("statistics").get<int>() > 2) problem = "statistics must be 0, 1 or 2";
                else read.statistics = data.at("statistics").get<int>();
            }
            if (problem.empty()) return read;
        } catch (const json::exception& error) {
            problem = std::string("the file is damaged: ") + error.what();
        }
        if (note != nullptr) *note = "settings.json had a problem (" + problem + "): the defaults are used and the file was rewritten";
    }
    saveSettings(settings, file);
    return settings;
}

} // namespace odysseus::game
