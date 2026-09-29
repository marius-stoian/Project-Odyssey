#include "luna/platform/system.h"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <string>

namespace luna::platform {

System::System() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error(std::string("SDL could not start: ") + SDL_GetError());
    }
}

System::~System() {
    SDL_Quit();
}

std::uint64_t nowNanoseconds() {
    return SDL_GetTicksNS();
}

void sleepNanoseconds(std::uint64_t nanoseconds) {
    SDL_DelayNS(nanoseconds);
}

void requestQuit() {
    SDL_Event event{};
    event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&event);
}

} // namespace luna::platform
