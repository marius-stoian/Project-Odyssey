#include "game/content_art.h"

#include "game/normal_art.h"

#include "luna/engine/image_io.h"
#include "luna/engine/image_ops.h"
#include "luna/engine/ui.h"
#include "sim/data.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <fstream>
#include <set>

namespace odysseus::game {

using luna::engine::Image;
using nlohmann::json;

namespace {

core::Rect cellRect(int cell, int width, int height) {
    return {cell % kContentColumns * width, cell / kContentColumns * height, width, height};
}

// [x, y, width, height] in whole pixels, width and height above 0.
std::optional<core::Rect> readRect(const json& value) {
    if (!value.is_array() || value.size() != 4 ||
        !std::all_of(value.begin(), value.end(), [](const json& v) { return v.is_number_integer() && v.get<int>() >= 0; }) ||
        value.at(2).get<int>() == 0 || value.at(3).get<int>() == 0) {
        return std::nullopt;
    }
    return core::Rect{value.at(0).get<int>(), value.at(1).get<int>(), value.at(2).get<int>(), value.at(3).get<int>()};
}

// The colour seen most often on the picture's border: its background, even when a corner
// happens to touch the item.
luna::engine::Color borderColour(const Image& picture) {
    std::map<std::uint32_t, int> seen;
    auto count = [&](int x, int y) {
        const auto c = picture.get(x, y);
        ++seen[(static_cast<std::uint32_t>(c.red) << 16) | (static_cast<std::uint32_t>(c.green) << 8) | c.blue];
    };
    for (int x = 0; x < picture.width(); ++x) {
        count(x, 0);
        count(x, picture.height() - 1);
    }
    for (int y = 0; y < picture.height(); ++y) {
        count(0, y);
        count(picture.width() - 1, y);
    }
    const auto most = std::max_element(seen.begin(), seen.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    return {static_cast<std::uint8_t>(most->first >> 16), static_cast<std::uint8_t>(most->first >> 8), static_cast<std::uint8_t>(most->first)};
}

// One frame: cut, key the background away, keep the item, trim and scale into the cell.
Image makeFrame(const Image& sheet, const ContentCut& cut, const core::Rect& area, const ContentPage& page) {
    Image picture = luna::engine::crop(sheet, area);
    switch (cut.key) {
    case Backdrop::Flood: luna::engine::removeBackground(picture, cut.tolerance); break;
    case Backdrop::Colour: luna::engine::removeColour(picture, borderColour(picture), cut.tolerance); break;
    case Backdrop::Alpha: luna::engine::keyAlpha(picture, 128, true); break;
    case Backdrop::AlphaSoft: luna::engine::keyAlpha(picture, 24, false); break;
    case Backdrop::Brightness: luna::engine::keyBrightness(picture, picture.get(0, 0), 3); break;
    }
    if (cut.focus) {
        // The focus is given in sheet pixels; the picture starts at the area's corner.
        luna::engine::keepMainFigure(picture, {cut.focus->x - area.x, cut.focus->y - area.y, cut.focus->width, cut.focus->height});
    }
    if (page.trim) {
        const core::Rect used = luna::engine::opaqueBounds(picture);
        if (used.width > 0 && used.height > 0) picture = luna::engine::crop(picture, used);
    }
    return luna::engine::fitInto(picture, page.cellWidth, page.cellHeight, page.fit == Fit::Bottom);
}

} // namespace

const ContentPage* ContentAtlas::page(const std::string& name) const {
    const auto it = std::find_if(pages.begin(), pages.end(), [&](const ContentPage& p) { return p.name == name; });
    return it == pages.end() ? nullptr : &*it;
}

std::optional<core::Rect> ContentAtlas::rect(const std::string& frame) const {
    const auto it = frames.find(frame);
    if (it == frames.end()) return std::nullopt;
    const ContentPage* on = page(it->second.page);
    if (on == nullptr) return std::nullopt;
    return cellRect(it->second.cell, on->cellWidth, on->cellHeight);
}

std::string ContentAtlas::frameName(const std::string& item, int index) const {
    const auto it = frameCounts.find(item);
    if (it == frameCounts.end() || it->second <= 1) return item;
    return std::format("{}.{}", item, std::clamp(index, 0, it->second - 1));
}

ContentCuts loadContentCuts(const std::filesystem::path& file) {
    const json data = sim::readJsonFile(file);
    ContentCuts list;
    if (!data.contains("pages") || !data.at("pages").is_object() || data.at("pages").empty()) {
        throw sim::DataError(file, "pages", "must name every page with its cell size");
    }
    for (const auto& [name, entry] : data.at("pages").items()) {
        ContentPage page;
        page.name = name;
        const json& cell = entry.value("cell", json());
        if (!cell.is_array() || cell.size() != 2 || !cell.at(0).is_number_integer() || !cell.at(1).is_number_integer() || cell.at(0).get<int>() < 8 ||
            cell.at(1).get<int>() < 8 || cell.at(0).get<int>() > 256 || cell.at(1).get<int>() > 256) {
            throw sim::DataError(file, "pages." + name + ".cell", "must be [width, height], each 8 to 256 pixels");
        }
        page.cellWidth = cell.at(0).get<int>();
        page.cellHeight = cell.at(1).get<int>();
        const std::string fit = entry.value("fit", std::string("centre"));
        if (fit != "centre" && fit != "bottom") {
            throw sim::DataError(file, "pages." + name + ".fit", "must be \"centre\" or \"bottom\"");
        }
        page.fit = fit == "bottom" ? Fit::Bottom : Fit::Centre;
        page.trim = entry.value("trim", true);
        list.pages.push_back(page);
    }
    if (!data.contains("cuts") || !data.at("cuts").is_array()) {
        throw sim::DataError(file, "cuts", "must be a list of items to cut");
    }
    std::set<std::string> names;
    for (std::size_t i = 0; i < data.at("cuts").size(); ++i) {
        const json& entry = data.at("cuts").at(i);
        const std::string where = std::format("cuts[{}]", i);
        ContentCut cut;
        if (!entry.contains("name") || !entry.at("name").is_string() || entry.at("name").get<std::string>().empty()) {
            throw sim::DataError(file, where + ".name", "must be a name in quotes");
        }
        cut.name = entry.at("name").get<std::string>();
        if (!names.insert(cut.name).second) {
            throw sim::DataError(file, where + ".name", "\"" + cut.name + "\" is listed twice");
        }
        cut.page = entry.value("page", std::string());
        if (std::none_of(list.pages.begin(), list.pages.end(), [&](const ContentPage& p) { return p.name == cut.page; })) {
            throw sim::DataError(file, where + ".page", "\"" + cut.page + "\" is not one of the pages");
        }
        if (!entry.contains("sheet") || !entry.at("sheet").is_string()) {
            throw sim::DataError(file, where + ".sheet", "must be the file name of a sheet in assets/sprites");
        }
        cut.sheet = entry.at("sheet").get<std::string>();
        const auto rect = readRect(entry.value("rect", json()));
        if (!rect) throw sim::DataError(file, where + ".rect", "must be [x, y, width, height] in whole pixels, width and height above 0");
        cut.rect = *rect;
        const std::string key = entry.value("key", std::string());
        if (key == "flood") cut.key = Backdrop::Flood;
        else if (key == "colour") cut.key = Backdrop::Colour;
        else if (key == "alpha") cut.key = Backdrop::Alpha;
        else if (key == "alpha-soft") cut.key = Backdrop::AlphaSoft;
        else if (key == "luma") cut.key = Backdrop::Brightness;
        else throw sim::DataError(file, where + ".key", "must be \"flood\", \"colour\", \"alpha\", \"alpha-soft\" or \"luma\"");
        if (entry.contains("tolerance")) {
            const json& tolerance = entry.at("tolerance");
            if (!tolerance.is_number_integer() || tolerance.get<int>() < 0 || tolerance.get<int>() > 255) {
                throw sim::DataError(file, where + ".tolerance", "must be a whole number from 0 to 255");
            }
            cut.tolerance = tolerance.get<int>();
        }
        if (entry.contains("focus")) {
            cut.focus = readRect(entry.at("focus"));
            if (!cut.focus) throw sim::DataError(file, where + ".focus", "must be [x, y, width, height] in whole pixels");
        }
        if (entry.contains("frameRects")) {
            const json& frames = entry.at("frameRects");
            if (!frames.is_array() || frames.empty()) throw sim::DataError(file, where + ".frameRects", "must be a list of rectangles");
            for (std::size_t f = 0; f < frames.size(); ++f) {
                const auto frame = readRect(frames.at(f));
                if (!frame) throw sim::DataError(file, std::format("{}.frameRects[{}]", where, f), "must be [x, y, width, height] in whole pixels");
                cut.frames.push_back(*frame);
            }
        }
        list.cuts.push_back(cut);
    }
    return list;
}

ContentAtlas cutContent(const ContentCuts& list, const std::filesystem::path& spritesFolder) {
    ContentAtlas atlas;
    atlas.pages = list.pages;
    std::map<std::string, Image> sheets;
    std::map<std::string, std::vector<Image>> byPage;
    for (const ContentCut& cut : list.cuts) {
        if (!sheets.contains(cut.sheet)) {
            std::string error;
            auto sheet = luna::engine::loadPng(spritesFolder / cut.sheet, error);
            if (!sheet) throw sim::DataError(spritesFolder / cut.sheet, "(file)", error);
            sheets.emplace(cut.sheet, std::move(*sheet));
        }
        const Image& sheet = sheets.at(cut.sheet);
        const ContentPage& page = *atlas.page(cut.page);
        std::vector<Image>& cells = byPage[cut.page];
        const std::vector<core::Rect> areas = cut.frames.empty() ? std::vector<core::Rect>{cut.rect} : cut.frames;
        for (std::size_t f = 0; f < areas.size(); ++f) {
            const std::string name = areas.size() == 1 ? cut.name : std::format("{}.{}", cut.name, f);
            atlas.frames[name] = {cut.page, static_cast<int>(cells.size())};
            cells.push_back(makeFrame(sheet, cut, areas[f], page));
        }
        atlas.frameCounts[cut.name] = static_cast<int>(areas.size());
    }
    for (const ContentPage& page : atlas.pages) {
        const std::vector<Image>& cells = byPage[page.name];
        const int rows = std::max(1, (static_cast<int>(cells.size()) + kContentColumns - 1) / kContentColumns);
        Image picture(kContentColumns * page.cellWidth, rows * page.cellHeight);
        for (std::size_t i = 0; i < cells.size(); ++i) {
            const core::Rect at = cellRect(static_cast<int>(i), page.cellWidth, page.cellHeight);
            for (int y = 0; y < page.cellHeight; ++y) {
                for (int x = 0; x < page.cellWidth; ++x) picture.set(at.x + x, at.y + y, cells[i].get(x, y));
            }
        }
        atlas.pictures.emplace(page.name, std::move(picture));
    }
    return atlas;
}

void saveContent(const ContentAtlas& atlas, const std::filesystem::path& folder) {
    std::filesystem::create_directories(folder);
    json pages = json::object();
    for (const ContentPage& page : atlas.pages) {
        const std::string file = "content-" + page.name + ".png";
        if (!luna::engine::savePng(atlas.pictures.at(page.name), folder / file)) {
            throw sim::DataError(folder / file, "(file)", "the page picture cannot be written");
        }
        pages[page.name] = {{"file", file}, {"cell", {page.cellWidth, page.cellHeight}}, {"fit", page.fit == Fit::Bottom ? "bottom" : "centre"}, {"trim", page.trim}};
    }
    json frames = json::object();
    for (const auto& [name, frame] : atlas.frames) frames[name] = {frame.page, frame.cell};
    const json index{{"contentVersion", 1}, {"columns", kContentColumns}, {"pages", pages}, {"frames", frames}, {"frameCounts", atlas.frameCounts}};
    std::ofstream(folder / "content.json", std::ios::binary | std::ios::trunc) << index.dump(1) << '\n';
}

std::optional<ContentAtlas> loadContent(const std::filesystem::path& folder, std::string& problem) {
    ContentAtlas atlas;
    const std::filesystem::path indexFile = folder / "content.json";
    try {
        const json index = sim::readJsonFile(indexFile);
        if (index.value("contentVersion", 0) != 1 || index.value("columns", 0) != kContentColumns) {
            problem = indexFile.string() + ": contentVersion or columns is not what this game reads";
            return std::nullopt;
        }
        for (const auto& [name, entry] : index.at("pages").items()) {
            ContentPage page{name, entry.at("cell").at(0).get<int>(), entry.at("cell").at(1).get<int>(),
                             entry.value("fit", std::string("centre")) == "bottom" ? Fit::Bottom : Fit::Centre, entry.value("trim", true)};
            std::string error;
            auto picture = luna::engine::loadPng(folder / entry.at("file").get<std::string>(), error);
            if (!picture) {
                problem = (folder / entry.at("file").get<std::string>()).string() + " " + error;
                return std::nullopt;
            }
            if (auto normals = loadNormalAtlas(folder, "content-" + name, *picture)) atlas.normals.emplace(name, std::move(*normals));
            atlas.pictures.emplace(name, std::move(*picture));
            atlas.pages.push_back(page);
        }
        for (const auto& [name, entry] : index.at("frames").items()) {
            atlas.frames[name] = {entry.at(0).get<std::string>(), entry.at(1).get<int>()};
        }
        atlas.frameCounts = index.at("frameCounts").get<std::map<std::string, int>>();
    } catch (const std::exception& error) {
        problem = error.what();
        return std::nullopt;
    }
    for (const auto& [name, frame] : atlas.frames) {
        const auto at = atlas.rect(name);
        const auto picture = atlas.pictures.find(frame.page);
        if (!at || picture == atlas.pictures.end() || frame.cell < 0 || at->x + at->width > picture->second.width() ||
            at->y + at->height > picture->second.height()) {
            problem = std::format("{}: frame \"{}\" lies outside its page \"{}\"", indexFile.string(), name, frame.page);
            return std::nullopt;
        }
    }
    return atlas;
}

Image numberedSheet(const ContentAtlas& atlas, const std::string& pageName, const std::vector<std::string>& names) {
    const ContentPage* page = atlas.page(pageName);
    if (page == nullptr) return Image(1, 1);
    const int zoom = page->cellWidth <= 32 ? 3 : 2;
    const int gap = 6;
    const int label = luna::engine::kGlyphHeight * 2 + 4;
    const int cols = 10;
    const int boxW = page->cellWidth * zoom + gap;
    const int boxH = page->cellHeight * zoom + label + gap;
    const int rows = std::max(1, (static_cast<int>(names.size()) + cols - 1) / cols);
    Image out(cols * boxW + gap, rows * boxH + gap);
    for (int y = 0; y < out.height(); ++y) {
        for (int x = 0; x < out.width(); ++x) {
            const bool light = ((x / 8) + (y / 8)) % 2 == 0;
            out.set(x, y, light ? luna::engine::Color{200, 200, 200} : luna::engine::Color{160, 160, 160});
        }
    }
    const Image& picture = atlas.pictures.at(pageName);
    for (std::size_t i = 0; i < names.size(); ++i) {
        const int ox = gap + static_cast<int>(i % cols) * boxW;
        const int oy = gap + static_cast<int>(i / cols) * boxH;
        const auto at = atlas.rect(atlas.frameName(names[i], 0));
        if (at) {
            for (int y = 0; y < at->height * zoom; ++y) {
                for (int x = 0; x < at->width * zoom; ++x) {
                    const auto c = picture.get(at->x + x / zoom, at->y + y / zoom);
                    if (c.alpha > 0) {
                        const auto under = out.get(ox + x, oy + y);
                        const int a = c.alpha;
                        out.set(ox + x, oy + y, {static_cast<std::uint8_t>((c.red * a + under.red * (255 - a)) / 255),
                                                 static_cast<std::uint8_t>((c.green * a + under.green * (255 - a)) / 255),
                                                 static_cast<std::uint8_t>((c.blue * a + under.blue * (255 - a)) / 255)});
                    }
                }
            }
        }
        // The item's number, twice the font size, under its picture.
        const std::string number = std::to_string(i + 1);
        for (std::size_t k = 0; k < number.size(); ++k) {
            for (int gy = 0; gy < luna::engine::kGlyphHeight * 2; ++gy) {
                for (int gx = 0; gx < luna::engine::kGlyphWidth * 2; ++gx) {
                    if (luna::engine::glyphPixel(number[k], gx / 2, gy / 2)) {
                        out.set(ox + static_cast<int>(k) * luna::engine::kTextAdvance * 2 + gx, oy + page->cellHeight * zoom + 2 + gy, luna::engine::Color{20, 20, 20});
                    }
                }
            }
        }
    }
    return out;
}

} // namespace odysseus::game
