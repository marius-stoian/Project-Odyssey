#pragma once

#include "boundary.h"
#include "core/presentation.h"

#include <filesystem>
#include <string>

namespace odysseus::game {

// The player's basic settings (US-081, D-32): window mode, window size and volume, kept in settings.json and remembered next
// launch. Damaged or impossible values are replaced by the defaults and the file is written again.
struct GameSettings {
    core::Resolution resolution;
    int cameraZoom = 2;
    int uiScale = 1;
    std::string lighting = "Medium";
    int volume = 80; // 0..100 (there is no sound yet: it is kept for when there is)
    int statistics = 0; // local session statistics (US-092): 0 not asked yet, 1 agreed, 2 declined
    friend bool operator==(const GameSettings&, const GameSettings&) = default;
};

inline constexpr int kMinWindowWidth = 640;
inline constexpr int kMaxWindowWidth = 7680;
inline constexpr int kMinWindowHeight = 360;
inline constexpr int kMaxWindowHeight = 4320;

// Reads the file. A missing file gives the defaults (and is written); a file with anything invalid in it gives the defaults,
// is rewritten, and `note` says what was wrong.
GameSettings loadSettings(const std::filesystem::path& file, std::string* note = nullptr);
void saveSettings(const GameSettings& settings, const std::filesystem::path& file);

} // namespace odysseus::game
