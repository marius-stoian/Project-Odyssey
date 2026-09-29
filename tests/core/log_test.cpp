#include "core/assertions.h"
#include "core/log.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// A fresh, empty folder per test so runs never see each other's logs.
fs::path makeEmptyFolder(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / ("odysseus-tests-" + name);
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

std::vector<std::string> readLines(const fs::path& file) {
    std::vector<std::string> lines;
    std::ifstream in(file);
    for (std::string line; std::getline(in, line);) {
        lines.push_back(line);
    }
    return lines;
}

std::vector<fs::path> sessionFiles(const fs::path& folder) {
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(folder)) {
        const std::string name = entry.path().filename().string();
        if (name.starts_with("session-") && name.ends_with(".log")) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::string readAll(const fs::path& file) {
    std::ifstream in(file);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

} // namespace

TEST_CASE("US-004 Log file") {
    const fs::path folder = makeEmptyFolder("us004-log-file");
    fs::path file;
    {
        odysseus::core::LogSession session(folder);
        file = session.file();
        odysseus::core::logInfo("hello from the test");
        odysseus::core::logWarning("a warning");
    } // the session closes here

    REQUIRE(fs::exists(file));
    CHECK(file.parent_path() == folder);
    CHECK(file.filename().string().starts_with("session-"));

    // Every line: "2026-09-30 01:23:45.678Z [LEVEL] text"
    const std::regex timestamped(R"(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}Z \[(INFO|WARNING|ERROR)\] .+)");
    const std::vector<std::string> lines = readLines(file);
    REQUIRE(lines.size() >= 2);
    for (const std::string& line : lines) {
        INFO("line: ", line);
        CHECK(std::regex_match(line, timestamped));
    }
    const std::string text = readAll(file);
    CHECK(text.find("[INFO] hello from the test") != std::string::npos);
    CHECK(text.find("[WARNING] a warning") != std::string::npos);
}

TEST_CASE("US-004 Rotation") {
    const fs::path folder = makeEmptyFolder("us004-rotation");
    std::ofstream(folder / "notes.txt") << "not a session log";

    std::vector<fs::path> created;
    for (int session = 0; session < 6; ++session) {
        odysseus::core::LogSession logSession(folder);
        created.push_back(logSession.file());
    }

    const std::vector<fs::path> remaining = sessionFiles(folder);
    CHECK(remaining.size() == 5);
    CHECK_FALSE(fs::exists(created.front())); // the oldest session is gone
    for (std::size_t i = 1; i < created.size(); ++i) {
        CHECK(fs::exists(created[i]));
    }
    CHECK(fs::exists(folder / "notes.txt")); // other files are never touched
}

TEST_CASE("US-004 Assert") {
    SUBCASE("the log records the file and line") {
        const fs::path folder = makeEmptyFolder("us004-assert-report");
        fs::path file;
        {
            odysseus::core::LogSession session(folder);
            file = session.file();
            odysseus::core::reportAssertionFailure("health > 0", "a hero cannot act when dead", "src/sim/hero.cpp", 42);
        }
        const std::string text = readAll(file);
        CHECK(text.find("[ERROR] Assertion failed: health > 0 (a hero cannot act when dead) at src/sim/hero.cpp:42") !=
              std::string::npos);
    }

#if !defined(NDEBUG)
    SUBCASE("a false assert stops the program in a Debug build") {
        const fs::path folder = makeEmptyFolder("us004-assert-probe");
        const std::string command =
            std::string("\"\"") + ODYSSEUS_ASSERT_PROBE + "\" \"" + folder.string() + "\"\"";
        const int exitCode = std::system(command.c_str());
        CHECK(exitCode != 0); // the program did not run past the assert

        const std::vector<fs::path> logs = sessionFiles(folder);
        REQUIRE(logs.size() == 1);
        const std::string text = readAll(logs.front());
        CHECK(text.find("Assertion failed: 1 + 1 == 3 (the probe asserts on purpose)") != std::string::npos);
        CHECK(text.find("assert_probe.cpp:" + std::to_string(ODYSSEUS_ASSERT_PROBE_LINE)) != std::string::npos);
        CHECK(text.find("after the assert") == std::string::npos);
    }
#endif
}
