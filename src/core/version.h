#pragma once

#include "boundary.h"

#include <string_view>

namespace odysseus::core {

// Semantic version of this build, "MAJOR.MINOR.PATCH".
// The number lives in one place only: project(VERSION ...) in CMakeLists.txt.
std::string_view versionString();

} // namespace odysseus::core
