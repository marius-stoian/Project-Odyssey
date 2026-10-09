#include "game/generator_panel.h"

#include "sim/data_document.h"

#include <algorithm>
#include <format>

namespace odysseus::game {

using luna::engine::Button;
using luna::engine::Color;
using luna::engine::DrawStyle;
using luna::engine::Minimap;
using luna::engine::NumberField;
using luna::engine::Panel;
using luna::engine::Rect;
using luna::engine::UiColor;
using luna::engine::UiInput;
using luna::engine::UiPainter;

namespace {

constexpr int kRowPitch = 15;
constexpr int kFirstRow = 16;
constexpr Color kColours[5] = {{166, 176, 96, 255}, {58, 110, 58, 255}, {52, 98, 160, 255}, {120, 116, 112, 255}, {60, 52, 48, 255}};
const char* const kBiomeShort[5] = {"steppe", "forest", "water", "mount.", "cave"};

} // namespace

const std::vector<GeneratorSetting>& generatorSettings() {
    using C = sim::RegionConfig;
    static const std::vector<GeneratorSetting> settings = {
        {"lakeLevel", "Lake level: ", &C::lakeLevel, 0, 1023},
        {"mountainLevel", "Mountain level: ", &C::mountainLevel, 100, 1023},
        {"riverBand", "River width: ", &C::riverBand, 0, 100},
        {"forestMoisture", "Forest moisture: ", &C::forestMoisture, 0, 1023},
        {"caveNoise", "Cave noise: ", &C::caveNoise, 0, 1023},
        {"edgeWall", "Edge wall: ", &C::edgeWall, 0, 32},
        {"flintPerMille", "Flint per mille: ", &C::flintPerMille, 0, 1000},
        {"woodPerMille", "Wood per mille: ", &C::woodPerMille, 0, 1000},
        {"berriesPerMille", "Berries per mille: ", &C::berriesPerMille, 0, 1000},
        {"herdPerMille", "Herds per mille: ", &C::herdPerMille, 0, 1000},
        {"herdMinimum", "Herd minimum: ", &C::herdMinimum, 1, 100},
        {"herdMaximum", "Herd maximum: ", &C::herdMaximum, 1, 100},
        {"berryRegrowDays", "Berry regrow days: ", &C::berryRegrowDays, 1, 365},
        {"startNeedWithin", "Start needs within: ", &C::startNeedWithin, 5, 60},
    };
    return settings;
}

GeneratorPanel::GeneratorPanel(int viewWidth, int viewHeight, Say say) : viewWidth_(viewWidth), viewHeight_(viewHeight), say_(std::move(say)) { build(); }

void GeneratorPanel::setSource(std::filesystem::path configFile, std::function<void(const std::filesystem::path&)> saved, std::function<void(const sim::RegionConfig&)> applied) {
    configFile_ = std::move(configFile);
    saved_ = std::move(saved);
    applied_ = std::move(applied);
}

void GeneratorPanel::build() {
    const Rect area = bounds();
    panel_ = std::make_unique<Panel>(area);
    fields_.clear();
    int y = area.y + kFirstRow;
    for (const GeneratorSetting& setting : generatorSettings()) {
        const std::string key = setting.key;
        NumberField& field = panel_->add<NumberField>(Rect{area.x + 4, y, area.width - 8, 14}, setting.label, draft_.*setting.member, setting.minimum, setting.maximum,
                                                      [this, setting](int value) { draft_.*setting.member = value; });
        fields_.push_back(&field);
        y += kRowPitch;
    }
    const int buttonY = y + 2;
    auto add = [&](int x, int width, const std::string& label, const std::string& hint, auto action) {
        Button& button = panel_->add<Button>(Rect{x, buttonY, width, 14}, label, action);
        button.hint = hint;
    };
    add(area.x + 4, 56, "Preview", "Make a low-resolution map of the land these settings would make, beside the one now (nothing is saved)", [this] { preview(); });
    add(area.x + 62, 48, "Apply", "Write these settings to region.json and open the new land; your hand edits stay where they are", [this] { apply(); });
    add(area.x + 112, 52, "Revert", "Put the fields back to the settings of the land on screen", [this] { revert(); });
    add(area.x + 166, 56, "Close", "Hide the generator settings", [this] { show(false); });
}

void GeneratorPanel::bind(sim::Region* current, const sim::RegionEdits* edits) {
    current_ = current;
    edits_ = edits;
    previewRegion_.reset();
    previewMap_.reset();
    previewMix_ = {};
    conflicts_.clear();
    note_.clear();
    nowMap_.reset();
    nowMix_ = {};
    nowCounts_ = {};
    if (current_ != nullptr) {
        draft_ = current_->config();
        nowMap_ = std::make_unique<Minimap>(kThumbCells, kThumbCells);
    }
    for (std::size_t i = 0; i < fields_.size(); ++i) fields_[i]->value = draft_.*generatorSettings()[i].member;
}

void GeneratorPanel::applyHelp(EditorHelp& help) {
    for (std::size_t i = 0; i < fields_.size(); ++i) {
        const std::string id = std::string("data.sim-region.") + generatorSettings()[i].key;
        fields_[i]->helpId = id;
        const EditorHelp::Entry* entry = help.find(id);
        if (entry == nullptr) {
            fields_[i]->tip.text.clear();
            continue;
        }
        std::string text = entry->purpose + std::format("\nRange: {} to {}", fields_[i]->minimum, fields_[i]->maximum);
        if (!entry->example.empty()) text += "\nExample: " + entry->example;
        fields_[i]->tip.text = text;
    }
}

void GeneratorPanel::setDraft(const std::string& key, int value) {
    const auto& settings = generatorSettings();
    for (std::size_t i = 0; i < settings.size(); ++i) {
        if (key != settings[i].key) continue;
        draft_.*settings[i].member = value;
        fields_[i]->value = value;
        return;
    }
}

void GeneratorPanel::revert() {
    if (current_ == nullptr) return;
    draft_ = current_->config();
    for (std::size_t i = 0; i < fields_.size(); ++i) fields_[i]->value = draft_.*generatorSettings()[i].member;
    previewRegion_.reset();
    previewMap_.reset();
    previewMix_ = {};
    conflicts_.clear();
    note_ = "Back to the settings of the land on screen";
}

void GeneratorPanel::refreshConflicts(sim::Region& land) {
    conflicts_ = edits_ != nullptr ? sim::findConflicts(land, *edits_) : std::vector<sim::EditConflict>{};
}

bool GeneratorPanel::preview() {
    if (current_ == nullptr) return false;
    const std::vector<std::string> found = problems();
    if (!found.empty()) {
        note_ = found.front();
        if (say_) say_("Generator settings: " + found.front());
        return false;
    }
    previewRegion_ = std::make_unique<sim::Region>(current_->seed(), draft_);
    previewMap_ = std::make_unique<Minimap>(kThumbCells, kThumbCells);
    previewCounts_ = {};
    previewMix_ = {};
    refreshConflicts(*previewRegion_);
    note_ = "Previewing...";
    return true;
}

bool GeneratorPanel::apply() {
    if (current_ == nullptr) return false;
    const std::vector<std::string> found = problems();
    if (!found.empty()) {
        note_ = found.front();
        if (say_) say_("Generator settings: " + found.front());
        return false;
    }
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(configFile_, problem);
    if (!document) {
        note_ = "Cannot read region.json: " + problem;
        if (say_) say_(note_);
        return false;
    }
    const sim::RegionConfig& old = current_->config();
    for (const GeneratorSetting& setting : generatorSettings()) {
        if (draft_.*setting.member == old.*setting.member) continue; // only what changed is written
        if (!document->set(setting.key, sim::OrderedJson(draft_.*setting.member), problem)) {
            note_ = std::string("Cannot set ") + setting.key + ": " + problem;
            if (say_) say_(note_);
            return false;
        }
    }
    if (const std::optional<std::string> failure = document->save()) {
        note_ = "Cannot write region.json: " + *failure;
        if (say_) say_(note_);
        return false;
    }
    if (saved_) saved_(configFile_);
    sim::Region next(current_->seed(), draft_);
    refreshConflicts(next);
    const std::vector<sim::EditConflict> kept = conflicts_; // bind() below clears the list; the owner still needs to see it
    if (applied_) applied_(draft_);
    conflicts_ = kept;
    note_ = conflicts_.empty() ? "Applied: region.json now holds these settings" : std::format("Applied. {} of your edits no longer suit the land", conflicts_.size());
    if (say_) say_(note_);
    return true;
}

void GeneratorPanel::paint(Minimap& map, sim::Region& land, Mix& mix, std::array<int, 5>& counts) {
    if (map.complete()) return;
    const int size = land.size();
    map.build(kRowsPerTick, [&land, &counts, size](int x, int y) {
        const sim::Biome biome = land.biomeAt(x * size / kThumbCells, y * size / kThumbCells);
        ++counts[static_cast<std::size_t>(biome)];
        return kColours[static_cast<std::size_t>(biome)];
    });
    if (map.complete()) {
        const int total = kThumbCells * kThumbCells;
        for (std::size_t i = 0; i < 5; ++i) mix.percent[i] = counts[i] * 100 / total;
        mix.valid = true;
    }
}

void GeneratorPanel::update(const luna::engine::Intents& intents) {
    if (!shown_ || current_ == nullptr) return;
    panel_->handle(UiInput::from(intents));
    if (nowMap_) paint(*nowMap_, *current_, nowMix_, nowCounts_);
    if (previewMap_ && previewRegion_) paint(*previewMap_, *previewRegion_, previewMix_, previewCounts_);
}

void GeneratorPanel::draw(UiPainter& painter, luna::engine::Renderer& renderer) const {
    if (!shown_) return;
    const Rect area = bounds();
    painter.fill(area, UiColor::Panel);
    painter.outline(area, UiColor::Border);
    painter.text(area.x + 4, area.y + 4, "Generator settings", UiColor::Gold);
    panel_->draw(painter);

    const int thumbY = area.y + kFirstRow + static_cast<int>(fields_.size()) * kRowPitch + 28;
    const Rect nowBox{area.x + 4, thumbY, kThumbSize, kThumbSize};
    const Rect previewBox{area.x + area.width - 4 - kThumbSize, thumbY, kThumbSize, kThumbSize};
    painter.text(nowBox.x, thumbY - 10, "Now", UiColor::Dim);
    painter.text(previewBox.x, thumbY - 10, "Preview", UiColor::Dim);
    for (const Rect& box : {nowBox, previewBox}) painter.fill({box.x - 1, box.y - 1, box.width + 2, box.height + 2}, UiColor::Border);
    const Rect source{0, 0, kThumbCells, kThumbCells};
    if (nowMap_ && nowMap_->complete()) renderer.drawStyled(nowMap_->texture(renderer), source, nowBox, DrawStyle{});
    else painter.fill(nowBox, UiColor::Dark);
    if (previewMap_ && previewMap_->complete()) renderer.drawStyled(previewMap_->texture(renderer), source, previewBox, DrawStyle{});
    else painter.fill(previewBox, UiColor::Dark);

    int y = thumbY + kThumbSize + 4;
    const auto mixLine = [&](const char* title, const Mix& mix) {
        std::string line = title;
        if (mix.valid) {
            for (const std::size_t i : {2U, 3U, 1U}) line += std::format(" {} {}%", kBiomeShort[i], mix.percent[i]);
        }
        painter.text(area.x + 4, y, line, UiColor::Text);
        y += luna::engine::kLineHeight;
    };
    mixLine("Now:    ", nowMix_);
    mixLine("Preview:", previewMix_);
    if (!note_.empty()) {
        painter.text(area.x + 4, y, note_.substr(0, 36), problems().empty() ? UiColor::Dim : UiColor::Red);
        y += luna::engine::kLineHeight;
    }
    if (!conflicts_.empty()) {
        painter.text(area.x + 4, y, std::format("{} edit(s) no longer fit:", conflicts_.size()), UiColor::Gold);
        y += luna::engine::kLineHeight;
        for (std::size_t i = 0; i < std::min<std::size_t>(2, conflicts_.size()); ++i) {
            painter.text(area.x + 4, y, (conflicts_[i].id + ": " + conflicts_[i].reason).substr(0, 36), UiColor::Dim);
            y += luna::engine::kLineHeight;
        }
    }
}

void GeneratorPanel::drawOverlay(UiPainter& painter) const {
    if (shown_) panel_->drawOverlay(painter);
}

} // namespace odysseus::game
