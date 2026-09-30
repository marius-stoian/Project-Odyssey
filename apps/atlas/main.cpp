// odysseus_atlas.exe: turns the owner's sprite sheets into game-ready atlases (US-120).
//   odysseus_atlas                         cut every frame listed in assets/sprites/cuts.json and
//                                          write assets/sprites/atlas/ (atlas PNGs and atlas.json)
//   --preview <file.png>                   also write a contact sheet of every frame, for review
//   --find <sheet.png> <x> <y> <w> <h> [tolerance] [minSize]
//                                          list the figures (blobs) in a region of a sheet, to
//                                          measure the rectangles for cuts.json
//   --content-preview <folder>             also write, per content page, a numbered sheet
//                                          (<page>.png) and its names (<page>.md), for review
//   --sprites <folder>                     where the sheets are (default: assets/sprites)
//   It also cuts the M2d content (US-130) listed in assets/sprites/content-cuts.json into
//   assets/sprites/atlas/content-*.png and content.json.
#include "game/art.h"
#include "game/content_art.h"

#include "luna/engine/image_io.h"
#include "luna/engine/image_ops.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <string>
#include <vector>

namespace {

int find(const std::filesystem::path& sheetFile, const std::vector<std::string>& numbers) {
    std::string error;
    const auto sheet = luna::engine::loadPng(sheetFile, error);
    if (!sheet) {
        std::cerr << sheetFile.string() << " " << error << '\n';
        return 1;
    }
    const odysseus::core::Rect area{std::stoi(numbers.at(0)), std::stoi(numbers.at(1)), std::stoi(numbers.at(2)), std::stoi(numbers.at(3))};
    const int tolerance = numbers.size() > 4 ? std::stoi(numbers[4]) : 24;
    const int minSize = numbers.size() > 5 ? std::stoi(numbers[5]) : 20;
    const luna::engine::Color background = sheet->get(area.x, area.y);
    std::cout << std::format("{}: {}x{}, background ({}, {}, {})\n", sheetFile.filename().string(), sheet->width(), sheet->height(),
                             background.red, background.green, background.blue);
    for (const auto& blob : luna::engine::findBlobs(*sheet, area, background, tolerance, minSize)) {
        std::cout << std::format("  [{}, {}, {}, {}]\n", blob.x, blob.y, blob.width, blob.height);
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::vector<std::string> args(argv + 1, argv + argc);
        std::filesystem::path sprites = std::filesystem::path(ODYSSEUS_ASSETS_DIR) / "sprites";
        std::filesystem::path preview;
        std::filesystem::path contentPreview;
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--find" && i + 5 < args.size()) { // a sheet and four numbers follow
                const std::vector<std::string> numbers(args.begin() + static_cast<std::ptrdiff_t>(i) + 2, args.end());
                return find(args.at(i + 1), numbers);
            }
            if (args[i] == "--preview" && i + 1 < args.size()) {
                preview = args[++i];
            } else if (args[i] == "--content-preview" && i + 1 < args.size()) {
                contentPreview = args[++i];
            } else if (args[i] == "--sprites" && i + 1 < args.size()) {
                sprites = args[++i];
            } else {
                std::cerr << "Usage: odysseus_atlas [--sprites FOLDER] [--preview FILE.png]\n"
                             "       odysseus_atlas --find SHEET.png X Y W H [TOLERANCE] [MINSIZE]\n";
                return 2;
            }
        }
        const auto cuts = odysseus::game::loadCuts(sprites / "cuts.json");
        const auto atlas = odysseus::game::cutAtlas(cuts, sprites);
        odysseus::game::saveAtlas(atlas, sprites / "atlas");
        std::cout << std::format("Wrote {} character frames and {} tiles to {}\n", atlas.characterCount(), atlas.tileCount(),
                                 (sprites / "atlas").string());
        if (!preview.empty()) {
            luna::engine::savePng(odysseus::game::contactSheet(atlas), preview);
            std::cout << "Preview: " << preview.string() << '\n';
        }
        if (std::filesystem::exists(sprites / "content-cuts.json")) {
            const auto contentCuts = odysseus::game::loadContentCuts(sprites / "content-cuts.json");
            const auto content = odysseus::game::cutContent(contentCuts, sprites);
            odysseus::game::saveContent(content, sprites / "atlas");
            std::cout << std::format("Wrote {} content items ({} frames) to {}\n", content.frameCounts.size(), content.frames.size(),
                                     (sprites / "atlas").string());
            if (!contentPreview.empty()) {
                std::filesystem::create_directories(contentPreview);
                for (const auto& page : content.pages) {
                    std::vector<std::string> names;
                    for (const auto& cut : contentCuts.cuts) {
                        if (cut.page == page.name) names.push_back(cut.name);
                    }
                    luna::engine::savePng(odysseus::game::numberedSheet(content, page.name, names), contentPreview / (page.name + ".png"));
                    std::ofstream list(contentPreview / (page.name + ".md"), std::ios::binary | std::ios::trunc);
                    list << "# " << page.name << " (" << names.size() << ")\n\nNumbers match " << page.name << ".png.\n\n";
                    for (std::size_t n = 0; n < names.size(); ++n) list << n + 1 << ". " << names[n] << "\n";
                }
                std::cout << "Content preview: " << contentPreview.string() << '\n';
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
