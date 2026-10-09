#include "minimap.h"

#include <algorithm>

namespace luna::engine {

bool Minimap::build(int rows, const std::function<Color(int, int)>& colourAt) {
    const int last = std::min(image_.height(), rowsDone_ + rows);
    for (; rowsDone_ < last; ++rowsDone_) {
        for (int x = 0; x < image_.width(); ++x) image_.set(x, rowsDone_, colourAt(x, rowsDone_));
    }
    return complete();
}

const Texture& Minimap::texture(Renderer& renderer) {
    if (complete() && !uploaded_) {
        texture_ = renderer.createTexture(image_);
        uploaded_ = true;
    }
    return texture_;
}

std::optional<Point> Minimap::cellAt(const Rect& area, int screenX, int screenY) const {
    if (area.width <= 0 || area.height <= 0 || !area.contains({screenX, screenY})) return std::nullopt;
    return Point{(screenX - area.x) * image_.width() / area.width, (screenY - area.y) * image_.height() / area.height};
}

} // namespace luna::engine
