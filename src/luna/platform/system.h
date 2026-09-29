#pragma once

#include "boundary.h"

#include <cstdint>

namespace luna::platform {

// Starts SDL (video and gamepads) and stops it again when destroyed. Create exactly one,
// before any Window, and keep it alive for the whole program (RAII).
class System {
public:
    System();
    ~System();

    System(const System&) = delete;
    System& operator=(const System&) = delete;
};

// Monotonic time since SDL started, in nanoseconds. Only for pacing frames and measuring
// performance, never for simulation results (Charter rule 6: determinism).
std::uint64_t nowNanoseconds();

// Waits at least this long without using the CPU.
void sleepNanoseconds(std::uint64_t nanoseconds);

// Asks the running program to close, exactly as if the user pressed the window's close
// button. Used by automated tests and by the "--quit-after" option.
void requestQuit();

} // namespace luna::platform
