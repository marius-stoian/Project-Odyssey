#include "core/log.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace odysseus::core {

namespace {

// The file of the session that is currently open. Single-threaded by design (ADR-007),
// so a plain variable is enough; unique_ptr closes the file automatically.
std::unique_ptr<std::ofstream> gActiveLog;

bool isSessionLog(const std::filesystem::path& path) {
    const std::string name = path.filename().string();
    return name.starts_with("session-") && name.ends_with(".log");
}

// Oldest first: the names start with the date and time, so text order is time order.
std::vector<std::filesystem::path> sessionLogsIn(const std::filesystem::path& directory) {
    std::vector<std::filesystem::path> logs;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && isSessionLog(entry.path())) {
            logs.push_back(entry.path());
        }
    }
    std::sort(logs.begin(), logs.end());
    return logs;
}

// UTC wall-clock time: logs are for people, not simulation state, so this does not break
// determinism (Charter rule 6), and UTC needs no time-zone database.
std::chrono::sys_time<std::chrono::milliseconds> now() {
    return std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
}

// "session-20260930-012345-678-00.log"; the counter keeps same-millisecond runs apart.
std::filesystem::path newSessionFile(const std::filesystem::path& directory) {
    const auto time = now();
    const auto wholeSeconds = std::chrono::floor<std::chrono::seconds>(time);
    const auto milliseconds = (time - wholeSeconds).count();
    const std::string stamp = std::format("{:%Y%m%d-%H%M%S}-{:03}", wholeSeconds, milliseconds);
    for (int counter = 0; counter < 100; ++counter) {
        const std::filesystem::path candidate = directory / std::format("session-{}-{:02}.log", stamp, counter);
        if (!std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    throw std::runtime_error("LogSession: too many sessions started in the same millisecond");
}

void writeLine(std::string_view level, std::string_view message) {
    // %T prints seconds with the milliseconds of the time point, e.g. 01:23:45.678.
    const std::string line = std::format("{:%Y-%m-%d %T}Z [{}] {}", now(), level, message);
    if (gActiveLog) {
        *gActiveLog << line << '\n';
        gActiveLog->flush(); // a crash right after this line still leaves it on disk
    } else {
        std::cerr << line << '\n';
    }
}

} // namespace

LogSession::LogSession(const std::filesystem::path& directory, int keptSessions) {
    std::filesystem::create_directories(directory);

    // Make room first, so that with the new file exactly `keptSessions` remain.
    std::vector<std::filesystem::path> existing = sessionLogsIn(directory);
    const std::size_t keepBeforeNew = keptSessions > 1 ? static_cast<std::size_t>(keptSessions - 1) : 0;
    while (existing.size() > keepBeforeNew) {
        std::filesystem::remove(existing.front());
        existing.erase(existing.begin());
    }

    file_ = newSessionFile(directory);
    gActiveLog = std::make_unique<std::ofstream>(file_);
    if (!*gActiveLog) {
        gActiveLog.reset();
        throw std::runtime_error("LogSession: cannot create " + file_.string());
    }
    logInfo("Log session started");
}

LogSession::~LogSession() {
    logInfo("Log session ended");
    gActiveLog.reset();
}

const std::filesystem::path& LogSession::file() const {
    return file_;
}

void logInfo(std::string_view message) {
    writeLine("INFO", message);
}

void logWarning(std::string_view message) {
    writeLine("WARNING", message);
}

void logError(std::string_view message) {
    writeLine("ERROR", message);
}

} // namespace odysseus::core
