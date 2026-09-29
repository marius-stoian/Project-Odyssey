// odysseus.exe: the game. For now it logs its session and prints its version;
// the window and game loop arrive in US-020.
#include "core/log.h"
#include "core/version.h"
#include "luna/platform/user_paths.h"

#include <format>
#include <iostream>

int main() {
    const std::string_view version = odysseus::core::versionString();

    // One log file per run in the per-user folder; closed automatically at the end of main.
    const odysseus::core::LogSession log(luna::platform::userDataDirectory() / "logs");
    odysseus::core::logInfo(std::format("Project Odyssey {} started", version));

    std::cout << "Project Odyssey " << version << '\n';
    std::cout << "Log: " << log.file().string() << '\n';

    odysseus::core::logInfo("Shutting down");
    return 0;
}
