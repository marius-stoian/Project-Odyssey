#pragma once

#include "boundary.h"

#include "core/geometry.h"
#include "luna/engine/image.h"

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The M2d content art (US-130): weapons, plants, animals, effects and weather, cut from the
// owner's seven new sheets. It lives beside the M2c atlas (art.h), in pages of equal cells, one
// page per size: assets/sprites/content-cuts.json says what to cut, odysseus_atlas cuts it, and
// the game reads assets/sprites/atlas/content.json and its pictures.

enum class Fit { Centre, Bottom };

struct ContentPage {
    std::string name;       // "icons"
    int cellWidth = 32;
    int cellHeight = 32;
    Fit fit = Fit::Centre;  // Bottom: the figure stands on the cell's bottom row (feet)
    bool trim = true;       // cut away the empty border before scaling
};

// How a cut tells the picture from its background.
enum class Backdrop {
    Flood,      // a plain background colour, flooded from the border
    Colour,     // a plain background colour removed everywhere, holes too (the weapon sheets)
    Alpha,      // the sheet is transparent already; crisp edges (plants, animals)
    AlphaSoft,  // the sheet is transparent already; soft glows kept (animated effects)
    Brightness, // light on a dark background: brighter = more opaque (still effects, weather)
};

struct ContentCut {
    std::string name;                     // "iron sword"; frames of an animation get ".0", ".1", ...
    std::string page;
    std::string sheet;
    core::Rect rect{};                    // the whole item on the sheet
    Backdrop key = Backdrop::Flood;
    int tolerance = 40;                   // Backdrop::Flood and Backdrop::Colour
    std::optional<core::Rect> focus;      // keep only pixel groups whose middle lies inside
    std::vector<core::Rect> frames;       // an animation: one rectangle per frame (empty: one frame)
};

struct ContentCuts {
    std::vector<ContentPage> pages;
    std::vector<ContentCut> cuts;
};

// Reads content-cuts.json; every problem is a DataError naming the file and the field.
ContentCuts loadContentCuts(const std::filesystem::path& file);

inline constexpr int kContentColumns = 16; // cells per page row

// Where a frame is: its page and its cell there.
struct ContentFrame {
    std::string page;
    int cell = 0;
    friend bool operator==(const ContentFrame&, const ContentFrame&) = default;
};

struct ContentAtlas {
    std::vector<ContentPage> pages;
    std::map<std::string, luna::engine::Image> pictures;  // page name -> its picture
    std::map<std::string, luna::engine::Image> normals;   // page name -> its normal map (US-241); a page without one is lit flat
    std::map<std::string, ContentFrame> frames;            // "iron sword", "spark.0", ...
    std::map<std::string, int> frameCounts;                // item name -> 1, or its animation's length

    const ContentPage* page(const std::string& name) const;
    // The frame's rectangle in its page's picture; nothing when the frame is unknown.
    std::optional<core::Rect> rect(const std::string& frame) const;
    // The name of frame `index` of an item: "iron sword" for a still, "spark.2" for an animation.
    std::string frameName(const std::string& item, int index) const;
};

// Cuts every item from the sheets in `spritesFolder`. A sheet that cannot be read is a DataError.
ContentAtlas cutContent(const ContentCuts& cuts, const std::filesystem::path& spritesFolder);
// Writes content-<page>.png for every page and content.json into `folder`.
void saveContent(const ContentAtlas& atlas, const std::filesystem::path& folder);
// Reads what saveContent wrote. On any problem returns nothing and says which file and why.
std::optional<ContentAtlas> loadContent(const std::filesystem::path& folder, std::string& problem);
// One page's frames enlarged on a checkerboard, each numbered (1, 2, ...) in reading order, for
// the owner's review; the numbers match the order of `names`.
luna::engine::Image numberedSheet(const ContentAtlas& atlas, const std::string& page, const std::vector<std::string>& names);

} // namespace odysseus::game
