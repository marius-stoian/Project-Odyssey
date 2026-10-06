#include "game/picture_tool.h"

#include "game/atlas_cuts.h"
#include "luna/engine/image_io.h"

#include <algorithm>
#include <format>

namespace odysseus::game {

using luna::engine::Button;
using luna::engine::Intent;
using luna::engine::Label;
using luna::engine::ListBox;
using luna::engine::Panel;
using luna::engine::PointerButton;
using luna::engine::Rect;
using luna::engine::TextField;
using luna::engine::UiColor;
using luna::engine::UiInput;
using luna::engine::UiPainter;

// ---- PictureGrid

PictureGrid::PictureGrid(Rect bounds, int cellWidth, int cellHeight, std::vector<Cell> cells, std::string current, std::function<void(const std::string&)> onPick)
    : Widget(bounds), cellWidth_(cellWidth), cellHeight_(cellHeight), cells_(std::move(cells)), current_(std::move(current)), onPick_(std::move(onPick)) {}

std::optional<Rect> PictureGrid::cellRect(std::size_t index) const {
    const int row = static_cast<int>(index) / columns() - scroll_;
    if (row < 0 || row >= visibleRows()) return std::nullopt;
    const int column = static_cast<int>(index) % columns();
    return Rect{bounds.x + column * (cellWidth_ + kPad), bounds.y + row * (cellHeight_ + kPad), cellWidth_, cellHeight_};
}

bool PictureGrid::handle(const UiInput& input) {
    const luna::engine::Pointer& pointer = input.pointer;
    hover_ = -1;
    if (!contains(pointer.x, pointer.y)) return false;
    hoverX_ = pointer.x;
    hoverY_ = pointer.y;
    if (pointer.wheel != 0) {
        const int rows = (static_cast<int>(cells_.size()) + columns() - 1) / columns();
        scroll_ = std::clamp(scroll_ - pointer.wheel, 0, std::max(0, rows - visibleRows()));
    }
    for (std::size_t i = 0; i < cells_.size(); ++i) {
        const std::optional<Rect> rect = cellRect(i);
        if (rect && pointer.x >= rect->x && pointer.y >= rect->y && pointer.x < rect->x + rect->width && pointer.y < rect->y + rect->height) {
            hover_ = static_cast<int>(i);
            break;
        }
    }
    if (pointer.wasPressed(PointerButton::Left) && hover_ >= 0 && onPick_) onPick_(cells_[static_cast<std::size_t>(hover_)].name);
    return true;
}

void PictureGrid::draw(UiPainter& painter) const {
    painter.fill(bounds, UiColor::Dark);
    for (std::size_t i = 0; i < cells_.size(); ++i) {
        const std::optional<Rect> rect = cellRect(i);
        if (!rect) continue;
        painter.fill(*rect, static_cast<int>(i) == hover_ ? UiColor::Hover : UiColor::Panel);
        const Cell& cell = cells_[i];
        const int x = rect->x + (rect->width - cell.source.width) / 2;
        const int y = rect->y + (rect->height - cell.source.height) / 2;
        if (cell.texture.id >= 0) painter.image(cell.texture, cell.source, {x, y});
        if (cell.name == current_) painter.outline(*rect, UiColor::Gold);
    }
    painter.outline(bounds, UiColor::Border);
}

void PictureGrid::drawOverlay(UiPainter& painter) const {
    if (hover_ < 0) return;
    const std::string& name = cells_[static_cast<std::size_t>(hover_)].name;
    const int width = UiPainter::textWidth(name) + 6;
    const Rect box{std::min(hoverX_ + 8, bounds.x + bounds.width - width), hoverY_ + 10, width, luna::engine::kLineHeight + 4};
    painter.fill(box, UiColor::Shade);
    painter.outline(box, UiColor::Gold);
    painter.text(box.x + 3, box.y + 3, name, UiColor::Gold);
}

// ---- SheetView

SheetView::SheetView(Rect bounds, luna::engine::Texture sheet, std::function<void(const Rect&)> onRect) : Widget(bounds), sheet_(sheet), onRect_(std::move(onRect)) {}

luna::engine::Point SheetView::sheetPoint(int x, int y) const {
    return {std::clamp(x - bounds.x + offsetX_, 0, std::max(0, sheet_.width - 1)), std::clamp(y - bounds.y + offsetY_, 0, std::max(0, sheet_.height - 1))};
}

std::optional<Rect> SheetView::selection() const {
    if (!hasSelection_ && !dragging_) return std::nullopt;
    const int x = std::min(from_.x, to_.x);
    const int y = std::min(from_.y, to_.y);
    return Rect{x, y, std::abs(to_.x - from_.x) + 1, std::abs(to_.y - from_.y) + 1};
}

bool SheetView::handle(const UiInput& input) {
    const luna::engine::Pointer& pointer = input.pointer;
    const bool inside = contains(pointer.x, pointer.y);
    if (dragging_) {
        to_ = sheetPoint(pointer.x, pointer.y);
        if (!pointer.isHeld(PointerButton::Left)) {
            dragging_ = false;
            hasSelection_ = true;
            if (onRect_ && selection()) onRect_(*selection());
        }
        return true;
    }
    if (pointer.isHeld(PointerButton::Right) && inside && lastX_ >= 0) { // the right button drags the sheet
        offsetX_ = std::clamp(offsetX_ - (pointer.x - lastX_), 0, std::max(0, sheet_.width - bounds.width));
        offsetY_ = std::clamp(offsetY_ - (pointer.y - lastY_), 0, std::max(0, sheet_.height - bounds.height));
    }
    lastX_ = pointer.isHeld(PointerButton::Right) && inside ? pointer.x : -1;
    lastY_ = pointer.y;
    if (!inside) return false;
    if (pointer.wheel != 0) offsetY_ = std::clamp(offsetY_ - pointer.wheel * 48, 0, std::max(0, sheet_.height - bounds.height));
    if (pointer.wasPressed(PointerButton::Left)) {
        dragging_ = true;
        from_ = to_ = sheetPoint(pointer.x, pointer.y);
    }
    return true;
}

void SheetView::draw(UiPainter& painter) const {
    painter.fill(bounds, UiColor::Dark);
    const int width = std::min(bounds.width, sheet_.width - offsetX_);
    const int height = std::min(bounds.height, sheet_.height - offsetY_);
    if (width > 0 && height > 0 && sheet_.id >= 0) painter.image(sheet_, {offsetX_, offsetY_, width, height}, {bounds.x, bounds.y});
    if (const std::optional<Rect> chosen = selection()) {
        Rect box{bounds.x + chosen->x - offsetX_, bounds.y + chosen->y - offsetY_, chosen->width, chosen->height};
        painter.outline(box, UiColor::Gold);
    }
    painter.outline(bounds, UiColor::Border);
}

// ---- PictureTool

PictureTool::PictureTool(int width, int height, std::filesystem::path sprites, MakeTexture makeTexture, Say say)
    : width_(width), height_(height), sprites_(std::move(sprites)), makeTexture_(std::move(makeTexture)), say_(std::move(say)) {}

void PictureTool::say(const std::string& text) {
    message_ = text;
    stale_ = true;
    if (say_) say_(text);
}

void PictureTool::loadContent() {
    pageTextures_.clear();
    std::string problem;
    content_ = game::loadContent(sprites_ / "atlas", problem);
    if (!content_) message_ = "the atlas cannot be read: " + problem;
}

void PictureTool::openPicker(const std::string& title, const std::string& current, std::function<void(const std::string&)> onPick) {
    mode_ = Mode::Pick;
    closed_ = false;
    title_ = title;
    current_ = current;
    onPick_ = std::move(onPick);
    message_.clear();
    loadContent();
    page_.clear();
    if (content_) {
        // Start on the page of the current frame, else the first page.
        const auto frame = content_->frames.find(current_);
        page_ = frame != content_->frames.end() ? frame->second.page : (content_->pages.empty() ? std::string() : content_->pages.front().name);
    }
    stale_ = true;
}

void PictureTool::openCutter(std::function<void()> onCut) {
    mode_ = Mode::Cut;
    closed_ = false;
    onCut_ = std::move(onCut);
    message_.clear();
    sheets_ = cutSheets(sprites_);
    targets_ = cutTargets(sprites_);
    sheet_.clear();
    hasSheet_ = false;
    rect_.reset();
    name_.clear();
    target_ = "character";
    stale_ = true;
}

void PictureTool::chooseSheet(const std::string& sheet) {
    std::string error;
    const auto image = luna::engine::loadPng(sprites_ / sheet, error);
    if (!image) {
        say(sheet + " " + error);
        return;
    }
    sheet_ = sheet;
    sheetTexture_ = makeTexture_ ? makeTexture_(*image) : luna::engine::Texture{};
    hasSheet_ = true;
    rect_.reset();
    stale_ = true;
}

void PictureTool::chooseTarget(const std::string& target) {
    target_ = target;
    stale_ = true;
}

bool PictureTool::cut() {
    if (sheet_.empty() || !rect_) {
        say("choose a sheet and drag a rectangle on it first");
        return false;
    }
    std::string summary;
    const std::optional<std::string> problem = addCut(sprites_, {sheet_, {rect_->x, rect_->y, rect_->width, rect_->height}, name_, target_}, &summary);
    if (problem) {
        say(*problem);
        return false;
    }
    say(std::format("cut \"{}\" added to {}; the atlas is cut again: {}", name_, target_ == "character" || target_ == "tile" ? "cuts.json" : "content-cuts.json", summary));
    name_.clear();
    rect_.reset();
    if (onCut_) onCut_();
    return true;
}

void PictureTool::buildPicker() {
    panel_ = std::make_unique<Panel>(Rect{0, 0, width_, height_});
    panel_->add<Label>(Rect{4, 2, width_ - 8, 10}, std::format("Pick a picture for {} (now: {}). Click one; Esc leaves.", title_, current_.empty() ? "none" : current_), UiColor::Gold);
    if (!content_) {
        panel_->add<Label>(Rect{4, 16, width_ - 8, 10}, message_, UiColor::Red);
        return;
    }
    std::vector<std::string> pages;
    int selected = -1;
    for (const ContentPage& page : content_->pages) {
        int count = 0;
        for (const auto& [name, frame] : content_->frames) count += frame.page == page.name ? 1 : 0;
        pages.push_back(std::format("{} ({})", page.name, count));
        if (page.name == page_) selected = static_cast<int>(pages.size()) - 1;
    }
    ListBox& list = panel_->add<ListBox>(Rect{4, 16, 120, height_ - 22}, pages, [this](int index) {
        if (index >= 0 && static_cast<std::size_t>(index) < content_->pages.size()) {
            page_ = content_->pages[static_cast<std::size_t>(index)].name;
            stale_ = true;
        }
    });
    list.selected = selected;
    const ContentPage* page = content_->page(page_);
    if (page == nullptr) return;
    luna::engine::Texture& texture = pageTextures_[page_];
    if (texture.id < 0 && makeTexture_) texture = makeTexture_(content_->pictures.at(page_));
    std::vector<PictureGrid::Cell> cells;
    for (const auto& [name, frame] : content_->frames) {
        if (frame.page != page_) continue;
        if (const std::optional<core::Rect> rect = content_->rect(name)) cells.push_back({name, texture, {rect->x, rect->y, rect->width, rect->height}});
    }
    panel_->add<PictureGrid>(Rect{130, 16, width_ - 134, height_ - 22}, page->cellWidth, page->cellHeight, std::move(cells), current_, [this](const std::string& name) {
        if (onPick_) onPick_(name);
        closed_ = true;
    });
}

void PictureTool::buildCutter() {
    panel_ = std::make_unique<Panel>(Rect{0, 0, width_, height_});
    panel_->add<Label>(Rect{4, 2, width_ - 8, 10}, "CUT TOOL: drag a rectangle on the sheet (left button); the wheel and the right button move the sheet. Esc leaves.", UiColor::Gold);
    constexpr int kLeft = 170;
    int y = 16;
    panel_->add<Label>(Rect{4, y, kLeft, 10}, "Sheet:", UiColor::Dim);
    y += 10;
    ListBox& sheets = panel_->add<ListBox>(Rect{4, y, kLeft, 90}, sheets_, [this](int index) {
        if (index >= 0 && static_cast<std::size_t>(index) < sheets_.size() && sheets_[static_cast<std::size_t>(index)] != sheet_) chooseSheet(sheets_[static_cast<std::size_t>(index)]);
    });
    for (std::size_t i = 0; i < sheets_.size(); ++i) {
        if (sheets_[i] == sheet_) sheets.selected = static_cast<int>(i);
    }
    y += 94;
    panel_->add<Label>(Rect{4, y, kLeft, 10}, "The cut goes to:", UiColor::Dim);
    y += 10;
    ListBox& targets = panel_->add<ListBox>(Rect{4, y, kLeft, 63}, targets_, [this](int index) {
        if (index >= 0 && static_cast<std::size_t>(index) < targets_.size()) chooseTarget(targets_[static_cast<std::size_t>(index)]);
    });
    for (std::size_t i = 0; i < targets_.size(); ++i) {
        if (targets_[i] == target_) targets.selected = static_cast<int>(i);
    }
    y += 68;
    panel_->add<TextField>(Rect{4, y, kLeft, 11}, "name: ", name_, 48, [this](const std::string& value) { name_ = value; });
    y += 14;
    panel_->add<Label>(Rect{4, y, kLeft, 10}, rect_ ? std::format("rectangle {} {} {} x {}", rect_->x, rect_->y, rect_->width, rect_->height) : std::string("no rectangle yet"), UiColor::Dim);
    y += 12;
    panel_->add<Button>(Rect{4, y, 50, 12}, "Cut", [this] { cut(); }).hint = "Add the cut to the cuts file and cut the atlas again";
    panel_->add<Button>(Rect{58, y, 50, 12}, "Close", [this] { closed_ = true; });
    y += 16;
    if (!message_.empty()) {
        // the message under the buttons, broken into lines that fit the column
        std::string rest = message_;
        const std::size_t columns = static_cast<std::size_t>(kLeft / luna::engine::kTextAdvance);
        while (!rest.empty() && y < height_ - 10) {
            const std::size_t take = std::min(columns, rest.size());
            panel_->add<Label>(Rect{4, y, kLeft, 10}, rest.substr(0, take), UiColor::Gold);
            rest.erase(0, take);
            y += 10;
        }
    }
    if (hasSheet_) {
        panel_->add<SheetView>(Rect{kLeft + 10, 16, width_ - kLeft - 14, height_ - 20}, sheetTexture_, [this](const Rect& rect) {
            rect_ = rect;
            stale_ = true;
        });
    } else {
        panel_->add<Label>(Rect{kLeft + 10, 20, width_ - kLeft - 14, 10}, "Choose a sheet on the left.", UiColor::Dim);
    }
}

bool PictureTool::update(const luna::engine::Intents& intents) {
    if (mode_ == Mode::None) return false;
    if (stale_ && !typing()) {
        stale_ = false;
        if (mode_ == Mode::Pick) buildPicker();
        else buildCutter();
    }
    if (panel_ != nullptr) panel_->handle(UiInput::from(intents));
    if (!typing() && intents.pressed(Intent::OpenMenu)) closed_ = true;
    if (closed_) {
        mode_ = Mode::None;
        panel_.reset();
        pageTextures_.clear();
        return false;
    }
    return true;
}

void PictureTool::draw(UiPainter& painter) const {
    if (panel_ == nullptr) return;
    painter.fill({0, 0, width_, height_}, UiColor::Panel);
    panel_->draw(painter);
}

void PictureTool::drawOverlay(UiPainter& painter) const {
    if (panel_ != nullptr) panel_->drawOverlay(painter);
}

} // namespace odysseus::game
