#include "luna/platform/crash.h"

#include <cstdlib>
#include <exception>
#include <format>
#include <fstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace luna::platform {

namespace {

// The handler runs with no arguments, so where to write is kept here.
std::filesystem::path gCrashDirectory;
std::filesystem::path gSaveDirectory;
bool gReporting = false; // a crash inside the handler must not start it again

[[noreturn]] void onTerminate() {
    std::string what = "terminate called";
    if (const std::exception_ptr active = std::current_exception()) {
        try {
            std::rethrow_exception(active);
        } catch (const std::exception& error) {
            what = std::string("unhandled exception: ") + error.what();
        } catch (...) {
            what = "unhandled exception of an unknown kind";
        }
    }
    if (!gReporting) {
        gReporting = true;
        try {
            writeCrashReport(gCrashDirectory, gSaveDirectory, what);
        } catch (...) {
        }
    }
    std::_Exit(3);
}

#ifdef _WIN32
LONG WINAPI onWindowsCrash(EXCEPTION_POINTERS* info) {
    if (!gReporting) {
        gReporting = true;
        try {
            writeCrashReport(gCrashDirectory, gSaveDirectory, std::format("Windows exception code 0x{:08X}", static_cast<unsigned long>(info->ExceptionRecord->ExceptionCode)));
        } catch (...) {
        }
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
#endif

} // namespace

std::filesystem::path writeCrashReport(const std::filesystem::path& crashDirectory, const std::filesystem::path& saveDirectory, const std::string& what) {
    std::filesystem::create_directories(crashDirectory);
    int number = 1;
    std::filesystem::path log;
    do {
        log = crashDirectory / std::format("crash-{}.log", number++);
    } while (std::filesystem::exists(log));
    {
        std::ofstream out(log, std::ios::binary | std::ios::trunc);
        out << "Project Odyssey crashed.\n" << what << "\n";
        out << "Please send this file and the folder last-save next to it.\n";
    }
    // The last autosave, so the problem can be replayed.
    std::error_code problem;
    if (std::filesystem::is_directory(saveDirectory, problem)) {
        const std::filesystem::path target = crashDirectory / "last-save";
        std::filesystem::remove_all(target, problem);
        std::filesystem::create_directories(target, problem);
        for (const auto& entry : std::filesystem::directory_iterator(saveDirectory, problem)) {
            if (entry.is_regular_file()) std::filesystem::copy_file(entry.path(), target / entry.path().filename(), std::filesystem::copy_options::overwrite_existing, problem);
        }
    }
    return log;
}

void installCrashHandler(const std::filesystem::path& crashDirectory, const std::filesystem::path& saveDirectory) {
    gCrashDirectory = crashDirectory;
    gSaveDirectory = saveDirectory;
    std::set_terminate(onTerminate);
#ifdef _WIN32
    SetUnhandledExceptionFilter(onWindowsCrash);
#endif
}

} // namespace luna::platform
