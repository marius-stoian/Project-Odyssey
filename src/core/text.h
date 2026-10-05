#pragma once

#include "boundary.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::core {

// Small text and file helpers every layer needs. They lived as private copies in a dozen files; one shared copy means one place to fix.

// The whole file, byte for byte; nothing when it cannot be opened.
std::optional<std::string> readTextFile(const std::filesystem::path& file);

// Writes `text` to "<file>.tmp" and renames it over `file` (ADR-010), so a crash never leaves half a file behind. The folder is made when
// missing. With `backups` above 0 the old file becomes "<file>.bak1", the one before ".bak2", and so on; the oldest is dropped.
// Returns what went wrong in plain words, or nothing when the file was written.
std::optional<std::string> writeTextFileSafely(const std::filesystem::path& file, std::string_view text, int backups = 0);

// Lower case for the letters A to Z; every other byte stays (names and keys are ASCII).
std::string lowered(std::string text);

// The words of `text`, split at spaces, tabs and line breaks; never an empty word.
std::vector<std::string> splitWords(std::string_view text);

// The words put back together with `separator` between them: joined({"a", "b"}, ", ") is "a, b".
std::string joined(const std::vector<std::string>& words, std::string_view separator);

// Every `from` in `text` replaced by `to`, left to right; a replacement is never searched again.
std::string replaceAll(std::string text, std::string_view from, std::string_view to);

} // namespace odysseus::core
