#pragma once

#include "boundary.h"

#include "image.h"

#include "core/geometry.h"

#include <vector>

namespace luna::engine {

// Small, pure picture operations used to turn artists' sheets into game-ready frames.

// The part of `source` inside `area` (clipped to the picture).
Image crop(const Image& source, const odysseus::core::Rect& area);

// `source` shrunk (or grown) to fit inside a width x height box, keeping its proportions, by
// averaging every source pixel that falls on a target pixel (a box filter). Transparent pixels
// do not darken the edges. With `bottom` the picture stands on the bottom row (a character's
// feet); otherwise it is centred. The rest of the box is transparent.
Image fitInto(const Image& source, int width, int height, bool bottom);

// Left and right swapped: a west-facing frame from an east-facing one.
Image mirrored(const Image& source);

// The same picture painted black, keeping its transparency: what a shadow is cut from (US-244).
Image silhouette(const Image& source);

// A normal map for an atlas of cells (US-241): per cell, a height is made from how far each opaque pixel is from the figure's edge (the middle
// stands higher, like a rounded body) and from its brightness (folds and highlights stand out), smoothed a little, and the slopes of that height
// (Sobel) become the surface direction: red is x (right), green is y (down), blue is z (toward the viewer), each 0..255 for -1..1. Transparent
// pixels are flat (128 128 255). `strength` is how steep the slopes look (about 2). The result has the size of `atlas` and is opaque.
Image normalAtlas(const Image& atlas, int cellWidth, int cellHeight, double strength);

// `normals` left and right swapped, with the x of each direction turned around, so a mirrored sprite is lit from the right side.
Image mirroredNormals(const Image& normals);

// Makes the background transparent: every pixel connected to the picture's border whose
// colour is within `tolerance` of the top-left pixel's colour (the sheet's background).
void removeBackground(Image& image, int tolerance);

// The smallest rectangle holding every pixel that is not transparent (empty when none is).
odysseus::core::Rect opaqueBounds(const Image& image);

// Connected groups of pixels (8 neighbours) inside `area` whose colour differs from
// `background` by more than `tolerance`, as bounding boxes in reading order (top to bottom,
// then left to right). Groups smaller than `minSize` pixels on both sides are left out.
std::vector<odysseus::core::Rect> findBlobs(const Image& image, const odysseus::core::Rect& area, Color background, int tolerance,
                                            int minSize);

// How far apart two colours are: the largest difference of red, green and blue.
int colourDistance(Color a, Color b);

// Sheets that already carry transparency (M2d): pixels less opaque than `threshold` become
// fully transparent. With `hard` the rest become fully opaque (crisp pixel art); otherwise they
// keep their alpha (soft glows).
void keyAlpha(Image& image, int threshold, bool hard);

// Light effects painted on a dark background: each pixel's opacity becomes how much brighter it
// is than `background` (its brightest channel), times `gain`, so glows fade out softly.
void keyBrightness(Image& image, Color background, int gain);

// Keeps one figure: the largest connected group of visible pixels (8 neighbours) whose middle
// lies inside `focus`, and the groups centred in `focus` that touch its box (loose parts like an
// antler). Everything else (a neighbour reaching into the picture) becomes transparent.
void keepMainFigure(Image& image, const odysseus::core::Rect& focus);

// Every pixel within `tolerance` of `colour` becomes transparent, wherever it is: a background
// that also shows through holes (inside a bow, between chain links).
void removeColour(Image& image, Color colour, int tolerance);

} // namespace luna::engine
