#include "luna/engine/image_ops.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>

namespace luna::engine {

using odysseus::core::Rect;

int colourDistance(Color a, Color b) {
    return std::max({std::abs(a.red - b.red), std::abs(a.green - b.green), std::abs(a.blue - b.blue)});
}

Image crop(const Image& source, const Rect& area) {
    const int left = std::clamp(area.x, 0, source.width());
    const int top = std::clamp(area.y, 0, source.height());
    const int right = std::clamp(area.x + area.width, left, source.width());
    const int bottom = std::clamp(area.y + area.height, top, source.height());
    Image out(right - left, bottom - top);
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            out.set(x - left, y - top, source.get(x, y));
        }
    }
    return out;
}

Image fitInto(const Image& source, int width, int height, bool bottom) {
    Image out(width, height);
    if (source.width() == 0 || source.height() == 0) {
        return out;
    }
    // The scale is a fraction: target / source, the same on both axes (proportions kept).
    // Integers only: a target of tw x th pixels, each covering sw/tw x sh/th source pixels.
    const bool widthLimits = static_cast<long long>(width) * source.height() <= static_cast<long long>(height) * source.width();
    const int targetWidth = widthLimits ? width : std::max(1, source.width() * height / source.height());
    const int targetHeight = widthLimits ? std::max(1, source.height() * width / source.width()) : height;
    const int offsetX = (width - targetWidth) / 2;
    const int offsetY = bottom ? height - targetHeight : (height - targetHeight) / 2;
    for (int ty = 0; ty < targetHeight; ++ty) {
        const int y0 = ty * source.height() / targetHeight;
        const int y1 = std::max(y0 + 1, (ty + 1) * source.height() / targetHeight);
        for (int tx = 0; tx < targetWidth; ++tx) {
            const int x0 = tx * source.width() / targetWidth;
            const int x1 = std::max(x0 + 1, (tx + 1) * source.width() / targetWidth);
            // Colours weighted by their opacity, so transparent pixels add no black.
            long long red = 0, green = 0, blue = 0, alpha = 0, count = 0;
            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    const Color c = source.get(x, y);
                    red += c.red * c.alpha;
                    green += c.green * c.alpha;
                    blue += c.blue * c.alpha;
                    alpha += c.alpha;
                    ++count;
                }
            }
            if (alpha == 0) {
                continue;
            }
            // Pixel art stays crisp: a target pixel is opaque when most of its area is.
            const auto a = static_cast<std::uint8_t>(alpha * 2 >= count * 255 ? 255 : 0);
            if (a == 0) {
                continue;
            }
            out.set(offsetX + tx, offsetY + ty,
                    Color{static_cast<std::uint8_t>(red / alpha), static_cast<std::uint8_t>(green / alpha), static_cast<std::uint8_t>(blue / alpha), a});
        }
    }
    return out;
}

Image mirrored(const Image& source) {
    Image out(source.width(), source.height());
    for (int y = 0; y < source.height(); ++y) {
        for (int x = 0; x < source.width(); ++x) {
            out.set(source.width() - 1 - x, y, source.get(x, y));
        }
    }
    return out;
}

Image normalAtlas(const Image& atlas, int cellWidth, int cellHeight, double strength) {
    Image out(atlas.width(), atlas.height());
    out.fillRect(0, 0, atlas.width(), atlas.height(), Color{128, 128, 255, 255});
    constexpr double kEdgeReach = 6.0; // pixels from the edge over which the body rises
    for (int top = 0; top + cellHeight <= atlas.height(); top += cellHeight) {
        for (int left = 0; left + cellWidth <= atlas.width(); left += cellWidth) {
            const auto at = [&](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(cellWidth) + static_cast<std::size_t>(x); };
            const std::size_t count = static_cast<std::size_t>(cellWidth) * static_cast<std::size_t>(cellHeight);
            std::vector<double> distance(count, 0.0);
            // Distance to the nearest see-through pixel (two sweeps of a 3 x 3 chamfer), capped.
            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    distance[at(x, y)] = atlas.get(left + x, top + y).alpha < 128 ? 0.0 : kEdgeReach;
                }
            }
            const auto relax = [&](int x, int y, int dx, int dy, double step) {
                const int nx = x + dx;
                const int ny = y + dy;
                const double beyond = (nx < 0 || ny < 0 || nx >= cellWidth || ny >= cellHeight) ? 0.0 : distance[at(nx, ny)];
                distance[at(x, y)] = std::min(distance[at(x, y)], beyond + step);
            };
            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    relax(x, y, -1, 0, 1.0); relax(x, y, 0, -1, 1.0); relax(x, y, -1, -1, 1.4142); relax(x, y, 1, -1, 1.4142);
                }
            }
            for (int y = cellHeight - 1; y >= 0; --y) {
                for (int x = cellWidth - 1; x >= 0; --x) {
                    relax(x, y, 1, 0, 1.0); relax(x, y, 0, 1, 1.0); relax(x, y, 1, 1, 1.4142); relax(x, y, -1, 1, 1.4142);
                }
            }
            std::vector<double> height(count, 0.0);
            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    const Color c = atlas.get(left + x, top + y);
                    if (c.alpha < 128) continue;
                    const double brightness = (0.299 * c.red + 0.587 * c.green + 0.114 * c.blue) / 255.0;
                    height[at(x, y)] = 0.65 * (distance[at(x, y)] / kEdgeReach) + 0.35 * brightness;
                }
            }
            // A light blur, so single bright pixels do not turn into spikes.
            std::vector<double> smooth(count, 0.0);
            const auto heightAt = [&](int x, int y) { return height[at(std::clamp(x, 0, cellWidth - 1), std::clamp(y, 0, cellHeight - 1))]; };
            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    smooth[at(x, y)] = (4.0 * heightAt(x, y) + 2.0 * (heightAt(x - 1, y) + heightAt(x + 1, y) + heightAt(x, y - 1) + heightAt(x, y + 1)) +
                                        heightAt(x - 1, y - 1) + heightAt(x + 1, y - 1) + heightAt(x - 1, y + 1) + heightAt(x + 1, y + 1)) / 16.0;
                }
            }
            const auto s = [&](int x, int y) { return smooth[at(std::clamp(x, 0, cellWidth - 1), std::clamp(y, 0, cellHeight - 1))]; };
            for (int y = 0; y < cellHeight; ++y) {
                for (int x = 0; x < cellWidth; ++x) {
                    if (atlas.get(left + x, top + y).alpha < 128) continue;
                    const double dx = (s(x + 1, y - 1) + 2.0 * s(x + 1, y) + s(x + 1, y + 1)) - (s(x - 1, y - 1) + 2.0 * s(x - 1, y) + s(x - 1, y + 1));
                    const double dy = (s(x - 1, y + 1) + 2.0 * s(x, y + 1) + s(x + 1, y + 1)) - (s(x - 1, y - 1) + 2.0 * s(x, y - 1) + s(x + 1, y - 1));
                    // The surface leans toward lower ground: the direction is minus the slope.
                    double nx = -dx * strength / 4.0;
                    double ny = -dy * strength / 4.0;
                    double nz = 1.0;
                    const double length = std::sqrt(nx * nx + ny * ny + nz * nz);
                    nx /= length; ny /= length; nz /= length;
                    const auto encode = [](double v) { return static_cast<std::uint8_t>(std::lround(std::clamp(v, -1.0, 1.0) * 127.5 + 127.5)); };
                    out.set(left + x, top + y, Color{encode(nx), encode(ny), encode(nz), 255});
                }
            }
        }
    }
    return out;
}

Image mirroredNormals(const Image& normals) {
    Image out = mirrored(normals);
    for (int y = 0; y < out.height(); ++y) {
        for (int x = 0; x < out.width(); ++x) {
            Color c = out.get(x, y);
            c.red = static_cast<std::uint8_t>(255 - c.red);
            out.set(x, y, c);
        }
    }
    return out;
}

void removeBackground(Image& image, int tolerance) {
    if (image.width() == 0 || image.height() == 0) {
        return;
    }
    const Color background = image.get(0, 0);
    std::vector<bool> seen(static_cast<std::size_t>(image.width()) * static_cast<std::size_t>(image.height()), false);
    std::vector<std::pair<int, int>> stack;
    auto push = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= image.width() || y >= image.height()) return;
        const std::size_t at = static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width()) + static_cast<std::size_t>(x);
        if (seen[at] || colourDistance(image.get(x, y), background) > tolerance) return;
        seen[at] = true;
        stack.push_back({x, y});
    };
    for (int x = 0; x < image.width(); ++x) {
        push(x, 0);
        push(x, image.height() - 1);
    }
    for (int y = 0; y < image.height(); ++y) {
        push(0, y);
        push(image.width() - 1, y);
    }
    // An explicit stack, never recursion: a large background would overflow the call stack.
    while (!stack.empty()) {
        const auto [x, y] = stack.back();
        stack.pop_back();
        image.set(x, y, Color{0, 0, 0, 0});
        push(x + 1, y);
        push(x - 1, y);
        push(x, y + 1);
        push(x, y - 1);
    }
}

Rect opaqueBounds(const Image& image) {
    int left = image.width(), top = image.height(), right = -1, bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.get(x, y).alpha > 0) {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
    }
    return right < 0 ? Rect{0, 0, 0, 0} : Rect{left, top, right - left + 1, bottom - top + 1};
}

std::vector<Rect> findBlobs(const Image& image, const Rect& area, Color background, int tolerance, int minSize) {
    const int left = std::clamp(area.x, 0, image.width());
    const int top = std::clamp(area.y, 0, image.height());
    const int right = std::clamp(area.x + area.width, left, image.width());
    const int bottom = std::clamp(area.y + area.height, top, image.height());
    const int w = right - left;
    const int h = bottom - top;
    std::vector<bool> seen(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), false);
    auto index = [w](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x); };
    auto isInk = [&](int x, int y) { return colourDistance(image.get(left + x, top + y), background) > tolerance; };
    std::vector<Rect> blobs;
    std::vector<std::pair<int, int>> stack;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (seen[index(x, y)] || !isInk(x, y)) continue;
            int minX = x, maxX = x, minY = y, maxY = y;
            seen[index(x, y)] = true;
            stack.push_back({x, y});
            while (!stack.empty()) {
                const auto [cx, cy] = stack.back();
                stack.pop_back();
                minX = std::min(minX, cx); maxX = std::max(maxX, cx);
                minY = std::min(minY, cy); maxY = std::max(maxY, cy);
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = cx + dx;
                        const int ny = cy + dy;
                        if (nx < 0 || ny < 0 || nx >= w || ny >= h || seen[index(nx, ny)] || !isInk(nx, ny)) continue;
                        seen[index(nx, ny)] = true;
                        stack.push_back({nx, ny});
                    }
                }
            }
            if (maxX - minX + 1 >= minSize || maxY - minY + 1 >= minSize) {
                blobs.push_back({left + minX, top + minY, maxX - minX + 1, maxY - minY + 1});
            }
        }
    }
    // Reading order: sorted by top edge, then cut into lines (a blob starting more than half the
    // line's first blob height lower begins a new line), and each line sorted left to right.
    std::sort(blobs.begin(), blobs.end(), [](const Rect& a, const Rect& b) { return a.y != b.y ? a.y < b.y : a.x < b.x; });
    std::vector<Rect> ordered;
    std::size_t start = 0;
    while (start < blobs.size()) {
        std::size_t end = start + 1;
        const int limit = blobs[start].y + std::max(8, blobs[start].height / 2);
        while (end < blobs.size() && blobs[end].y <= limit) ++end;
        std::sort(blobs.begin() + static_cast<std::ptrdiff_t>(start), blobs.begin() + static_cast<std::ptrdiff_t>(end),
                  [](const Rect& a, const Rect& b) { return a.x != b.x ? a.x < b.x : a.y < b.y; });
        ordered.insert(ordered.end(), blobs.begin() + static_cast<std::ptrdiff_t>(start), blobs.begin() + static_cast<std::ptrdiff_t>(end));
        start = end;
    }
    return ordered;
}


void keyAlpha(Image& image, int threshold, bool hard) {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            Color c = image.get(x, y);
            if (c.alpha < threshold) {
                c = Color{0, 0, 0, 0};
            } else if (hard) {
                c.alpha = 255;
            }
            image.set(x, y, c);
        }
    }
}

void keyBrightness(Image& image, Color background, int gain) {
    const int floor = std::max({background.red, background.green, background.blue});
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            Color c = image.get(x, y);
            const int lift = std::max({c.red, c.green, c.blue}) - floor;
            c.alpha = static_cast<std::uint8_t>(std::clamp(lift * gain, 0, 255));
            image.set(x, y, c.alpha == 0 ? Color{0, 0, 0, 0} : c);
        }
    }
}

void keepMainFigure(Image& image, const odysseus::core::Rect& focus) {
    const int w = image.width();
    const int h = image.height();
    struct Group {
        odysseus::core::Rect box;
        std::vector<std::pair<int, int>> members;
        bool centred = false; // its middle lies inside the focus
    };
    std::vector<Group> groups;
    std::vector<bool> seen(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), false);
    auto index = [w](int x, int y) { return static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x); };
    std::vector<std::pair<int, int>> stack;
    for (int y0 = 0; y0 < h; ++y0) {
        for (int x0 = 0; x0 < w; ++x0) {
            if (seen[index(x0, y0)] || image.get(x0, y0).alpha == 0) continue;
            // One group: flood it (an explicit stack, never recursion), remembering its box.
            Group group;
            int left = x0, right = x0, top = y0, bottom = y0;
            seen[index(x0, y0)] = true;
            stack.push_back({x0, y0});
            while (!stack.empty()) {
                const auto [x, y] = stack.back();
                stack.pop_back();
                group.members.push_back({x, y});
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx;
                        const int ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= w || ny >= h || seen[index(nx, ny)] || image.get(nx, ny).alpha == 0) continue;
                        seen[index(nx, ny)] = true;
                        stack.push_back({nx, ny});
                    }
                }
            }
            group.box = {left, top, right - left + 1, bottom - top + 1};
            const int middleX = (left + right) / 2;
            const int middleY = (top + bottom) / 2;
            group.centred = middleX >= focus.x && middleY >= focus.y && middleX < focus.x + focus.width && middleY < focus.y + focus.height;
            groups.push_back(std::move(group));
        }
    }
    // The main figure: the largest group centred in the focus.
    const Group* main = nullptr;
    for (const Group& group : groups) {
        if (group.centred && (main == nullptr || group.members.size() > main->members.size())) main = &group;
    }
    // Its loose parts (an antler, a spark) lie mostly within its width; a neighbour's head
    // reaching in from the side does not.
    auto touchesMain = [&](const Group& group) {
        if (main == nullptr) return false;
        const int overlap = std::min(group.box.x + group.box.width, main->box.x + main->box.width) - std::max(group.box.x, main->box.x);
        return overlap * 2 >= group.box.width;
    };
    for (const Group& group : groups) {
        if (&group == main || (group.centred && touchesMain(group))) continue;
        for (const auto& [x, y] : group.members) image.set(x, y, Color{0, 0, 0, 0});
    }
}


void removeColour(Image& image, Color colour, int tolerance) {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (colourDistance(image.get(x, y), colour) <= tolerance) image.set(x, y, Color{0, 0, 0, 0});
        }
    }
}

} // namespace luna::engine
