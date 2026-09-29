#pragma once

#include "boundary.h"

#include <filesystem>

namespace luna::platform {

// The folder where this user's saves, settings and logs belong, created if missing.
// Windows: %APPDATA%\Project Odyssey\Odysseus\. Android and iOS get their own
// app-private folder from SDL3, which is why this lives in Platform (Charter rule 2).
std::filesystem::path userDataDirectory();

} // namespace luna::platform
