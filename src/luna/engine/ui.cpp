#include "luna/engine/ui.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>

namespace luna::engine {

namespace {

// The classic public 5x7 font: five columns per character, left to right; in each byte bit 0
// is the top row and bit 6 the bottom row. Characters 32 (space) to 126 (~).
constexpr std::uint8_t kFont[95][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5F, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62}, {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1C, 0x00}, {0x08, 0x2A, 0x1C, 0x2A, 0x08}, {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00}, {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E}, {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14}, {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E}, {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36}, {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01}, {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01}, {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40}, {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F}, {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06}, {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01}, {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F}, {0x3F, 0x40, 0x38, 0x40, 0x3F},
    {0x63, 0x14, 0x08, 0x14, 0x63}, {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7F, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7F, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04}, {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78}, {0x7F, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20},
    {0x38, 0x44, 0x44, 0x48, 0x7F}, {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7E, 0x09, 0x01, 0x02}, {0x0C, 0x52, 0x52, 0x52, 0x3E},
    {0x7F, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7D, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3D, 0x00}, {0x7F, 0x10, 0x28, 0x44, 0x00},
    {0x00, 0x41, 0x7F, 0x40, 0x00}, {0x7C, 0x04, 0x18, 0x04, 0x78}, {0x7C, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7C, 0x14, 0x14, 0x14, 0x08}, {0x08, 0x14, 0x14, 0x18, 0x7C}, {0x7C, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20}, {0x3C, 0x40, 0x40, 0x20, 0x7C}, {0x1C, 0x20, 0x40, 0x20, 0x1C}, {0x3C, 0x40, 0x30, 0x40, 0x3C},
    {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0C, 0x50, 0x50, 0x50, 0x3C}, {0x44, 0x64, 0x54, 0x4C, 0x44}, {0x00, 0x08, 0x36, 0x41, 0x00},
    {0x00, 0x00, 0x7F, 0x00, 0x00}, {0x00, 0x41, 0x36, 0x08, 0x00}, {0x08, 0x04, 0x08, 0x10, 0x08},
};

constexpr int kGlyphCount = 95;
constexpr int kCellWidth = kGlyphWidth + 1;
constexpr int kCellHeight = kGlyphHeight + 1;
constexpr int kColors = static_cast<int>(UiColor::Count);
constexpr int kSwatch = 32;                       // solid blocks, drawn tile by tile to fill any area
constexpr int kSwatchTop = kColors * kCellHeight; // swatches sit below the font rows

constexpr Color colorOf(UiColor color) {
    constexpr std::array<Color, kColors> kPalette = {{
        {236, 230, 210, 255}, // Text
        {150, 146, 134, 255}, // Dim
        {232, 196, 96, 255},  // Gold
        {222, 84, 70, 255},   // Red
        {16, 16, 20, 255},    // Dark
        {28, 30, 38, 240},    // Panel
        {52, 56, 70, 255},    // PanelLight
        {118, 104, 72, 255},  // Border
        {70, 120, 180, 255},  // Selected
        {70, 76, 96, 255},    // Hover
        {255, 255, 255, 56},  // Grid
        {0, 0, 0, 150},       // Shade
    }};
    return kPalette[static_cast<std::size_t>(color)];
}

int glyphIndex(char c) {
    return c >= 32 && c <= 126 ? c - 32 : '?' - 32;
}

} // namespace

bool glyphPixel(char c, int column, int row) {
    if (column < 0 || column >= kGlyphWidth || row < 0 || row >= kGlyphHeight) return false;
    return (kFont[glyphIndex(c)][column] >> row & 1) != 0;
}

Image makeUiSheet() {
    Image sheet(kGlyphCount * kCellWidth, kSwatchTop + kSwatch);
    for (int color = 0; color < kColors; ++color) {
        const Color ink = colorOf(static_cast<UiColor>(color));
        for (int g = 0; g < kGlyphCount; ++g) {
            for (int row = 0; row < kGlyphHeight; ++row) {
                for (int column = 0; column < kGlyphWidth; ++column) {
                    if (glyphPixel(static_cast<char>(g + 32), column, row)) {
                        sheet.set(g * kCellWidth + column, color * kCellHeight + row, ink);
                    }
                }
            }
        }
        sheet.fillRect(color * kSwatch, kSwatchTop, kSwatch, kSwatch, ink);
    }
    return sheet;
}

void UiPainter::fill(const Rect& area, UiColor color) {
    const int sx = static_cast<int>(color) * kSwatch;
    for (int y = area.y; y < area.y + area.height; y += kSwatch) {
        for (int x = area.x; x < area.x + area.width; x += kSwatch) {
            const int w = std::min(kSwatch, area.x + area.width - x);
            const int h = std::min(kSwatch, area.y + area.height - y);
            renderer_.draw(sheet_, {sx, kSwatchTop, w, h}, {x, y});
        }
    }
}

void UiPainter::outline(const Rect& area, UiColor color) {
    fill({area.x, area.y, area.width, 1}, color);
    fill({area.x, area.y + area.height - 1, area.width, 1}, color);
    fill({area.x, area.y + 1, 1, area.height - 2}, color);
    fill({area.x + area.width - 1, area.y + 1, 1, area.height - 2}, color);
}

void UiPainter::text(int x, int y, std::string_view text, UiColor color) {
    for (const char c : text) {
        if (c != ' ') {
            renderer_.draw(sheet_, {glyphIndex(c) * kCellWidth, static_cast<int>(color) * kCellHeight, kGlyphWidth, kGlyphHeight}, {x, y});
        }
        x += kTextAdvance;
    }
}

// --- Button ---

bool Button::handle(const UiInput& input) {
    hovered_ = visible && contains(input.pointer.x, input.pointer.y);
    hoverX_ = input.pointer.x;
    hoverY_ = input.pointer.y;
    if (!hovered_) return false;
    if (input.pointer.wasReleased(PointerButton::Left) && onClick) {
        onClick(); // a click counts when the button is let go over it
    }
    return input.pointer.wasPressed(PointerButton::Left) || input.pointer.wasReleased(PointerButton::Left) ||
           input.pointer.isHeld(PointerButton::Left);
}

void Button::draw(UiPainter& painter) const {
    painter.fill(bounds, selected ? UiColor::Selected : (hovered_ ? UiColor::Hover : UiColor::PanelLight));
    painter.outline(bounds, selected ? UiColor::Gold : UiColor::Border);
    if (icon != nullptr) {
        painter.image(*icon, iconSource,
                      {bounds.x + (bounds.width - iconSource.width) / 2, bounds.y + (bounds.height - iconSource.height) / 2});
    } else {
        painter.text(bounds.x + (bounds.width - UiPainter::textWidth(label)) / 2, bounds.y + (bounds.height - kGlyphHeight) / 2, label,
                     UiColor::Text);
    }
}

void Button::drawOverlay(UiPainter& painter) const {
    if (!hovered_ || hint.empty()) return;
    const int width = UiPainter::textWidth(hint) + 6;
    const Rect box{hoverX_ + 8, hoverY_ + 10, width, kGlyphHeight + 6};
    painter.fill(box, UiColor::Dark);
    painter.outline(box, UiColor::Border);
    painter.text(box.x + 3, box.y + 3, hint, UiColor::Text);
}

// --- ListBox ---

bool ListBox::handle(const UiInput& input) {
    if (!visible || !contains(input.pointer.x, input.pointer.y)) return false;
    const int count = static_cast<int>(items.size());
    if (input.pointer.wheel != 0) {
        first = std::clamp(first - input.pointer.wheel, 0, std::max(0, count - rows()));
    }
    if (input.pointer.wasPressed(PointerButton::Left)) {
        const int row = first + (input.pointer.y - bounds.y) / kLineHeight;
        if (row >= 0 && row < count) {
            selected = row;
            if (onSelect) onSelect(row);
        }
    }
    return true;
}

void ListBox::draw(UiPainter& painter) const {
    painter.fill(bounds, UiColor::Dark);
    const int count = static_cast<int>(items.size());
    for (int i = 0; i < rows() && first + i < count; ++i) {
        const int y = bounds.y + i * kLineHeight;
        if (first + i == selected) {
            painter.fill({bounds.x, y, bounds.width, kLineHeight}, UiColor::Selected);
        }
        painter.text(bounds.x + 2, y + 1, items[static_cast<std::size_t>(first + i)], UiColor::Text);
    }
    if (count > rows()) { // a thin scroll bar: where the visible rows are in the whole list
        const int barHeight = std::max(4, bounds.height * rows() / count);
        const int barY = bounds.y + (bounds.height - barHeight) * first / std::max(1, count - rows());
        painter.fill({bounds.x + bounds.width - 2, barY, 2, barHeight}, UiColor::Gold);
    }
}

// --- NumberField ---

void NumberField::commit() {
    int typed = value;
    if (!editing_.empty() && editing_ != "-") {
        std::from_chars(editing_.data(), editing_.data() + editing_.size(), typed);
    }
    const int kept = std::clamp(typed, minimum, maximum);
    focused_ = false;
    if (kept != value) {
        value = kept;
        if (onChange) onChange(value);
    }
}

bool NumberField::handle(const UiInput& input) {
    if (!visible) return false;
    const bool over = contains(input.pointer.x, input.pointer.y);
    if (input.pointer.wasPressed(PointerButton::Left)) {
        if (over && !focused_) {
            focused_ = true;
            editing_.clear();
        } else if (!over && focused_) {
            commit();
        }
    }
    if (focused_) {
        for (const char c : input.text) {
            if ((c >= '0' && c <= '9' && editing_.size() < 6) || (c == '-' && editing_.empty() && minimum < 0)) editing_ += c;
        }
        if (input.erase && !editing_.empty()) editing_.pop_back();
        if (input.confirm) commit();
        return true;
    }
    if (over && input.pointer.wheel != 0) {
        const int kept = std::clamp(value + input.pointer.wheel, minimum, maximum);
        if (kept != value) {
            value = kept;
            if (onChange) onChange(value);
        }
    }
    return over && (input.pointer.wasPressed(PointerButton::Left) || input.pointer.wheel != 0);
}

void NumberField::draw(UiPainter& painter) const {
    painter.text(bounds.x, bounds.y + 2, label, UiColor::Dim);
    const int boxX = bounds.x + UiPainter::textWidth(label) + 4;
    const Rect box{boxX, bounds.y, bounds.x + bounds.width - boxX, bounds.height};
    painter.fill(box, UiColor::Dark);
    painter.outline(box, focused_ ? UiColor::Gold : UiColor::Border);
    painter.text(box.x + 3, box.y + (box.height - kGlyphHeight) / 2, focused_ ? editing_ + "_" : std::to_string(value), UiColor::Text);
}

// --- TextField ---

void TextField::commit() {
    focused_ = false;
    if (editing_ != value) {
        value = editing_;
        if (onChange) onChange(value);
    }
}

bool TextField::handle(const UiInput& input) {
    if (!visible) return false;
    const bool over = contains(input.pointer.x, input.pointer.y);
    if (input.pointer.wasPressed(PointerButton::Left)) {
        if (over && !focused_) {
            focused_ = true;
            editing_ = value;
        } else if (!over && focused_) {
            commit();
        }
    }
    if (!focused_) {
        return over && input.pointer.wasPressed(PointerButton::Left);
    }
    for (const char c : input.text) {
        if (editing_.size() < maxLength) editing_ += c;
    }
    if (input.erase && !editing_.empty()) editing_.pop_back();
    if (input.confirm) commit();
    return true;
}

void TextField::draw(UiPainter& painter) const {
    painter.text(bounds.x, bounds.y + 2, label, UiColor::Dim);
    const int boxX = bounds.x + UiPainter::textWidth(label) + 4;
    const Rect box{boxX, bounds.y, bounds.x + bounds.width - boxX, bounds.height};
    painter.fill(box, UiColor::Dark);
    painter.outline(box, focused_ ? UiColor::Gold : UiColor::Border);
    painter.text(box.x + 3, box.y + (box.height - kGlyphHeight) / 2, focused_ ? editing_ + "_" : value, UiColor::Text);
}

// --- Panel ---

bool Panel::handle(const UiInput& input) {
    if (!visible) return false;
    bool used = false;
    // Every child sees the input (a text field must lose focus when another is clicked); the
    // last drawn is on top, so it is asked first.
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        used = (*it)->handle(input) || used;
    }
    const bool onPanel = contains(input.pointer.x, input.pointer.y) &&
                         (input.pointer.wasPressed(PointerButton::Left) || input.pointer.isHeld(PointerButton::Left) ||
                          input.pointer.wasReleased(PointerButton::Left) || input.pointer.wheel != 0);
    return used || onPanel;
}

void Panel::draw(UiPainter& painter) const {
    if (!visible) return;
    painter.fill(bounds, UiColor::Panel);
    painter.outline(bounds, UiColor::Border);
    for (const auto& child : children_) {
        if (child->visible) child->draw(painter);
    }
}

void Panel::drawOverlay(UiPainter& painter) const {
    if (!visible) return;
    for (const auto& child : children_) {
        if (child->visible) child->drawOverlay(painter);
    }
}

bool Panel::typing() const {
    return visible && std::any_of(children_.begin(), children_.end(), [](const auto& child) { return child->typing(); });
}

// --- ImageRenderer ---

Texture ImageRenderer::createTexture(const Image& image) {
    textures_.push_back(image);
    return {static_cast<int>(textures_.size()) - 1, image.width(), image.height()};
}

void ImageRenderer::draw(const Texture& texture, const Rect& source, Point at) {
    const Image& from = textures_.at(static_cast<std::size_t>(texture.id));
    for (int y = 0; y < source.height; ++y) {
        for (int x = 0; x < source.width; ++x) {
            const int tx = at.x + x;
            const int ty = at.y + y;
            if (tx < 0 || ty < 0 || tx >= target_.width() || ty >= target_.height()) continue;
            const Color c = from.get(source.x + x, source.y + y);
            if (c.alpha == 0) continue;
            const Color under = target_.get(tx, ty);
            auto mix = [&](int top, int bottom) { return static_cast<std::uint8_t>((top * c.alpha + bottom * (255 - c.alpha)) / 255); };
            target_.set(tx, ty, {mix(c.red, under.red), mix(c.green, under.green), mix(c.blue, under.blue), 255});
        }
    }
}

} // namespace luna::engine
