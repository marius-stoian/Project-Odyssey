#include "game/region_view.h"

#include "luna/engine/ui.h"
#include "sim/region_shapes.h"

#include <algorithm>
#include <charconv>
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
    settings_ = std::make_unique<GeneratorPanel>(viewWidth, viewHeight, say_);
    buildBar();
    buildTools();
    buildPlaceRow();
    buildInspector();
}

void RegionView::setSource(std::filesystem::path configFile, std::function<std::uint64_t()> seedNow, std::function<void(const std::filesystem::path&)> saved) {
    configFile_ = std::move(configFile);
    seedNow_ = std::move(seedNow);
    settings_->setSource(configFile_, std::move(saved), [this](const sim::RegionConfig& config) { reopen(config); });
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

void RegionView::setWorldsFolder(std::filesystem::path folder, std::string name) {
    worldsFolder_ = std::move(folder);
    worldName_ = std::move(name);
}

void RegionView::open(std::uint64_t seed, sim::RegionConfig config) {
    baseConfig_ = config;
    for (const auto& [key, texture] : textures_) pendingDestroy_.push_back(texture); // given back at the next drawn frame
    textures_.clear();
    textureLayer_.clear();
    stale_.clear();
    history_.clear();
    strokeOpen_ = false;
    painting_ = false;
    edits_ = {};
    setup_ = {};
    setupIssues_.clear();
    dirty_ = false;

    // The world file, when there is one for this seed: the land is made from the seed, then the painted tiles are laid over it.
    std::optional<sim::WorldFile> world;
    if (!worldsFolder_.empty() && std::filesystem::exists(worldFile())) {
        try {
            sim::WorldFile found = sim::loadWorld(worldFile(), config);
            if (found.seed == seed) {
                world = std::move(found);
            } else if (say_) {
                say_(std::format("{} is for seed {}; this region is seed {}, so it is not applied", worldFile().filename().string(), found.seed, seed));
            }
        } catch (const std::exception& error) {
            if (say_) say_(std::string("The world file cannot be read: ") + error.what());
        }
    }
    if (world) {
        region_ = std::make_unique<sim::Region>(sim::makeWorldRegion(*world, config));
        edits_ = world->edits;
        setup_ = world->setup;
    } else {
        region_ = std::make_unique<sim::Region>(seed, config);
    }
    overview_ = std::make_unique<luna::engine::Minimap>(region_->size(), region_->size());
    zoom_ = kMinZoom;
    centreOn(region_->size() / 2.0, region_->size() / 2.0);
    syncTileEdits();
    settings_->bind(region_.get(), &edits_);
    refreshSetupIssues();
    shown_ = true;
}

// Apply (US-201): the same view, the same seed and the same edits over the land the new settings make.
void RegionView::reopen(const sim::RegionConfig& config) {
    const int zoom = zoom_;
    const double x = centreX_;
    const double y = centreY_;
    const sim::RegionEdits kept = edits_;
    const sim::WorldSetup keptSetup = setup_;
    const bool wasDirty = dirty_;
    const std::uint64_t seed = region_->seed();
    open(seed, config);
    for (const auto& [tile, biome] : region_->tileEditList()) region_->clearTileEdit(tile.x, tile.y); // what is in memory wins over what the file said
    for (const sim::TileEdit& edit : kept.tiles) region_->setTileEdit(edit.x, edit.y, edit.biome);
    edits_ = kept;
    setup_ = keptSetup;
    syncTileEdits();
    sim::applyPlacedToRegion(*region_, edits_);
    dirty_ = wasDirty;
    zoom_ = zoom;
    centreOn(x, y);
}

void RegionView::syncTileEdits() {
    edits_.tiles.clear();
    if (region_ == nullptr) return;
    for (const auto& [tile, biome] : region_->tileEditList()) edits_.tiles.push_back({tile.x, tile.y, biome});
}

bool RegionView::saveWorld() {
    if (region_ == nullptr || worldsFolder_.empty()) return false;
    try {
        if (std::filesystem::exists(worldFile())) { // never over a world made for another seed
            const sim::WorldFile existing = sim::loadWorld(worldFile(), baseConfig_);
            if (existing.seed != region_->seed()) {
                if (say_) say_(std::format("{} is for seed {}: not overwritten", worldFile().filename().string(), existing.seed));
                return false;
            }
        }
        syncTileEdits();
        sim::WorldFile world;
        world.seed = region_->seed();
        world.generator = sim::generatorDifferences(region_->config(), baseConfig_);
        world.edits = edits_;
        world.setup = setup_;
        std::filesystem::create_directories(worldsFolder_);
        sim::saveWorld(world, worldFile(), baseConfig_);
    } catch (const std::exception& error) {
        if (say_) say_(std::string("The world cannot be saved: ") + error.what());
        return false;
    }
    dirty_ = false;
    if (say_) say_(std::format("Saved {} ({} painted tiles)", worldFile().filename().string(), region_->tileEditCount()));
    return true;
}

bool RegionView::saveBeforePlay() {
    if (region_ == nullptr) return false;
    if (!dirty_ && std::filesystem::exists(worldFile())) return true;
    return saveWorld();
}

void RegionView::askToPlay(int x, int y) {
    if (region_ == nullptr) return;
    if (x < 0 || y < 0 || x >= region_->size() || y >= region_->size() || !sim::walkable(sim::effectiveBiome(*region_, edits_, x, y))) {
        if (say_) say_("Play here needs land to stand on: pick a tile that is not water, mountain or the edge of the map");
        return;
    }
    playRequest_ = sim::Tile{x, y};
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
    add("Settings", "The generator settings of the land: change them, Preview a map beside this one, Apply to write region.json", [this] { settings_->show(!settings_->shown()); });
    add("Inspect", "The inspector: set up a clan (store, debts, leader, stance, partners) and a clan member (kin, opinions, grudges)", [this] { inspectorShown_ = !inspectorShown_; });
    add("Play here", "Play this world as a new game with the hero in the middle of the view (or press P over a tile to play from it); the world is saved first, and Esc comes back here", [this] { askToPlay(static_cast<int>(centreX_), static_cast<int>(centreY_)); });
    x += 4;
    for (int i = 0; i < kRegionLayerCount; ++i) {
        const auto layer = static_cast<RegionLayer>(i);
        Button& button = add(regionLayerName(layer), std::string("Show or hide the ") + regionLayerName(layer) + " layer", [this, layer] { setLayerShown(layer, !layerShown(layer)); });
        layerButtons_.push_back(&button);
    }
}

void RegionView::syncBar() {
    for (std::size_t i = 0; i < layerButtons_.size(); ++i) layerButtons_[i]->selected = layers_[i];
    for (std::size_t i = 0; i < toolButtons_.size(); ++i) toolButtons_[i]->selected = static_cast<std::size_t>(tool_) == i;
    for (std::size_t i = 0; i < biomeButtons_.size(); ++i) biomeButtons_[i]->selected = static_cast<std::size_t>(paintBiome_) == i && tool_ != RegionTool::Pan && tool_ != RegionTool::Erase;
    if (!sizeField_->focused()) sizeField_->value = brushSize_;
    static constexpr sim::EditGroup kGroupOrder[] = {sim::EditGroup::Thing, sim::EditGroup::Person, sim::EditGroup::Place, sim::EditGroup::Camp, sim::EditGroup::Resource};
    for (std::size_t i = 0; i < groupButtons_.size(); ++i) groupButtons_[i]->selected = kGroupOrder[i] == placeGroup_;
    if (forcedButton_ != nullptr) forcedButton_->selected = placeForced_;
    for (InspectorRow& row : inspectorRows_) {
        if (!row.field->focused()) row.field->value = row.text();
    }
    if (clanField_ != nullptr && !clanField_->focused()) clanField_->value = clanKey_;
    if (memberField_ != nullptr && !memberField_->focused()) memberField_->value = memberKey_;
    if (kindField_ != nullptr && !kindField_->focused()) kindField_->value = placeKind_;
    if (nameField_ != nullptr && !nameField_->focused()) nameField_->value = placeName_;
    if (!fordField_->focused()) fordField_->value = fordEvery_;
}

void RegionView::update(const luna::engine::Intents& intents) {
    if (!shown_ || region_ == nullptr) return;
    const luna::engine::Pointer& pointer = intents.pointer();
    bar_->handle(UiInput::from(intents));
    tools_->handle(UiInput::from(intents));
    if (placing()) placeRow_->handle(UiInput::from(intents));
    if (inspectorShown_) inspector_->handle(UiInput::from(intents));
    syncBar();
    settings_->update(intents);
    const bool overSettings = settings_->shown() && pointer.inside() && settings_->bounds().contains({pointer.x, pointer.y});
    const bool typing = this->typing();

    if (!overview_->complete()) {
        overview_->build(kOverviewRowsPerTick, [this](int x, int y) { return biomeColour(region_->biomeAt(x, y)); });
    }

    const bool overBar = pointer.inside() && (pointer.y < topHeight() || (inspectorShown_ && inspectorArea().contains({pointer.x, pointer.y}))); // the bar, the rows of tools and the inspector
    const std::optional<Point> overviewCell = pointer.inside() ? overview_->cellAt(overviewArea(), pointer.x, pointer.y) : std::nullopt;

    if (overSettings || typing) {
        dragFrom_.reset(); // the settings panel has the pointer and the keys
    } else if (overviewCell && pointer.isHeld(PointerButton::Left) && !painting_) {
        centreOn(overviewCell->x + 0.5, overviewCell->y + 0.5);
        dragFrom_.reset();
    } else if (tool_ != RegionTool::Pan && (painting_ || (pointer.inside() && !overBar && pointer.wasPressed(PointerButton::Left))) && pointer.isHeld(PointerButton::Left)) {
        handlePaint(pointer);
        dragFrom_.reset();
    } else if (pointer.inside() && !overBar && (pointer.isHeld(PointerButton::Middle) || pointer.isHeld(PointerButton::Right) || (tool_ == RegionTool::Pan && pointer.isHeld(PointerButton::Left)))) {
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
    if (painting_ && !pointer.isHeld(PointerButton::Left)) { // the button went up: a brush stroke ends, a rectangle is painted
        painting_ = false;
        if (rectFrom_ && rectTo_) {
            const sim::Tile from{rectFrom_->x, rectFrom_->y};
            const sim::Tile to{rectTo_->x, rectTo_->y};
            if (tool_ == RegionTool::Rectangle) paintRectangle(from.x, from.y, to.x, to.y);
            else if (tool_ == RegionTool::River) paintRiver(from, to, brushSize_, fordEvery_);
            else if (tool_ == RegionTool::Ridge) paintRidge(from, to, brushSize_);
            else if (tool_ == RegionTool::Lake) paintLake(from, static_cast<int>(std::lround(std::sqrt(static_cast<double>((to.x - from.x) * (to.x - from.x) + (to.y - from.y) * (to.y - from.y))))));
        } else {
            endStroke();
        }
        rectFrom_.reset();
        rectTo_.reset();
    }

    if (overSettings || typing) {
        hover_.clear();
        return;
    }
    if (intents.pressed(Intent::Undo)) undo();
    if (intents.pressed(Intent::Redo)) redo();
    if (intents.pressed(Intent::Save)) saveWorld();
    if (intents.pressed(Intent::PlayHere) && pointer.inside() && !overBar) {
        if (const std::optional<Point> at = tileAtScreen(pointer.x, pointer.y)) askToPlay(at->x, at->y);
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
        if (const sim::PlacedEdit* entry = sim::placedAt(edits_, tile->x, tile->y)) {
            hover_ += std::format(", {} {}{}", entry->id, entry->kind, entry->name.empty() ? std::string() : " '" + entry->name + "'");
        }
    }
}

const Texture* RegionView::chunkTexture(luna::engine::Renderer& renderer, int cx, int cy, RegionLayer layer) const {
    const auto key = std::make_tuple(cx, cy, static_cast<int>(layer));
    const auto found = textures_.find(key);
    if (found != textures_.end() && stale_.find(key) == stale_.end()) return &found->second;
    if (builtThisFrame_ >= kChunkBuildsPerFrame) return found != textures_.end() ? &found->second : nullptr; // the old picture stays until there is room for the new one
    if (found != textures_.end()) { // painted since: made again, the old one is given back at the start of the next frame
        pendingDestroy_.push_back(found->second);
        textureLayer_.erase(found->second.id);
        textures_.erase(found);
        stale_.erase(key);
    }
    if (false) return nullptr; // over the frame's budget: it is made in a later frame
    ++builtThisFrame_;
    const Texture texture = renderer.createTexture(chunkImage(cx, cy, layer));
    textureLayer_[texture.id] = layer;
    return &textures_.emplace(key, texture).first->second;
}

void RegionView::render(luna::engine::Renderer& renderer, UiPainter& painter) const {
    for (const Texture& old : pendingDestroy_) renderer.destroyTexture(old); // pictures replaced since the last frame; nothing drawn this frame names them
    pendingDestroy_.clear();
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
    // What the owner put on the land (US-204): a square for each entry, in the colour of its layer; a seed thing taken away is a crossed outline; the picked one is ringed.
    // These are drawn straight from the list, so a move, a removal or an Undo shows at once and no chunk picture is made again for them.
    for (const sim::PlacedEdit& entry : edits_.placed) {
        const RegionLayer layer = entry.group == sim::EditGroup::Person ? RegionLayer::People : entry.group == sim::EditGroup::Place ? RegionLayer::Places : entry.group == sim::EditGroup::Camp ? RegionLayer::Camps : RegionLayer::Things;
        if (!layers_[static_cast<std::size_t>(layer)] || entry.x < range.x0 || entry.x > range.x1 || entry.y < range.y0 || entry.y > range.y1) continue;
        const int size = std::max(tile, 3);
        const Rect box{at.x + entry.x * tile + (tile - size) / 2, at.y + entry.y * tile + (tile - size) / 2, size, size};
        if (entry.removal) {
            painter.outline(box, UiColor::Dim);
        } else {
            painter.fill(box, entry.group == sim::EditGroup::Person ? UiColor::Red : entry.group == sim::EditGroup::Place ? UiColor::Gold : entry.group == sim::EditGroup::Camp ? UiColor::Hover : entry.group == sim::EditGroup::Resource ? UiColor::Dim : UiColor::Text);
            painter.outline({box.x - 1, box.y - 1, box.width + 2, box.height + 2}, UiColor::Dark);
        }
        if (entry.id == selected_) painter.outline({box.x - 2, box.y - 2, box.width + 4, box.height + 4}, UiColor::Selected);
        if (tile >= 16 && !entry.removal) painter.text(box.x, box.y + box.height + 1, entry.name.empty() ? entry.kind : entry.name, UiColor::Text);
    }
    if (rectFrom_ && rectTo_) { // the shape being dragged
        const sim::Tile from{rectFrom_->x, rectFrom_->y};
        const sim::Tile to{rectTo_->x, rectTo_->y};
        if (tool_ == RegionTool::River || tool_ == RegionTool::Ridge) {
            for (const sim::Tile& on : sim::thicken(sim::line4(from, to), brushSize_, region_->size())) painter.outline({at.x + on.x * tile, at.y + on.y * tile, tile, tile}, UiColor::Gold);
        } else if (tool_ == RegionTool::Lake) {
            const int radius = static_cast<int>(std::lround(std::sqrt(static_cast<double>((to.x - from.x) * (to.x - from.x) + (to.y - from.y) * (to.y - from.y)))));
            painter.outline({at.x + (from.x - radius) * tile, at.y + (from.y - radius) * tile, (2 * radius + 1) * tile, (2 * radius + 1) * tile}, UiColor::Gold);
        } else {
            const int x0 = std::min(from.x, to.x);
            const int y0 = std::min(from.y, to.y);
            const int x1 = std::max(from.x, to.x);
            const int y1 = std::max(from.y, to.y);
            painter.outline({at.x + x0 * tile, at.y + y0 * tile, (x1 - x0 + 1) * tile, (y1 - y0 + 1) * tile}, UiColor::Gold);
        }
    }
    const sim::Tile start = region_->start();
    painter.outline({at.x + start.x * tile - 1, at.y + start.y * tile - 1, std::max(tile, 4) + 2, std::max(tile, 4) + 2}, UiColor::Gold);

    // The overview: the whole map in the corner, with the part on screen outlined.
    const Rect box = overviewArea();
    painter.fill({box.x - 1, box.y - 1, box.width + 2, box.height + 2}, UiColor::Border);
    if (overview_->complete() || overview_->hasPicture()) { // while it is painted again after an edit the old picture stays
        renderer.drawStyled(overview_->complete() ? overview_->texture(renderer) : overview_->picture(), {0, 0, overview_->width(), overview_->height()}, box, DrawStyle{});
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
    if (!warnings_.empty()) { // the land as it is would trouble the owner: said, never refused
        painter.fill({0, viewHeight_ - 22, viewWidth_ - kOverviewSize - 10, 11}, UiColor::Panel);
        painter.text(4, viewHeight_ - 20, "Warning: " + warnings_.front() + (warnings_.size() > 1 ? std::format(" (+{} more)", warnings_.size() - 1) : std::string()), UiColor::Red);
    }
    painter.text(4, viewHeight_ - 9, std::format("seed {}  zoom {} px  {}{}  {}", region_->seed(), tile, region_->tileEditCount() + edits_.placed.size() == 0 ? std::string("no hand edits") : std::format("{} painted tiles, {} placed", region_->tileEditCount(), edits_.placed.size()), dirty_ ? " (unsaved)" : "", hover_), UiColor::Text);
    settings_->draw(painter, renderer);
    bar_->draw(painter);
    tools_->draw(painter);
    if (placing()) placeRow_->draw(painter);
    if (inspectorShown_) {
        const Rect area = inspectorArea();
        painter.fill({area.x - 1, area.y - 1, area.width + 2, area.height + 2}, UiColor::Border);
        inspector_->draw(painter);
        int line = area.y + area.height + 2;
        for (std::size_t i = 0; i < std::min<std::size_t>(3, setupIssues_.size()); ++i, line += 10) painter.text(area.x, line, setupIssues_[i].substr(0, 42), UiColor::Red);
    }
    settings_->drawOverlay(painter);
    bar_->drawOverlay(painter);
    tools_->drawOverlay(painter); // hints of the buttons
    if (placing()) placeRow_->drawOverlay(painter);
    if (inspectorShown_) inspector_->drawOverlay(painter);
}


// --- Painting (US-202) ---

namespace {

const char* const kBiomeLabels[5] = {"Steppe", "Forest", "Water", "Mountain", "Cave"};

} // namespace

void RegionView::buildTools() {
    tools_ = std::make_unique<Panel>(Rect{0, kBarHeight, viewWidth_, kToolRowHeight});
    toolButtons_.clear();
    biomeButtons_.clear();
    int x = 4;
    auto add = [&](const std::string& label, const std::string& hint, auto action) -> Button& {
        const int width = UiPainter::textWidth(label) + 8;
        Button& added = tools_->add<Button>(Rect{x, kBarHeight + 2, width, kToolRowHeight - 4}, label, action);
        added.hint = hint;
        x += width + 2;
        return added;
    };
    const struct {
        RegionTool tool;
        const char* label;
        const char* hint;
    } tools[] = {{RegionTool::Pan, "Pan", "Left drag moves the map (the default)"},
                 {RegionTool::Brush, "Brush", "Paint the chosen biome: click or drag; the size is next to the biomes"},
                 {RegionTool::Rectangle, "Rect", "Paint a rectangle: drag from corner to corner"},
                 {RegionTool::Fill, "Fill", "Paint the connected tiles of the same biome (refused above 20,000 tiles)"},
                 {RegionTool::Erase, "Reset", "Give the tiles back to the land of the seed: click or drag"},
                 {RegionTool::River, "River", "Drag from the source to the mouth: a river of the brush width; Fords sets how often it can be crossed"},
                 {RegionTool::Lake, "Lake", "Press at the middle, drag out to the shore: a lake"},
                 {RegionTool::Ridge, "Ridge", "Drag a line of cliffs of the brush width"},
                 {RegionTool::Cave, "Cave", "Click a cliff (a mountain tile) to put a cave mouth into it"},
                 {RegionTool::Ford, "Ford", "Click a river: a crossing of the brush width, where people can wade through"},
                 {RegionTool::Dry, "Dry", "Click a lake or a river: the connected water becomes land"},
                 {RegionTool::Place, "Place", "Put the chosen thing, person or place on a tile (choose its kind in the row below)"},
                 {RegionTool::Move, "Move", "Click an entry to pick it, then click an empty tile to put it there"},
                 {RegionTool::Take, "Take away", "Click an entry to remove it, or one of the seed's own trees, bushes, flint or herds to hide it"}};
    for (const auto& entry : tools) {
        const RegionTool tool = entry.tool;
        toolButtons_.push_back(&add(entry.label, std::string(entry.hint) + " (with a painting tool the middle or right button pans)", [this, tool] { tool_ = tool; }));
    }
    x += 4;
    for (int i = 0; i < 5; ++i) {
        const auto biome = static_cast<sim::Biome>(i);
        biomeButtons_.push_back(&add(kBiomeLabels[i], std::string("Paint with ") + kBiomeLabels[i], [this, biome] {
            paintBiome_ = biome;
            if (tool_ == RegionTool::Pan || tool_ == RegionTool::Erase) tool_ = RegionTool::Brush;
        }));
    }
    x += 4;
    sizeField_ = &tools_->add<luna::engine::NumberField>(Rect{x, kBarHeight + 2, 64, kToolRowHeight - 4}, "Size: ", brushSize_, 1, kMaxBrush, [this](int size) { brushSize_ = size; });
    x += 70;
    fordField_ = &tools_->add<luna::engine::NumberField>(Rect{x, kBarHeight + 2, 70, kToolRowHeight - 4}, "Fords: ", fordEvery_, 0, 64, [this](int every) { fordEvery_ = every; });
    x += 76;
    add("Undo", "Undo the last stroke, rectangle or fill (Ctrl+Z)", [this] { undo(); });
    add("Redo", "Redo (Ctrl+Y)", [this] { redo(); });
    add("Save", "Write the hand edits to the world file (Ctrl+S)", [this] { saveWorld(); });
}

void RegionView::markStale(int x, int y) {
    const int n = region_->config().chunkSize;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            const int tx = x + dx;
            const int ty = y + dy;
            if (tx < 0 || ty < 0 || tx >= region_->size() || ty >= region_->size()) continue;
            for (int layer = 0; layer < kRegionLayerCount; ++layer) {
                const auto key = std::make_tuple(tx / n, ty / n, layer);
                if (textures_.contains(key)) stale_.insert(key);
            }
        }
    }
}

bool RegionView::editTile(int x, int y, std::optional<sim::Biome> target) {
    const std::optional<sim::Biome> before = region_->tileEdit(x, y);
    if (target) {
        if (!before && region_->tileEditCount() + edits_.placed.size() >= sim::kMaxWorldEntries) {
            if (say_) say_(std::format("A world keeps at most {} edits", sim::kMaxWorldEntries));
            return false;
        }
        if (!region_->setTileEdit(x, y, *target)) return false;
    } else if (!region_->clearTileEdit(x, y)) {
        return false;
    }
    const std::optional<sim::Biome> after = region_->tileEdit(x, y);
    if (before == after) return false;
    stroke_.changes.push_back({x, y, before, after});
    markStale(x, y);
    return true;
}

void RegionView::beginStroke() {
    if (strokeOpen_ || region_ == nullptr) return;
    strokeOpen_ = true;
    stroke_ = {};
}

void RegionView::endStroke() {
    if (!strokeOpen_) return;
    strokeOpen_ = false;
    lastDab_.reset();
    finishStep(tool_ == RegionTool::Erase ? "reset to the seed" : "paint");
}

void RegionView::finishStep(const std::string& label) {
    if (stroke_.changes.empty()) return;
    stroke_.label = label;
    history_.record(std::move(stroke_));
    stroke_ = {};
    syncTileEdits();
    dirty_ = true;
    refreshWarnings();
    overview_->invalidate();
    settings_->refreshNow();
}

void RegionView::finishStepAs(const std::string& label) {
    strokeOpen_ = false;
    lastDab_.reset();
    finishStep(label);
}

int RegionView::paintBrush(int x, int y) {
    if (region_ == nullptr) return 0;
    const bool own = !strokeOpen_;
    if (own) beginStroke();
    const std::optional<sim::Biome> target = tool_ == RegionTool::Erase ? std::nullopt : std::optional<sim::Biome>(paintBiome_);
    const std::size_t before = stroke_.changes.size();
    const int radiusSquared = brushSize_ * brushSize_ / 4;
    const int reach = (brushSize_ + 1) / 2;
    for (int dy = -reach; dy <= reach; ++dy) {
        for (int dx = -reach; dx <= reach; ++dx) {
            if (dx * dx + dy * dy <= radiusSquared) editTile(x + dx, y + dy, target);
        }
    }
    const int changed = static_cast<int>(stroke_.changes.size() - before);
    if (own) endStroke();
    return changed;
}

int RegionView::paintRectangle(int x0, int y0, int x1, int y1) {
    if (region_ == nullptr) return 0;
    beginStroke();
    const std::optional<sim::Biome> target = tool_ == RegionTool::Erase ? std::nullopt : std::optional<sim::Biome>(paintBiome_);
    for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y) {
        for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x) editTile(x, y, target);
    }
    const int changed = static_cast<int>(stroke_.changes.size());
    finishStepAs("rectangle");
    return changed;
}

int RegionView::paintFill(int x, int y) { return fillFrom(x, y, paintBiome_, "fill"); }

int RegionView::fillFrom(int x, int y, sim::Biome biome, const std::string& label) {
    if (region_ == nullptr || x < 0 || y < 0 || x >= region_->size() || y >= region_->size()) return 0;
    const sim::Biome from = region_->biomeAt(x, y);
    if (from == biome) return 0;
    // The connected tiles (four neighbours) of the same biome; the edge wall is not painted and not crossed.
    std::vector<Point> found;
    std::set<std::pair<int, int>> seen{{x, y}};
    std::vector<Point> open{{x, y}};
    while (!open.empty()) {
        const Point at = open.back();
        open.pop_back();
        if (region_->onEdgeWall(at.x, at.y)) continue;
        found.push_back(at);
        if (static_cast<int>(found.size()) > kMaxFill) {
            if (say_) say_(std::format("Fill refused: more than {} tiles are connected", kMaxFill));
            return 0;
        }
        for (const Point next : {Point{at.x + 1, at.y}, Point{at.x - 1, at.y}, Point{at.x, at.y + 1}, Point{at.x, at.y - 1}}) {
            if (next.x < 0 || next.y < 0 || next.x >= region_->size() || next.y >= region_->size()) continue;
            if (region_->biomeAt(next.x, next.y) != from || !seen.insert({next.x, next.y}).second) continue;
            open.push_back(next);
        }
    }
    beginStroke();
    for (const Point at : found) editTile(at.x, at.y, biome);
    const int changed = static_cast<int>(stroke_.changes.size());
    finishStepAs(label);
    return changed;
}

void RegionView::applyChanges(const std::vector<TileChange>& changes, bool forward) {
    const auto apply = [this, forward](const TileChange& change) {
        const std::optional<sim::Biome> wanted = forward ? change.after : change.before;
        if (wanted) region_->setTileEdit(change.x, change.y, *wanted);
        else region_->clearTileEdit(change.x, change.y);
        markStale(change.x, change.y);
    };
    if (forward) {
        for (const TileChange& change : changes) apply(change);
    } else {
        for (auto it = changes.rbegin(); it != changes.rend(); ++it) apply(*it); // the last thing done is the first undone
    }
    syncTileEdits();
    refreshWarnings();
    dirty_ = true;
    overview_->invalidate();
    settings_->refreshNow();
}

bool RegionView::undo() {
    if (strokeOpen_ || region_ == nullptr) return false;
    const WorldCommand* step = history_.undo();
    if (step == nullptr) return false;
    applyChanges(step->changes, false);
    for (auto it = step->placed.rbegin(); it != step->placed.rend(); ++it) sim::applyPlacedChange(edits_, *it, false);
    if (step->setup) {
        setup_ = step->setup->first;
        refreshSetupIssues();
    }
    refreshPlaced(step->placed);
    if (!selected_.empty() && sim::findPlaced(edits_, selected_) == nullptr) selected_.clear();
    settings_->refreshNow(); // the conflicts list reads the entries as they are now
    return true;
}

bool RegionView::redo() {
    if (strokeOpen_ || region_ == nullptr) return false;
    const WorldCommand* step = history_.redo();
    if (step == nullptr) return false;
    applyChanges(step->changes, true);
    for (const sim::PlacedChange& change : step->placed) sim::applyPlacedChange(edits_, change, true);
    if (step->setup) {
        setup_ = step->setup->second;
        refreshSetupIssues();
    }
    refreshPlaced(step->placed);
    if (!selected_.empty() && sim::findPlaced(edits_, selected_) == nullptr) selected_.clear();
    settings_->refreshNow(); // the conflicts list reads the entries as they are now
    return true;
}

// The left button with a painting tool: a brush leaves a line of dabs from the last tile to this one; a rectangle waits for the button to go up; a fill acts at once.
void RegionView::handlePaint(const luna::engine::Pointer& pointer) {
    const std::optional<Point> tile = tileAtScreen(pointer.x, pointer.y);
    if (placing()) { // one click, one action: nothing is dragged
        if (!painting_ && pointer.wasPressed(PointerButton::Left) && tile) {
            painting_ = true;
            handlePlace(tile->x, tile->y);
        }
        return;
    }
    if (!painting_) {
        if (!pointer.wasPressed(PointerButton::Left) || !tile) return;
        painting_ = true;
        if (tool_ == RegionTool::Rectangle || tool_ == RegionTool::River || tool_ == RegionTool::Lake || tool_ == RegionTool::Ridge) {
            rectFrom_ = tile; // the first corner, the source, the middle: the shape is made when the button goes up
            rectTo_ = tile;
        } else if (tool_ == RegionTool::Fill) {
            paintFill(tile->x, tile->y);
            return;
        } else if (tool_ == RegionTool::Cave) {
            placeCave(tile->x, tile->y);
            return;
        } else if (tool_ == RegionTool::Ford) {
            paintFord(tile->x, tile->y);
            return;
        } else if (tool_ == RegionTool::Dry) {
            dryWater(tile->x, tile->y);
            return;
        } else {
            beginStroke();
        }
    }
    if (tool_ == RegionTool::Rectangle || tool_ == RegionTool::River || tool_ == RegionTool::Lake || tool_ == RegionTool::Ridge) {
        if (tile) rectTo_ = tile;
    } else if (tool_ == RegionTool::Brush || tool_ == RegionTool::Erase) {
        if (!tile) return;
        const Point from = lastDab_ ? *lastDab_ : *tile;
        const int steps = std::max(std::abs(tile->x - from.x), std::abs(tile->y - from.y));
        for (int i = 0; i <= steps; ++i) {
            const int px = steps == 0 ? tile->x : from.x + (tile->x - from.x) * i / steps;
            const int py = steps == 0 ? tile->y : from.y + (tile->y - from.y) * i / steps;
            paintBrush(px, py);
        }
        lastDab_ = tile;
    }
}

// --- Water and mountains (US-203) ---

void RegionView::refreshWarnings() {
    warnings_ = region_ != nullptr ? sim::landWarnings(*region_) : std::vector<std::string>{};
}

// The tiles of a shape as one step of Undo. The edge wall is not painted; tiles that already have the biome change nothing.
int RegionView::paintTiles(const std::vector<sim::Tile>& tiles, sim::Biome biome, const std::string& label) {
    if (region_ == nullptr) return 0;
    beginStroke();
    for (const sim::Tile& at : tiles) editTile(at.x, at.y, biome);
    const int changed = static_cast<int>(stroke_.changes.size());
    finishStepAs(label);
    return changed;
}

int RegionView::paintRidge(sim::Tile a, sim::Tile b, int width) {
    if (region_ == nullptr) return 0;
    return paintTiles(sim::thicken(sim::line4(a, b), width, region_->size()), sim::Biome::Mountain, "ridge");
}

int RegionView::paintLake(sim::Tile centre, int radius) {
    if (region_ == nullptr) return 0;
    return paintTiles(sim::disc(centre, radius, region_->size()), sim::Biome::Water, "lake");
}

// The river is the thickened line, minus its fords. A ford is made of land across the whole width of the river (the tiles of the river near the middle
// of each span of `fordEvery` tiles), so there is a way over and no other.
int RegionView::paintRiver(sim::Tile a, sim::Tile b, int width, int fordEvery) {
    if (region_ == nullptr) return 0;
    const std::vector<sim::Tile> line = sim::line4(a, b);
    const std::vector<sim::Tile> river = sim::thicken(line, width, region_->size());
    std::set<std::pair<int, int>> ford;
    if (fordEvery > 0) {
        std::vector<sim::Tile> crossings;
        for (std::size_t i = static_cast<std::size_t>(fordEvery) / 2; i < line.size(); i += static_cast<std::size_t>(fordEvery)) crossings.push_back(line[i]);
        for (const sim::Tile& at : crossings) {
            // The ford covers the river a little beyond its width along the line, so the crossing is a strip of land, not a single tile on a diagonal.
            for (const sim::Tile& tile : sim::thicken({at}, width + 2, region_->size())) ford.insert({tile.x, tile.y});
        }
    }
    beginStroke();
    for (const sim::Tile& at : river) {
        if (ford.contains({at.x, at.y})) editTile(at.x, at.y, sim::Biome::Steppe);
        else editTile(at.x, at.y, sim::Biome::Water);
    }
    const int changed = static_cast<int>(stroke_.changes.size());
    finishStepAs("river");
    return changed;
}

bool RegionView::placeCave(int x, int y) {
    if (region_ == nullptr) return false;
    if (region_->biomeAt(x, y) != sim::Biome::Mountain || region_->onEdgeWall(x, y)) {
        if (say_) say_("A cave mouth goes into a cliff: click a mountain tile");
        return false;
    }
    return paintTiles({{x, y}}, sim::Biome::Cave, "cave mouth") > 0;
}

// A ford over water: the water under the brush becomes land (a crossing); land under it is left as it is.
int RegionView::paintFord(int x, int y) {
    if (region_ == nullptr) return 0;
    std::vector<sim::Tile> water;
    for (const sim::Tile& at : sim::thicken({{x, y}}, brushSize_, region_->size())) {
        if (region_->biomeAt(at.x, at.y) == sim::Biome::Water) water.push_back(at);
    }
    if (water.empty()) {
        if (say_) say_("A ford crosses water: click a river or a lake");
        return 0;
    }
    return paintTiles(water, sim::Biome::Steppe, "ford");
}

// Removes a lake or a river: the water connected to the clicked tile becomes meadow, as edits over the seed (refused above kMaxFill tiles).
int RegionView::dryWater(int x, int y) {
    if (region_ == nullptr) return 0;
    if (region_->biomeAt(x, y) != sim::Biome::Water) {
        if (say_) say_("Dry the water: click a lake or a river");
        return 0;
    }
    return fillFrom(x, y, sim::Biome::Steppe, "dry the water");
}

// --- Things, people and places (US-204) ---

namespace {

const char* const kDefaultPlaceKinds[] = {"shrine", "meeting-ground", "grave", "camp-site", "market", "well"};

} // namespace

bool RegionView::typing() const {
    if (settings_->typing()) return true;
    if (inspectorShown_ && inspector_ != nullptr && inspector_->typing()) return true;
    return placing() && placeRow_ != nullptr && ((kindField_ != nullptr && kindField_->focused()) || (nameField_ != nullptr && nameField_->focused()) || (propertyField_ != nullptr && propertyField_->focused()));
}

void RegionView::setPalette(PlacePalette palette) {
    palette_ = std::move(palette);
    if (palette_.places.empty()) palette_.places.assign(std::begin(kDefaultPlaceKinds), std::end(kDefaultPlaceKinds));
}

void RegionView::setPlaceGroup(sim::EditGroup group) {
    if (group != placeGroup_) placeKind_.clear(); // a kind of one group means nothing in another
    placeGroup_ = group;
}

std::vector<std::string> RegionView::kindSuggestions() const {
    switch (placeGroup_) {
    case sim::EditGroup::Person: return palette_.people;
    case sim::EditGroup::Place: return palette_.places;
    case sim::EditGroup::Camp: return {"player", "rival"};
    case sim::EditGroup::Resource: return {"flint", "wood", "berries", "herd"};
    default: return palette_.things;
    }
}

void RegionView::buildPlaceRow() {
    using luna::engine::Button;
    using luna::engine::Rect;
    using luna::engine::TextField;
    placeRow_ = std::make_unique<luna::engine::Panel>(Rect{0, kPlaceRowTop, viewWidth_, kPlaceRowHeight});
    groupButtons_.clear();
    int x = 4;
    const int y = kPlaceRowTop + 2;
    const int height = kPlaceRowHeight - 4;
    const struct {
        sim::EditGroup group;
        const char* label;
        const char* hint;
    } groups[] = {{sim::EditGroup::Thing, "Thing", "Plants, objects and animals"},
                  {sim::EditGroup::Person, "Person", "A character kind from the characters catalog; the game makes it when the region loads"},
                  {sim::EditGroup::Place, "Place", "A named spot a quest marker or a dialogue can point to"},
                  {sim::EditGroup::Camp, "Camp", "A clan seat: kind player (where you start) or rival (a rival clan starts there; Name is its clan)"},
                  {sim::EditGroup::Resource, "Resource", "Flint, wood, berries or herd: on a spot of the seed Set amount=50 sets how many can be taken; elsewhere it puts a new spot"}};
    for (const auto& entry : groups) {
        const sim::EditGroup group = entry.group;
        const int width = luna::engine::UiPainter::textWidth(entry.label) + 8;
        Button& button = placeRow_->add<Button>(Rect{x, y, width, height}, entry.label, [this, group] { setPlaceGroup(group); });
        button.hint = entry.hint;
        groupButtons_.push_back(&button);
        x += width + 2;
    }
    x += 4;
    kindField_ = &placeRow_->add<TextField>(Rect{x, y, 150, height}, "Kind: ", "", 40, [this](const std::string& value) { placeKind_ = value; });
    kindField_->suggest = [this](const std::string&) { return kindSuggestions(); };
    x += 156;
    nameField_ = &placeRow_->add<TextField>(Rect{x, y, 130, height}, "Name: ", "", 40, [this](const std::string& value) { placeName_ = value; });
    x += 136;
    propertyField_ = &placeRow_->add<TextField>(Rect{x, y, 150, height}, "Set: ", "", 60, [this](const std::string& value) {
        if (value.empty()) return;
        const std::size_t equals = value.find('=');
        if (equals == std::string::npos) {
            if (say_) say_("Write the property as key=value (key= clears it)");
        } else if (selected_.empty()) {
            if (say_) say_("Pick an entry first (the Move tool), then set its properties");
        } else {
            setProperty(value.substr(0, equals), value.substr(equals + 1));
        }
        propertyField_->value.clear();
    });
    x += 156;
    const struct {
        const char* label;
        sim::ConflictChoice choice;
        const char* hint;
    } fixes[] = {{"Fix: move", sim::ConflictChoice::Move, "Put every entry the land no longer suits on the nearest tile where it can stand"},
                 {"Fix: drop", sim::ConflictChoice::Remove, "Take off every entry the land no longer suits"}};
    {
        const int width = luna::engine::UiPainter::textWidth("Anyway") + 8;
        forcedButton_ = &placeRow_->add<Button>(Rect{x, y, width, height}, "Anyway", [this] { placeForced_ = !placeForced_; });
        forcedButton_->hint = "A camp placed anyway stays where the site check fails (no water and food within reach); it is listed as forced";
        x += width + 2;
    }
    for (const auto& fix : fixes) {
        const sim::ConflictChoice which = fix.choice;
        const int width = luna::engine::UiPainter::textWidth(fix.label) + 8;
        Button& button = placeRow_->add<Button>(Rect{x, y, width, height}, fix.label, [this, which] { settleConflicts(which); });
        button.hint = fix.hint;
        x += width + 2;
    }
}

void RegionView::recordPlaced(const std::string& label, std::vector<sim::PlacedChange> changes) {
    if (changes.empty()) return;
    WorldCommand command;
    command.label = label;
    command.placed = changes;
    history_.record(std::move(command));
    dirty_ = true;
    refreshPlaced(changes);
    refreshSetupIssues(); // a camp put or taken away changes which clans the setup may name
    settings_->refreshNow();
}

void RegionView::refreshPlaced(const std::vector<sim::PlacedChange>& changes) {
    if (region_ == nullptr) return;
    sim::applyPlacedToRegion(*region_, edits_); // the resources, hidden spots and the start follow the entries
    for (const sim::PlacedChange& change : changes) {
        if (change.before) markStale(change.before->x, change.before->y);
        if (change.after) markStale(change.after->x, change.after->y);
    }
}

std::string RegionView::placeEntry(int x, int y) {
    if (region_ == nullptr) return {};
    sim::PlacedEdit entry;
    entry.group = placeGroup_;
    entry.kind = placeKind_;
    entry.name = placeName_;
    if (placeGroup_ == sim::EditGroup::Person) entry.npcClass = placeClass_;
    entry.forced = placeGroup_ == sim::EditGroup::Camp && placeForced_;
    entry.x = x;
    entry.y = y;
    std::string problem;
    const std::optional<sim::PlacedChange> change = sim::addPlaced(*region_, edits_, entry, problem);
    if (!change) {
        if (say_) say_("Not placed: " + problem);
        return {};
    }
    selected_ = change->id;
    if (placeGroup_ == sim::EditGroup::Place) placeName_.clear(); // the next place needs a name of its own
    recordPlaced("place " + placeKind_, {*change});
    return change->id;
}

bool RegionView::select(const std::string& id) {
    if (sim::findPlaced(edits_, id) == nullptr) return false;
    selected_ = id;
    return true;
}

bool RegionView::selectAt(int x, int y) {
    const sim::PlacedEdit* entry = sim::placedAt(edits_, x, y);
    if (entry == nullptr) return false;
    selected_ = entry->id;
    return true;
}

bool RegionView::moveSelectedTo(int x, int y) {
    if (region_ == nullptr || selected_.empty()) return false;
    std::string problem;
    const std::optional<sim::PlacedChange> change = sim::movePlaced(*region_, edits_, selected_, x, y, problem);
    if (!change) {
        if (say_ && !problem.empty()) say_("Not moved: " + problem);
        return false;
    }
    recordPlaced("move " + selected_, {*change});
    return true;
}

bool RegionView::takeAway(int x, int y) {
    if (region_ == nullptr) return false;
    std::string problem;
    std::optional<sim::PlacedChange> change;
    if (const sim::PlacedEdit* entry = sim::placedAt(edits_, x, y)) {
        const std::string id = entry->id; // removePlaced erases the entry the pointer names
        change = sim::removePlaced(edits_, id, problem);
        if (change && id == selected_) selected_.clear();
    } else {
        change = sim::hideSeedThing(*region_, edits_, x, y, problem);
    }
    if (!change) {
        if (say_ && !problem.empty()) say_("Nothing taken away: " + problem);
        return false;
    }
    recordPlaced("take away " + change->id, {*change});
    return true;
}

bool RegionView::setProperty(const std::string& key, const std::string& value) {
    if (selected_.empty()) return false;
    std::string problem;
    const std::optional<sim::PlacedChange> change = sim::setPlacedProperty(edits_, selected_, key, value, problem);
    if (!change) {
        if (say_ && !problem.empty()) say_("Property not set: " + problem);
        return false;
    }
    recordPlaced("set " + key + " of " + selected_, {*change});
    return true;
}

int RegionView::settleConflicts(sim::ConflictChoice choice) {
    if (region_ == nullptr) return 0;
    std::vector<sim::PlacedChange> changes = sim::resolveConflicts(*region_, edits_, sim::findConflicts(*region_, edits_), choice);
    const int count = static_cast<int>(changes.size());
    if (!selected_.empty() && sim::findPlaced(edits_, selected_) == nullptr) selected_.clear();
    if (count > 0) {
        recordPlaced(choice == sim::ConflictChoice::Move ? "move the entries that no longer fit" : "drop the entries that no longer fit", std::move(changes));
        if (say_) say_(std::format("{} {}", count, choice == sim::ConflictChoice::Move ? "entries moved to the nearest place they can stand" : "entries dropped"));
    } else if (say_) {
        say_("Nothing to settle");
    }
    return count;
}

void RegionView::handlePlace(int x, int y) {
    if (tool_ == RegionTool::Place) {
        placeEntry(x, y);
    } else if (tool_ == RegionTool::Take) {
        takeAway(x, y);
    } else if (tool_ == RegionTool::Move) {
        const sim::PlacedEdit* hit = sim::placedAt(edits_, x, y);
        if (hit != nullptr && hit->id != selected_) select(hit->id);
        else if (hit == nullptr && !selected_.empty()) moveSelectedTo(x, y);
    }
}


// --- The inspector: clans and clan members (US-206) ---

void RegionView::refreshSetupIssues() { setupIssues_ = sim::setupProblems(setup_, edits_); }

void RegionView::recordSetup(const std::string& label, const sim::WorldSetup& before) {
    WorldCommand command;
    command.label = label;
    command.setup = std::make_pair(before, setup_);
    history_.record(std::move(command));
    dirty_ = true;
    refreshSetupIssues();
}

bool RegionView::setClanField(const std::string& clan, const std::string& field, const std::string& text) {
    if (clan.empty()) {
        if (say_) say_("Choose the clan first: player, or the name of a rival camp");
        return false;
    }
    const sim::WorldSetup before = setup_;
    sim::ClanSetup next = setup_.clans.contains(clan) ? setup_.clans.at(clan) : sim::ClanSetup{};
    std::string problem;
    bool fine = true;
    if (field == "leader") next.leader = text;
    else if (field == "stance") next.stance = text;
    else if (field == "store") fine = sim::setStoreText(next, text, problem);
    else if (field == "debts") fine = sim::setDebtsText(next, text, problem);
    else if (field == "partners") next.partners = sim::parseList(text);
    else if (field == "members") next.members = sim::parseList(text);
    else {
        fine = false;
        problem = "a clan has leader, stance, store, debts, partners and members";
    }
    if (!fine) {
        if (say_) say_("Not set: " + problem);
        return false;
    }
    if (next.empty()) setup_.clans.erase(clan);
    else setup_.clans[clan] = std::move(next);
    if (setup_ == before) return false;
    recordSetup("set " + field + " of " + clan, before);
    return true;
}

bool RegionView::setPersonField(const std::string& member, const std::string& field, const std::string& text) {
    int id = -1;
    const auto parsed = std::from_chars(member.data(), member.data() + member.size(), id);
    if (member.empty() || parsed.ec != std::errc{} || parsed.ptr != member.data() + member.size() || id < 0) {
        if (say_) say_("Choose the clan member first: their number");
        return false;
    }
    const sim::WorldSetup before = setup_;
    sim::PersonSetup next = setup_.people.contains(member) ? setup_.people.at(member) : sim::PersonSetup{};
    std::string problem;
    bool fine = true;
    if (field == "kin") fine = sim::setKinText(next, text, problem);
    else if (field == "opinions") fine = sim::setOpinionsText(next, text, problem);
    else if (field == "grudges") fine = sim::setGrudgesText(next, text, problem);
    else {
        fine = false;
        problem = "a member has kin, opinions and grudges";
    }
    if (!fine) {
        if (say_) say_("Not set: " + problem);
        return false;
    }
    if (next.empty()) setup_.people.erase(member);
    else setup_.people[member] = std::move(next);
    if (setup_ == before) return false;
    recordSetup("set " + field + " of member " + member, before);
    return true;
}

void RegionView::buildInspector() {
    using luna::engine::Rect;
    using luna::engine::TextField;
    const Rect area = inspectorArea();
    inspector_ = std::make_unique<luna::engine::Panel>(area);
    inspectorRows_.clear();
    int y = area.y + 3;
    const int width = area.width - 8;
    const int x = area.x + 4;
    const auto row = [&](const std::string& label, std::function<std::string()> text, std::function<void(const std::string&)> commit, const std::string& tip) {
        TextField& field = inspector_->add<TextField>(Rect{x, y, width, 14}, label, "", 120, std::move(commit));
        field.tip.text = tip;
        inspectorRows_.push_back({&field, std::move(text)});
        y += 16;
        return &field;
    };
    clanField_ = &inspector_->add<TextField>(Rect{x, y, width, 14}, "Clan: ", clanKey_, 40, [this](const std::string& value) { clanKey_ = value; });
    clanField_->suggest = [this](const std::string&) {
        std::vector<std::string> names = {"player"};
        for (const sim::CampSite& camp : sim::campsOf(edits_)) {
            if (!camp.player && !camp.clan.empty()) names.push_back(camp.clan);
        }
        return names;
    };
    y += 16;
    const auto clan = [this]() -> const sim::ClanSetup& {
        static const sim::ClanSetup none;
        const auto found = setup_.clans.find(clanKey_);
        return found == setup_.clans.end() ? none : found->second;
    };
    row("Leader: ", [clan] { return clan().leader; }, [this](const std::string& v) { setClanField(clanKey_, "leader", v); }, "A member's number or a name");
    row("Stance: ", [clan] { return clan().stance; }, [this](const std::string& v) { setClanField(clanKey_, "stance", v); }, "A rival's stance towards you: friendly, wary, hostile...");
    row("Store: ", [clan] { return sim::storeText(clan()); }, [this](const std::string& v) { setClanField(clanKey_, "store", v); }, "food=80 flint=20 (food is the meals; the player's items go in the hero's store)");
    row("Debts: ", [clan] { return sim::debtsText(clan()); }, [this](const std::string& v) { setClanField(clanKey_, "debts", v); }, "the River Clan: fur=3 value=12 days=10; ...");
    row("Partners: ", [clan] { return sim::listText(clan().partners); }, [this](const std::string& v) { setClanField(clanKey_, "partners", v); }, "Clans it trades with, separated by commas");
    row("Members: ", [clan] { return sim::listText(clan().members); }, [this](const std::string& v) { setClanField(clanKey_, "members", v); }, "Member numbers this clan claims, separated by commas");
    memberField_ = &inspector_->add<TextField>(Rect{x, y, width, 14}, "Member: ", memberKey_, 8, [this](const std::string& value) { memberKey_ = value; });
    y += 16;
    const auto person = [this]() -> const sim::PersonSetup& {
        static const sim::PersonSetup none;
        const auto found = setup_.people.find(memberKey_);
        return found == setup_.people.end() ? none : found->second;
    };
    row("Kin: ", [person] { return sim::kinText(person()); }, [this](const std::string& v) { setPersonField(memberKey_, "kin", v); }, "mother=2 father=3 partner=4");
    row("Opinions: ", [person] { return sim::opinionsText(person()); }, [this](const std::string& v) { setPersonField(memberKey_, "opinions", v); }, "What they think of others: 5=-50 7=20 (-100 to 100)");
    row("Grudges: ", [person] { return sim::grudgesText(person()); }, [this](const std::string& v) { setPersonField(memberKey_, "grudges", v); }, "member:weight:reason; ... The chronicle tells each reason");
}

} // namespace odysseus::game
