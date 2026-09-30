#pragma once

#include "boundary.h"

#include "image.h"
#include "input.h"
#include "renderer.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace luna::engine {

// Luna's small UI toolkit (US-121): a bitmap font, colours, and widgets, drawn with the same
// texture-only renderer as everything else. Game-agnostic: an editor, a menu or a shop can use it.

// Every colour the toolkit can fill or write with. The UI sheet has a text row and a solid
// swatch for each.
enum class UiColor { Text, Dim, Gold, Red, Dark, Panel, PanelLight, Border, Selected, Hover, Grid, Shade, Count };

inline constexpr int kGlyphWidth = 5;
inline constexpr int kGlyphHeight = 7;
inline constexpr int kTextAdvance = 6;   // a glyph and one pixel of space
inline constexpr int kLineHeight = 9;    // a line of text and two pixels of space

// The picture the toolkit draws from: the font once per colour, then the colour swatches.
Image makeUiSheet();

// Is pixel (column, row) of this printable ASCII glyph lit? Other characters draw as '?'.
bool glyphPixel(char c, int column, int row);

// Draws UI shapes and text with the UI sheet's texture.
class UiPainter {
public:
    UiPainter(Renderer& renderer, const Texture& sheet) : renderer_(renderer), sheet_(sheet) {}

    void fill(const Rect& area, UiColor color);
    void outline(const Rect& area, UiColor color); // a 1-pixel frame just inside `area`
    void text(int x, int y, std::string_view text, UiColor color);
    void image(const Texture& texture, const Rect& source, Point at) { renderer_.draw(texture, source, at); }

    // The visible screen, so pop-ups (hints) can stay inside it. Unset: anywhere.
    void setScreen(const Rect& screen) { screen_ = screen; }
    // `box` moved (not resized) so it lies inside the screen where it can.
    Rect keepOnScreen(Rect box) const;

    static int textWidth(std::string_view text) {
        return text.empty() ? 0 : static_cast<int>(text.size()) * kTextAdvance - 1;
    }

private:
    Renderer& renderer_;
    const Texture& sheet_;
    Rect screen_{0, 0, 1 << 20, 1 << 20};
};

// What a widget sees of the player's input in one tick.
struct UiInput {
    Pointer pointer;
    std::string text;     // typed characters
    bool erase = false;   // Backspace
    bool confirm = false; // Enter

    static UiInput from(const Intents& intents) {
        return {intents.pointer(), intents.text(), intents.pressed(Intent::Erase), intents.pressed(Intent::Confirm)};
    }
};

// A rectangle on screen that draws itself and may react to the pointer. "virtual" functions
// let every kind of widget answer in its own way while panels treat them all alike.
class Widget {
public:
    explicit Widget(Rect bounds) : bounds(bounds) {}
    virtual ~Widget() = default;

    Rect bounds;
    bool visible = true;

    bool contains(int x, int y) const {
        return x >= bounds.x && y >= bounds.y && x < bounds.x + bounds.width && y < bounds.y + bounds.height;
    }
    // Returns true when the widget used the input, so nothing behind it should.
    virtual bool handle(const UiInput& /*input*/) { return false; }
    virtual void draw(UiPainter& painter) const = 0;
    // Drawn after every widget (hover labels), so nothing covers it.
    virtual void drawOverlay(UiPainter& /*painter*/) const {}
    // True while the widget takes typed text (then keyboard shortcuts must wait).
    virtual bool typing() const { return false; }
};

// A clickable button with a text label or an icon, and a hint shown on hover.
class Button final : public Widget {
public:
    Button(Rect bounds, std::string label, std::function<void()> onClick)
        : Widget(bounds), label(std::move(label)), onClick(std::move(onClick)) {}

    std::string label;
    std::string hint;          // shown next to the pointer while it rests on the button
    bool selected = false;     // drawn highlighted (the chosen tool)
    const Texture* icon = nullptr;
    Rect iconSource{};
    std::function<void()> onClick;

    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;
    void drawOverlay(UiPainter& painter) const override;

private:
    bool hovered_ = false;
    int hoverX_ = 0;
    int hoverY_ = 0;
};

// A scrolling list of names; one may be selected.
class ListBox final : public Widget {
public:
    ListBox(Rect bounds, std::vector<std::string> items, std::function<void(int)> onSelect)
        : Widget(bounds), items(std::move(items)), onSelect(std::move(onSelect)) {}

    std::vector<std::string> items;
    int selected = -1;
    int first = 0; // the top visible row (scrolled)
    std::function<void(int)> onSelect;

    int rows() const { return bounds.height / kLineHeight; }
    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;
};

// A labelled whole number: click, type digits, Enter (or click elsewhere) to keep it; the wheel
// steps it by one. The value always stays between minimum and maximum.
class NumberField final : public Widget {
public:
    NumberField(Rect bounds, std::string label, int value, int minimum, int maximum, std::function<void(int)> onChange)
        : Widget(bounds), label(std::move(label)), value(value), minimum(minimum), maximum(maximum), onChange(std::move(onChange)) {}

    std::string label;
    int value;
    int minimum;
    int maximum;
    std::function<void(int)> onChange;

    bool focused() const { return focused_; }
    bool typing() const override { return focused_; }
    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;

private:
    void commit();
    bool focused_ = false;
    std::string editing_;
};

// A labelled line of text: click, type, Backspace, Enter (or click elsewhere) to keep it.
class TextField final : public Widget {
public:
    TextField(Rect bounds, std::string label, std::string value, std::size_t maxLength, std::function<void(const std::string&)> onChange)
        : Widget(bounds), label(std::move(label)), value(std::move(value)), maxLength(maxLength), onChange(std::move(onChange)) {}

    std::string label;
    std::string value;
    std::size_t maxLength;
    std::function<void(const std::string&)> onChange;

    bool focused() const { return focused_; }
    bool typing() const override { return focused_; }
    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;

private:
    void commit();
    bool focused_ = false;
    std::string editing_;
};

// A background with widgets on it. It owns them (std::unique_ptr): when the panel goes, so do
// they. A click on the panel's background is used too, so the world behind does not get it.
class Panel final : public Widget {
public:
    explicit Panel(Rect bounds) : Widget(bounds) {}

    template <typename T, typename... Args>
    T& add(Args&&... args) {
        auto widget = std::make_unique<T>(std::forward<Args>(args)...);
        T& added = *widget;
        children_.push_back(std::move(widget));
        return added;
    }
    void clear() { children_.clear(); }
    const std::vector<std::unique_ptr<Widget>>& children() const { return children_; }

    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;
    void drawOverlay(UiPainter& painter) const override;
    bool typing() const override;

private:
    std::vector<std::unique_ptr<Widget>> children_;
};

// A renderer that draws into a picture in memory instead of a window, blending by alpha.
// Tests use it to check UI pixels exactly; tools use it to save screens.
class ImageRenderer final : public Renderer {
public:
    ImageRenderer(int width, int height) : target_(width, height) {}

    Texture createTexture(const Image& image) override;
    void draw(const Texture& texture, const Rect& source, Point at) override;
    void drawStyled(const Texture& texture, const Rect& source, const Rect& destination, DrawStyle style) override;
    const Image& image() const { return target_; }
    void clear(Color color) { target_.fillRect(0, 0, target_.width(), target_.height(), color); }

private:
    Image target_;
    std::vector<Image> textures_;
};

} // namespace luna::engine
