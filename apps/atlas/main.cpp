// odysseus_atlas.exe: turns the owner's sprite sheets into game-ready atlases (US-120).
//   odysseus_atlas                         cut every frame listed in assets/sprites/cuts.json and
//                                          write assets/sprites/atlas/ (atlas PNGs and atlas.json)
//   --preview <file.png>                   also write a contact sheet of every frame, for review
//   --find <sheet.png> <x> <y> <w> <h> [tolerance] [minSize]
//                                          list the figures (blobs) in a region of a sheet, to
//                                          measure the rectangles for cuts.json
//   --sprites <folder>                     where the sheets are (default: assets/sprites)
#include "game/art.h"

#include "luna/engine/image_io.h"
#include "luna/engine/image_ops.h"

#include <exception>
#include <filesystem>
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
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--find" && i + 5 < args.size()) { // a sheet and four numbers follow
                const std::vector<std::string> numbers(args.begin() + static_cast<std::ptrdiff_t>(i) + 2, args.end());
                return find(args.at(i + 1), numbers);
            }
            if (args[i] == "--preview" && i + 1 < args.size()) {
                preview = args[++i];
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
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
