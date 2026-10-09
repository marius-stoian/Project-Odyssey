#pragma once

#include "boundary.h"

#include "image.h"
#include "renderer.h"

#include <functional>
#include <optional>

namespace luna::engine {

// A small picture of a big map, painted a few rows at a time so a frame never stalls, then kept (US-200, game-agnostic).
// The game decides the colour of each cell; the minimap only holds the picture, uploads it once it is complete, and turns a
// click on it back into a map cell. `invalidate` starts a new painting; the old texture stays in use until the new one is
// complete. (A new painting gives the old texture back to the renderer when it replaces it.)
class Minimap {
public:
    Minimap(int width, int height) : image_(width, height) {}

    int width() const { return image_.width(); }
    int height() const { return image_.height(); }

    // Paints up to `rows` more rows with `colourAt(x, y)`; true once every row is painted.
    bool build(int rows, const std::function<Color(int, int)>& colourAt);
    bool complete() const { return rowsDone_ >= image_.height(); }
    int rowsDone() const { return rowsDone_; }
    void invalidate() { rowsDone_ = 0; uploaded_ = false; }

    const Image& image() const { return image_; }
    // The picture as a texture: uploaded once, as soon as it is complete. `id` is -1 before the first upload.
    const Texture& texture(Renderer& renderer);
    // The last uploaded picture, even while a new painting is under way (nothing before the first upload).
    bool hasPicture() const { return texture_.id >= 0; }
    const Texture& picture() const { return texture_; }

    // The map cell under screen point (screenX, screenY) when the picture is stretched over `area`; nothing outside `area`.
    std::optional<Point> cellAt(const Rect& area, int screenX, int screenY) const;

private:
    Image image_;
    int rowsDone_ = 0;
    bool uploaded_ = false;
    Texture texture_;
};

} // namespace luna::engine
