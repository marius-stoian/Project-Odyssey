#pragma once

#include "boundary.h"

#include "core/geometry.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The Cut tool's work without its screen (US-192, D-05): a rectangle on one of the owner's sheets, named, becomes a cut in cuts.json (a character or a tile) or in
// content-cuts.json (a picture of a content page: a weapon icon, a plant, an animal, an effect), and the atlas is cut again by the same code odysseus_atlas runs.
// The sheets themselves are never changed.
struct CutRequest {
    std::string sheet;   // a file of the sprites folder: "Pixel-Art Animal Sprite Sheet.png"
    core::Rect rect{};   // where in the sheet
    std::string name;    // "grey wolf", "hero.S.0"
    std::string target;  // "character" or "tile" (cuts.json), or the name of a content page ("icons", "animals", ...) of content-cuts.json
};

// The sheets the owner can cut from: the PNG files at the top of the sprites folder, sorted.
std::vector<std::string> cutSheets(const std::filesystem::path& sprites);

// Where a cut can go: "character", "tile" and the pages of content-cuts.json.
std::vector<std::string> cutTargets(const std::filesystem::path& sprites);

// Adds the cut: checks it (the sheet exists and holds the rectangle, the name is new and a name, the target exists), writes the line into the cuts file in the style of the
// lines there, and cuts the atlases again. A cut that makes the atlas fail is taken out again. Returns what is wrong, nothing when it worked; `summary` says what was rebuilt.
std::optional<std::string> addCut(const std::filesystem::path& sprites, const CutRequest& request, std::string* summary = nullptr);

// Cuts cuts.json and content-cuts.json again into `<sprites>/atlas` (and the normal-map atlases next to the pictures when there already are some), as odysseus_atlas does.
// A mistake in the cut lists is a DataError.
std::string rebuildAtlases(const std::filesystem::path& sprites);

} // namespace odysseus::game
