#pragma once

#include "boundary.h"

#include "game.h"
#include "input.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

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

// "Hold this intent from `fromSeconds` until `toSeconds`", for automated tests and demos.
struct ScriptedHold {
    Intent intent = Intent::MoveUp;
    double fromSeconds = 0.0;
    double toSeconds = 0.0;
};

// "Move the pointer from (x1, y1) to (x2, y2) between two times, holding `button`" (or no
// button: a hover), in virtual pixels. A click is a short one that does not move.
struct ScriptedPointer {
    double fromSeconds = 0.0;
    double toSeconds = 0.0;
    int x1 = 0;
    int y1 = 0;
    int x2 = 0;
    int y2 = 0;
    bool pressing = true;
    PointerButton button = PointerButton::Left;
};

// "Type this text at this time."
struct ScriptedText {
    double atSeconds = 0.0;
    std::string text;
};

struct RunOptions {
    std::uint64_t startNanoseconds = 0; // when the program started, to time the first frame
    double quitAfterSeconds = 0.0;      // > 0: close by itself, like pressing the close button
    std::filesystem::path screenshot;   // not empty: save the last frame there (.bmp)
    std::vector<ScriptedHold> holds;    // scripted input, see ScriptedHold
    std::vector<ScriptedPointer> pointer; // scripted mouse, see ScriptedPointer
    std::vector<ScriptedText> typing;     // scripted typing, see ScriptedText
    std::string renderer = "auto";        // "auto" (the GPU, else SDL_Renderer), "gpu" (fails when it cannot start) or "sdl" (US-230)
};

// Opens the window and runs the loop until the player closes it. Logs the window size,
// the time to the first frame and the average frame rate. Returns the program exit code.
int run(const AppConfig& config, Game& game, const RunOptions& options = {});

} // namespace luna::engine
