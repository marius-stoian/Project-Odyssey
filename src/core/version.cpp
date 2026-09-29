#include "core/version.h"

namespace odysseus::core {

std::string_view versionString() {
    // ODYSSEUS_VERSION is passed in by CMake (target_compile_definitions),
    // so the code can never disagree with the build about the version.
    return ODYSSEUS_VERSION;
}

} // namespace odysseus::core
