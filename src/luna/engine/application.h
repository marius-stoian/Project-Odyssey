#pragma once

#include "boundary.h"

#include "game.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace luna::engine {

struct AppConfig {
    std::string title = "Luna";
    int windowWidth = 1280;
    int windowHeight = 720;
    int virtualWidth = 480;
    int virtualHeight = 270;
    int ticksPerSecond = 20; // ADR-006
    int clearRed = 0;
    int clearGreen = 0;
    int clearBlue = 0;
};

struct RunOptions {
    std::uint64_t startNanoseconds = 0; // when the program started, to time the first frame
    double quitAfterSeconds = 0.0;      // > 0: close by itself, like pressing the close button
    std::filesystem::path screenshot;   // not empty: save the last frame there (.bmp)
};

// Opens the window and runs the loop until the player closes it. Logs the window size,
// the time to the first frame and the average frame rate. Returns the program exit code.
int run(const AppConfig& config, Game& game, const RunOptions& options = {});

} // namespace luna::engine
