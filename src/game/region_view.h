#pragma once

#include "boundary.h"

#include "game/generator_panel.h"
#include "game/world_history.h"
#include "luna/engine/image.h"
#include "luna/engine/input.h"
#include "luna/engine/minimap.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"
#include "sim/region.h"
#include "sim/region_edits.h"
#include "sim/world_file.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace odysseus::game {

// The layers of the region view (US-200, EDT-04). Each one is drawn from its own pictures, so hiding one only skips its drawing.
// People and animals share a layer until people can be placed (US-204); Places and Camps are empty until US-204 and US-205.
enum class RegionLayer { Terrain, Water, Plants, Things, People, Places, Camps, Count };

inline constexpr int kRegionLayerCount = static_cast<int>(RegionLayer::Count);

const char* regionLayerName(RegionLayer layer);

// What a left click on the map does (US-202). Pan is the default; the others paint the biome chosen on the bar, and then the middle or right button pans.
enum class RegionTool { Pan, Brush, Rectangle, Fill, Erase, River, Lake, Ridge, Cave, Ford, Dry };

// The region in the Editor (US-200, design docs/plans/M12-world-editing-design.md section 4): the whole generated land on a zoomable map.
// It makes its own Region from the same seed and the same region.json as the game, so the two are the same world (the "Same world" test
// compares them tile for tile). Each 32-tile chunk is drawn from one small picture per layer (one pixel a tile, stretched to the zoom), made
// the first time the chunk is seen and kept; at most kChunkBuildsPerFrame are made in a frame, so a pan never stalls. A cached overview in
// the corner shows the whole map and moves the view when clicked. It takes the whole screen like the Data tab; Esc comes back.
class RegionView {
public:
    using Say = std::function<void(const std::string&)>;

    static constexpr int kMinZoom = 0;
    static constexpr int kMaxZoom = 5;                // tile size on screen = 2 << zoom: 2 px (the whole 256 x 256 map) up to 64 px (single tiles)
    static constexpr int kChunkBuildsPerFrame = 4;    // chunk pictures made in one drawn frame
    static constexpr int kOverviewRowsPerTick = 32;   // rows of the overview painted in one tick
    static constexpr int kBarHeight = 20;
    static constexpr int kToolRowHeight = 18;         // the row of tools under the bar
    static constexpr int kTopHeight = kBarHeight + kToolRowHeight;
    static constexpr int kMaxBrush = 9;
    static constexpr int kOverviewSize = 96;          // the overview on screen, in pixels

    RegionView(int viewWidth, int viewHeight, Say say);

    // Where the land comes from: region.json, and the seed of the game being edited (called each time the view opens).
    // `saved` is called with region.json after the generator settings wrote it (the game reads the sets that watch it again).
    void setSource(std::filesystem::path configFile, std::function<std::uint64_t()> seedNow, std::function<void(const std::filesystem::path&)> saved = {});

    // Where the world file lives (assets/worlds) and its name (US-202). When the view opens a region it reads <folder>/<name>.json if that file is for the same seed.
    void setWorldsFolder(std::filesystem::path folder, std::string name = "default");
    std::filesystem::path worldFile() const { return worldsFolder_ / (worldName_ + ".json"); }
    // Writes the hand edits (and the generator settings that differ from region.json) to the world file. False, with the reason said, when it cannot.
    bool saveWorld();
    bool worldDirty() const { return dirty_; }

    bool shown() const { return shown_; }
    void show(bool shown);
    // Makes the region for a seed and shows it from the whole-map zoom, centred on the start. False (with a message) when region.json cannot be read.
    bool open(std::uint64_t seed);
    void open(std::uint64_t seed, sim::RegionConfig config);
    bool isOpen() const { return region_ != nullptr; }
    // The generator settings with Preview and Apply (US-201), and the hand edits laid over the land (empty until US-202): changing the settings keeps them
    // and lists the ones the new land does not suit.
    GeneratorPanel& settings() { return *settings_; }
    sim::RegionEdits& edits() { return edits_; }
    void applyHelp(EditorHelp& help) { settings_->applyHelp(help); }
    bool typing() const { return settings_->typing(); } // a settings field is being typed in: the keys belong to it
    sim::Region* region() { return region_.get(); }
    const sim::Region* region() const { return region_.get(); }

    // What lies on a tile (the same data the game builds its level from).
    struct Cell {
        sim::Biome biome = sim::Biome::Steppe;
        std::optional<sim::ResourceKind> resource;
    };
    Cell cellAt(int x, int y) const;
    // The layer a resource is drawn in.
    static RegionLayer layerOf(sim::ResourceKind kind);
    // The picture of one chunk in one layer: chunkSize x chunkSize, one pixel a tile, see-through where the layer has nothing.
    luna::engine::Image chunkImage(int cx, int cy, RegionLayer layer) const;

    // Painting (US-202): the tools of the second row. Every stroke, rectangle and fill is one step of Undo (Ctrl+Z, Ctrl+Y) in the world history. A tile painted
    // the biome the seed already gives is not an edit at all, so the file holds only differences.
    RegionTool tool() const { return tool_; }
    void setTool(RegionTool tool) { tool_ = tool; }
    sim::Biome paintBiome() const { return paintBiome_; }
    void setPaintBiome(sim::Biome biome) { paintBiome_ = biome; }
    int brushSize() const { return brushSize_; }
    void setBrushSize(int size) { brushSize_ = std::clamp(size, 1, kMaxBrush); }
    void beginStroke();                         // the brush dabs until endStroke are one step
    void endStroke();
    int paintBrush(int x, int y);               // stamps the brush (the Erase tool clears) at a tile; returns the tiles that changed
    int paintRectangle(int x0, int y0, int x1, int y1); // one step; returns the tiles that changed
    int paintFill(int x, int y);                // the connected tiles of the same biome, one step; refused (0, with a message) above kMaxFill tiles
    // Water and mountains (US-203). Each is one step of Undo and only tile edits, so they save, undo and regenerate like any painting.
    // A river flows along the line from a to b, `width` tiles wide; every `fordEvery` tiles (0: none) it is broken by a ford, a crossing of land. A ridge is the same line of
    // mountain. A lake is a disc of water. A cave mouth is put into a cliff (a mountain tile); Ford wades a crossing over water under the brush; Dry turns the connected
    // water (a lake or a river) back into land.
    int paintRiver(sim::Tile a, sim::Tile b, int width, int fordEvery);
    int paintRidge(sim::Tile a, sim::Tile b, int width);
    int paintLake(sim::Tile centre, int radius);
    bool placeCave(int x, int y);
    int paintFord(int x, int y);
    int dryWater(int x, int y);
    int fordEvery() const { return fordEvery_; }
    void setFordEvery(int every) { fordEvery_ = std::clamp(every, 0, 64); }
    // What the land as it is would trouble the owner with (the start without water, a sealed cave mouth); shown in the status line. Never a block.
    const std::vector<std::string>& warnings() const { return warnings_; }
    bool undo();
    bool redo();
    const WorldHistory& history() const { return history_; }
    static constexpr int kMaxFill = 20000;

    // The view.
    int zoom() const { return zoom_; }
    void setZoom(int zoom);
    int tilePixels() const { return 2 << zoom_; }
    double centreX() const { return centreX_; }
    double centreY() const { return centreY_; }
    void centreOn(double tileX, double tileY);
    // Which tiles are on screen: the first and the last (inclusive), clipped to the region.
    struct TileRange {
        int x0 = 0, y0 = 0, x1 = -1, y1 = -1;
        bool empty() const { return x1 < x0 || y1 < y0; }
    };
    TileRange visibleTiles() const;
    // The tile under a screen point, if it is inside the region.
    std::optional<luna::engine::Point> tileAtScreen(int screenX, int screenY) const;

    bool layerShown(RegionLayer layer) const { return layers_[static_cast<std::size_t>(layer)]; }
    void setLayerShown(RegionLayer layer, bool shown);

    void update(const luna::engine::Intents& intents);
    // Draws the map, the bar and the overview. Chunk pictures are made here (the renderer owns textures), so the caches are mutable.
    void render(luna::engine::Renderer& renderer, luna::engine::UiPainter& painter) const;

    // For tests and the status line.
    int chunkBuildsThisFrame() const { return builtThisFrame_; }
    int chunkPicturesMade() const { return static_cast<int>(textures_.size()); }
    // The layer a chunk picture (a texture id) belongs to; nothing for other textures (the overview, the UI).
    std::optional<RegionLayer> layerOfTexture(int id) const;
    int overviewRowsDone() const { return overview_ ? overview_->rowsDone() : 0; }
    bool overviewComplete() const { return overview_ && overview_->complete(); }
    luna::engine::Rect overviewArea() const { return {viewWidth_ - kOverviewSize - 4, viewHeight_ - kOverviewSize - 14, kOverviewSize, kOverviewSize}; }
    const std::string& hoverText() const { return hover_; }

private:
    void buildBar();
    void syncBar();
    luna::engine::Point origin() const; // screen position of tile (0, 0)
    void zoomAround(int newZoom, int screenX, int screenY);
    void clampCentre();
    void reopen(const sim::RegionConfig& config); // Apply: the same view over the new land
    void buildTools();
    void finishStepAs(const std::string& label); // a step closed by a tool that is not a stroke
    int paintTiles(const std::vector<sim::Tile>& tiles, sim::Biome biome, const std::string& label); // the tiles as one step; the number that changed
    int fillFrom(int x, int y, sim::Biome biome, const std::string& label);
    void refreshWarnings();
    bool editTile(int x, int y, std::optional<sim::Biome> target); // one tile into the open step; true when it changed
    void finishStep(const std::string& label);                     // the open step into the history, the pictures and the overview
    void applyChanges(const std::vector<TileChange>& changes, bool forward);
    void syncTileEdits();                                          // edits_.tiles from the region
    void markStale(int x, int y);
    void handlePaint(const luna::engine::Pointer& pointer);
    const luna::engine::Texture* chunkTexture(luna::engine::Renderer& renderer, int cx, int cy, RegionLayer layer) const;

    int viewWidth_;
    int viewHeight_;
    Say say_;
    std::filesystem::path configFile_;
    std::function<std::uint64_t()> seedNow_;
    bool shown_ = false;
    std::unique_ptr<sim::Region> region_;
    std::unique_ptr<luna::engine::Minimap> overview_;
    int zoom_ = kMinZoom;
    double centreX_ = 0.0; // in tiles
    double centreY_ = 0.0;
    std::array<bool, kRegionLayerCount> layers_{};
    std::optional<luna::engine::Point> dragFrom_;
    std::string hover_;
    std::unique_ptr<luna::engine::Panel> bar_;
    std::vector<luna::engine::Button*> layerButtons_;
    std::unique_ptr<GeneratorPanel> settings_;
    sim::RegionEdits edits_;
    sim::RegionConfig baseConfig_;                 // region.json as it was read: the world file stores the settings that differ from it
    std::filesystem::path worldsFolder_;
    std::string worldName_ = "default";
    bool dirty_ = false;                           // edits not yet in the world file
    RegionTool tool_ = RegionTool::Pan;
    sim::Biome paintBiome_ = sim::Biome::Water;
    int brushSize_ = 3;
    int fordEvery_ = 0;
    std::vector<std::string> warnings_;
    WorldHistory history_;
    bool strokeOpen_ = false;
    WorldCommand stroke_;
    bool painting_ = false;                        // the left button is down with a painting tool
    std::optional<luna::engine::Point> lastDab_;   // the previous tile of a brush stroke, so a fast drag leaves no gaps
    std::optional<luna::engine::Point> rectFrom_;  // the first corner of a rectangle being dragged
    std::optional<luna::engine::Point> rectTo_;
    std::unique_ptr<luna::engine::Panel> tools_;
    std::vector<luna::engine::Button*> toolButtons_;
    std::vector<luna::engine::Button*> biomeButtons_;
    luna::engine::NumberField* sizeField_ = nullptr;
    luna::engine::NumberField* fordField_ = nullptr;

    mutable std::map<std::tuple<int, int, int>, luna::engine::Texture> textures_; // (chunk x, chunk y, layer)
    mutable std::set<std::tuple<int, int, int>> stale_;      // pictures whose land was painted since they were made: made again when there is room in the frame
    mutable std::vector<luna::engine::Texture> pendingDestroy_; // pictures given back at the start of the next drawn frame
    mutable std::map<int, RegionLayer> textureLayer_;
    mutable int builtThisFrame_ = 0;
};

} // namespace odysseus::game
