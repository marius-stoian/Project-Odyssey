#include "game/atlas_cuts.h"

#include "core/text.h"
#include "game/art.h"
#include "game/content_art.h"
#include "game/normal_art.h"
#include "luna/engine/image_io.h"
#include "sim/data_document.h"
#include "sim/json_data.h"

#include <algorithm>
#include <cctype>
#include <format>

namespace odysseus::game {

namespace fs = std::filesystem;

std::vector<std::string> cutSheets(const fs::path& sprites) {
    std::vector<std::string> sheets;
    std::error_code error;
    for (const fs::directory_entry& entry : fs::directory_iterator(sprites, error)) {
        if (entry.is_regular_file() && entry.path().extension() == ".png") sheets.push_back(entry.path().filename().string());
    }
    std::sort(sheets.begin(), sheets.end());
    return sheets;
}

std::vector<std::string> cutTargets(const fs::path& sprites) {
    std::vector<std::string> targets{"character", "tile"};
    if (fs::exists(sprites / "content-cuts.json")) {
        try {
            for (const ContentPage& page : loadContentCuts(sprites / "content-cuts.json").pages) targets.push_back(page.name);
        } catch (const std::exception&) {
            // a broken content-cuts.json is said when a cut is added to it
        }
    }
    return targets;
}

std::string rebuildAtlases(const fs::path& sprites) {
    const CutList cuts = loadCuts(sprites / "cuts.json");
    const Atlas atlas = cutAtlas(cuts, sprites);
    saveAtlas(atlas, sprites / "atlas");
    std::string summary = std::format("{} character frames and {} tiles", atlas.characterCount(), atlas.tileCount());
    if (fs::exists(sprites / "content-cuts.json")) {
        const ContentCuts contentCuts = loadContentCuts(sprites / "content-cuts.json");
        const ContentAtlas content = cutContent(contentCuts, sprites);
        saveContent(content, sprites / "atlas");
        summary += std::format(", {} content items ({} frames)", content.frameCounts.size(), content.frames.size());
    }
    bool hasNormals = false;
    std::error_code error;
    for (const fs::directory_entry& entry : fs::directory_iterator(sprites / "atlas", error)) {
        hasNormals = hasNormals || entry.path().stem().string().ends_with("_n");
    }
    if (hasNormals) writeNormalAtlases(sprites);
    return summary;
}

namespace {

bool nameOk(const std::string& name, bool content) {
    if (name.empty() || name.size() > 48 || name.front() == ' ' || name.back() == ' ') return false;
    return std::all_of(name.begin(), name.end(), [content](unsigned char c) { return std::isalnum(c) != 0 || c == '.' || c == '-' || c == '_' || (content && c == ' '); });
}

} // namespace

std::optional<std::string> addCut(const fs::path& sprites, const CutRequest& request, std::string* summary) {
    const std::vector<std::string> targets = cutTargets(sprites);
    if (std::find(targets.begin(), targets.end(), request.target) == targets.end()) return std::format("\"{}\" is not a place for a cut ({})", request.target, core::joined(targets, ", "));
    const bool content = request.target != "character" && request.target != "tile";
    if (!nameOk(request.name, content)) return "the name is letters, digits, . - _ (and spaces for content), up to 48 characters";
    // The sheet and the rectangle in it.
    if (request.sheet.empty() || request.sheet.find('/') != std::string::npos || request.sheet.find('\\') != std::string::npos) return "choose a sheet of the sprites folder";
    std::string error;
    const auto sheet = luna::engine::loadPng(sprites / request.sheet, error);
    if (!sheet) return request.sheet + " " + error;
    const core::Rect& r = request.rect;
    if (r.width < 4 || r.height < 4) return "the rectangle is too small (at least 4 by 4 pixels)";
    if (r.x < 0 || r.y < 0 || r.x + r.width > sheet->width() || r.y + r.height > sheet->height()) {
        return std::format("the rectangle [{}, {}, {}, {}] is not inside {} ({} by {})", r.x, r.y, r.width, r.height, request.sheet, sheet->width(), sheet->height());
    }
    // The line, patched into the cuts file.
    const fs::path file = sprites / (content ? "content-cuts.json" : "cuts.json");
    const std::optional<std::string> original = core::readTextFile(file);
    if (!original) return file.filename().string() + " cannot be read";
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(file, problem);
    if (!document) return problem;
    const sim::OrderedJson* list = document->find("cuts");
    if (list == nullptr || !list->is_array()) return file.filename().string() + " has no list of cuts";
    for (const sim::OrderedJson& cut : *list) {
        if (cut.is_object() && cut.contains("name") && cut["name"] == request.name) return std::format("{} already has a cut named \"{}\"", file.filename().string(), request.name);
    }
    sim::OrderedJson entry = sim::OrderedJson::object();
    entry["name"] = request.name;
    if (content) {
        entry["page"] = request.target;
        entry["key"] = "flood"; // the sheet's plain background is flooded away from the border; the owner can change it in the file
    } else {
        entry["kind"] = request.target;
    }
    entry["sheet"] = request.sheet;
    entry["rect"] = sim::OrderedJson::array({r.x, r.y, r.width, r.height});
    if (!document->insertElement("cuts", list->size(), std::move(entry), problem)) return problem;
    if (const std::optional<std::string> failed = document->save(1)) return *failed;
    // The atlases again; a cut that breaks them is taken out.
    try {
        const std::string rebuilt = rebuildAtlases(sprites);
        if (summary != nullptr) *summary = rebuilt;
    } catch (const std::exception& broken) {
        core::writeTextFileSafely(file, *original);
        return std::format("the cut was taken out again, the atlas could not be cut: {}", broken.what());
    }
    return std::nullopt;
}

} // namespace odysseus::game
