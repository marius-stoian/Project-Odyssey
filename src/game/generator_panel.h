#pragma once

#include "boundary.h"

#include "game/editor_help.h"
#include "luna/engine/input.h"
#include "luna/engine/minimap.h"
#include "luna/engine/renderer.h"
#include "luna/engine/ui.h"
#include "sim/region.h"
#include "sim/region_edits.h"

#include <array>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace odysseus::game {

// One generator setting the owner can change: its key in assets/data/sim/region.json, the label on screen, where it lives in RegionConfig and its
// range (the same range sim::regionConfigProblems and the schema enforce). `size` and `chunkSize` are not here: a different size moves every chunk
// and breaks saves (D-62 Q2).
struct GeneratorSetting {
    const char* key;
    const char* label;
    int sim::RegionConfig::*member;
    int minimum;
    int maximum;
};

const std::vector<GeneratorSetting>& generatorSettings();

// The generator settings of the Region view (US-201, EDT-04, design M12 section 5): every setting of region.json as a number field, a Preview that
// regenerates a low-resolution map of the land the draft would make beside the one on screen (and says how the mix of land changes), Apply that writes
// region.json and reopens the region with the new land, and the list of hand edits the new land no longer suits (they stay where they are, ADR-020).
// The preview calls the same sim::Region as the game, so what it shows is what Apply makes.
class GeneratorPanel {
public:
    using Say = std::function<void(const std::string&)>;

    static constexpr int kThumbCells = 128;     // the low-resolution map: 128 x 128 samples of the land, whatever its size
    static constexpr int kThumbSize = 92;       // and how big it is on screen
    static constexpr int kRowsPerTick = 32;     // rows of a thumbnail painted in one tick: a preview is ready in 4 ticks (0.2 s)
    static constexpr int kPanelWidth = 226;

    GeneratorPanel(int viewWidth, int viewHeight, Say say);

    // region.json (written by Apply), what to call after writing it (the game reads the sets that watch it again), and what to call with the
    // new settings once written (the Region view opens the new land).
    void setSource(std::filesystem::path configFile, std::function<void(const std::filesystem::path&)> saved, std::function<void(const sim::RegionConfig&)> applied);
    // The land on screen and the hand edits laid over it; the draft starts from its settings. Call when the view opens a region.
    void bind(sim::Region* current, const sim::RegionEdits* edits);
    // The land on screen was painted: its small map is painted again.
    void refreshNow();
    // Gives the fields their tooltips from the schema's help (the entries "data.sim-region.<key>").
    void applyHelp(EditorHelp& help);

    bool shown() const { return shown_; }
    void show(bool shown) { shown_ = shown; }
    bool typing() const { return shown_ && panel_ && panel_->typing(); }
    luna::engine::Rect bounds() const { return {viewWidth_ - kPanelWidth - 2, 40, kPanelWidth, 386}; }

    // The draft: what the fields say. Not the land on screen until Apply.
    const sim::RegionConfig& draft() const { return draft_; }
    void setDraft(const std::string& key, int value); // as typing in the field would (also moves the field)
    std::vector<std::string> problems() const { return sim::regionConfigProblems(draft_); }
    void revert();

    // Preview: false when the draft breaks a rule (the first problem is said). Otherwise the preview region is made and its map painted over the next ticks.
    bool preview();
    bool previewStarted() const { return previewRegion_ != nullptr; }
    bool previewComplete() const { return previewRegion_ && previewMap_ && previewMap_->complete(); }
    const sim::Region* previewRegion() const { return previewRegion_.get(); }

    // Apply: writes region.json by patching only the settings that changed, then reopens the region. False (with the reason said) when it cannot.
    bool apply();

    // The share of the land that is each biome, in whole per cent, over the low-resolution map. Valid once that map is complete.
    struct Mix {
        std::array<int, 5> percent{};
        bool valid = false;
    };
    Mix nowMix() const { return nowMix_; }
    Mix previewMix() const { return previewMix_; }
    // The hand edits the previewed (or just applied) land does not suit.
    const std::vector<sim::EditConflict>& conflicts() const { return conflicts_; }

    void update(const luna::engine::Intents& intents);
    void draw(luna::engine::UiPainter& painter, luna::engine::Renderer& renderer) const;
    void drawOverlay(luna::engine::UiPainter& painter) const;

private:
    void build();
    void paint(luna::engine::Minimap& map, sim::Region& land, Mix& mix, std::array<int, 5>& counts);
    void refreshConflicts(sim::Region& land);

    int viewWidth_;
    int viewHeight_;
    Say say_;
    std::filesystem::path configFile_;
    std::function<void(const std::filesystem::path&)> saved_;
    std::function<void(const sim::RegionConfig&)> applied_;
    bool shown_ = false;
    sim::Region* current_ = nullptr;
    const sim::RegionEdits* edits_ = nullptr;
    sim::RegionConfig draft_;
    std::unique_ptr<sim::Region> previewRegion_;
    std::unique_ptr<luna::engine::Minimap> nowMap_;
    std::unique_ptr<luna::engine::Minimap> previewMap_;
    std::array<int, 5> nowCounts_{};
    std::array<int, 5> previewCounts_{};
    Mix nowMix_;
    Mix previewMix_;
    std::vector<sim::EditConflict> conflicts_;
    std::string note_;
    std::unique_ptr<luna::engine::Panel> panel_;
    std::vector<luna::engine::NumberField*> fields_;
};

} // namespace odysseus::game
