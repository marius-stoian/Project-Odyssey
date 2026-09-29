#pragma once

#include "boundary.h"

namespace odysseus::core {

// Whole-pixel positions and rectangles: pixel art lives on whole pixels.
struct Point {
    int x = 0;
    int y = 0;

    friend bool operator==(const Point&, const Point&) = default;
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool contains(Point point) const {
        return point.x >= x && point.y >= y && point.x < x + width && point.y < y + height;
    }

    friend bool operator==(const Rect&, const Rect&) = default;
};

} // namespace odysseus::core
