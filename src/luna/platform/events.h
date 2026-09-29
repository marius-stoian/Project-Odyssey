#pragma once

#include "boundary.h"

namespace luna::platform {

// Luna's own description of what happened, so nothing above Platform ever sees SDL.
enum class EventType {
    Quit, // the close button, Alt+F4, or requestQuit()
};

struct Event {
    EventType type = EventType::Quit;
};

} // namespace luna::platform
