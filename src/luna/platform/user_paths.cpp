#include "luna/platform/user_paths.h"

#include <SDL3/SDL.h>

#include <memory>
#include <stdexcept>
#include <string>

namespace luna::platform {

namespace {

// SDL hands us memory that SDL_free must release; unique_ptr with this deleter does it
// for us, even if an exception is thrown (RAII around a C API).
struct SdlFree {
    void operator()(char* text) const { SDL_free(text); }
};

// Folder names: organisation, then application (SDL_GetPrefPath's two arguments).
constexpr const char* kOrganisation = "Project Odyssey";
constexpr const char* kApplication = "Odysseus";

} // namespace

std::filesystem::path userDataDirectory() {
    const std::unique_ptr<char, SdlFree> path(SDL_GetPrefPath(kOrganisation, kApplication));
    if (!path) {
        throw std::runtime_error(std::string("Cannot find the user data folder: ") + SDL_GetError());
    }
    // SDL paths are UTF-8; reading them as char8_t keeps non-English user names (e.g. "Ștefan") intact.
    return std::filesystem::path(reinterpret_cast<const char8_t*>(path.get()));
}

} // namespace luna::platform
