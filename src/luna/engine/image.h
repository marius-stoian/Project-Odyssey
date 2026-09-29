#pragma once

#include "boundary.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace luna::engine {

struct Color {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
    std::uint8_t alpha = 255; // 0 = fully transparent

    friend bool operator==(const Color&, const Color&) = default;
};

// A picture in memory: width x height pixels, 4 bytes each (red, green, blue, alpha).
// Starts fully transparent. Games paint into it, then upload it as a texture.
class Image {
public:
    Image(int width, int height)
        : width_(width), height_(height), rgba_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4, 0) {}

    int width() const { return width_; }
    int height() const { return height_; }
    const std::uint8_t* data() const { return rgba_.data(); }

    void set(int x, int y, Color color) {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            return; // painting outside the picture is ignored, like drawing past a page's edge
        }
        const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)) * 4;
        rgba_[at] = color.red;
        rgba_[at + 1] = color.green;
        rgba_[at + 2] = color.blue;
        rgba_[at + 3] = color.alpha;
    }

    Color get(int x, int y) const {
        const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(x)) * 4;
        return {rgba_[at], rgba_[at + 1], rgba_[at + 2], rgba_[at + 3]};
    }

    void fillRect(int x, int y, int width, int height, Color color) {
        for (int row = y; row < y + height; ++row) {
            for (int column = x; column < x + width; ++column) {
                set(column, row, color);
            }
        }
    }

private:
    int width_;
    int height_;
    std::vector<std::uint8_t> rgba_;
};

} // namespace luna::engine
