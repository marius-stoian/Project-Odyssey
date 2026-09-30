// odysseus.exe: the game. Opens the window and runs Project Odyssey on the Luna engine.
//   --quit-after <seconds>   close by itself after that long, exactly like the close button
//   --log-dir <folder>       write the session log there instead of the per-user folder
//   --screenshot <file.bmp>  save the last frame as a picture (for evidence and progress reports)
//   --hold <Intent>:<from>:<to>  hold an intent (MoveUp, MoveDown, MoveLeft, MoveRight, Interact,
//                            OpenMenu) between two times in seconds: scripted play for tests
#include "core/log.h"
#include "core/version.h"
#include "game/odyssey_game.h"
#include "luna/engine/application.h"
#include "luna/engine/physics_view.h"
#include "luna/platform/system.h"
#include "luna/platform/user_paths.h"

#include <exception>
#include <stdexcept>
#include <vector>
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
    std::vector<luna::engine::ScriptedHold> holds;
};

luna::engine::Intent intentNamed(std::string_view name) {
    using luna::engine::Intent;
    if (name == "MoveUp") return Intent::MoveUp;
    if (name == "MoveDown") return Intent::MoveDown;
    if (name == "MoveLeft") return Intent::MoveLeft;
    if (name == "MoveRight") return Intent::MoveRight;
    if (name == "Interact") return Intent::Interact;
    if (name == "OpenMenu") return Intent::OpenMenu;
    throw std::invalid_argument("unknown intent: " + std::string(name));
}

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
        } else if (name == "--hold") {
            // "MoveRight:0.5:3" -> hold MoveRight from 0.5 s to 3 s
            const std::string value = argv[++i];
            const std::size_t first = value.find(':');
            const std::size_t second = value.find(':', first + 1);
            arguments.holds.push_back({intentNamed(value.substr(0, first)), std::stod(value.substr(first + 1, second - first - 1)),
                                       std::stod(value.substr(second + 1))});
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
        odysseus::game::OdysseyGame game(ODYSSEUS_DATA_DIR);
        const int exitCode = luna::engine::run(odysseus::game::odysseyAppConfig(), game,
                                               {start, arguments.quitAfterSeconds, arguments.screenshot, arguments.holds});
        const odysseus::game::Hero& hero = game.hero();
        odysseus::core::logInfo(std::format("Hero at ({:.1f}, {:.1f}) facing {}, {}", hero.feetX(), hero.feetY(),
                                            odysseus::game::facingName(hero.facing()), hero.walking() ? "walking" : "idle"));
        for (const auto& target : game.range().targets()) {
            odysseus::core::logInfo(std::format("Straw target at ({:.2f}, {:.2f}) m: {} hits, {:.1f} damage",
                                                luna::engine::toDouble(target.base.x), luna::engine::toDouble(target.base.y),
                                                target.hits, luna::engine::toDouble(target.damageTaken)));
        }
        odysseus::core::logInfo("Shutting down");
        return exitCode;
    } catch (const std::exception& error) {
        // Anything unexpected ends up in the log, so bug reports show what happened.
        odysseus::core::logError(std::format("Fatal error: {}", error.what()));
        std::cerr << "Fatal error: " << error.what() << '\n';
        return 1;
    }
}
