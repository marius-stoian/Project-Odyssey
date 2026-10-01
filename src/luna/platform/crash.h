#pragma once

#include "boundary.h"

#include <filesystem>
#include <string>

namespace luna::platform {

// After a crash (an unhandled exception, a failed terminate) writes `crash-<n>.log` into `crashDirectory` and copies the
// files of `saveDirectory` next to it (in a folder named `last-save`), so a playtester can send both (US-091). Call once at
// start-up. It lives in Platform because it talks to the operating system (Charter rule 2).
void installCrashHandler(const std::filesystem::path& crashDirectory, const std::filesystem::path& saveDirectory);

// What the handler does, callable on its own (for tests and for the handler itself): writes the crash log and copies the
// saves. Returns the log file.
std::filesystem::path writeCrashReport(const std::filesystem::path& crashDirectory, const std::filesystem::path& saveDirectory, const std::string& what);

} // namespace luna::platform
