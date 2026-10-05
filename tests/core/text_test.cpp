#include "core/text.h"

#include <doctest/doctest.h>

#include <filesystem>

namespace fs = std::filesystem;
using namespace odysseus::core;

TEST_CASE("Refactor: lowered, splitWords, joined and replaceAll") {
    CHECK(lowered("Hunger 42-Ok") == "hunger 42-ok");
    CHECK(splitWords("  gather\tberries \n 3 ") == std::vector<std::string>{"gather", "berries", "3"});
    CHECK(splitWords(" \t ").empty());
    CHECK(joined({"a", "b", "c"}, ", ") == "a, b, c");
    CHECK(replaceAll("{hero} met {hero}", "{hero}", "Ana {hero}") == "Ana {hero} met Ana {hero}");
}

TEST_CASE("Refactor: a safe write replaces the file, keeps the backups and can be read back") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-text-test";
    fs::remove_all(folder);
    const fs::path file = folder / "save.json";
    CHECK_FALSE(readTextFile(file).has_value());
    for (const char* text : {"one", "two", "three", "four"}) CHECK_FALSE(writeTextFileSafely(file, text, 2).has_value());
    CHECK(readTextFile(file) == "four");
    CHECK(readTextFile(fs::path(file).concat(".bak1")) == "three");
    CHECK(readTextFile(fs::path(file).concat(".bak2")) == "two");
    CHECK_FALSE(fs::exists(fs::path(file).concat(".bak3")));
    CHECK_FALSE(fs::exists(fs::path(file).concat(".tmp")));
    fs::remove_all(folder);
}
