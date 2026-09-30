#include "game/editor.h"

#include "core/log.h"
#include "sim/data.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace odysseus::game {

using luna::engine::Button;
using luna::engine::Intent;
using luna::engine::Intents;
using luna::engine::Panel;
using luna::engine::PointerButton;
using luna::engine::Rect;
using luna::engine::UiColor;
using luna::engine::UiPainter;

namespace {
constexpr int kStatusTicks = 60;  // a message stays three seconds
constexpr int kPaletteColumns = 3;
constexpr int kPaletteCell = 22;  // a tile button: a 20x20 piece of the tile and a border
constexpr int kToolbarHeight = 18;
} // namespace

const char* toolName(EditorTool tool) {
    switch (tool) {
    case EditorTool::Brush: return "Brush";
    case EditorTool::Rectangle: return "Rectangle";
    case EditorTool::Fill: return "Fill";
    case EditorTool::Eraser: return "Eraser";
    }
    return "?";
}

Editor::Editor(Level& level, const Definitions& definitions, std::filesystem::path levelFile, int viewWidth, int viewHeight)
    : level_(level), definitions_(definitions), levelFile_(std::move(levelFile)), viewWidth_(viewWidth), viewHeight_(viewHeight),
      map_(buildTileMap(level, definitions)), camera_(viewWidth, viewHeight, map_.pixelWidth(), map_.pixelHeight()) {
    buildPanels();
}

void Editor::setTextures(const EditorTextures& textures) {
    textures_ = textures;
    buildPanels(); // the palette's buttons show pieces of the tile texture
}

void Editor::buildPanels() {
    toolbar_ = std::make_unique<Panel>(Rect{2, 2, 300, kToolbarHeight});
    int x = 4;
    auto button = [&](const std::string& label, const std::string& hint, auto action) -> Button& {
        const int width = UiPainter::textWidth(label) + 8;
        Button& added = toolbar_->add<Button>(Rect{x, 4, width, kToolbarHeight - 4}, label, action);
        added.hint = hint;
        x += width + 2;
        return added;
    };
    button("Brush", "Paint cells: click or drag", [this] { tool_ = EditorTool::Brush; });
    button("Rect", "Fill a rectangle: drag from corner to corner", [this] { tool_ = EditorTool::Rectangle; });
    button("Fill", "Fill the connected area of the same ground", [this] { tool_ = EditorTool::Fill; });
    button("Erase", "Paint the level's default ground", [this] { tool_ = EditorTool::Eraser; });
    x += 4;
    button("Grid", "Show or hide the grid (G)", [this] { grid_ = !grid_; });
    button("Undo", "Undo (Ctrl+Z)", [this] { undo(); });
    button("Redo", "Redo (Ctrl+Y)", [this] { redo(); });
    button("Save", "Save the level (Ctrl+S)", [this] { save(); });
    toolbar_->bounds.width = x;

    const int count = static_cast<int>(definitions_.tiles.size());
    const int rows = (count + kPaletteColumns - 1) / kPaletteColumns;
    palette_ = std::make_unique<Panel>(Rect{2, kToolbarHeight + 4, kPaletteColumns * kPaletteCell + 4, rows * kPaletteCell + 4});
    for (int i = 0; i < count; ++i) {
        const Rect cell{4 + (i % kPaletteColumns) * kPaletteCell, kToolbarHeight + 6 + (i / kPaletteColumns) * kPaletteCell, kPaletteCell - 2,
                        kPaletteCell - 2};
        Button& tile = palette_->add<Button>(cell, "", [this, i] {
            tile_ = i;
            if (tool_ == EditorTool::Eraser) tool_ = EditorTool::Brush; // choosing a ground means painting with it
        });
        tile.hint = definitions_.tiles[static_cast<std::size_t>(i)].name;
        tile.icon = &textures_.tiles;
        tile.iconSource = {i * kTileSize + 7, 7, kPaletteCell - 4, kPaletteCell - 4};
    }
}

void Editor::levelChanged() {
    map_ = buildTileMap(level_, definitions_);
    camera_ = luna::engine::Camera(viewWidth_, viewHeight_, map_.pixelWidth(), map_.pixelHeight());
    panTo(centreX_, centreY_);
    camera_.centreOn(centreX_, centreY_);
}

void Editor::enter(double centreX, double centreY) {
    centreX_ = centreX;
    centreY_ = centreY;
    levelChanged();
    tile_ = std::clamp(tile_, 0, static_cast<int>(definitions_.tiles.size()) - 1);
}

void Editor::panTo(double x, double y) {
    // The centre stays where a camera can look, so panning back responds at once.
    const double halfW = viewWidth_ / 2.0;
    const double halfH = viewHeight_ / 2.0;
    const double worldW = map_.pixelWidth();
    const double worldH = map_.pixelHeight();
    centreX_ = worldW <= viewWidth_ ? worldW / 2.0 : std::clamp(x, halfW, worldW - halfW);
    centreY_ = worldH <= viewHeight_ ? worldH / 2.0 : std::clamp(y, halfH, worldH - halfH);
}

void Editor::say(std::string message) {
    core::logInfo("Editor: " + message);
    status_ = std::move(message);
    statusTicks_ = kStatusTicks;
}

void Editor::run(std::unique_ptr<Command> command) {
    const std::string what = command->name();
    history_.run(std::move(command), level_);
    unsaved_ = true;
    levelChanged();
    say(what);
}

bool Editor::undo() {
    finishStroke();
    if (!history_.undo(level_)) {
        say("Nothing to undo");
        return false;
    }
    unsaved_ = true;
    levelChanged();
    say("Undone");
    return true;
}

bool Editor::redo() {
    if (!history_.redo(level_)) {
        say("Nothing to redo");
        return false;
    }
    unsaved_ = true;
    levelChanged();
    say("Redone");
    return true;
}

bool Editor::save() {
    finishStroke();
    try {
        saveLevel(level_, definitions_, levelFile_);
    } catch (const sim::DataError& error) {
        say(std::string("Not saved: ") + error.what());
        return false;
    }
    unsaved_ = false;
    say("Saved " + levelFile_.filename().string());
    return true;
}

std::optional<std::pair<int, int>> Editor::cellAt(int screenX, int screenY) const {
    if (screenX < 0 || screenY < 0) return std::nullopt;
    const Rect view = camera_.view();
    const int wx = view.x + screenX;
    const int wy = view.y + screenY;
    if (wx < 0 || wy < 0) return std::nullopt;
    const int cx = wx / kTileSize;
    const int cy = wy / kTileSize;
    if (!level_.inside(cx, cy)) return std::nullopt;
    return std::pair{cx, cy};
}

void Editor::paintAt(int x, int y) {
    if (strokeCells_.contains({x, y})) return;
    const int paint = tool_ == EditorTool::Eraser ? level_.defaultGround : tile_;
    strokeCells_.insert({x, y});
    if (level_.at(x, y) == paint) return;
    stroke_.push_back({x, y, level_.at(x, y), paint});
    level_.set(x, y, paint); // painted at once, so the owner sees the stroke as it grows
    map_.set(x, y, paint);
}

void Editor::finishStroke() {
    if (!stroking_) return;
    stroking_ = false;
    strokeCells_.clear();
    if (stroke_.empty()) return;
    auto command = std::make_unique<PaintCommand>(tool_ == EditorTool::Eraser ? "erase" : "paint " + definitions_.tiles[static_cast<std::size_t>(tile_)].name,
                                                  std::move(stroke_));
    stroke_.clear();
    say(command->name());
    history_.record(std::move(command)); // already on the level
    unsaved_ = true;
}

bool Editor::handlePanels(const luna::engine::UiInput& input) {
    for (auto& button : toolbar_->children()) {
        auto* b = dynamic_cast<Button*>(button.get());
        if (b == nullptr) continue;
        b->selected = (b->label == "Brush" && tool_ == EditorTool::Brush) || (b->label == "Rect" && tool_ == EditorTool::Rectangle) ||
                      (b->label == "Fill" && tool_ == EditorTool::Fill) || (b->label == "Erase" && tool_ == EditorTool::Eraser) ||
                      (b->label == "Grid" && grid_);
    }
    for (std::size_t i = 0; i < palette_->children().size(); ++i) {
        if (auto* b = dynamic_cast<Button*>(palette_->children()[i].get())) b->selected = static_cast<int>(i) == tile_;
    }
    const bool onToolbar = toolbar_->handle(input);
    const bool onPalette = palette_->handle(input);
    return onToolbar || onPalette;
}

void Editor::useTool(const luna::engine::Pointer& pointer, bool overPanel) {
    hover_ = overPanel ? std::nullopt : cellAt(pointer.x, pointer.y);
    const bool pressed = pointer.wasPressed(PointerButton::Left) && !overPanel;
    const bool held = pointer.isHeld(PointerButton::Left);
    const bool released = pointer.wasReleased(PointerButton::Left);

    if (tool_ == EditorTool::Brush || tool_ == EditorTool::Eraser) {
        if (pressed && hover_) {
            stroking_ = true;
            lastCell_ = *hover_;
            paintAt(hover_->first, hover_->second);
        }
        if (stroking_ && held && hover_) {
            for (const auto& [x, y] : lineCells(lastCell_.first, lastCell_.second, hover_->first, hover_->second)) {
                if (level_.inside(x, y)) paintAt(x, y);
            }
            lastCell_ = *hover_;
        }
        if (stroking_ && (released || !held)) finishStroke();
    } else if (tool_ == EditorTool::Rectangle) {
        if (pressed && hover_) {
            rectangleStart_ = *hover_;
            rectangleEnd_ = *hover_;
        }
        if (rectangleStart_ && hover_) rectangleEnd_ = *hover_;
        if (rectangleStart_ && (released || !held)) {
            auto changes = rectangleFill(level_, rectangleStart_->first, rectangleStart_->second, rectangleEnd_.first, rectangleEnd_.second, tile_);
            rectangleStart_.reset();
            if (!changes.empty()) run(std::make_unique<PaintCommand>("rectangle of " + definitions_.tiles[static_cast<std::size_t>(tile_)].name, std::move(changes)));
        }
    } else if (tool_ == EditorTool::Fill) {
        if (pressed && hover_) {
            auto changes = floodFill(level_, hover_->first, hover_->second, tile_);
            if (!changes.empty()) run(std::make_unique<PaintCommand>("fill with " + definitions_.tiles[static_cast<std::size_t>(tile_)].name, std::move(changes)));
        }
    }
}

void Editor::update(const Intents& intents) {
    if (statusTicks_ > 0 && --statusTicks_ == 0) status_.clear();
    const luna::engine::UiInput input = luna::engine::UiInput::from(intents);
    const bool overPanel = handlePanels(input);
    const bool typing = toolbar_->typing() || palette_->typing();
    if (!typing) {
        if (intents.pressed(Intent::Undo)) undo();
        if (intents.pressed(Intent::Redo)) redo();
        if (intents.pressed(Intent::Save)) save();
        if (intents.pressed(Intent::ToggleGrid)) grid_ = !grid_;
    }

    double x = centreX_ + (typing ? 0 : intents.moveX() * kPanPerTick);
    double y = centreY_ + (typing ? 0 : intents.moveY() * kPanPerTick);
    // Right-button drag: the world moves with the pointer, like sliding a map on a table.
    const auto& pointer = intents.pointer();
    if (pointer.isHeld(PointerButton::Right) && pointer.inside()) {
        if (dragging_) {
            x -= pointer.x - dragX_;
            y -= pointer.y - dragY_;
        }
        dragging_ = true;
        dragX_ = pointer.x;
        dragY_ = pointer.y;
    } else {
        dragging_ = false;
    }
    panTo(x, y);
    camera_.follow(centreX_, centreY_, 1.0);
    useTool(pointer, overPanel);
}

void Editor::render(luna::engine::Renderer& renderer, double alpha) const {
    map_.draw(renderer, textures_.tiles, camera_, alpha);
    const Rect view = camera_.view(alpha);
    auto screen = [&view](int worldX, int worldY) { return luna::engine::Point{worldX - view.x, worldY - view.y}; };
    UiPainter painter(renderer, textures_.ui);

    if (grid_) {
        for (int gx = (view.x / kTileSize) * kTileSize; gx < view.x + view.width; gx += kTileSize) {
            painter.fill({gx - view.x, 0, 1, view.height}, UiColor::Grid);
        }
        for (int gy = (view.y / kTileSize) * kTileSize; gy < view.y + view.height; gy += kTileSize) {
            painter.fill({0, gy - view.y, view.width, 1}, UiColor::Grid);
        }
    }
    for (const PixelPoint& target : level_.targets) {
        renderer.draw(textures_.props, kTargetFrame, screen(target.x - kTargetFrame.width / 2, target.y - kTargetFrame.height));
    }
    for (const PlacedCharacter& placed : level_.characters) {
        const CharacterKindDef* kind = definitions_.character(placed.kind);
        if (kind == nullptr || textures_.art == nullptr) continue;
        renderer.draw(textures_.characters, textures_.art->frame(kind->frames, kind->directions, placed.facing, 0),
                      screen(placed.feet.x - kCharacterWidth / 2, placed.feet.y - kCharacterHeight));
    }
    // Where the hero will begin: the hero, framed in gold, with a label.
    const auto start = screen(level_.heroStart.x - kCharacterWidth / 2, level_.heroStart.y - kCharacterHeight);
    renderer.draw(textures_.heroSheet, {0, 0, kCharacterWidth, kCharacterHeight}, start);
    painter.outline({start.x - 1, start.y - 1, kCharacterWidth + 2, kCharacterHeight + 2}, UiColor::Gold);
    painter.text(start.x + (kCharacterWidth - UiPainter::textWidth("START")) / 2, start.y - 9, "START", UiColor::Gold);

    // What the tool would do: the rectangle being dragged, or the cell under the pointer.
    if (rectangleStart_) {
        const int x0 = std::min(rectangleStart_->first, rectangleEnd_.first) * kTileSize;
        const int y0 = std::min(rectangleStart_->second, rectangleEnd_.second) * kTileSize;
        const int x1 = (std::max(rectangleStart_->first, rectangleEnd_.first) + 1) * kTileSize;
        const int y1 = (std::max(rectangleStart_->second, rectangleEnd_.second) + 1) * kTileSize;
        const auto corner = screen(x0, y0);
        painter.outline({corner.x, corner.y, x1 - x0, y1 - y0}, UiColor::Gold);
    } else if (hover_) {
        const auto corner = screen(hover_->first * kTileSize, hover_->second * kTileSize);
        painter.outline({corner.x, corner.y, kTileSize, kTileSize}, UiColor::Text);
    }

    toolbar_->draw(painter);
    palette_->draw(painter);
    // The status line: tool, ground, cell, and the last thing done.
    std::string line = std::format("{}  {}", toolName(tool_),
                                   tool_ == EditorTool::Eraser ? definitions_.tiles[static_cast<std::size_t>(level_.defaultGround)].name
                                                               : definitions_.tiles[static_cast<std::size_t>(tile_)].name);
    if (hover_) line += std::format("  ({}, {})", hover_->first, hover_->second);
    if (unsaved_) line += "  *unsaved";
    if (!status_.empty()) line += "  " + status_;
    const Rect bar{0, viewHeight_ - luna::engine::kGlyphHeight - 5, viewWidth_, luna::engine::kGlyphHeight + 5};
    painter.fill(bar, UiColor::Shade);
    painter.text(4, bar.y + 3, line.substr(0, static_cast<std::size_t>(viewWidth_ / luna::engine::kTextAdvance - 1)), UiColor::Text);
    toolbar_->drawOverlay(painter);
    palette_->drawOverlay(painter);
}

} // namespace odysseus::game
