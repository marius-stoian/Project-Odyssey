#pragma once

#include "boundary.h"

#include "luna/engine/image.h"

#include <vector>

namespace luna::engine {

// Characters built from layers (US-030): body, hair, outfit and held item are separate pictures of the same size,
// drawn on top of each other. Luna only knows how to recolour a layer and how to stack them; what the layers show
// is the game's business.

// One colour turned into another. Only pixels of exactly the colour `from` (whatever their alpha) change; their
// alpha is kept.
struct ColourSwap {
    Color from;
    Color to;
};

// The layer with the colours swapped: the same art in another palette (three hair colours from one hair picture).
Image recoloured(const Image& layer, const std::vector<ColourSwap>& swaps);

// The layers stacked, the first at the bottom. All must have the same size (the first one's size is used; a
// different one is ignored). A see-through pixel shows what is below it; a solid one hides it.
Image composed(const std::vector<const Image*>& layersBottomFirst);

} // namespace luna::engine
