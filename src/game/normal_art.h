#pragma once

#include "boundary.h"

#include "luna/engine/image.h"

#include <filesystem>
#include <optional>
#include <string>

namespace odysseus::game {

// Normal maps for the owner's atlases (US-241). `odysseus_atlas --normals` makes one per atlas picture, in the atlas folder, named with `_n`:
// characters_n.png, tiles_n.png and content-<page>_n.png. A hand-made map named like the frame (`<frame>_n.png`, for example
// `hero.S.0_n.png` or `oak_n.png`, in the sprites folder next to cuts.json) is used for that frame instead of the generated one.
struct NormalReport {
    int atlases = 0;  // pictures written
    int handMade = 0; // frames whose map came from a hand-made file
};

// Reads the atlas folder (`<sprites>/atlas`) written by odysseus_atlas and writes the matching normal atlases next to the pictures. A hand-made file of
// the wrong size is a DataError naming it. Throws sim::DataError when the atlas cannot be read.
NormalReport writeNormalAtlases(const std::filesystem::path& spritesFolder);

// `<folder>/<name>_n.png` when it exists and has the size of `base`; nothing otherwise. A missing or wrong map is not an error: the sprites are then
// lit flat (a note goes to `note` when the file was there but did not fit).
std::optional<luna::engine::Image> loadNormalAtlas(const std::filesystem::path& folder, const std::string& name, const luna::engine::Image& base,
                                                    std::string* note = nullptr);

} // namespace odysseus::game
