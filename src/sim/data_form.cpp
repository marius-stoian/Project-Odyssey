#include "sim/data_form.h"

#include "sim/json_text.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <map>

namespace odysseus::sim::form {

namespace {

// The members of the top of the document that hold the entries of a catalog: "weapons[].name" in `provides` makes "weapons" such a list.
std::vector<std::string> listKeysOf(const schema::Schema& schema) {
    std::vector<std::string> keys;
    for (const schema::Provide& provide : schema.provides) {
        const std::size_t brackets = provide.at.find("[]");
        if (brackets == std::string::npos || provide.at.find('.') < brackets) continue; // only a list at the top of the file
        const std::string key = provide.at.substr(0, brackets);
        if (!key.empty() && std::find(keys.begin(), keys.end(), key) == keys.end()) keys.push_back(key);
    }
    return keys;
}

bool isNoteKey(const std::string& key) { return key == "note" || key == "comment" || key.starts_with("_") || key.starts_with("//"); }

std::string trimmed(const std::string& text) {
    const std::size_t first = text.find_first_not_of(" \t");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t") - first + 1);
}

std::string numberRangeText(const schema::Node& node) {
    if (node.minimum && node.maximum) return std::format("between {} and {}", node.minimum.value(), node.maximum.value());
    if (node.minimum) return std::format("at least {}", node.minimum.value());
    return std::format("at most {}", node.maximum.value());
}

class Builder {
public:
    Builder(const OrderedJson& document, const schema::Schema& schema, const std::set<std::string>& collapsed, const std::vector<schema::Issue>& issues)
        : document_(document), schema_(schema), collapsed_(collapsed) {
        for (const schema::Issue& issue : issues) errors_.emplace(issue.path, issue.message); // the first one at a path stays
    }

    std::vector<FormRow> rows(const std::string& entryPath) {
        const std::optional<DocPath> parsed = parsePath(entryPath);
        const OrderedJson* entry = &document_;
        if (parsed) {
            for (const PathSegment& segment : *parsed) {
                if (segment.index) entry = entry->is_array() && segment.position < entry->size() ? &(*entry)[segment.position] : nullptr;
                else entry = entry != nullptr && entry->is_object() && entry->contains(segment.key) ? &entry->at(segment.key) : nullptr;
                if (entry == nullptr) return {};
            }
        }
        std::set<std::string> skip;
        if (entryPath.empty()) {
            for (const std::string& key : listKeysOf(schema_)) skip.insert(key); // the file's own entry leaves out the lists that have an entry each
        }
        // The entry itself: an object shows its members, an element of a list its fields; anything else shows as one row. The help of a field is named by the schema
        // and the path without list positions, so the entry "weapons[2]" gives its fields the ids "data.weapons.weapons.damage".
        std::string help;
        if (parsed) {
            for (const PathSegment& segment : *parsed) {
                if (!segment.index) help += "." + segment.key;
            }
        }
        const schema::Node* node = entryPath.empty() ? schema_.root.get() : nodeOfEntry(entryPath);
        if (entry->is_object()) addMembers(*entry, node, entryPath, 0, help, skip);
        else addValue(entry, node, entryPath, labelOfElement(*entry, 0), 0, help, false);
        return std::move(rows_);
    }

private:
    // The schema of an entry such as "weapons[2]": the items schema of the list "weapons".
    const schema::Node* nodeOfEntry(const std::string& entryPath) const {
        const std::optional<DocPath> parsed = parsePath(entryPath);
        const schema::Node* node = schema_.root.get();
        if (!parsed || node == nullptr) return nullptr;
        for (const PathSegment& segment : *parsed) {
            if (node == nullptr) return nullptr;
            if (segment.index) node = node->items.get();
            else node = node->property(segment.key) != nullptr ? node->property(segment.key) : node->additional.get();
        }
        return node;
    }

    std::string errorAt(const std::string& path) const {
        const auto found = errors_.find(path);
        return found == errors_.end() ? std::string() : found->second;
    }

    bool collapsedAt(const std::string& path) const { return collapsed_.count(path) != 0; }

    FormRow base(RowKind kind, const schema::Node* node, const std::string& path, const std::string& label, int depth, const std::string& helpPath) const {
        FormRow row;
        row.kind = kind;
        row.node = node;
        row.path = path;
        row.label = label;
        row.depth = depth;
        row.helpId = "data." + schema_.name + helpPath;
        row.error = errorAt(path);
        return row;
    }

    static RowKind scalarKind(const schema::Node* node, const OrderedJson* value) {
        if (node != nullptr) {
            switch (node->type) {
            case schema::Type::Integer: return RowKind::Whole;
            case schema::Type::Number: return RowKind::Decimal;
            case schema::Type::Boolean: return RowKind::Flag;
            case schema::Type::String:
                if (!node->choices.empty()) return RowKind::Choice;
                if (node->ref.starts_with("catalog:")) return RowKind::Reference;
                return RowKind::Text;
            default: break;
            }
        }
        if (value != nullptr) {
            if (value->is_boolean()) return RowKind::Flag;
            if (value->is_number_integer()) return RowKind::Whole;
            if (value->is_number()) return RowKind::Decimal;
            if (value->is_string()) return RowKind::Text;
        }
        return RowKind::Raw;
    }

    // The named fields of an object in the schema's order (the ones the file leaves out too), then the members the schema does not know.
    void addMembers(const OrderedJson& object, const schema::Node* node, const std::string& path, int depth, const std::string& helpPath, const std::set<std::string>& skip) {
        if (node != nullptr) {
            for (const schema::Property& property : node->properties) {
                if (skip.count(property.name) != 0) continue;
                const bool present = object.contains(property.name);
                addValue(present ? &object.at(property.name) : nullptr, property.node.get(), JsonLines::childPath(path, property.name), property.name, depth, helpPath + "." + property.name,
                         node->isRequired(property.name));
            }
        }
        for (const auto& item : object.items()) {
            if (skip.count(item.key()) != 0) continue;
            if (node != nullptr && node->property(item.key()) != nullptr) continue;
            const std::string childPath = JsonLines::childPath(path, item.key());
            FormRow row = base(isNoteKey(item.key()) && item.value().is_string() ? RowKind::Text : RowKind::Raw, nullptr, childPath, item.key(), depth, helpPath + "." + item.key());
            row.value = valueText(item.value());
            row.canRemove = true;
            rows_.push_back(std::move(row));
        }
    }

    void addValue(const OrderedJson* value, const schema::Node* node, const std::string& path, const std::string& label, int depth, const std::string& helpPath, bool required) {
        const bool isObject = value != nullptr ? value->is_object() : (node != nullptr && node->type == schema::Type::Object);
        const bool isArray = value != nullptr ? value->is_array() : (node != nullptr && node->type == schema::Type::Array);
        if (isObject) {
            addObject(value, node, path, label, depth, helpPath, required);
        } else if (isArray) {
            addArray(value, node, path, label, depth, helpPath, required);
        } else {
            FormRow row = base(scalarKind(node, value), node, path, label, depth, helpPath);
            row.present = value != nullptr;
            row.required = required;
            row.canRemove = value != nullptr && !required;
            if (value != nullptr) row.value = valueText(*value);
            else if (node != nullptr && !node->defaultText.empty()) row.value = valueText(OrderedJson::parse(node->defaultText, nullptr, false));
            if (row.kind == RowKind::Choice && node != nullptr) {
                for (const nlohmann::json& choice : node->choices) row.choices.push_back(choice.is_string() ? choice.get<std::string>() : choice.dump());
            }
            if (row.kind == RowKind::Reference) row.catalog = node->ref.substr(8);
            if (value == nullptr && required && row.error.empty()) row.error = "is missing";
            rows_.push_back(std::move(row));
        }
    }

    void addObject(const OrderedJson* value, const schema::Node* node, const std::string& path, const std::string& label, int depth, const std::string& helpPath, bool required) {
        const bool isMap = node != nullptr && node->additional && node->properties.empty();
        FormRow heading = base(isMap ? RowKind::MapHeader : RowKind::Heading, node, path, label, depth, helpPath);
        heading.present = value != nullptr;
        heading.required = required;
        heading.canAdd = isMap || value == nullptr;
        heading.canRemove = value != nullptr && !required;
        heading.collapsed = value == nullptr || collapsedAt(path);
        rows_.push_back(heading);
        if (value == nullptr || heading.collapsed) return;
        if (!isMap) {
            addMembers(*value, node, path, depth + 1, helpPath, {});
            return;
        }
        const schema::Node* item = node->additional.get();
        const bool objectItems = item != nullptr && item->type == schema::Type::Object;
        for (const auto& member : value->items()) {
            const std::string memberPath = JsonLines::childPath(path, member.key());
            if (objectItems || member.value().is_object() || member.value().is_array()) {
                FormRow entry = base(RowKind::ItemHeader, item, memberPath, member.key(), depth + 1, helpPath + ".*");
                entry.canRemove = true; // the keys of a map keep the order of the file: no arrows
                entry.collapsed = collapsedAt(memberPath);
                rows_.push_back(entry);
                if (!entry.collapsed) {
                    if (member.value().is_object()) addMembers(member.value(), item, memberPath, depth + 2, helpPath + ".*", {});
                    else addValue(&member.value(), item, memberPath, "value", depth + 2, helpPath + ".*", false);
                }
            } else {
                FormRow row = base(scalarKind(item, &member.value()), item, memberPath, member.key(), depth + 1, helpPath + ".*");
                row.value = valueText(member.value());
                row.canRemove = true;
                if (row.kind == RowKind::Choice && item != nullptr) {
                    for (const nlohmann::json& choice : item->choices) row.choices.push_back(choice.is_string() ? choice.get<std::string>() : choice.dump());
                }
                if (row.kind == RowKind::Reference && item != nullptr) row.catalog = item->ref.substr(8);
                rows_.push_back(std::move(row));
            }
        }
    }

    void addArray(const OrderedJson* value, const schema::Node* node, const std::string& path, const std::string& label, int depth, const std::string& helpPath, bool required) {
        const bool present = value != nullptr && !value->empty();
        const bool allObjects = present && std::all_of(value->begin(), value->end(), [](const OrderedJson& element) { return element.is_object(); });
        const bool anyStructured = present && std::any_of(value->begin(), value->end(), [](const OrderedJson& element) { return element.is_structured(); });
        const bool anyComma = present && std::any_of(value->begin(), value->end(), [](const OrderedJson& element) { return element.is_string() && element.get<std::string>().find(',') != std::string::npos; });
        const bool objects = present ? allObjects : (node != nullptr && node->items && node->items->type == schema::Type::Object);
        if (!objects && !anyStructured && !anyComma) {
            // Words and numbers: one row of comma separated text.
            FormRow row = base(RowKind::Words, node, path, label, depth, helpPath);
            row.present = value != nullptr;
            row.required = required;
            row.canRemove = value != nullptr && !required;
            if (value != nullptr) row.value = valueText(*value);
            if (node != nullptr && node->items && node->items->ref.starts_with("catalog:")) row.catalog = node->items->ref.substr(8);
            rows_.push_back(std::move(row));
            return;
        }
        FormRow heading = base(RowKind::ListHeader, node, path, label, depth, helpPath);
        heading.present = value != nullptr;
        heading.required = required;
        heading.canAdd = true;
        heading.collapsed = collapsedAt(path);
        rows_.push_back(heading);
        if (value == nullptr || heading.collapsed) return;
        if (!objects) { // sentences with commas in them, or a mix of texts and objects: one row for each element, so no element is cut at a comma
            for (std::size_t i = 0; i < value->size(); ++i) {
                const OrderedJson& element = (*value)[i];
                const schema::Node* itemNode = node != nullptr ? node->items.get() : nullptr;
                const bool structured = element.is_structured();
                FormRow row = base(structured ? RowKind::Raw : scalarKind(itemNode, &element), structured ? nullptr : itemNode, JsonLines::indexPath(path, i), "#" + std::to_string(i + 1), depth + 1, helpPath);
                row.value = structured ? element.dump() : valueText(element);
                row.canRemove = true;
                row.canMoveUp = i > 0;
                row.canMoveDown = i + 1 < value->size();
                rows_.push_back(std::move(row));
            }
            return;
        }
        for (std::size_t i = 0; i < value->size(); ++i) {
            const std::string elementPath = JsonLines::indexPath(path, i);
            FormRow entry = base(RowKind::ItemHeader, node != nullptr ? node->items.get() : nullptr, elementPath, labelOfElement((*value)[i], i), depth + 1, helpPath);
            entry.canRemove = true;
            entry.canMoveUp = i > 0;
            entry.canMoveDown = i + 1 < value->size();
            entry.collapsed = collapsedAt(elementPath);
            rows_.push_back(entry);
            if (entry.collapsed) continue;
            if ((*value)[i].is_object()) addMembers((*value)[i], node != nullptr ? node->items.get() : nullptr, elementPath, depth + 2, helpPath, {});
            else addValue(&(*value)[i], node != nullptr ? node->items.get() : nullptr, elementPath, "value", depth + 2, helpPath, false);
        }
    }

    const OrderedJson& document_;
    const schema::Schema& schema_;
    const std::set<std::string>& collapsed_;
    std::map<std::string, std::string> errors_;
    std::vector<FormRow> rows_;
};

} // namespace

std::string labelOfElement(const OrderedJson& element, std::size_t index) {
    if (element.is_string()) return element.get<std::string>();
    if (element.is_object()) {
        for (const char* key : {"name", "id", "label", "title", "kind"}) {
            if (element.contains(key) && element.at(key).is_string() && !element.at(key).get<std::string>().empty()) return element.at(key).get<std::string>();
        }
    }
    return "#" + std::to_string(index + 1);
}

std::vector<Entry> entriesOf(const OrderedJson& document, const schema::Schema& schema) {
    std::vector<Entry> entries;
    const std::vector<std::string> lists = listKeysOf(schema);
    entries.push_back({"(file)", std::string(), std::string(), false});
    if (!document.is_object() || lists.empty()) return entries;
    for (const auto& item : document.items()) {
        if (std::find(lists.begin(), lists.end(), item.key()) == lists.end() || !item.value().is_array()) continue;
        for (std::size_t i = 0; i < item.value().size(); ++i) entries.push_back({labelOfElement(item.value()[i], i), JsonLines::indexPath(JsonLines::childPath("", item.key()), i), item.key(), true});
    }
    return entries;
}

std::vector<FormRow> buildRows(const OrderedJson& document, const schema::Schema& schema, const std::string& entryPath, const std::set<std::string>& collapsed,
                               const std::vector<schema::Issue>& issues) {
    return Builder(document, schema, collapsed, issues).rows(entryPath);
}

std::string valueText(const OrderedJson& value) {
    if (value.is_string()) return value.get<std::string>();
    if (value.is_array()) {
        std::string out;
        for (const OrderedJson& element : value) out += (out.empty() ? "" : ", ") + valueText(element);
        return out;
    }
    return value.dump();
}

bool applyText(DataDocument& document, const FormRow& row, const std::string& text, std::string& problem) {
    const std::string typed = trimmed(text);
    const schema::Node* node = row.node;
    OrderedJson value;
    switch (row.kind) {
    case RowKind::Whole: {
        if (typed.empty()) {
            if (!row.present) return true; // nothing typed into a field the file leaves out
            problem = "needs a whole number";
            return false;
        }
        long long number = 0;
        const auto [end, error] = std::from_chars(typed.data(), typed.data() + typed.size(), number);
        if (error != std::errc() || end != typed.data() + typed.size()) {
            problem = "must be a whole number";
            return false;
        }
        if (node != nullptr && ((node->minimum && number < *node->minimum) || (node->maximum && number > *node->maximum))) {
            problem = "must be " + numberRangeText(*node) + " (is " + typed + ")";
            return false;
        }
        value = number;
        break;
    }
    case RowKind::Decimal: {
        if (typed.empty()) {
            if (!row.present) return true;
            problem = "needs a number";
            return false;
        }
        double number = 0.0;
        const auto [end, error] = std::from_chars(typed.data(), typed.data() + typed.size(), number);
        if (error != std::errc() || end != typed.data() + typed.size()) {
            problem = "must be a number";
            return false;
        }
        if (node != nullptr && ((node->minimum && number < *node->minimum) || (node->maximum && number > *node->maximum))) {
            problem = "must be " + numberRangeText(*node) + " (is " + typed + ")";
            return false;
        }
        const bool wholeText = typed.find_first_of(".eE") == std::string::npos;
        if (row.present && valueText(OrderedJson(number)) == row.value) return true; // the same number: the file keeps the way it was written
        if (wholeText && std::floor(number) == number && std::abs(number) < 1e15) value = static_cast<long long>(number);
        else value = number;
        break;
    }
    case RowKind::Flag: {
        if (typed == "true" || typed == "yes" || typed == "1") value = true;
        else if (typed == "false" || typed == "no" || typed == "0") value = false;
        else {
            problem = "must be true or false";
            return false;
        }
        break;
    }
    case RowKind::Choice: {
        if (!row.choices.empty() && std::find(row.choices.begin(), row.choices.end(), typed) == row.choices.end()) {
            std::string allowed;
            for (const std::string& choice : row.choices) allowed += (allowed.empty() ? "" : ", ") + choice;
            problem = "must be one of: " + allowed;
            return false;
        }
        value = typed;
        break;
    }
    case RowKind::Text:
    case RowKind::Reference: {
        if (!row.present && typed.empty()) return true;
        if (node != nullptr && node->maxLength && static_cast<int>(text.size()) > *node->maxLength) {
            problem = std::format("is too long: at most {} letters", *node->maxLength);
            return false;
        }
        value = row.kind == RowKind::Text ? text : typed; // a text keeps its spaces; a name does not need them
        break;
    }
    case RowKind::Words: {
        OrderedJson list = OrderedJson::array();
        const bool numbers = node != nullptr && node->items && (node->items->type == schema::Type::Integer || node->items->type == schema::Type::Number);
        std::size_t at = 0;
        while (at <= typed.size() && !typed.empty()) {
            const std::size_t comma = typed.find(',', at);
            const std::string item = trimmed(typed.substr(at, comma == std::string::npos ? std::string::npos : comma - at));
            if (!item.empty()) {
                if (numbers) {
                    double number = 0.0;
                    const auto [end, error] = std::from_chars(item.data(), item.data() + item.size(), number);
                    if (error != std::errc() || end != item.data() + item.size()) {
                        problem = "\"" + item + "\" is not a number";
                        return false;
                    }
                    list.push_back(item.find_first_of(".eE") == std::string::npos ? OrderedJson(static_cast<long long>(number)) : OrderedJson(number));
                } else {
                    list.push_back(item);
                }
            }
            if (comma == std::string::npos) break;
            at = comma + 1;
        }
        value = std::move(list);
        break;
    }
    case RowKind::Raw: {
        try {
            value = OrderedJson::parse(typed, nullptr, true, true);
        } catch (const OrderedJson::parse_error& error) {
            problem = std::string("is not JSON: ") + error.what();
            return false;
        }
        break;
    }
    default:
        problem = "this row holds no value of its own";
        return false;
    }
    return document.set(row.path, std::move(value), problem);
}

OrderedJson defaultValue(const schema::Node& node) {
    if (!node.defaultText.empty()) {
        const OrderedJson parsed = OrderedJson::parse(node.defaultText, nullptr, false);
        if (!parsed.is_discarded()) return parsed;
    }
    switch (node.type) {
    case schema::Type::Integer: {
        double number = 0.0;
        if (node.minimum && number < *node.minimum) number = *node.minimum;
        if (node.maximum && number > *node.maximum) number = *node.maximum;
        return static_cast<long long>(number);
    }
    case schema::Type::Number: {
        double number = 0.0;
        if (node.minimum && number < *node.minimum) number = *node.minimum;
        if (node.maximum && number > *node.maximum) number = *node.maximum;
        return number;
    }
    case schema::Type::Boolean: return false;
    case schema::Type::Array: {
        OrderedJson list = OrderedJson::array();
        if (node.items) {
            for (int i = 0; i < node.minItems.value_or(0); ++i) list.push_back(defaultValue(*node.items));
        }
        return list;
    }
    case schema::Type::Object: {
        OrderedJson object = OrderedJson::object();
        for (const schema::Property& property : node.properties) {
            if (node.isRequired(property.name)) object[property.name] = defaultValue(*property.node);
        }
        return object;
    }
    case schema::Type::String:
        if (!node.choices.empty() && node.choices.front().is_string()) return node.choices.front().get<std::string>();
        return std::string();
    case schema::Type::Any: break;
    }
    return std::string();
}

std::string uniqueName(const std::string& base, const std::set<std::string>& taken) {
    if (taken.count(base) == 0) return base;
    for (int n = 2;; ++n) {
        const std::string candidate = base + "-" + std::to_string(n);
        if (taken.count(candidate) == 0) return candidate;
    }
}

} // namespace odysseus::sim::form
