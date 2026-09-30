#include "game/clan_art.h"

#include "luna/engine/sprite_layers.h"

#include <cstdint>
#include <vector>

namespace odysseus::game {

namespace {

using luna::engine::Color;
using luna::engine::ColourSwap;
using luna::engine::Image;

// The marker colours each layer is drawn in; the palettes replace them.
constexpr Color kSkinMark{224, 172, 128};
constexpr Color kSkinShadeMark{190, 140, 100};
constexpr Color kHairMark{96, 58, 36};
constexpr Color kClothMark{70, 110, 160};
constexpr Color kClothShadeMark{52, 84, 126};
constexpr Color kTrimMark{120, 84, 48};
// Colours that never change.
constexpr Color kOutline{40, 30, 30};
constexpr Color kEye{30, 30, 40};
constexpr Color kShaft{128, 96, 56};
constexpr Color kTip{170, 170, 176};

struct SkinPalette {
    Color main;
    Color shade;
};
constexpr std::array<SkinPalette, kSkinTones> kSkins{{{{224, 172, 128}, {190, 140, 100}},
                                                      {{240, 200, 165}, {205, 165, 130}},
                                                      {{176, 120, 80}, {145, 95, 62}},
                                                      {{120, 78, 52}, {95, 60, 40}}}};
constexpr std::array<Color, kHairColours> kHairs{{{96, 58, 36}, {30, 28, 32}, {214, 182, 96}, {166, 70, 40}, {170, 170, 175}, {130, 88, 60}}};
struct ClothPalette {
    Color main;
    Color shade;
    Color trim;
};
constexpr std::array<ClothPalette, kOutfitColours> kCloths{{{{70, 110, 160}, {52, 84, 126}, {120, 84, 48}},
                                                            {{84, 130, 80}, {62, 100, 60}, {110, 80, 48}},
                                                            {{150, 80, 60}, {118, 60, 44}, {200, 170, 120}},
                                                            {{190, 150, 70}, {150, 116, 50}, {110, 70, 40}},
                                                            {{120, 80, 140}, {92, 60, 110}, {210, 190, 120}},
                                                            {{110, 110, 120}, {84, 84, 94}, {190, 180, 160}}}};

struct Look {
    int x;
    int y;
};

Look lookToward(int facing) {
    constexpr std::array<Look, 8> looks{{{0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}}};
    return looks.at(static_cast<std::size_t>(facing) % 8);
}

// Walking: the legs swing (frames 1 and 3) and the body bobs one pixel, as in the character sheet.
struct Pose {
    int left;
    int top;
    int body;
    int stride;
    Look look;
    int headLeft;
};

Pose poseOf(int column, int row) {
    Pose pose{};
    pose.left = column * kCharacterWidth;
    pose.top = row * kCharacterHeight;
    pose.stride = column == 1 ? 3 : (column == 3 ? -3 : 0);
    pose.body = pose.top + ((column == 1 || column == 3) ? -1 : 0);
    pose.look = lookToward(row);
    pose.headLeft = pose.left + 9 + pose.look.x;
    return pose;
}

template <typename Draw>
Image makeSheet(Draw draw) {
    Image sheet(kCharacterWidth * kWalkFrames, kCharacterHeight * 8);
    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < kWalkFrames; ++column) draw(sheet, poseOf(column, row));
    }
    return sheet;
}

// The body: bare legs with shoes, arms and hands, the head with its eyes.
void drawBody(Image& sheet, const Pose& p) {
    sheet.fillRect(p.left + 11 + p.stride, p.top + 36, 4, 11, kSkinShadeMark);
    sheet.fillRect(p.left + 17 - p.stride, p.top + 36, 4, 11, kSkinShadeMark);
    sheet.fillRect(p.left + 10 + p.stride, p.top + 46, 6, 2, kOutline);
    sheet.fillRect(p.left + 16 - p.stride, p.top + 46, 6, 2, kOutline);
    sheet.fillRect(p.left + 5, p.body + 21 - p.stride / 2, 3, 15, kSkinShadeMark);
    sheet.fillRect(p.left + 24, p.body + 21 + p.stride / 2, 3, 15, kSkinShadeMark);
    sheet.fillRect(p.left + 12, p.body + 18, 8, 4, kSkinShadeMark); // neck
    sheet.fillRect(p.headLeft - 1, p.body + 3, 16, 18, kOutline);
    sheet.fillRect(p.headLeft, p.body + 4, 14, 16, kSkinMark);
    if (p.look.y >= 0) {
        if (p.look.x == 0) {
            sheet.fillRect(p.headLeft + 3, p.body + 12, 2, 3, kEye);
            sheet.fillRect(p.headLeft + 9, p.body + 12, 2, 3, kEye);
        } else {
            sheet.fillRect(p.look.x < 0 ? p.headLeft + 2 : p.headLeft + 10, p.body + 12, 2, 3, kEye);
        }
    }
}

// Hair: 0 short (a cap over the top), 1 long (falls to the shoulders), 2 a top knot.
void drawHair(Image& sheet, const Pose& p, int style) {
    const int rows = p.look.y < 0 ? 16 : (style == 0 ? 6 : 5);
    sheet.fillRect(p.headLeft, p.body + 4, 14, rows, kHairMark);
    if (style == 1) {
        if (p.look.x == 0) {
            sheet.fillRect(p.headLeft - 1, p.body + 6, 3, 14, kHairMark);
            sheet.fillRect(p.headLeft + 12, p.body + 6, 3, 14, kHairMark);
        } else {
            const int back = p.look.x < 0 ? p.headLeft + 8 : p.headLeft - 1;
            sheet.fillRect(back, p.body + 6, 7, 14, kHairMark);
        }
    } else if (style == 2) {
        sheet.fillRect(p.headLeft + 5, p.body, 5, 5, kHairMark);
    }
}

// Outfit: 0 tunic and trousers, 1 fur wrap over bare legs, 2 a long robe.
void drawOutfit(Image& sheet, const Pose& p, int style) {
    if (style == 0) {
        sheet.fillRect(p.left + 11 + p.stride, p.top + 36, 4, 10, kClothShadeMark);
        sheet.fillRect(p.left + 17 - p.stride, p.top + 36, 4, 10, kClothShadeMark);
        sheet.fillRect(p.left + 8, p.body + 20, 16, 18, kClothMark);
        sheet.fillRect(p.left + 8, p.body + 30, 16, 2, kTrimMark);
        sheet.fillRect(p.left + 5, p.body + 21 - p.stride / 2, 3, 8, kClothShadeMark);
        sheet.fillRect(p.left + 24, p.body + 21 + p.stride / 2, 3, 8, kClothShadeMark);
    } else if (style == 1) {
        sheet.fillRect(p.left + 8, p.body + 26, 16, 12, kClothMark);
        sheet.fillRect(p.left + 8, p.body + 36, 16, 3, kTrimMark); // the fur edge
        sheet.fillRect(p.left + 8, p.body + 20, 4, 8, kClothShadeMark); // a strap over one shoulder
        sheet.fillRect(p.left + 12, p.body + 22, 4, 6, kClothShadeMark);
    } else {
        sheet.fillRect(p.left + 8, p.body + 20, 16, 26, kClothMark);
        sheet.fillRect(p.left + 8, p.body + 44, 16, 2, kTrimMark);
        sheet.fillRect(p.left + 8, p.body + 29, 16, 2, kTrimMark);
        sheet.fillRect(p.left + 5, p.body + 21 - p.stride / 2, 3, 12, kClothShadeMark);
        sheet.fillRect(p.left + 24, p.body + 21 + p.stride / 2, 3, 12, kClothShadeMark);
    }
}

// The spear, held upright in the hand on the side away from where the person looks.
void drawSpear(Image& sheet, const Pose& p) {
    const int x = p.left + (p.look.x > 0 ? 3 : 27);
    const int handY = p.body + 33 + (p.look.x > 0 ? -p.stride / 2 : p.stride / 2);
    sheet.fillRect(x, p.body + 2, 2, 46, kShaft);
    sheet.fillRect(x - 1, p.body - 3, 4, 6, kTip);
    sheet.fillRect(x - 1, handY, 4, 3, kSkinMark);
}

} // namespace

LayerSheets makeLayerSheets() {
    LayerSheets sheets{makeSheet([](Image& s, const Pose& p) { drawBody(s, p); }),
                       {makeSheet([](Image& s, const Pose& p) { drawHair(s, p, 0); }), makeSheet([](Image& s, const Pose& p) { drawHair(s, p, 1); }),
                        makeSheet([](Image& s, const Pose& p) { drawHair(s, p, 2); })},
                       {makeSheet([](Image& s, const Pose& p) { drawOutfit(s, p, 0); }), makeSheet([](Image& s, const Pose& p) { drawOutfit(s, p, 1); }),
                        makeSheet([](Image& s, const Pose& p) { drawOutfit(s, p, 2); })},
                       makeSheet([](Image& s, const Pose& p) { drawSpear(s, p); })};
    return sheets;
}

Image hairLayer(const LayerSheets& sheets, int style, int colour) {
    return luna::engine::recoloured(sheets.hair.at(static_cast<std::size_t>(style)), {{kHairMark, kHairs.at(static_cast<std::size_t>(colour))}});
}

Image outfitLayer(const LayerSheets& sheets, int style, int colour) {
    const ClothPalette& cloth = kCloths.at(static_cast<std::size_t>(colour));
    return luna::engine::recoloured(sheets.outfit.at(static_cast<std::size_t>(style)),
                                    {{kClothMark, cloth.main}, {kClothShadeMark, cloth.shade}, {kTrimMark, cloth.trim}});
}

Image composeLook(const LayerSheets& sheets, const LookSpec& look) {
    const SkinPalette& skin = kSkins.at(static_cast<std::size_t>(look.skin));
    const Image body = luna::engine::recoloured(sheets.body, {{kSkinMark, skin.main}, {kSkinShadeMark, skin.shade}});
    const Image hair = hairLayer(sheets, look.hairStyle, look.hairColour);
    const Image outfit = outfitLayer(sheets, look.outfitStyle, look.outfitColour);
    const Image item = luna::engine::recoloured(sheets.item, {{kSkinMark, skin.main}});
    std::vector<const Image*> layers{&body, &outfit, &hair};
    if (look.spear) layers.push_back(&item);
    return luna::engine::composed(layers);
}

LookSpec lookOf(const sim::Person& person, int ageYears) {
    // A fixed scramble of the id, so neighbours in the clan do not look alike.
    std::uint32_t h = static_cast<std::uint32_t>(person.id + 1) * 2654435761U;
    h ^= h >> 15;
    h *= 2246822519U;
    h ^= h >> 13;
    LookSpec look;
    look.skin = static_cast<int>((h >> 3) % kSkinTones);
    look.hairColour = static_cast<int>((h >> 7) % kHairColours);
    look.outfitStyle = static_cast<int>((h >> 11) % kOutfitStyles);
    look.outfitColour = static_cast<int>((h >> 15) % kOutfitColours);
    // Women mostly wear their hair long or knotted, men short; children short.
    const int pick = static_cast<int>((h >> 19) % 4);
    look.hairStyle = ageYears < 12 ? 0 : (person.sex == sim::Sex::Female ? (pick == 0 ? 0 : (pick == 1 ? 2 : 1)) : (pick == 0 ? 2 : 0));
    look.spear = person.sex == sim::Sex::Male && ageYears >= 15;
    return look;
}

} // namespace odysseus::game
