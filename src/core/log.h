#pragma once

#include "boundary.h"

#include <filesystem>
#include <string_view>

namespace odysseus::core {

// Architecture 7.8: keep the last 5 session logs, so bug reports have recent history.
inline constexpr int kKeptLogSessions = 5;

// One log file per program run. Creating a LogSession opens a new timestamped file in
// `directory` (and deletes the oldest session files beyond `keptSessions`); destroying it
// closes the file. RAII means the file is closed even when a function exits early.
class LogSession {
public:
    explicit LogSession(const std::filesystem::path& directory, int keptSessions = kKeptLogSessions);
    ~LogSession();

    // Exactly one owner of the open file: copying a session would make two closers.
    LogSession(const LogSession&) = delete;
    LogSession& operator=(const LogSession&) = delete;

    const std::filesystem::path& file() const;

private:
    std::filesystem::path file_;
};

// Each call writes one timestamped line to the active session (or standard error if none).
void logInfo(std::string_view message);
void logWarning(std::string_view message);
void logError(std::string_view message);

} // namespace odysseus::core
