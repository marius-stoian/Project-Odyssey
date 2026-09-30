#include "luna/engine/application.h"

#include "core/log.h"
#include "luna/engine/fixed_step_clock.h"
#include "luna/engine/frame_stats.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "luna/platform/system.h"
#include "luna/platform/window.h"

#include <algorithm>
#include <array>
#include <format>
#include <vector>

namespace luna::engine {

namespace {

constexpr std::uint64_t kNanosecondsPerSecond = 1'000'000'000ULL;
constexpr std::uint64_t kFallbackFrameNanoseconds = kNanosecondsPerSecond / 60; // when VSync is off

double toMilliseconds(std::uint64_t nanoseconds) {
    return static_cast<double>(nanoseconds) / 1e6;
}

} // namespace

int run(const AppConfig& config, Game& game, const RunOptions& options) {
    using namespace odysseus::core;

    platform::System system;
    platform::Window window({config.title, config.windowWidth, config.windowHeight, config.virtualWidth,
                             config.virtualHeight});
    logInfo(std::format("Window opened: {}x{}, virtual screen {}x{}, VSync {}", config.windowWidth,
                        config.windowHeight, config.virtualWidth, config.virtualHeight,
                        window.vsyncEnabled() ? "on" : "off"));

    WindowRenderer renderer(window);
    InputMap input;
    const auto logScale = [&](int width, int height) {
        const PixelScale pixels = integerScale(width, height, config.virtualWidth, config.virtualHeight);
        input.setPointerArea(pixels.area, pixels.scale); // mouse positions become virtual pixels
        logInfo(std::format("Window {}x{}: pixel art scaled x{}, picture {}x{} at ({}, {})", width, height, pixels.scale,
                            pixels.area.width, pixels.area.height, pixels.area.x, pixels.area.y));
    };
    logScale(config.windowWidth, config.windowHeight);
    game.start(renderer);

    FixedStepClock clock(config.ticksPerSecond);
    FrameStats stats;
    std::vector<platform::Event> events;
    const std::uint64_t loopStart = platform::nowNanoseconds();
    std::uint64_t previous = loopStart;
    bool firstFrame = true;
    bool quitRequested = false;
    bool running = true;
    std::vector<bool> scriptedActive(options.pointer.size(), false);
    std::vector<bool> typed(options.typing.size(), false);

    while (running) {
        const std::uint64_t now = platform::nowNanoseconds();
        const std::uint64_t elapsed = now - previous;
        previous = now;

        events.clear();
        window.pollEvents(events);
        for (const platform::Event& event : events) {
            if (event.type == platform::EventType::Quit) {
                running = false;
            } else if (event.type == platform::EventType::GamepadAdded) {
                logInfo(std::format("Gamepad {} connected", event.gamepad));
            } else if (event.type == platform::EventType::GamepadRemoved) {
                logInfo(std::format("Gamepad {} disconnected", event.gamepad));
            } else if (event.type == platform::EventType::WindowResized) {
                logScale(event.width, event.height);
            }
            input.handle(event);
        }

        const double runSeconds = static_cast<double>(now - loopStart) / 1e9;
        // Several holds may name the same intent (two presses): it is held while any of them is
        // active, and set once, so an inactive hold never releases an active one.
        std::array<bool, kIntentCount> scripted{};
        std::array<bool, kIntentCount> named{};
        for (const ScriptedHold& hold : options.holds) {
            const auto i = static_cast<std::size_t>(hold.intent);
            named[i] = true;
            scripted[i] = scripted[i] || (runSeconds >= hold.fromSeconds && runSeconds < hold.toSeconds);
        }
        for (std::size_t i = 0; i < kIntentCount; ++i) {
            if (named[i]) input.setScripted(static_cast<Intent>(i), scripted[i]);
        }
        for (std::size_t i = 0; i < options.pointer.size(); ++i) {
            const ScriptedPointer& move = options.pointer[i];
            const bool active = runSeconds >= move.fromSeconds && runSeconds < move.toSeconds;
            if (active) {
                // Straight from start to end over the time given (a drag), whole pixels.
                const double span = move.toSeconds - move.fromSeconds;
                const double part = span > 0.0 ? std::min(1.0, (runSeconds - move.fromSeconds) / span) : 1.0;
                const int x = move.x1 + static_cast<int>((move.x2 - move.x1) * part);
                const int y = move.y1 + static_cast<int>((move.y2 - move.y1) * part);
                input.setScriptedPointer(x, y, move.button, move.pressing);
                scriptedActive[i] = true;
            } else if (scriptedActive[i]) {
                input.setScriptedPointer(move.x2, move.y2, move.button, false); // let go at the end
                scriptedActive[i] = false;
            }
        }
        for (std::size_t i = 0; i < options.typing.size(); ++i) {
            if (!typed[i] && runSeconds >= options.typing[i].atSeconds) {
                input.typeScripted(options.typing[i].text);
                typed[i] = true;
            }
        }

        const int ticks = clock.advance(elapsed);
        for (int tick = 0; tick < ticks; ++tick) {
            game.update(input.nextTick());
        }

        window.clear(config.clearRed, config.clearGreen, config.clearBlue);
        game.render(renderer, clock.alpha());
        if (!running && !options.screenshot.empty()) {
            window.saveScreenshot(options.screenshot); // the last frame, just before closing
            logInfo(std::format("Screenshot saved: {}", options.screenshot.string()));
        }
        window.present();

        if (firstFrame) {
            firstFrame = false;
            logInfo(std::format("First frame after {:.0f} ms", toMilliseconds(platform::nowNanoseconds() - options.startNanoseconds)));
        } else {
            stats.addFrame(elapsed);
        }

        if (!window.vsyncEnabled()) {
            const std::uint64_t frameTime = platform::nowNanoseconds() - now;
            if (frameTime < kFallbackFrameNanoseconds) {
                platform::sleepNanoseconds(kFallbackFrameNanoseconds - frameTime);
            }
        }

        if (!quitRequested && options.quitAfterSeconds > 0.0 &&
            static_cast<double>(platform::nowNanoseconds() - loopStart) / 1e9 >= options.quitAfterSeconds) {
            quitRequested = true;
            platform::requestQuit(); // the same path as the close button
        }
    }

    logInfo(std::format("Average frame rate: {:.1f} FPS over {:.1f} s ({} frames); {} simulation ticks",
                        stats.averageFps(), stats.seconds(), stats.frames(), clock.totalTicks()));
    logInfo("Window closed by the player");
    return 0;
}

} // namespace luna::engine
