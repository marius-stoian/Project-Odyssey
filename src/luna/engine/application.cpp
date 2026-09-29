#include "luna/engine/application.h"

#include "core/log.h"
#include "luna/engine/fixed_step_clock.h"
#include "luna/engine/frame_stats.h"
#include "luna/engine/input.h"
#include "luna/platform/system.h"
#include "luna/platform/window.h"

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

    FixedStepClock clock(config.ticksPerSecond);
    FrameStats stats;
    InputMap input;
    std::vector<platform::Event> events;
    const std::uint64_t loopStart = platform::nowNanoseconds();
    std::uint64_t previous = loopStart;
    bool firstFrame = true;
    bool quitRequested = false;
    bool running = true;

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
            }
            input.handle(event);
        }

        const int ticks = clock.advance(elapsed);
        for (int tick = 0; tick < ticks; ++tick) {
            game.update(input.nextTick());
        }

        window.clear(config.clearRed, config.clearGreen, config.clearBlue);
        game.render(clock.alpha());
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
