#pragma once

#include "boundary.h"

#include "image.h"

#include <filesystem>
#include <optional>
#include <string>

namespace luna::engine {

// Reads a PNG file (any colour type; the result is always RGBA). On failure returns nothing
// and says why in `error` (missing file, not a PNG, damaged data).
std::optional<Image> loadPng(const std::filesystem::path& file, std::string& error);

// Writes the image as an RGBA PNG. Returns false when the file cannot be written.
bool savePng(const Image& image, const std::filesystem::path& file);

} // namespace luna::engine
