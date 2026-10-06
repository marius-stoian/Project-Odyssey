#pragma once

#include "boundary.h"

#include "game/content_art.h"
#include "luna/engine/image.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The frames of the content atlas as pictures to pick from (US-192). A click on a picture reports its name; resting on one shows the name.
class PictureGrid final : public luna::engine::Widget {
public:
    struct Cell {
        std::string name;
        luna::engine::Texture texture;
        luna::engine::Rect source; // the frame in its page
    };
    PictureGrid(luna::engine::Rect bounds, int cellWidth, int cellHeight, std::vector<Cell> cells, std::string current, std::function<void(const std::string&)> onPick);

    static constexpr int kPad = 4;
    int columns() const { return std::max(1, bounds.width / (cellWidth_ + kPad)); }
    int visibleRows() const { return std::max(1, bounds.height / (cellHeight_ + kPad)); }
    const std::vector<Cell>& cells() const { return cells_; }
    // Where the cell `index` is drawn now, or nothing when it is scrolled out of sight.
    std::optional<luna::engine::Rect> cellRect(std::size_t index) const;

    bool handle(const luna::engine::UiInput& input) override;
    void draw(luna::engine::UiPainter& painter) const override;
    void drawOverlay(luna::engine::UiPainter& painter) const override;

private:
    int cellWidth_;
    int cellHeight_;
    std::vector<Cell> cells_;
    std::string current_;
    std::function<void(const std::string&)> onPick_;
    int scroll_ = 0; // rows
    int hover_ = -1;
    int hoverX_ = 0;
    int hoverY_ = 0;
};

// One of the owner's sheets at its own size, with a rectangle to drag on it. The wheel scrolls it and the right button drags it; the rectangle is in the sheet's pixels.
class SheetView final : public luna::engine::Widget {
public:
    SheetView(luna::engine::Rect bounds, luna::engine::Texture sheet, std::function<void(const luna::engine::Rect&)> onRect);

    std::optional<luna::engine::Rect> selection() const;
    void clearSelection() { dragging_ = false, hasSelection_ = false; }
    // The sheet's pixel under a screen pixel.
    luna::engine::Point sheetPoint(int x, int y) const;

    bool handle(const luna::engine::UiInput& input) override;
    void draw(luna::engine::UiPainter& painter) const override;

private:
    luna::engine::Texture sheet_;
    std::function<void(const luna::engine::Rect&)> onRect_;
    int offsetX_ = 0;
    int offsetY_ = 0;
    bool dragging_ = false;
    bool hasSelection_ = false;
    luna::engine::Point from_{};
    luna::engine::Point to_{};
    int lastX_ = -1;
    int lastY_ = -1;
};

// The picture tools of the Data tab (US-192): the **picker** a field that holds an atlas frame opens, and the **Cut tool** that takes a new frame from a sheet. Whole-screen,
// like the other tools; Esc closes it. The pictures are made into textures by the game (`MakeTexture`), so the tool draws with the renderer in use.
class PictureTool {
public:
    using MakeTexture = std::function<luna::engine::Texture(const luna::engine::Image&)>;
    using Say = std::function<void(const std::string&)>;
    PictureTool(int width, int height, std::filesystem::path sprites, MakeTexture makeTexture, Say say);

    // The frames of the content atlas as a grid of pictures, one page at a time; picking one calls `onPick` with its name and closes the tool.
    void openPicker(const std::string& title, const std::string& current, std::function<void(const std::string&)> onPick);
    // Take a frame from a sheet: choose the sheet and where the cut goes, drag a rectangle, name it, press Cut. `onCut` is called after the atlas was cut again.
    void openCutter(std::function<void()> onCut);

    bool open() const { return mode_ != Mode::None; }
    bool typing() const { return panel_ != nullptr && panel_->typing(); }
    const luna::engine::Panel* panel() const { return panel_.get(); }
    const std::string& message() const { return message_; }

    // The cut from the screen's own state (what the Cut button does): the chosen sheet, rectangle, name and target.
    bool cut();
    void chooseSheet(const std::string& sheet);
    void chooseTarget(const std::string& target);
    void setName(const std::string& name) { name_ = name; }
    void setRect(const luna::engine::Rect& rect) { rect_ = rect; }
    const std::string& sheet() const { return sheet_; }
    const std::optional<luna::engine::Rect>& rect() const { return rect_; }

    // false once the tool has closed.
    bool update(const luna::engine::Intents& intents);
    void draw(luna::engine::UiPainter& painter) const;
    void drawOverlay(luna::engine::UiPainter& painter) const;

private:
    enum class Mode { None, Pick, Cut };
    void buildPicker();
    void buildCutter();
    void loadContent();
    void say(const std::string& text);

    int width_;
    int height_;
    std::filesystem::path sprites_;
    MakeTexture makeTexture_;
    Say say_;
    Mode mode_ = Mode::None;
    std::unique_ptr<luna::engine::Panel> panel_;
    bool stale_ = true;
    bool closed_ = false;
    std::string message_;
    // the picker
    std::string title_;
    std::string current_;
    std::string page_;
    std::function<void(const std::string&)> onPick_;
    std::optional<ContentAtlas> content_;
    std::map<std::string, luna::engine::Texture> pageTextures_;
    // the Cut tool
    std::function<void()> onCut_;
    std::vector<std::string> sheets_;
    std::vector<std::string> targets_;
    std::string sheet_;
    std::string target_ = "character";
    std::string name_;
    std::optional<luna::engine::Rect> rect_;
    luna::engine::Texture sheetTexture_;
    bool hasSheet_ = false;
};

} // namespace odysseus::game
