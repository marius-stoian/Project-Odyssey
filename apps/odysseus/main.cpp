// odysseus.exe: the game. Opens the window and runs Project Odyssey on the Luna engine.
//   --quit-after <seconds>   close by itself after that long, exactly like the close button
//   --log-dir <folder>       write the session log there instead of the per-user folder
//   --screenshot <file.bmp>  save the last frame as a picture (for evidence and progress reports)
//   --level <file.json>      play this level (default: assets/levels/valley.json)
//   --editor                 start in Editor mode (F1 plays, F2 edits)
//   --click <x>:<y>:<time>[:right]  click there (virtual pixels, 480x270) at that time
//   --drag <x1>:<y1>:<x2>:<y2>:<from>:<to>  hold the left button and move from one point to the other
//   --point <x>:<y>:<from>:<to>  rest the pointer there without pressing (hover)
//   --type <text>:<time>     type the text at that time
//   --hold <Intent>:<from>:<to>  hold an intent (MoveUp, MoveDown, MoveLeft, MoveRight, Interact,
//                            OpenMenu, SwitchWeapon, ModeGame, ModeEditor, Undo, Redo, Save, Delete,
//                            ToggleGrid, Rotate, Erase, Confirm) between two times in seconds: scripted play for tests
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
    std::filesystem::path level;
    bool editor = false;
    std::vector<luna::engine::ScriptedHold> holds;
    std::vector<luna::engine::ScriptedPointer> pointer;
    std::vector<luna::engine::ScriptedText> typing;
};

luna::engine::Intent intentNamed(std::string_view name) {
    using luna::engine::Intent;
    if (name == "MoveUp") return Intent::MoveUp;
    if (name == "MoveDown") return Intent::MoveDown;
    if (name == "MoveLeft") return Intent::MoveLeft;
    if (name == "MoveRight") return Intent::MoveRight;
    if (name == "Interact") return Intent::Interact;
    if (name == "OpenMenu") return Intent::OpenMenu;
    if (name == "SwitchWeapon") return Intent::SwitchWeapon;
    if (name == "ModeGame") return Intent::ModeGame;
    if (name == "ModeEditor") return Intent::ModeEditor;
    if (name == "Undo") return Intent::Undo;
    if (name == "Redo") return Intent::Redo;
    if (name == "Save") return Intent::Save;
    if (name == "Delete") return Intent::Delete;
    if (name == "ToggleGrid") return Intent::ToggleGrid;
    if (name == "Rotate") return Intent::Rotate;
    if (name == "Erase") return Intent::Erase;
    if (name == "Confirm") return Intent::Confirm;
    throw std::invalid_argument("unknown intent: " + std::string(name));
}

// "a:b:c" -> {"a", "b", "c"}
std::vector<std::string> fields(const std::string& value) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    for (std::size_t colon = value.find(':'); colon != std::string::npos; colon = value.find(':', start)) {
        parts.push_back(value.substr(start, colon - start));
        start = colon + 1;
    }
    parts.push_back(value.substr(start));
    return parts;
}

Arguments parseArguments(int argc, char* argv[]) {
    Arguments arguments;
    for (int i = 1; i < argc; ++i) {
        const std::string_view name = argv[i];
        if (name == "--editor") { // the one flag without a value
            arguments.editor = true;
            continue;
        }
        if (i + 1 >= argc) {
            throw std::invalid_argument(std::string(name) + " needs a value");
        }
        if (name == "--quit-after") {
            arguments.quitAfterSeconds = std::stod(argv[++i]);
        } else if (name == "--log-dir") {
            arguments.logDirectory = argv[++i];
        } else if (name == "--level") {
            arguments.level = argv[++i];
        } else if (name == "--screenshot") {
            arguments.screenshot = argv[++i];
        } else if (name == "--click") {
            const auto f = fields(argv[++i]);
            const double t = std::stod(f.at(2));
            const bool right = f.size() > 3 && f[3] == "right";
            arguments.pointer.push_back({t, t + 0.12, std::stoi(f.at(0)), std::stoi(f.at(1)), std::stoi(f.at(0)), std::stoi(f.at(1)), true,
                                         right ? luna::engine::PointerButton::Right : luna::engine::PointerButton::Left});
        } else if (name == "--drag") {
            const auto f = fields(argv[++i]);
            arguments.pointer.push_back({std::stod(f.at(4)), std::stod(f.at(5)), std::stoi(f.at(0)), std::stoi(f.at(1)), std::stoi(f.at(2)),
                                         std::stoi(f.at(3)), true, luna::engine::PointerButton::Left});
        } else if (name == "--point") {
            const auto f = fields(argv[++i]);
            arguments.pointer.push_back({std::stod(f.at(2)), std::stod(f.at(3)), std::stoi(f.at(0)), std::stoi(f.at(1)), std::stoi(f.at(0)),
                                         std::stoi(f.at(1)), false, luna::engine::PointerButton::Left});
        } else if (name == "--type") {
            const std::string value = argv[++i];
            const std::size_t colon = value.rfind(':');
            arguments.typing.push_back({std::stod(value.substr(colon + 1)), value.substr(0, colon)});
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
        odysseus::game::OdysseyGame game(ODYSSEUS_DATA_DIR, arguments.level);
        if (arguments.editor) {
            game.switchMode(odysseus::game::Mode::Editor);
        }
        const int exitCode = luna::engine::run(odysseus::game::odysseyAppConfig(), game,
                                               {start, arguments.quitAfterSeconds, arguments.screenshot, arguments.holds, arguments.pointer, arguments.typing});
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
