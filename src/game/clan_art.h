#pragma once

#include "boundary.h"

#include "game/placeholder_art.h"
#include "luna/engine/image.h"
#include "sim/person.h"

#include <array>

namespace odysseus::game {

// The people of the clan, drawn from layers (US-030, D-30): a body, a hair style, an outfit and a held item, each its
// own picture in the same 4 x 8 grid of 32 x 48 cells as the character sheet (columns = walking frames, rows = the 8
// facings), so the layers line up in every frame and direction. Every layer is drawn in a few "marker" colours and
// recoloured by a palette, so a few pictures make hundreds of different people. Programmer art (D-05).

inline constexpr int kSkinTones = 4;
inline constexpr int kHairStyles = 3;
inline constexpr int kHairColours = 6;
inline constexpr int kOutfitStyles = 3;
inline constexpr int kOutfitColours = 6;

// What one person looks like.
struct LookSpec {
    int skin = 0;
    int hairStyle = 0;
    int hairColour = 0;
    int outfitStyle = 0;
    int outfitColour = 0;
    bool spear = false; // carries a spear
    friend bool operator==(const LookSpec&, const LookSpec&) = default;
    friend bool operator<(const LookSpec& a, const LookSpec& b) {
        return std::tie(a.skin, a.hairStyle, a.hairColour, a.outfitStyle, a.outfitColour, a.spear) <
               std::tie(b.skin, b.hairStyle, b.hairColour, b.outfitStyle, b.outfitColour, b.spear);
    }
};

// The layer pictures in their marker colours.
struct LayerSheets {
    luna::engine::Image body;
    std::array<luna::engine::Image, kHairStyles> hair;
    std::array<luna::engine::Image, kOutfitStyles> outfit;
    luna::engine::Image item; // the spear
};

LayerSheets makeLayerSheets();

// The layers recoloured for `look` and stacked: one character sheet for this person.
luna::engine::Image composeLook(const LayerSheets& sheets, const LookSpec& look);

// The recoloured single layers, for tests and tools.
luna::engine::Image hairLayer(const LayerSheets& sheets, int style, int colour);
luna::engine::Image outfitLayer(const LayerSheets& sheets, int style, int colour);

// A person's look, the same every time for the same person (from the id, the sex and the age): children wear no
// spear, and only grown men carry one.
LookSpec lookOf(const sim::Person& person, int ageYears);

} // namespace odysseus::game
