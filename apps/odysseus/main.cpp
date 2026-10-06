// odysseus.exe: the game. Opens the window and runs Project Odyssey on the Luna engine.
//   --quit-after <seconds>   close by itself after that long, exactly like the close button
//   --log-dir <folder>       write the session log there instead of the per-user folder
//   --screenshot <file.bmp>  save the last frame as a picture (for evidence and progress reports)
//   --renderer <auto|gpu|sdl>  how the picture is drawn: auto (the graphics card, else the first renderer with the reason in the log; the default), gpu (stop if the card cannot be used), sdl (the first renderer)
//   --level <file.json>      play this level (default: assets/levels/valley.json)
//   --editor                 start in Editor mode (F1 plays, F2 edits)
//   --no-watch               do not watch the data files for changes made outside the game (they are still read on F5 and on an Editor save)
//   --new-game               start at the New Game screen (seed, Growing Period, Comfort)
//   --region <seed>          play a generated region: land, resources, the clan at its start and two rival clans
//   --save-dir <folder>      where the autosaves go (default: the user's save folder); --load brings the autosave back
//   --clan-speed <n>         run the clan's simulation n ticks per game tick (fast forward for demos)
//   --clan                   run the simulated clan in this level (levels marked "clan": true do it by themselves)
//   --weather <name>         start under that weather, for example `steady rain` (screenshots and demos)
//   --seed <number>          fix the weather sequence (the same seed gives the same weathers in the same order)
//   --click <x>:<y>:<time>[:right]  click there (virtual pixels, 960x540) at that time
//   --drag <x1>:<y1>:<x2>:<y2>:<from>:<to>  hold the left button and move from one point to the other
//   --point <x>:<y>:<from>:<to>  rest the pointer there without pressing (hover)
//   --aim <x>:<y>:<from>:<to>  the same as --point: where the hero aims (Game mode); fire with --hold Attack:<from>:<to>
//   --type <text>:<time>     type the text at that time
//   --hold <Intent>:<from>:<to>  hold an intent (MoveUp, MoveDown, MoveLeft, MoveRight, Interact,
//                            OpenMenu, SwitchWeapon, ModeGame, ModeEditor, Undo, Redo, Save, Delete,
//                            ToggleGrid, Rotate, Erase, Confirm, Attack, Inspect, DevTools, Overlay, Slot1..Slot9) between two times in seconds: scripted play for tests
#include "core/log.h"
#include "core/version.h"
#include "game/odyssey_game.h"
#include "luna/engine/application.h"
#include "luna/engine/physics_view.h"
#include "luna/platform/crash.h"
#include "luna/platform/system.h"
#include "sim/schema_install.h"
#include "luna/platform/user_paths.h"

#include <exception>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

namespace {

struct Arguments {
    double quitAfterSeconds = 0.0;
    std::filesystem::path logDirectory;
    std::filesystem::path screenshot;
    std::string renderer = "auto";
    std::filesystem::path level;
    bool editor = false;
    bool watch = true;                 // notice files saved outside the game and read them again (US-304); --no-watch turns it off
    std::optional<std::uint64_t> seed; // fixes the weather sequence (US-138)
    std::string weather;               // start under this weather (screenshots)
    bool clan = false;                 // run the simulated clan in this level
    int clanSpeed = 1;                 // clan simulation ticks per game tick (fast forward)
    bool perf = false;                 // a performance run: overlay on, GPU time measured, figures logged each minute (US-234)
    int people = 0;                    // start the clan with this many people (performance runs)
    std::optional<std::uint64_t> region; // play a generated region
    std::filesystem::path saveDirectory; // where autosaves go
    bool load = false;                 // load the autosave at start
    bool newGame = false;              // open the New Game screen
    std::vector<luna::engine::ScriptedHold> holds;
    std::vector<luna::engine::ScriptedPointer> pointer;
    std::vector<luna::engine::ScriptedText> typing;
};

luna::engine::Intent intentNamed(std::string_view name) {
    if (const auto intent = luna::engine::intentFromName(name)) return *intent;
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
        if (name == "--new-game") { // a flag without a value: start at the New Game screen
            arguments.newGame = true;
            continue;
        }
        if (name == "--load") { // a flag without a value: bring back the autosave
            arguments.load = true;
            continue;
        }
        if (name == "--perf") { // a flag without a value
            arguments.perf = true;
            continue;
        }
        if (name == "--clan") { // a flag without a value: run the simulated clan in this level
            arguments.clan = true;
            continue;
        }
        if (name == "--no-watch") { // a flag without a value: do not watch the data files (tests, headless runs)
            arguments.watch = false;
            continue;
        }
        if (name == "--editor") { // a flag without a value
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
        } else if (name == "--region") {
            arguments.region = std::stoull(argv[++i]);
        } else if (name == "--save-dir") {
            arguments.saveDirectory = argv[++i];
        } else if (name == "--people") {
            arguments.people = std::stoi(argv[++i]);
        } else if (name == "--clan-speed") {
            arguments.clanSpeed = std::stoi(argv[++i]);
        } else if (name == "--weather") {
            arguments.weather = argv[++i];
        } else if (name == "--seed") {
            arguments.seed = std::stoull(argv[++i]);
        } else if (name == "--level") {
            arguments.level = argv[++i];
        } else if (name == "--screenshot") {
            arguments.screenshot = argv[++i];
        } else if (name == "--renderer") {
            arguments.renderer = argv[++i];
            if (arguments.renderer != "auto" && arguments.renderer != "gpu" && arguments.renderer != "sdl") {
                throw std::invalid_argument("--renderer is auto, gpu or sdl");
            }
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
        } else if (name == "--point" || name == "--aim") { // --aim is --point, named for what it does in Game mode
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
        // A packaged build (the zip, US-091) keeps assets/ next to odysseus.exe and its saves in the user's folder; a developer's
        // build uses the source folder it was built from.
        const std::filesystem::path packaged = luna::platform::executableDirectory() / "assets" / "data";
        const bool isPackaged = std::filesystem::exists(packaged / "hero" / "hero.json");
        const std::filesystem::path dataFolder = isPackaged ? packaged : std::filesystem::path(ODYSSEUS_DATA_DIR);
        // Every data file is checked against its schema as it loads (US-190); a broken schema folder is said in the log and never stops the game.
        if (const std::optional<std::string> problem = odysseus::sim::schema::installFromFolder(dataFolder)) odysseus::core::logWarning("Schemas: " + *problem);
        odysseus::game::OdysseyGame game(dataFolder, arguments.level);
        if (arguments.watch) game.setWatching(true); // the files of the game are watched while it runs (US-304)
        if (!arguments.saveDirectory.empty()) {
            game.setSaveDirectory(arguments.saveDirectory);
        } else if (isPackaged) {
            game.setSaveDirectory(luna::platform::userDataDirectory() / "saves");
        }
        luna::platform::installCrashHandler(luna::platform::userDataDirectory() / "crash", game.saveDirectory());
        if (arguments.region) {
            game.loadRegion(*arguments.region);
        }
        if (arguments.load && !game.loadAutosave()) {
            odysseus::core::logWarning("--load found nothing to load");
        }
        if (arguments.newGame) {
            game.run().openNewGame();
        }
        game.setStartingPeople(arguments.people);
        game.setPerformanceLog(arguments.perf);
        if (arguments.clan) {
            game.setClan(true);
        }
        game.setClanSpeed(arguments.clanSpeed);
        if (arguments.seed) {
            game.setWeatherSeed(*arguments.seed);
        }
        if (!arguments.weather.empty() && !game.setWeatherNamed(arguments.weather)) {
            throw std::invalid_argument("unknown weather: " + arguments.weather);
        }
        if (arguments.editor) {
            game.switchMode(odysseus::game::Mode::Editor);
        }
        luna::engine::AppConfig app = odysseus::game::odysseyAppConfig();
        app.windowWidth = game.settings().resolution.width;
        app.windowHeight = game.settings().resolution.height;
        app.windowMode = game.settings().resolution.mode;
        app.scaling = game.settings().resolution.scaling;
        game.takeWindowChange(); // the first window already uses the saved settings
        const int exitCode = luna::engine::run(app, game,
                                               {start, arguments.quitAfterSeconds, arguments.screenshot, arguments.holds, arguments.pointer, arguments.typing, arguments.renderer});
        const odysseus::game::Hero& hero = game.hero();
        odysseus::core::logInfo(std::format("Hero at ({:.1f}, {:.1f}) facing {}, {}", hero.feetX(), hero.feetY(),
                                            odysseus::game::facingName(hero.facing()), hero.walking() ? "walking" : "idle"));
        for (const auto& target : game.range().targets()) {
            odysseus::core::logInfo(std::format("Straw target at ({:.2f}, {:.2f}) m: {} hits, {:.1f} damage",
                                                luna::engine::toDouble(target.base.x), luna::engine::toDouble(target.base.y),
                                                target.hits, luna::engine::toDouble(target.damageTaken)));
        }
        if (const std::filesystem::path stats = game.finishSession(); !stats.empty()) {
            odysseus::core::logInfo("Session statistics written to " + stats.string());
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
