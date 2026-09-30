#include "game/placeholder_art.h"

#include <cmath>
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

constexpr Color kShaft{150, 104, 60};
constexpr Color kShaftDark{110, 74, 42};
constexpr Color kFlint{70, 74, 86};
constexpr Color kFlintEdge{150, 156, 170};
constexpr Color kStraw{222, 190, 96};
constexpr Color kStrawDark{178, 144, 64};
constexpr Color kRing{176, 52, 44};
constexpr Color kPost{104, 72, 44};

// A spear in a 32x32 cell, pointing along (dx, dy): a shaft with the tip at the front.
void drawSpear(Image& sheet, int left, int top, double dx, double dy, bool flintTip) {
    const double size = std::sqrt(dx * dx + dy * dy);
    dx /= size;
    dy /= size;
    for (double t = -13.0; t <= 13.0; t += 0.25) {
        const int x = left + 16 + static_cast<int>(std::lround(dx * t));
        const int y = top + 16 + static_cast<int>(std::lround(dy * t));
        const bool tip = t > 8.0;
        Color color = t < -11.0 ? kShaftDark : kShaft;
        if (tip) {
            color = flintTip ? (t > 11.5 ? kFlintEdge : kFlint) : kShaftDark;
        }
        sheet.set(x, y, color);
        if (tip && t < 12.0) {
            // The tip is two pixels wide: a leaf-shaped point.
            sheet.set(x + static_cast<int>(std::lround(-dy)), y + static_cast<int>(std::lround(dx)), color);
        }
    }
}

// A straw bale on a post with painted rings; a hit one has straw sticking out.
void drawTarget(Image& sheet, int left, int top, bool hit) {
    sheet.fillRect(left + 14, top + 30, 4, 18, kPost);
    for (int y = 0; y < 26; ++y) {
        for (int x = 0; x < 24; ++x) {
            const int dx = x - 12;
            const int dy = y - 13;
            if (dx * dx + dy * dy <= 12 * 12) {
                const int ring = static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
                Color color = (ring == 3 || ring == 8) ? kRing : ((x + y) % 5 == 0 ? kStrawDark : kStraw);
                if (ring <= 1) {
                    color = kRing;
                }
                sheet.set(left + 4 + x, top + 4 + y, color);
            }
        }
    }
    if (hit) {
        for (int i = 0; i < 6; ++i) {
            sheet.set(left + 2 + i * 5, top + 2 + (i % 3) * 9, kStrawDark);
            sheet.set(left + 3 + i * 5, top + 3 + (i % 3) * 9, kStraw);
        }
    }
}

} // namespace

luna::engine::Image makePropSheet() {
    Image sheet(8 * kSpearFrameSize, 112);
    for (int row = 0; row < 2; ++row) {
        for (int facing = 0; facing < static_cast<int>(Facing::Count); ++facing) {
            const Look look = lookOf(static_cast<Facing>(facing));
            drawSpear(sheet, facing * kSpearFrameSize, row * kSpearFrameSize, look.x, look.y, row == 0);
        }
    }
    drawTarget(sheet, kTargetFrame.x, kTargetFrame.y, false);
    drawTarget(sheet, kTargetHitFrame.x, kTargetHitFrame.y, true);
    // A soft oval shadow, see-through so the ground shows.
    for (int y = 0; y < kShadowFrame.height; ++y) {
        for (int x = 0; x < kShadowFrame.width; ++x) {
            const double nx = (x - 11.5) / 12.0;
            const double ny = (y - 3.5) / 4.0;
            if (nx * nx + ny * ny <= 1.0) {
                sheet.set(kShadowFrame.x + x, kShadowFrame.y + y, Color{20, 24, 20, 110});
            }
        }
    }
    return sheet;
}

odysseus::core::Rect spearFrame(Facing facing, bool flintTip) {
    return {static_cast<int>(facing) * kSpearFrameSize, flintTip ? 0 : kSpearFrameSize, kSpearFrameSize, kSpearFrameSize};
}

Facing facingForVector(double x, double y) {
    if (x == 0.0 && y == 0.0) {
        return Facing::South;
    }
    // Eight sectors of 45 degrees; sector 0 is East, counting towards South (y down).
    const double eighth = std::atan2(y, x) / (std::acos(-1.0) / 4.0);
    const int sector = (static_cast<int>(std::lround(eighth)) + 8) % 8;
    constexpr Facing kBySector[8] = {Facing::East, Facing::SouthEast, Facing::South, Facing::SouthWest,
                                     Facing::West, Facing::NorthWest, Facing::North, Facing::NorthEast};
    return kBySector[sector];
}

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
