#include "game/settings.h"

#include "core/presentation.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace fs = std::filesystem;
namespace core = odysseus::core;
namespace game = odysseus::game;
using nlohmann::json;

namespace {

fs::path settingsFile(const char* name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us231-settings";
    fs::create_directories(folder);
    return folder / name;
}

void writeJson(const fs::path& file, const json& value) {
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    REQUIRE(out.good());
    out << value.dump(2) << '\n';
}

json readJson(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    REQUIRE(in.good());
    return json::parse(in);
}

} // namespace

TEST_CASE("US-231 First start") {
    const fs::path file = settingsFile("first-start.json");
    fs::remove(file);
    std::string note;
    const game::GameSettings settings = game::loadSettings(file, &note);
    CHECK(settings.resolution.mode == core::WindowMode::Windowed);
    CHECK(settings.resolution.width == 1280);
    CHECK(settings.resolution.height == 720);
    CHECK(settings.resolution.scaling == core::ScalingMode::Whole);
    CHECK(settings.cameraZoom == 2);
    CHECK(settings.uiScale == 1);
    CHECK(settings.lighting == "Medium");
    CHECK(fs::exists(file));
    CHECK(readJson(file).at("version") == 2);
}

TEST_CASE("US-231 Modes") {
    const fs::path file = settingsFile("round-trip.json");
    for (const auto [width, height] : {std::pair{1280, 720}, std::pair{1600, 900}, std::pair{1920, 1080}, std::pair{2560, 1440}}) {
        for (const core::WindowMode mode : {core::WindowMode::Windowed, core::WindowMode::Borderless, core::WindowMode::Exclusive}) {
            game::GameSettings chosen;
            chosen.resolution.width = width;
            chosen.resolution.height = height;
            chosen.resolution.mode = mode;
            chosen.resolution.scaling = core::ScalingMode::Fill;
            chosen.cameraZoom = 1;
            chosen.uiScale = 2;
            chosen.lighting = "Low";
            chosen.volume = 37;
            chosen.statistics = 2;
            game::saveSettings(chosen, file);
            const json saved = readJson(file);
            CHECK(saved.at("version") == 2);
            CHECK(saved.at("resolution").at("width") == width);
            CHECK(saved.at("resolution").at("height") == height);
            CHECK(saved.at("resolution").at("mode").is_string());
            CHECK(saved.at("resolution").at("scaling") == "Fill");
            CHECK(saved.at("cameraZoom") == 1);
            CHECK(saved.at("uiScale") == 2);
            CHECK(saved.at("lighting") == "Low");
            CHECK(saved.at("volume") == 37);
            CHECK(saved.at("statistics") == 2);
            CHECK(game::loadSettings(file) == chosen);
        }
    }
}

TEST_CASE("US-231 Modes migrate version 1 settings") {
    const fs::path file = settingsFile("migration.json");
    for (const bool fullscreen : {false, true}) {
        writeJson(file, {{"fullscreen", fullscreen}, {"width", 1600}, {"height", 900}, {"volume", 23}, {"statistics", 1}});
        const game::GameSettings migrated = game::loadSettings(file);
        CHECK(migrated.resolution.mode == (fullscreen ? core::WindowMode::Borderless : core::WindowMode::Windowed));
        CHECK(migrated.resolution.width == 1600);
        CHECK(migrated.resolution.height == 900);
        CHECK(migrated.resolution.scaling == core::ScalingMode::Whole);
        CHECK(migrated.cameraZoom == 2);
        CHECK(migrated.uiScale == 1);
        CHECK(migrated.lighting == "Medium");
        CHECK(migrated.volume == 23);
        CHECK(migrated.statistics == 1);
        CHECK(readJson(file).at("version") == 2);
    }
    SUBCASE("an old unsupported size falls back to the first-start window size") {
        writeJson(file, {{"fullscreen", false}, {"width", 1366}, {"height", 768}, {"volume", 23}, {"statistics", 1}});
        const game::GameSettings migrated = game::loadSettings(file);
        CHECK(migrated.resolution.width == 1280);
        CHECK(migrated.resolution.height == 720);
        CHECK(migrated.volume == 23);
        CHECK(migrated.statistics == 1);
    }
}

TEST_CASE("US-231 Invalid settings") {
    const fs::path file = settingsFile("invalid.json");
    writeJson(file, {{"version", 2}, {"resolution", {{"width", "wide"}, {"height", 720}, {"mode", "Windowed"}, {"scaling", "Whole"}}},
                     {"cameraZoom", 2}, {"uiScale", 1}, {"lighting", "Medium"}, {"volume", 80}, {"statistics", 0}});
    std::string note;
    const game::GameSettings repaired = game::loadSettings(file, &note);
    CHECK_FALSE(note.empty());
    CHECK(repaired == game::GameSettings{});
    CHECK(readJson(file).at("version") == 2);

    SUBCASE("a future version is reported and not overwritten as version 2") {
        const json future{{"version", 99}, {"resolution", {{"width", 2560}, {"height", 1440}, {"mode", "Exclusive"}, {"scaling", "Fill"}}}};
        writeJson(file, future);
        note.clear();
        game::loadSettings(file, &note);
        CHECK_FALSE(note.empty());
        CHECK(readJson(file) == future);
    }
}

TEST_CASE("US-183 The Markers switch is on by default and survives a save") {
    const fs::path file = settingsFile("markers.json");
    fs::remove(file);
    CHECK(game::loadSettings(file).markers == 1);
    game::GameSettings chosen;
    chosen.markers = 0;
    game::saveSettings(chosen, file);
    CHECK(game::loadSettings(file).markers == 0);
    json data = readJson(file);
    data["markers"] = 7; // out of range: the file is not trusted
    writeJson(file, data);
    CHECK(game::loadSettings(file).markers == 1);
}
