#include "luna/engine/shadow_draw.h"

#include <algorithm>
#include <cmath>

namespace luna::engine {

void drawShadow(Renderer& renderer, const Texture& silhouette, const Rect& source, Point feet, double dirX, double dirY, double groundPerHeight,
                std::uint8_t alpha) {
    if (alpha == 0 || source.width <= 0 || source.height <= 0 || groundPerHeight <= 0.0) return;
    // Straight overhead or exactly sideways: keep some depth so the shadow is a shape, not a line.
    const double sign = dirY < 0.0 ? -1.0 : 1.0;
    const double depthShare = std::max(std::abs(dirY), kMinShadowDepth);
    const double depthPerHeight = depthShare * groundPerHeight; // ground rows covered per sprite row
    const int rows = std::max(1, static_cast<int>(std::lround(depthPerHeight * source.height)));
    const int rowsPerDraw = std::max(1, (rows + 63) / 64); // very long shadows are drawn two or more rows at a time
    for (int row = 0; row < rows; row += rowsPerDraw) {
        // Ground rows [row, row + rowsPerDraw) show the sprite rows from `low` up to `high` above the feet.
        const double low = static_cast<double>(row) / rows * source.height;
        const double high = std::min(static_cast<double>(row + rowsPerDraw) / rows * source.height, static_cast<double>(source.height));
        const int sourceRows = std::max(1, static_cast<int>(std::lround(high - low)));
        const int sourceTop = std::clamp(source.y + source.height - static_cast<int>(std::lround(high)), source.y, source.y + source.height - sourceRows);
        const double middle = (low + high) / 2.0; // height above the feet of this strip, in sprite pixels
        const int shift = static_cast<int>(std::lround(dirX * middle * groundPerHeight));
        const int top = sign > 0.0 ? feet.y + row : feet.y - row - rowsPerDraw;
        renderer.drawStyled(silhouette, {source.x, sourceTop, source.width, sourceRows},
                            {feet.x - source.width / 2 + shift, top, source.width, rowsPerDraw}, DrawStyle{alpha, Blend::Normal});
    }
}

} // namespace luna::engine
