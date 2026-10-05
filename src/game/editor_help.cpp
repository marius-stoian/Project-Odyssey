#include "game/editor_help.h"

#include <nlohmann/json.hpp>

#include <algorithm>
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
        if (const std::string bad = suggestProblem(entry.suggest); !bad.empty()) say(id, "\"" + id + "\": " + bad);
        entries_[id] = std::move(entry);
    }
    if (!problems_.empty()) entries_.clear(); // all or nothing, like every data file: a mistake means no tooltips, not half of them
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

void EditorHelp::apply(luna::engine::Panel& panel, std::string_view prefix) {
    const auto tipText = [](const Entry& entry, const std::string& range) {
        std::string text = entry.purpose;
        if (!range.empty()) text += "\nRange: " + range;
        if (!entry.example.empty()) text += "\nExample: " + entry.example;
        return text;
    };
    for (const auto& child : panel.children()) {
        std::string* helpId = nullptr;
        luna::engine::FieldHint* tip = nullptr;
        std::string label;
        std::string range;
        if (auto* text = dynamic_cast<TextField*>(child.get())) {
            helpId = &text->helpId;
            tip = &text->tip;
            label = text->label;
        } else if (auto* number = dynamic_cast<NumberField*>(child.get())) {
            helpId = &number->helpId;
            tip = &number->tip;
            label = number->label;
            range = std::format("{} to {}", number->minimum, number->maximum);
        } else {
            continue;
        }
        *helpId = fieldId(prefix, label);
        asked_.insert(*helpId);
        const Entry* entry = find(*helpId);
        tip->text = entry != nullptr ? tipText(*entry, entry->range.empty() ? range : entry->range) : std::string();
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
