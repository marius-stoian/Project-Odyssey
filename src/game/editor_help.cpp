#include "game/editor_help.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <system_error>
#include <format>
#include <fstream>
#include <iterator>

namespace odysseus::game {

using luna::engine::NumberField;
using luna::engine::TextField;

namespace {

// The 1-based line of a byte position.
int lineOf(const std::string& text, std::size_t position) {
    const std::size_t end = std::min(position, text.size());
    return 1 + static_cast<int>(std::count(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(end), '\n'));
}

// The names `catalog:` may use (the sources of US-302).
bool knownCatalog(const std::string& name) {
    static const std::set<std::string> names = {"npc-classes", "npc-kinds", "partner-types", "interactions", "interaction-fields", "light-kinds", "objects",
                                                "plants", "characters", "items", "building-kinds", "prefabs", "quests", "levels", "tags", "places", "markers"};
    return names.contains(name);
}

// "" when `suggest` is a form the Editor knows, else what is wrong.
std::string suggestProblem(const std::string& suggest) {
    if (suggest.empty() || suggest == "number" || suggest == "none") return {};
    if (suggest.starts_with("values:")) return suggest.size() > 7 ? std::string() : "values: needs at least one value";
    if (suggest.starts_with("files:")) return suggest.find('/') != std::string::npos ? std::string() : "files: needs folder/pattern";
    if (suggest.starts_with("catalog:")) return knownCatalog(suggest.substr(8)) ? std::string() : "\"" + suggest.substr(8) + "\" is not a catalog";
    return "\"" + suggest + "\" is not number, none, values:, files: or catalog:";
}

} // namespace

void EditorHelp::load(const std::filesystem::path& file) {
    entries_.clear();
    problems_.clear();
    const std::string name = file.filename().string();
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        problems_.push_back(std::format("{}: not found; the Editor shows no tooltips", name));
        return;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    nlohmann::json root;
    try {
        root = nlohmann::json::parse(text, nullptr, true, true); // comments allowed, like the other data files
    } catch (const nlohmann::json::parse_error& error) {
        problems_.push_back(std::format("{}:{}: {}", name, lineOf(text, error.byte > 0 ? error.byte - 1 : 0), error.what()));
        return;
    }
    const auto say = [&](const std::string& key, const std::string& message) {
        const std::size_t at = key.empty() ? std::string::npos : text.find("\"" + key + "\"");
        problems_.push_back(std::format("{}:{}: {}", name, at == std::string::npos ? 1 : lineOf(text, at), message));
    };
    if (!root.is_object() || root.value("version", 0) != 1) say("version", "version must be 1");
    if (!root.is_object() || !root.contains("fields") || !root["fields"].is_object()) {
        say("fields", "\"fields\" must be an object of field ids");
        entries_.clear();
        return;
    }
    for (const auto& [id, value] : root["fields"].items()) {
        if (!value.is_object()) {
            say(id, "\"" + id + "\" must be an object with purpose and example");
            continue;
        }
        const auto text_of = [&](const char* key) { return value.contains(key) && value[key].is_string() ? value[key].get<std::string>() : std::string(); };
        Entry entry{text_of("purpose"), text_of("range"), text_of("example"), text_of("suggest"), value.value("list", false)};
        if (entry.purpose.empty()) say(id, "\"" + id + "\" has no purpose");
        if (entry.example.empty()) say(id, "\"" + id + "\" has no example");
        if (entry.suggest.empty()) say(id, "\"" + id + "\" has no suggest (write \"none\" for a field that offers nothing)");
        if (const std::string bad = suggestProblem(entry.suggest); !bad.empty()) say(id, "\"" + id + "\": " + bad);
        entries_[id] = std::move(entry);
    }
    if (!problems_.empty()) entries_.clear(); // all or nothing, like every data file: a mistake means no tooltips, not half of them
}

std::vector<std::string> EditorHelp::reload(const std::filesystem::path& file) {
    EditorHelp fresh;
    fresh.load(file);
    if (!fresh.problems_.empty()) return fresh.problems_; // all or nothing: the entries in use stay
    entries_ = std::move(fresh.entries_);
    problems_.clear();
    return {};
}

const EditorHelp::Entry* EditorHelp::find(const std::string& id) const {
    const auto found = entries_.find(id);
    return found == entries_.end() ? nullptr : &found->second;
}

std::string EditorHelp::fieldId(std::string_view panel, std::string_view label) {
    std::string id(panel);
    id += '.';
    if (label.starts_with("  ")) id += "sub-"; // an indented label is a field of the row above it (a partner type, an option)
    bool dash = false;
    int parentheses = 0; // "who (elder or friend):" is the field "who": the hint in brackets is not part of its name
    for (const char c : label) {
        if (c == '(') ++parentheses;
        if (parentheses > 0) {
            if (c == ')') --parentheses;
            continue;
        }
        const bool word = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        const bool upper = c >= 'A' && c <= 'Z';
        if (word || upper) {
            if (dash && id.back() != '.' && id.back() != '-') id += '-';
            dash = false;
            id += upper ? static_cast<char>(c - 'A' + 'a') : c;
        } else {
            dash = true;
        }
    }
    return id;
}

namespace {

constexpr std::size_t kRecentNumbers = 5; // the last numbers typed in a field that its list offers

std::vector<std::string> splitOn(const std::string& text, char separator) {
    std::vector<std::string> parts;
    std::size_t from = 0;
    while (from <= text.size()) {
        const std::size_t at = text.find(separator, from);
        parts.push_back(text.substr(from, at == std::string::npos ? std::string::npos : at - from));
        if (at == std::string::npos) break;
        from = at + 1;
    }
    return parts;
}

// Does a file name match a pattern of the forms `*`, `*.ext` or a whole name?
bool matchesGlob(const std::string& name, const std::string& glob) {
    if (glob == "*") return true;
    if (glob.size() > 1 && glob.front() == '*') return name.size() >= glob.size() - 1 && name.compare(name.size() - (glob.size() - 1), glob.size() - 1, glob, 1, std::string::npos) == 0;
    return name == glob;
}

} // namespace

std::vector<std::string> EditorHelp::suggestionsFor(const std::string& id, int minimum, int maximum) const {
    const Entry* entry = find(id);
    if (entry == nullptr) return {};
    const std::string& source = entry->suggest;
    std::vector<std::string> out;
    const auto add = [&out](const std::string& value) {
        if (!value.empty() && std::find(out.begin(), out.end(), value) == out.end()) out.push_back(value);
    };
    if (source == "number") { // the default of what is being edited, the smallest, the largest, then what was typed here lately
        if (sources_.numberDefault) {
            if (const std::optional<int> value = sources_.numberDefault(id)) add(std::to_string(*value));
        }
        if (minimum != maximum) {
            add(std::to_string(minimum));
            add(std::to_string(maximum));
        }
        if (const auto found = recent_.find(id); found != recent_.end()) {
            for (const int value : found->second) add(std::to_string(value));
        }
    } else if (source.starts_with("values:")) {
        for (const std::string& value : splitOn(source.substr(7), '|')) add(value);
    } else if (source.starts_with("files:")) { // files:<folder>/<glob>, under the data folder; the names with their extension
        const std::string where = source.substr(6);
        const std::size_t slash = where.rfind('/');
        if (slash != std::string::npos && !sources_.dataFolder.empty()) {
            std::error_code error;
            std::set<std::string> names;
            for (const auto& file : std::filesystem::directory_iterator(sources_.dataFolder / where.substr(0, slash), error)) {
                const std::string name = file.path().filename().string();
                if (file.is_regular_file() && matchesGlob(name, where.substr(slash + 1))) names.insert(name);
            }
            for (const std::string& name : names) add(name);
        }
    } else if (source.starts_with("catalog:")) {
        if (sources_.catalog) {
            for (const std::string& name : sources_.catalog(source.substr(8))) add(name);
        }
    }
    return out;
}

void EditorHelp::apply(luna::engine::Panel& panel, std::string_view prefix) {
    const auto tipText = [](const Entry& entry, const std::string& range) {
        std::string text = entry.purpose;
        if (!range.empty()) text += "\nRange: " + range;
        if (!entry.example.empty()) text += "\nExample: " + entry.example;
        return text;
    };
    for (const auto& child : panel.children()) {
        TextField* text = dynamic_cast<TextField*>(child.get());
        NumberField* number = text == nullptr ? dynamic_cast<NumberField*>(child.get()) : nullptr;
        if (text == nullptr && number == nullptr) continue;
        std::string& helpId = text != nullptr ? text->helpId : number->helpId;
        luna::engine::FieldHint& tip = text != nullptr ? text->tip : number->tip;
        const std::string id = fieldId(prefix, text != nullptr ? text->label : number->label);
        const bool fresh = helpId != id; // a widget this call has not met: it is wired once, not every tick
        helpId = id;
        asked_.insert(id);
        const Entry* entry = find(id);
        const std::string range = number != nullptr ? std::format("{} to {}", number->minimum, number->maximum) : std::string();
        tip.text = entry != nullptr ? tipText(*entry, entry->range.empty() ? range : entry->range) : std::string();
        if (number != nullptr && fresh) { // what is typed here is offered again: the last few numbers
            number->onChange = [this, id, inner = std::move(number->onChange)](int value) {
                std::vector<int>& recent = recent_[id];
                recent.erase(std::remove(recent.begin(), recent.end(), value), recent.end());
                recent.insert(recent.begin(), value);
                if (recent.size() > kRecentNumbers) recent.resize(kRecentNumbers);
                if (inner) inner(value);
            };
        }
        const bool offers = entry != nullptr && entry->suggest != "none";
        if (!offers) continue;
        const bool wired = text != nullptr ? static_cast<bool>(text->suggest) : static_cast<bool>(number->suggest);
        if (!fresh && wired) continue;
        wired_.insert(id);
        if (text != nullptr) {
            text->listItems = entry->list;
            text->suggest = [this, id](const std::string&) { return suggestionsFor(id); };
        } else {
            number->suggest = [this, id, number](const std::string&) { return suggestionsFor(id, number->minimum, number->maximum); };
        }
    }
}

std::vector<std::string> EditorHelp::missing() const {
    std::vector<std::string> out;
    for (const std::string& id : asked_) {
        if (find(id) == nullptr) out.push_back(id);
    }
    return out;
}

} // namespace odysseus::game
