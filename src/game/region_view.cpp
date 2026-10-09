#include "game/region_view.h"

#include "luna/engine/ui.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace odysseus::game {

using luna::engine::Button;
using luna::engine::Color;
using luna::engine::DrawStyle;
using luna::engine::Image;
using luna::engine::Intent;
using luna::engine::Panel;
using luna::engine::Point;
using luna::engine::PointerButton;
using luna::engine::Rect;
using luna::engine::Texture;
using luna::engine::UiColor;
using luna::engine::UiInput;
using luna::engine::UiPainter;

namespace {

constexpr Color kSteppe{166, 176, 96, 255};
constexpr Color kForest{58, 110, 58, 255};
constexpr Color kWater{52, 98, 160, 255};
constexpr Color kMountain{120, 116, 112, 255};
constexpr Color kCave{60, 52, 48, 255};
constexpr Color kWood{24, 64, 28, 255};
constexpr Color kBerries{200, 50, 80, 255};
constexpr Color kFlint{210, 210, 224, 255};
constexpr Color kHerd{150, 100, 60, 255};
constexpr Color kNothing{0, 0, 0, 0};

Color biomeColour(sim::Biome biome) {
    switch (biome) {
    case sim::Biome::Steppe: return kSteppe;
    case sim::Biome::Forest: return kForest;
    case sim::Biome::Water: return kWater;
    case sim::Biome::Mountain: return kMountain;
    case sim::Biome::Cave: return kCave;
    }
    return kSteppe;
}

Color resourceColour(sim::ResourceKind kind) {
    switch (kind) {
    case sim::ResourceKind::Flint: return kFlint;
    case sim::ResourceKind::Wood: return kWood;
    case sim::ResourceKind::Berries: return kBerries;
    case sim::ResourceKind::Herd: return kHerd;
    }
    return kNothing;
}

int floorDiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

} // namespace

const char* regionLayerName(RegionLayer layer) {
    switch (layer) {
    case RegionLayer::Terrain: return "Terrain";
    case RegionLayer::Water: return "Water";
    case RegionLayer::Plants: return "Plants";
    case RegionLayer::Things: return "Things";
    case RegionLayer::People: return "People";
    case RegionLayer::Places: return "Places";
    case RegionLayer::Camps: return "Camps";
    case RegionLayer::Count: break;
    }
    return "?";
}

RegionLayer RegionView::layerOf(sim::ResourceKind kind) {
    switch (kind) {
    case sim::ResourceKind::Wood:
    case sim::ResourceKind::Berries: return RegionLayer::Plants;
    case sim::ResourceKind::Flint: return RegionLayer::Things;
    case sim::ResourceKind::Herd: return RegionLayer::People;
    }
    return RegionLayer::Things;
}

RegionView::RegionView(int viewWidth, int viewHeight, Say say) : viewWidth_(viewWidth), viewHeight_(viewHeight), say_(std::move(say)) {
    layers_.fill(true);
    buildBar();
}

void RegionView::setSource(std::filesystem::path configFile, std::function<std::uint64_t()> seedNow) {
    configFile_ = std::move(configFile);
    seedNow_ = std::move(seedNow);
}

void RegionView::show(bool shown) {
    if (shown && !shown_) {
        const std::uint64_t seed = seedNow_ ? seedNow_() : 1;
        if (region_ == nullptr || region_->seed() != seed) {
            if (!open(seed)) return;
        }
    }
    shown_ = shown;
}

bool RegionView::open(std::uint64_t seed) {
    try {
        open(seed, sim::loadRegionConfig(configFile_));
        return true;
    } catch (const std::exception& error) {
        if (say_) say_(std::string("The region cannot be opened: ") + error.what());
        return false;
    }
}

void RegionView::open(std::uint64_t seed, sim::RegionConfig config) {
    region_ = std::make_unique<sim::Region>(seed, std::move(config));
    overview_ = std::make_unique<luna::engine::Minimap>(region_->size(), region_->size());
    textures_.clear();
    textureLayer_.clear();
    zoom_ = kMinZoom;
    centreOn(region_->size() / 2.0, region_->size() / 2.0);
    shown_ = true;
}

RegionView::Cell RegionView::cellAt(int x, int y) const {
    Cell cell;
    cell.biome = region_->biomeAt(x, y);
    if (x < 0 || y < 0 || x >= region_->size() || y >= region_->size()) return cell;
    const sim::Tile chunkCoords = region_->chunkOf(x, y);
    for (const sim::Resource& resource : region_->chunk(chunkCoords.x, chunkCoords.y).resources) {
        if (resource.x == x && resource.y == y) {
            cell.resource = resource.kind;
            break;
        }
    }
    return cell;
}

Image RegionView::chunkImage(int cx, int cy, RegionLayer layer) const {
    const int n = region_->config().chunkSize;
    Image image(n, n);
    if (layer == RegionLayer::Terrain || layer == RegionLayer::Water) {
        const bool wantWater = layer == RegionLayer::Water;
        for (int ty = 0; ty < n; ++ty) {
            for (int tx = 0; tx < n; ++tx) {
                const sim::Biome biome = region_->biomeAt(cx * n + tx, cy * n + ty);
                if ((biome == sim::Biome::Water) == wantWater) image.set(tx, ty, biomeColour(biome));
            }
        }
        return image;
    }
    for (const sim::Resource& resource : region_->chunk(cx, cy).resources) {
        if (layerOf(resource.kind) == layer) image.set(resource.x - cx * n, resource.y - cy * n, resourceColour(resource.kind));
    }
    return image; // Places and Camps have nothing to show yet
}

void RegionView::setZoom(int zoom) { zoom_ = std::clamp(zoom, kMinZoom, kMaxZoom); }

void RegionView::centreOn(double tileX, double tileY) {
    centreX_ = tileX;
    centreY_ = tileY;
    clampCentre();
}

void RegionView::clampCentre() {
    const double size = region_ != nullptr ? region_->size() : 0.0;
    centreX_ = std::clamp(centreX_, 0.0, size);
    centreY_ = std::clamp(centreY_, 0.0, size);
}

Point RegionView::origin() const {
    const int tile = tilePixels();
    return {viewWidth_ / 2 - static_cast<int>(std::lround(centreX_ * tile)), viewHeight_ / 2 - static_cast<int>(std::lround(centreY_ * tile))};
}

RegionView::TileRange RegionView::visibleTiles() const {
    TileRange range;
    if (region_ == nullptr) return range;
    const int tile = tilePixels();
    const Point at = origin();
    const int size = region_->size();
    range.x0 = std::max(0, floorDiv(-at.x, tile));
    range.y0 = std::max(0, floorDiv(-at.y, tile));
    range.x1 = std::min(size - 1, floorDiv(viewWidth_ - 1 - at.x, tile));
    range.y1 = std::min(size - 1, floorDiv(viewHeight_ - 1 - at.y, tile));
    return range;
}

std::optional<Point> RegionView::tileAtScreen(int screenX, int screenY) const {
    if (region_ == nullptr) return std::nullopt;
    const Point at = origin();
    const int tile = tilePixels();
    const int x = floorDiv(screenX - at.x, tile);
    const int y = floorDiv(screenY - at.y, tile);
    if (x < 0 || y < 0 || x >= region_->size() || y >= region_->size()) return std::nullopt;
    return Point{x, y};
}

void RegionView::setLayerShown(RegionLayer layer, bool shown) { layers_[static_cast<std::size_t>(layer)] = shown; }

std::optional<RegionLayer> RegionView::layerOfTexture(int id) const {
    const auto found = textureLayer_.find(id);
    if (found == textureLayer_.end()) return std::nullopt;
    return found->second;
}

// Zooming keeps the tile under (screenX, screenY) where it is on screen.
void RegionView::zoomAround(int newZoom, int screenX, int screenY) {
    newZoom = std::clamp(newZoom, kMinZoom, kMaxZoom);
    if (newZoom == zoom_) return;
    const Point at = origin();
    const double tileX = static_cast<double>(screenX - at.x) / tilePixels();
    const double tileY = static_cast<double>(screenY - at.y) / tilePixels();
    zoom_ = newZoom;
    centreX_ = tileX - static_cast<double>(screenX - viewWidth_ / 2) / tilePixels();
    centreY_ = tileY - static_cast<double>(screenY - viewHeight_ / 2) / tilePixels();
    clampCentre();
}

void RegionView::buildBar() {
    bar_ = std::make_unique<Panel>(Rect{0, 0, viewWidth_, kBarHeight});
    layerButtons_.clear();
    int x = 4;
    auto add = [&](const std::string& label, const std::string& hint, auto action) -> Button& {
        const int width = UiPainter::textWidth(label) + 8;
        Button& added = bar_->add<Button>(Rect{x, 2, width, kBarHeight - 4}, label, action);
        added.hint = hint;
        x += width + 2;
        return added;
    };
    add("Back", "Back to the level editor (Esc)", [this] { show(false); });
    x += 4;
    add("-", "Zoom out (the wheel or the minus key); the whole map fits at the widest zoom", [this] { zoomAround(zoom_ - 1, viewWidth_ / 2, viewHeight_ / 2); });
    add("+", "Zoom in (the wheel or the plus key) down to single tiles", [this] { zoomAround(zoom_ + 1, viewWidth_ / 2, viewHeight_ / 2); });
    add("Start", "Centre the view on where the clan starts", [this] {
        if (region_ != nullptr) centreOn(region_->start().x + 0.5, region_->start().y + 0.5);
    });
    x += 4;
    for (int i = 0; i < kRegionLayerCount; ++i) {
        const auto layer = static_cast<RegionLayer>(i);
        Button& button = add(regionLayerName(layer), std::string("Show or hide the ") + regionLayerName(layer) + " layer", [this, layer] { setLayerShown(layer, !layerShown(layer)); });
        layerButtons_.push_back(&button);
    }
}

void RegionView::syncBar() {
    for (std::size_t i = 0; i < layerButtons_.size(); ++i) layerButtons_[i]->selected = layers_[i];
}

void RegionView::update(const luna::engine::Intents& intents) {
    if (!shown_ || region_ == nullptr) return;
    const luna::engine::Pointer& pointer = intents.pointer();
    bar_->handle(UiInput::from(intents));
    syncBar();

    if (!overview_->complete()) {
        overview_->build(kOverviewRowsPerTick, [this](int x, int y) { return biomeColour(region_->biomeAt(x, y)); });
    }

    const bool overBar = pointer.inside() && pointer.y < kBarHeight;
    const std::optional<Point> overviewCell = pointer.inside() ? overview_->cellAt(overviewArea(), pointer.x, pointer.y) : std::nullopt;

    if (overviewCell && pointer.isHeld(PointerButton::Left)) {
        centreOn(overviewCell->x + 0.5, overviewCell->y + 0.5);
        dragFrom_.reset();
    } else if (pointer.inside() && !overBar && (pointer.isHeld(PointerButton::Left) || pointer.isHeld(PointerButton::Middle))) {
        if (dragFrom_) {
            const int tile = tilePixels();
            centreX_ -= static_cast<double>(pointer.x - dragFrom_->x) / tile;
            centreY_ -= static_cast<double>(pointer.y - dragFrom_->y) / tile;
            clampCentre();
        }
        dragFrom_ = Point{pointer.x, pointer.y};
    } else {
        dragFrom_.reset();
    }

    if (pointer.inside() && pointer.wheel != 0) zoomAround(zoom_ + (pointer.wheel > 0 ? 1 : -1), pointer.x, pointer.y);
    if (intents.pressed(Intent::ZoomIn)) zoomAround(zoom_ + 1, viewWidth_ / 2, viewHeight_ / 2);
    if (intents.pressed(Intent::ZoomOut)) zoomAround(zoom_ - 1, viewWidth_ / 2, viewHeight_ / 2);

    const int step = 6; // screen pixels a tick, so a pan feels the same at every zoom
    if (intents.moveX() != 0 || intents.moveY() != 0) {
        centreX_ += static_cast<double>(intents.moveX() * step) / tilePixels();
        centreY_ += static_cast<double>(intents.moveY() * step) / tilePixels();
        clampCentre();
    }

    hover_.clear();
    const std::optional<Point> tile = pointer.inside() && !overBar ? tileAtScreen(pointer.x, pointer.y) : std::nullopt;
    if (tile) {
        static constexpr const char* kBiomeNames[] = {"steppe", "forest", "water", "mountain", "cave mouth"};
        static constexpr const char* kResourceNames[] = {"flint", "wood", "berries", "herd"};
        const Cell cell = cellAt(tile->x, tile->y);
        hover_ = std::format("tile {}, {}: {}", tile->x, tile->y, kBiomeNames[static_cast<int>(cell.biome)]);
        if (cell.resource) hover_ += std::format(", {}", kResourceNames[static_cast<int>(*cell.resource)]);
    }
}

const Texture* RegionView::chunkTexture(luna::engine::Renderer& renderer, int cx, int cy, RegionLayer layer) const {
    const auto key = std::make_tuple(cx, cy, static_cast<int>(layer));
    const auto found = textures_.find(key);
    if (found != textures_.end()) return &found->second;
    if (builtThisFrame_ >= kChunkBuildsPerFrame) return nullptr; // over the frame's budget: it is made in a later frame
    ++builtThisFrame_;
    const Texture texture = renderer.createTexture(chunkImage(cx, cy, layer));
    textureLayer_[texture.id] = layer;
    return &textures_.emplace(key, texture).first->second;
}

void RegionView::render(luna::engine::Renderer& renderer, UiPainter& painter) const {
    painter.fill({0, 0, viewWidth_, viewHeight_}, UiColor::Dark);
    if (region_ == nullptr) return;
    builtThisFrame_ = 0;
    const int tile = tilePixels();
    const Point at = origin();
    const int n = region_->config().chunkSize;
    const TileRange range = visibleTiles();
    if (!range.empty()) {
        for (int i = 0; i < kRegionLayerCount; ++i) {
            const auto layer = static_cast<RegionLayer>(i);
            if (!layers_[static_cast<std::size_t>(i)] || layer == RegionLayer::Places || layer == RegionLayer::Camps) continue;
            for (int cy = range.y0 / n; cy <= range.y1 / n; ++cy) {
                for (int cx = range.x0 / n; cx <= range.x1 / n; ++cx) {
                    const Texture* texture = chunkTexture(renderer, cx, cy, layer);
                    if (texture == nullptr) continue;
                    // Only the part of the chunk that is on screen is drawn, so a close zoom never asks for a huge rectangle.
                    const int tx0 = std::max(range.x0, cx * n);
                    const int ty0 = std::max(range.y0, cy * n);
                    const int tx1 = std::min(range.x1, cx * n + n - 1);
                    const int ty1 = std::min(range.y1, cy * n + n - 1);
                    const Rect source{tx0 - cx * n, ty0 - cy * n, tx1 - tx0 + 1, ty1 - ty0 + 1};
                    const Rect destination{at.x + tx0 * tile, at.y + ty0 * tile, source.width * tile, source.height * tile};
                    renderer.drawStyled(*texture, source, destination, DrawStyle{});
                }
            }
        }
        if (tile >= 32) { // single tiles: the cell lines
            for (int x = range.x0; x <= range.x1 + 1; ++x) painter.fill({at.x + x * tile, 0, 1, viewHeight_}, UiColor::Grid);
            for (int y = range.y0; y <= range.y1 + 1; ++y) painter.fill({0, at.y + y * tile, viewWidth_, 1}, UiColor::Grid);
        }
    }
    const sim::Tile start = region_->start();
    painter.outline({at.x + start.x * tile - 1, at.y + start.y * tile - 1, std::max(tile, 4) + 2, std::max(tile, 4) + 2}, UiColor::Gold);

    // The overview: the whole map in the corner, with the part on screen outlined.
    const Rect box = overviewArea();
    painter.fill({box.x - 1, box.y - 1, box.width + 2, box.height + 2}, UiColor::Border);
    if (overview_->complete()) {
        renderer.drawStyled(overview_->texture(renderer), {0, 0, overview_->width(), overview_->height()}, box, DrawStyle{});
        if (!range.empty()) {
            const int size = region_->size();
            painter.outline({box.x + range.x0 * box.width / size, box.y + range.y0 * box.height / size, std::max(2, (range.x1 - range.x0 + 1) * box.width / size),
                             std::max(2, (range.y1 - range.y0 + 1) * box.height / size)}, UiColor::Text);
        }
    } else {
        painter.fill(box, UiColor::Panel);
        painter.text(box.x + 4, box.y + 4, std::format("map {}%", overview_->rowsDone() * 100 / overview_->height()), UiColor::Dim);
    }

    painter.fill({0, viewHeight_ - 11, viewWidth_ - kOverviewSize - 10, 11}, UiColor::Panel);
    painter.text(4, viewHeight_ - 9, std::format("seed {}  zoom {} px a tile  {}", region_->seed(), tile, hover_), UiColor::Text);
    bar_->draw(painter);
    bar_->drawOverlay(painter); // hints of the buttons
}

} // namespace odysseus::game
