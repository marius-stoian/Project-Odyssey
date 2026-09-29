#pragma once

#include "boundary.h"

#include "events.h"

#include <SDL3/SDL_events.h>

#include <optional>

namespace luna::platform {

// Turns one SDL event into Luna's event, or nothing if Luna does not use it. Kept apart
// from Window so it can be tested with made-up SDL events, without a window or a gamepad.
std::optional<Event> translateEvent(const SDL_Event& event);

} // namespace luna::platform
