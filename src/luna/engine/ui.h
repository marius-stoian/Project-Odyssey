#pragma once

#include "boundary.h"

#include "image.h"
#include "input.h"
#include "renderer.h"

#include <functional>
#include <memory>
#include <optional>
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
    const Rect& screen() const { return screen_; }

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
    bool up = false;      // the arrow keys, Tab and Escape, for an open suggestion list (US-301)
    bool down = false;
    bool tab = false;
    bool escape = false;

    static UiInput from(const Intents& intents) {
        return {intents.pointer(), intents.text(), intents.pressed(Intent::Erase), intents.pressed(Intent::Confirm), intents.pressed(Intent::ListUp),
                intents.pressed(Intent::ListDown), intents.pressed(Intent::ListTab), intents.pressed(Intent::ListEscape)};
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

// The tooltip of a field (D-58, D-59): its lines show once the pointer has rested on the field for kDelayTicks, and go when the pointer
// moves, leaves, or the field is used. Game-agnostic: the game decides the text (purpose, range, example), one line each.
class FieldHint {
public:
    static constexpr int kDelayTicks = 8;    // 0.4 s at 20 ticks a second (D-59)
    static constexpr int kWrapColumns = 60;  // letters in a line of the tooltip before it breaks

    std::string text; // lines separated by '\n'; empty: no tooltip
    // Call once a tick: `resting` is true while the pointer is over the field and the field is not being typed in.
    void track(bool resting, int x, int y);
    bool showing() const { return !text.empty() && ticks_ >= kDelayTicks; }
    void draw(UiPainter& painter) const;

private:
    int ticks_ = 0;
    int x_ = 0;
    int y_ = 0;
};
// The list of values under a field (US-301, D-58, D-59). It opens when the field gets focus with every value; typing filters it: the values that
// start with the typed text first, then the ones that contain it, any letter case. At most kMaxRows rows show; Up and Down move the highlight and
// scroll. Tab accepts the highlighted row; Enter accepts it only after Up or Down (so Enter on text typed from scratch keeps that text, as before);
// Escape closes the list and keeps the text; a click on a row accepts it. Closed (or with no row left) it uses no input at all.
class SuggestList {
public:
    static constexpr int kMaxRows = 8;

    // Opens with these values, all shown. The typed text filters only once it changes (see filter).
    void open(std::vector<std::string> values);
    // The text of the field changed: shows the values that match it, and opens the list again if Escape had closed it.
    void filter(const std::string& typed);
    void close() { open_ = false; }
    bool isOpen() const { return open_ && !rows_.empty(); }
    const std::vector<std::string>& rows() const { return rows_; }
    int highlighted() const { return highlight_; }

    // Handles the keys and clicks of one tick. True when the list used the input; `accepted` is then set if a row was chosen.
    bool handle(const UiInput& input, std::optional<std::string>& accepted);
    // Draws under `field` (above it where it would leave the screen). Remembers where, for the clicks of the next tick.
    void draw(UiPainter& painter, const Rect& field) const;

private:
    void move(int step);

    std::vector<std::string> values_;
    std::vector<std::string> rows_;
    int highlight_ = 0;
    int first_ = 0;          // the top visible row
    bool open_ = false;
    bool navigated_ = false; // Up or Down was pressed since the rows last changed
    mutable Rect shown_{};   // where the rows were drawn last
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
    std::string helpId; // the field's entry in help.json (editor_help); empty: none
    FieldHint tip;
    // The values to offer while the field has focus (US-301): given the digits typed so far, numbers written as text. Empty: no list.
    std::function<std::vector<std::string>(const std::string& typed)> suggest;

    bool focused() const { return focused_; }
    bool typing() const override { return focused_; }
    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;
    void drawOverlay(UiPainter& painter) const override;
    const SuggestList& suggestions() const { return list_; }

private:
    void commit();
    Rect box() const;
    bool focused_ = false;
    std::string editing_;
    SuggestList list_;
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
    std::string helpId; // the field's entry in help.json (editor_help); empty: none
    FieldHint tip;
    // The values to offer while the field has focus (US-301): given the text typed so far (with listItems, the item after the last comma).
    // Empty: no list.
    std::function<std::vector<std::string>(const std::string& typed)> suggest;
    bool listItems = false; // a list of words (separated by commas or spaces): the suggestion completes the word after the last separator and keeps what is before it
    bool invalid = false;   // the value breaks a rule: the frame is red (a form shows the rule next to it)

    bool focused() const { return focused_; }
    bool typing() const override { return focused_; }
    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;
    void drawOverlay(UiPainter& painter) const override;
    const SuggestList& suggestions() const { return list_; }

private:
    void commit();
    Rect box() const;
    std::string typedItem() const; // what the list filters by: the whole text, or the item after the last comma
    bool focused_ = false;
    std::string editing_;
    SuggestList list_;
};

// A line of text that does nothing: the label of a form row, a heading, a note.
class Label final : public Widget {
public:
    Label(Rect bounds, std::string text, UiColor color = UiColor::Dim) : Widget(bounds), text(std::move(text)), color(color) {}

    std::string text;
    UiColor color;

    void draw(UiPainter& painter) const override;
};

// A labelled yes or no: a click flips it. It looks like a text field (the label, then a box that says yes or no), so a form lines up.
class Toggle final : public Widget {
public:
    Toggle(Rect bounds, std::string label, bool value, std::function<void(bool)> onChange)
        : Widget(bounds), label(std::move(label)), value(value), onChange(std::move(onChange)) {}

    std::string label;
    bool value;
    std::function<void(bool)> onChange;
    std::string helpId; // the field's entry in help.json (editor_help); empty: none
    FieldHint tip;
    bool invalid = false;

    bool handle(const UiInput& input) override;
    void draw(UiPainter& painter) const override;
    void drawOverlay(UiPainter& painter) const override;

private:
    Rect box() const;
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
