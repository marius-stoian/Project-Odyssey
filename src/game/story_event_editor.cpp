#include "game/story_event_editor.h"

#include <algorithm>
#include <fstream>
#include <sstream>

namespace odysseus::game {

namespace fs = std::filesystem;
using luna::engine::Button;
using luna::engine::ListBox;
using luna::engine::NumberField;
using luna::engine::Panel;
using luna::engine::Rect;
using luna::engine::TextField;
using luna::engine::UiColor;
using luna::engine::UiInput;
using luna::engine::UiPainter;

namespace {

constexpr int kBar = 18;
constexpr int kListWidth = 120;
constexpr int kRow = 15;

std::string readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

bool writeText(const fs::path& path, const std::string& text) {
    const fs::path temp = fs::path(path).concat(".tmp");
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << text;
        if (!out.good()) return false;
    }
    std::error_code ec;
    fs::rename(temp, path, ec);
    if (ec) {
        fs::remove(temp, ec);
        return false;
    }
    return true;
}

} // namespace

StoryEventEditor::StoryEventEditor(int viewWidth, int viewHeight, Say say) : viewWidth_(viewWidth), viewHeight_(viewHeight), say_(std::move(say)) {
    panel_ = std::make_unique<Panel>(Rect{0, 0, viewWidth, viewHeight});
}

std::string StoryEventEditor::affinityText(const sim::AffinityValues& values) {
    std::string text;
    for (std::size_t a = 0; a < sim::kAffinityCount; ++a) {
        if (values[a] != 0) text += (text.empty() ? "" : " ") + std::string(sim::affinityKey(static_cast<sim::Affinity>(a))) + "=" + std::to_string(values[a]);
    }
    return text;
}

bool StoryEventEditor::affinityFromText(const std::string& text, sim::AffinityValues& values) {
    sim::AffinityValues parsed{};
    std::istringstream in(text);
    for (std::string word; in >> word;) {
        const std::size_t eq = word.find('=');
        if (eq == std::string::npos) return false;
        const auto affinity = sim::affinityFromKey(word.substr(0, eq));
        if (!affinity) return false;
        const std::string number = word.substr(eq + 1);
        char* end = nullptr;
        const long value = std::strtol(number.c_str(), &end, 10);
        if (number.empty() || end == nullptr || *end != '\0' || value < -100 || value > 100) return false;
        parsed[static_cast<std::size_t>(*affinity)] = static_cast<int>(value);
    }
    values = parsed;
    return true;
}

std::vector<std::string> StoryEventEditor::files() const {
    std::vector<std::pair<int, std::string>> found;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(folder_, ec)) {
        const std::string name = entry.path().filename().string();
        if (!entry.is_regular_file() || entry.path().extension() != ".json" || name.size() > 12 && name.compare(name.size() - 12, 12, ".layout.json") == 0) continue;
        std::string problem;
        const auto event = sim::parseStoryEvent(readText(entry.path()), entry.path().string(), problem);
        found.emplace_back(event ? event->order : 1000000, entry.path().stem().string());
    }
    std::sort(found.begin(), found.end());
    std::vector<std::string> ids;
    for (const auto& [order, id] : found) {
        (void)order;
        ids.push_back(id);
    }
    return ids;
}

void StoryEventEditor::show(bool shown) {
    shown_ = shown;
    if (shown_) {
        if (!open_) {
            const std::vector<std::string> ids = files();
            if (!ids.empty()) open(ids.front());
        }
        rebuild_ = true;
    }
}

bool StoryEventEditor::typing() const { return shown_ && panel_ && panel_->typing(); }

void StoryEventEditor::applyHelp(EditorHelp& help) {
    if (panel_) help.apply(*panel_, "event");
}

bool StoryEventEditor::open(const std::string& id) {
    std::error_code ec;
    if (!fs::is_regular_file(fileOf(id), ec)) {
        say_("Story event: no file " + id + ".json");
        return false;
    }
    std::string problem;
    const auto event = sim::parseStoryEvent(readText(fileOf(id)), fileOf(id).string(), problem);
    if (!event) {
        say_("Story event " + id + ": " + problem);
        return false;
    }
    event_ = *event;
    savedText_ = formText();
    open_ = true;
    rebuild_ = true;
    return true;
}

bool StoryEventEditor::createNew(const std::string& id) {
    const bool valid = !id.empty() && id.size() <= 40 && std::all_of(id.begin(), id.end(), [](unsigned char c) { return std::islower(c) != 0 || std::isdigit(c) != 0 || c == '-'; });
    if (!valid) {
        say_("New: an id is lower-case letters, digits and hyphens (up to 40)");
        return false;
    }
    std::error_code ec;
    if (fs::exists(fileOf(id), ec)) {
        say_("New: " + id + " exists already");
        return false;
    }
    event_ = {};
    event_.id = id;
    event_.title = "A new event";
    event_.text = "Something happens.";
    event_.order = static_cast<int>(files().size());
    for (int i = 0; i < 2; ++i) {
        sim::CrossroadsOption option;
        option.text = i == 0 ? "Do something." : "Do nothing.";
        option.note = "{hero} chose.";
        event_.options.push_back(option);
    }
    savedText_.clear(); // not written yet: unsaved by definition
    open_ = true;
    rebuild_ = true;
    say_("New " + id + ".json: not written until you press Save");
    return true;
}

bool StoryEventEditor::save() {
    if (!open_) return false;
    const std::string text = formText();
    std::string problem;
    if (!sim::parseStoryEvent(text, fileOf(event_.id).string(), problem)) {
        say_("Not saved: " + problem);
        return false;
    }
    std::error_code ec;
    fs::create_directories(folder_, ec);
    if (fs::is_regular_file(fileOf(event_.id), ec)) fs::copy_file(fileOf(event_.id), fs::path(fileOf(event_.id)).concat(".bak"), fs::copy_options::overwrite_existing, ec);
    if (!writeText(fileOf(event_.id), text)) {
        say_("Not saved: " + event_.id + ".json cannot be written");
        return false;
    }
    savedText_ = text;
    say_("Saved " + event_.id + ".json");
    if (saved_) saved_();
    return true;
}

void StoryEventEditor::rebuild() {
    rebuild_ = false;
    panel_ = std::make_unique<Panel>(Rect{0, 0, viewWidth_, viewHeight_});
    int x = 2;
    const auto button = [&](const std::string& label, const std::string& hint, auto action) {
        const int width = UiPainter::textWidth(label) + 8;
        panel_->add<Button>(Rect{x, 2, width, kBar - 4}, label, action).hint = hint;
        x += width + 2;
    };
    button("Close", "Back to the map (Esc)", [this] { show(false); });
    button("Save", "Check the event and write its file", [this] { save(); });
    x += 6;
    panel_->add<TextField>(Rect{x, 2, 130, kBar - 4}, "id: ", newName_, 40, [this](const std::string& v) { newName_ = v; });
    x += 134;
    button("New", "Make a new event of this id; it is written when you press Save", [this] { createNew(newName_); });

    const std::vector<std::string> ids = files();
    ListBox& list = panel_->add<ListBox>(Rect{2, kBar + 4, kListWidth, viewHeight_ - kBar - 8}, ids, [this, ids](int index) {
        if (index >= 0 && index < static_cast<int>(ids.size()) && (!open_ || ids[static_cast<std::size_t>(index)] != event_.id)) open(ids[static_cast<std::size_t>(index)]);
    });
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (open_ && ids[i] == event_.id) list.selected = static_cast<int>(i);
    }
    if (!open_) return;

    const int left = kListWidth + 10;
    const int width = viewWidth_ - left - 6;
    int y = kBar + 18;
    const auto text = [&](const std::string& label, const std::string& value, std::size_t max, std::function<void(const std::string&)> set) {
        panel_->add<TextField>(Rect{left, y, width, kRow - 1}, label, value, max, std::move(set));
        y += kRow;
    };
    const auto number = [&](const std::string& label, int value, int low, int high, std::function<void(int)> set, int atX, int w) {
        panel_->add<NumberField>(Rect{atX, y, w, kRow - 1}, label, value, low, high, std::move(set));
    };
    text("title: ", event_.title, 80, [this](const std::string& v) { event_.title = v; });
    text("text: ", event_.text, 400, [this](const std::string& v) { event_.text = v; });
    number("min age: ", event_.minAge, 1, 80, [this](int v) { event_.minAge = v; }, left, 130);
    number("max age: ", event_.maxAge, 1, 80, [this](int v) { event_.maxAge = v; }, left + 140, 130);
    number("order: ", event_.order, 0, 1000, [this](int v) { event_.order = v; }, left + 280, 130);
    y += kRow;
    text("who (elder or friend): ", event_.role, 20, [this](const std::string& v) { event_.role = v; });
    text("trigger (rule language, empty = always): ", event_.trigger, 200, [this](const std::string& v) { event_.trigger = v; });
    affinityNeeded_ = event_.needsAffinity ? std::string(sim::affinityKey(*event_.needsAffinity)) : std::string();
    text("needs affinity (hunter, trade... or empty): ", affinityNeeded_, 20, [this](const std::string& v) {
        affinityNeeded_ = v;
        event_.needsAffinity = sim::affinityFromKey(v);
    });
    number("needs at least: ", event_.needsMinimum, 0, 100, [this](int v) { event_.needsMinimum = v; }, left, 160);
    y += kRow + 4;
    optionAffinity_.assign(event_.options.size(), {});
    for (std::size_t o = 0; o < event_.options.size(); ++o) {
        panel_->add<Button>(Rect{left, y, 8 * 8 + 8, kRow - 1}, "option " + std::to_string(o + 1), [] {});
        y += kRow;
        sim::CrossroadsOption& option = event_.options[o];
        optionAffinity_[o] = affinityText(option.affinity);
        text("  says: ", option.text, 200, [&option](const std::string& v) { option.text = v; });
        text("  affinities (name=number ...): ", optionAffinity_[o], 120, [this, o, &option](const std::string& v) {
            optionAffinity_[o] = v;
            sim::AffinityValues values{};
            if (affinityFromText(v, values)) option.affinity = values;
        });
        number("  opinion: ", option.opinion, -100, 100, [&option](int v) { option.opinion = v; }, left, 160);
        y += kRow;
        text("  trait (brave, kind... or empty): ", option.trait, 20, [&option](const std::string& v) { option.trait = v; });
        text("  chronicle ({hero}, {other}): ", option.note, 200, [&option](const std::string& v) { option.note = v; });
        y += 2;
    }
}

void StoryEventEditor::update(const luna::engine::Intents& intents) {
    if (!shown_) return;
    if (rebuild_ && !typing()) rebuild();
    panel_->handle(UiInput::from(intents));
}

void StoryEventEditor::draw(UiPainter& painter) const {
    if (!shown_) return;
    painter.fill({0, 0, viewWidth_, viewHeight_}, UiColor::Dark);
    panel_->draw(painter);
    const Rect bar{0, viewHeight_ - 14, viewWidth_, 14};
    painter.fill(bar, UiColor::Shade);
    std::string line = open_ ? "Story event: " + event_.id + ".json" + (dirty() ? "  *unsaved" : "") : std::string("Story event: no file");
    painter.text(4, bar.y + 3, line, UiColor::Text);
}

void StoryEventEditor::drawOverlay(UiPainter& painter) const {
    if (shown_) panel_->drawOverlay(painter);
}

} // namespace odysseus::game
