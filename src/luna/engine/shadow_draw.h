#pragma once

#include "boundary.h"

#include "luna/engine/renderer.h"

namespace luna::engine {

// A shadow on the ground (US-244): the upright picture `source` of `silhouette` (a black copy of the sprite's texture, see `silhouette` in
// image_ops.h) is laid down along the light. A point of the sprite `z` pixels above its feet lands (dirX, dirY) * z * `groundPerHeight` pixels
// from the feet on the ground, so the sprite is sheared along the light and flattened. (dirX, dirY) has length 1 and is the way the shadow falls on
// the picture (x right, y down). `groundPerHeight` is the shadow's length divided by the sprite's height: 1 / tan(elevation), with the caps of
// D-49 already applied. `alpha` is how dark it is (0..255). `feet` is the middle of the bottom edge of the sprite.
//
// It is drawn one picture row at a time, going down the screen, so no ground pixel is darkened twice (a shadow of many overlapping strips would
// have dark bands). A shadow that is nearly sideways on the screen is given a little depth (`kMinDepth`), because a flat sprite seen exactly along
// the light would otherwise be a line.
void drawShadow(Renderer& renderer, const Texture& silhouette, const Rect& source, Point feet, double dirX, double dirY, double groundPerHeight,
                std::uint8_t alpha);

inline constexpr double kMinShadowDepth = 0.25; // smallest share of the shadow's length that goes down or up the screen

} // namespace luna::engine
