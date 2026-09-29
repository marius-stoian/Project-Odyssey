// odysseus.exe: the game. Opens the window and runs Project Odyssey on the Luna engine.
//   --quit-after <seconds>   close by itself after that long, exactly like the close button
//   --log-dir <folder>       write the session log there instead of the per-user folder
//   --screenshot <file.bmp>  save the last frame as a picture (for evidence and progress reports)
#include "core/log.h"
#include "core/version.h"
#include "game/odyssey_game.h"
#include "luna/engine/application.h"
#include "luna/platform/system.h"
#include "luna/platform/user_paths.h"

#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

namespace {

struct Arguments {
    double quitAfterSeconds = 0.0;
    std::filesystem::path logDirectory;
    std::filesystem::path screenshot;
};

Arguments parseArguments(int argc, char* argv[]) {
    Arguments arguments;
    for (int i = 1; i + 1 < argc; ++i) {
        const std::string_view name = argv[i];
        if (name == "--quit-after") {
            arguments.quitAfterSeconds = std::stod(argv[++i]);
        } else if (name == "--log-dir") {
            arguments.logDirectory = argv[++i];
        } else if (name == "--screenshot") {
            arguments.screenshot = argv[++i];
        }
    }
    return arguments;
}

} // namespace

int main(int argc, char* argv[]) {
    const std::uint64_t start = luna::platform::nowNanoseconds();
    const Arguments arguments = parseArguments(argc, argv);
    const std::filesystem::path logDirectory =
        arguments.logDirectory.empty() ? luna::platform::userDataDirectory() / "logs" : arguments.logDirectory;

    const odysseus::core::LogSession log(logDirectory);
    const std::string_view version = odysseus::core::versionString();
    odysseus::core::logInfo(std::format("Project Odyssey {} started", version));
    std::cout << "Project Odyssey " << version << "\nLog: " << log.file().string() << '\n';

    try {
        odysseus::game::OdysseyGame game;
        const int exitCode = luna::engine::run(odysseus::game::odysseyAppConfig(), game,
                                               {start, arguments.quitAfterSeconds, arguments.screenshot});
        odysseus::core::logInfo("Shutting down");
        return exitCode;
    } catch (const std::exception& error) {
        // Anything unexpected ends up in the log, so bug reports show what happened.
        odysseus::core::logError(std::format("Fatal error: {}", error.what()));
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
