#include "core/version.h"

#include <doctest/doctest.h>

#include <string_view>

namespace {

// True when text looks like "1.2.3": three numbers separated by dots.
bool isSemanticVersion(std::string_view text) {
    int dots = 0;
    bool digitSinceDot = false;
    for (char c : text) {
        if (c == '.') {
            if (!digitSinceDot) {
                return false;
            }
            ++dots;
            digitSinceDot = false;
        } else if (c >= '0' && c <= '9') {
            digitSinceDot = true;
        } else {
            return false;
        }
    }
    return dots == 2 && digitSinceDot;
}

} // namespace

TEST_CASE("US-001 Clean build") {
    // If this runs, the test program compiled and linked against Core.
    // The version must come through from project(VERSION ...) in CMakeLists.txt.
    const std::string_view version = odysseus::core::versionString();
    CHECK_FALSE(version.empty());
    CHECK(isSemanticVersion(version));
}
