#include "game/object_art.h"

#include <algorithm>
#include <cmath>

namespace odysseus::game {

namespace {

using luna::engine::Color;
using luna::engine::Image;

constexpr Color kStone{128, 128, 134, 255};
constexpr Color kStoneDark{84, 84, 90, 255};
constexpr Color kStoneLight{170, 170, 176, 255};
constexpr Color kAsh{52, 46, 44, 255};
constexpr Color kEmber{232, 120, 40, 255};
constexpr Color kWood{128, 88, 52, 255};
constexpr Color kWoodDark{86, 56, 32, 255};
constexpr Color kHide{176, 140, 96, 255};
constexpr Color kBerry{196, 40, 60, 255};
constexpr Color kWater{70, 130, 200, 255};
constexpr Color kWaterLight{140, 190, 236, 255};
constexpr Color kFur{150, 108, 72, 255};
constexpr Color kFurDark{104, 70, 46, 255};
constexpr Color kFlint{60, 64, 72, 255};

// A filled ellipse inside the cell that starts at (ox, oy).
void ellipse(Image& image, int ox, int oy, int cx, int cy, int rx, int ry, Color color) {
    for (int y = -ry; y <= ry; ++y) {
        for (int x = -rx; x <= rx; ++x) {
            if (x * x * ry * ry + y * y * rx * rx <= rx * rx * ry * ry) image.set(ox + cx + x, oy + cy + y, color);
        }
    }
}

void box(Image& image, int ox, int oy, int x, int y, int w, int h, Color color) {
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) image.set(ox + x + i, oy + y + j, color);
    }
}

// The feet of an object are the middle of the bottom edge (28 of 32), like a plant's.
void drawOne(Image& image, int ox, const std::string& frame) {
    const int oy = 0;
    if (frame == "fire-pit") {
        ellipse(image, ox, oy, 16, 24, 12, 6, kStone);
        ellipse(image, ox, oy, 16, 24, 8, 4, kAsh);
        for (const auto& [x, y] : {std::pair{12, 23}, std::pair{16, 25}, std::pair{20, 23}, std::pair{15, 22}}) box(image, ox, oy, x, y, 2, 2, kEmber);
        for (const auto& [x, y] : {std::pair{4, 24}, std::pair{27, 24}, std::pair{10, 29}, std::pair{22, 29}}) ellipse(image, ox, oy, x, y, 3, 2, kStoneDark);
    } else if (frame == "knapping-stone") {
        ellipse(image, ox, oy, 16, 24, 11, 5, kStoneDark);
        ellipse(image, ox, oy, 16, 22, 11, 5, kStone);
        ellipse(image, ox, oy, 14, 21, 6, 2, kStoneLight);
        for (const auto& [x, y] : {std::pair{24, 27}, std::pair{6, 28}, std::pair{27, 24}}) box(image, ox, oy, x, y, 2, 2, kFlint);
    } else if (frame == "food-store") {
        box(image, ox, oy, 7, 14, 18, 14, kWood);
        box(image, ox, oy, 7, 14, 18, 3, kWoodDark);
        box(image, ox, oy, 7, 24, 18, 2, kWoodDark);
        for (const auto& [x, y] : {std::pair{10, 11}, std::pair{14, 10}, std::pair{18, 11}, std::pair{21, 12}, std::pair{12, 13}}) ellipse(image, ox, oy, x, y, 2, 2, kBerry);
    } else if (frame == "shelter") {
        for (int row = 0; row < 20; ++row) box(image, ox, oy, 16 - (3 + row / 2 + 2), 8 + row, 2 * (3 + row / 2 + 2), 1, row % 4 == 3 ? kWoodDark : kHide);
        box(image, ox, oy, 12, 18, 8, 10, kAsh);
        box(image, ox, oy, 4, 27, 24, 2, kWoodDark);
    } else if (frame == "sun") {
        ellipse(image, ox, oy, 16, 16, 9, 9, Color{255, 236, 140, 255});
        ellipse(image, ox, oy, 16, 16, 6, 6, Color{255, 250, 205, 255});
        for (const auto& [x, y] : {std::pair{15, 1}, std::pair{15, 27}, std::pair{1, 15}, std::pair{29, 15}}) box(image, ox, oy, x, y, 2, 4, Color{255, 220, 100, 255});
    } else if (frame == "moon") {
        ellipse(image, ox, oy, 16, 16, 9, 9, Color{222, 228, 240, 255});
        ellipse(image, ox, oy, 19, 14, 7, 7, Color{196, 204, 222, 255}); // the shaded side
        for (const auto& [x, y] : {std::pair{12, 18}, std::pair{15, 22}, std::pair{11, 12}}) ellipse(image, ox, oy, x, y, 2, 2, Color{180, 188, 208, 255});
    } else if (frame == "flint-nodule") {
        ellipse(image, ox, oy, 16, 23, 8, 5, kFlint);
        ellipse(image, ox, oy, 13, 21, 3, 2, kStone);
        for (const auto& [x, y] : {std::pair{19, 24}, std::pair{21, 22}, std::pair{11, 25}}) box(image, ox, oy, x, y, 1, 1, kStoneLight);
    } else if (frame == "water-source") {
        ellipse(image, ox, oy, 16, 22, 14, 7, kStone);
        ellipse(image, ox, oy, 16, 22, 12, 5, kWater);
        for (const auto& [x, y] : {std::pair{12, 21}, std::pair{18, 23}, std::pair{22, 21}}) box(image, ox, oy, x, y, 4, 1, kWaterLight);
    } else if (frame == "sleeping-furs") {
        ellipse(image, ox, oy, 16, 24, 13, 5, kFur);
        ellipse(image, ox, oy, 16, 22, 11, 4, kHide);
        box(image, ox, oy, 6, 22, 20, 2, kFurDark);
        ellipse(image, ox, oy, 8, 21, 3, 2, kFurDark);
    } else {
        box(image, ox, oy, 8, 12, 16, 16, kStone); // an object the game has no picture for yet
        box(image, ox, oy, 8, 12, 16, 2, kStoneLight);
    }
}

} // namespace

Image makeObjectPage(const std::vector<const PlantDef*>& objects, std::map<std::string, odysseus::core::Rect>& rects) {
    const int count = std::max(1, static_cast<int>(objects.size()));
    Image image(count * kObjectPictureSize, kObjectPictureSize);
    for (std::size_t i = 0; i < objects.size(); ++i) {
        const int ox = static_cast<int>(i) * kObjectPictureSize;
        drawOne(image, ox, objects[i]->frame);
        rects[objects[i]->name] = {ox, 0, kObjectPictureSize, kObjectPictureSize};
    }
    return image;
}

} // namespace odysseus::game
