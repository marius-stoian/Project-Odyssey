#include "game/placeholder_art.h"

#include <cstdint>

namespace odysseus::game {

namespace {

using luna::engine::Color;
using luna::engine::Image;

constexpr Color kOutline{40, 30, 30};
constexpr Color kSkin{224, 172, 128};
constexpr Color kHair{96, 58, 36};
constexpr Color kTunic{70, 110, 160};
constexpr Color kTunicShade{52, 84, 126};
constexpr Color kBelt{120, 84, 48};
constexpr Color kLegs{88, 72, 56};
constexpr Color kEye{30, 30, 40};

// Horizontal and vertical "looking" direction for each facing, -1/0/+1.
struct Look {
    int x;
    int y;
};

Look lookOf(Facing facing) {
    switch (facing) {
    case Facing::South: return {0, 1};
    case Facing::SouthWest: return {-1, 1};
    case Facing::West: return {-1, 0};
    case Facing::NorthWest: return {-1, -1};
    case Facing::North: return {0, -1};
    case Facing::NorthEast: return {1, -1};
    case Facing::East: return {1, 0};
    case Facing::SouthEast: return {1, 1};
    default: return {0, 1};
    }
}

// One 32x48 figure with its top-left corner at (left, top).
void drawFigure(Image& sheet, int left, int top, Facing facing, int frame) {
    const Look look = lookOf(facing);
    // Walking: the legs swing (frames 1 and 3) and the body bobs one pixel.
    const int stride = frame == 1 ? 3 : (frame == 3 ? -3 : 0);
    const int bob = (frame == 1 || frame == 3) ? -1 : 0;
    const int body = top + bob;

    // Legs (drawn first, the tunic overlaps them).
    sheet.fillRect(left + 11 + stride, top + 36, 4, 11, kLegs);
    sheet.fillRect(left + 17 - stride, top + 36, 4, 11, kLegs);
    sheet.fillRect(left + 10 + stride, top + 46, 6, 2, kOutline); // shoes
    sheet.fillRect(left + 16 - stride, top + 46, 6, 2, kOutline);

    // Tunic and belt.
    sheet.fillRect(left + 8, body + 20, 16, 18, kTunic);
    sheet.fillRect(left + 8, body + 20, 16, 2, kTunicShade);
    sheet.fillRect(left + 8, body + 30, 16, 2, kBelt);
    // Arms swing opposite to the legs.
    sheet.fillRect(left + 5, body + 21 - stride / 2, 3, 12, kTunicShade);
    sheet.fillRect(left + 24, body + 21 + stride / 2, 3, 12, kTunicShade);
    sheet.fillRect(left + 5, body + 33 - stride / 2, 3, 3, kSkin);
    sheet.fillRect(left + 24, body + 33 + stride / 2, 3, 3, kSkin);

    // Head: a 14x16 face, shifted a little towards where the character looks.
    const int headLeft = left + 9 + look.x;
    sheet.fillRect(headLeft - 1, body + 3, 16, 18, kOutline);
    sheet.fillRect(headLeft, body + 4, 14, 16, kSkin);
    // Hair covers the top; seen from behind (looking north) it covers the whole head.
    const int hairRows = look.y < 0 ? 16 : 6;
    sheet.fillRect(headLeft, body + 4, 14, hairRows, kHair);
    if (look.y >= 0) {
        // Eyes: two when facing forward, one when seen from the side.
        if (look.x == 0) {
            sheet.fillRect(headLeft + 3, body + 12, 2, 3, kEye);
            sheet.fillRect(headLeft + 9, body + 12, 2, 3, kEye);
        } else {
            const int eyeX = look.x < 0 ? headLeft + 2 : headLeft + 10;
            sheet.fillRect(eyeX, body + 12, 2, 3, kEye);
        }
    }
}

// A little per-pixel variety so tiles do not look like flat paint. Deterministic.
std::uint32_t noise(int x, int y) {
    std::uint32_t value = static_cast<std::uint32_t>(x) * 374761393U + static_cast<std::uint32_t>(y) * 668265263U;
    value = (value ^ (value >> 13)) * 1274126177U;
    return value ^ (value >> 16);
}

void drawTile(Image& sheet, int left, TileKind kind) {
    for (int y = 0; y < kTileSize; ++y) {
        for (int x = 0; x < kTileSize; ++x) {
            const int speck = static_cast<int>(noise(x + left, y) % 16U);
            Color color;
            switch (kind) {
            case TileKind::Grass:
                color = speck < 2 ? Color{110, 160, 70} : Color{86, 138, 60};
                break;
            case TileKind::Path:
                color = speck < 3 ? Color{176, 150, 104} : Color{160, 134, 92};
                break;
            case TileKind::Rock: {
                // A grey boulder on grass.
                const int dx = x - 16;
                const int dy = y - 17;
                const bool inside = dx * dx + dy * dy * 3 / 2 < 13 * 13;
                color = inside ? (speck < 4 ? Color{150, 150, 158} : Color{120, 120, 130}) : Color{86, 138, 60};
                if (inside && y > 22) {
                    color = Color{96, 96, 106}; // shadowed underside
                }
                break;
            }
            case TileKind::Water:
                color = speck < 2 ? Color{110, 170, 220} : Color{64, 120, 190};
                break;
            default:
                break;
            }
            sheet.set(left + x, y, color);
        }
    }
}

} // namespace

luna::engine::Image makeCharacterSheet() {
    const int directions = static_cast<int>(Facing::Count);
    Image sheet(kCharacterWidth * kWalkFrames, kCharacterHeight * directions);
    for (int row = 0; row < directions; ++row) {
        for (int frame = 0; frame < kWalkFrames; ++frame) {
            drawFigure(sheet, frame * kCharacterWidth, row * kCharacterHeight, static_cast<Facing>(row), frame);
        }
    }
    return sheet;
}

luna::engine::Image makeTileSheet() {
    const int kinds = static_cast<int>(TileKind::Count);
    Image sheet(kTileSize * kinds, kTileSize);
    for (int kind = 0; kind < kinds; ++kind) {
        drawTile(sheet, kind * kTileSize, static_cast<TileKind>(kind));
    }
    return sheet;
}

} // namespace odysseus::game
