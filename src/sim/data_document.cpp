#include "sim/data_document.h"

#include "core/text.h"

#include <algorithm>
#include <charconv>
#include <format>

namespace odysseus::sim {

// ---- Paths

std::optional<DocPath> parsePath(const std::string& text) {
    DocPath path;
    std::size_t i = 0;
    const auto key = [&](bool bare) -> std::optional<std::string> {
        std::string name;
        if (bare) {
            while (i < text.size() && text[i] != '.' && text[i] != '[') name += text[i++];
            if (name.empty()) return std::nullopt;
            return name;
        }
        // ["a.b"]: a quoted key, \" and \\ escaped
        ++i; // the quote
        while (i < text.size() && text[i] != '"') {
            if (text[i] == '\\' && i + 1 < text.size()) ++i;
            name += text[i++];
        }
        if (i >= text.size()) return std::nullopt;
        ++i; // the closing quote
        return name;
    };
    while (i < text.size()) {
        if (text[i] == '.') {
            if (path.empty()) return std::nullopt;
            ++i;
            const std::optional<std::string> name = key(true);
            if (!name) return std::nullopt;
            path.push_back({false, 0, *name});
        } else if (text[i] == '[') {
            ++i;
            if (i < text.size() && text[i] == '"') {
                const std::optional<std::string> name = key(false);
                if (!name || i >= text.size() || text[i] != ']') return std::nullopt;
                ++i;
                path.push_back({false, 0, *name});
            } else {
                std::size_t position = 0;
                const auto [end, error] = std::from_chars(text.data() + i, text.data() + text.size(), position);
                if (error != std::errc() || end == text.data() + i || end >= text.data() + text.size() || *end != ']') return std::nullopt;
                i = static_cast<std::size_t>(end - text.data()) + 1;
                path.push_back({true, position, {}});
            }
        } else {
            if (!path.empty()) return std::nullopt;
            const std::optional<std::string> name = key(true);
            if (!name) return std::nullopt;
            path.push_back({false, 0, *name});
        }
    }
    return path;
}

std::string formatPath(const DocPath& path) {
    std::string out;
    for (const PathSegment& segment : path) {
        if (segment.index) {
            out += std::format("[{}]", segment.position);
        } else if (segment.key.find_first_of(".[]\"") != std::string::npos) {
            std::string quoted;
            for (const char c : segment.key) quoted += (c == '"' || c == '\\' ? "\\" : "") + std::string(1, c);
            out += "[\"" + quoted + "\"]";
        } else {
            out += (out.empty() ? "" : ".") + segment.key;
        }
    }
    return out;
}

namespace {

OrderedJson* resolve(OrderedJson& root, const DocPath& path) {
    OrderedJson* at = &root;
    for (const PathSegment& segment : path) {
        if (segment.index) {
            if (!at->is_array() || segment.position >= at->size()) return nullptr;
            at = &(*at)[segment.position];
        } else {
            if (!at->is_object() || !at->contains(segment.key)) return nullptr;
            at = &at->at(segment.key);
        }
    }
    return at;
}

} // namespace

// ---- The document

std::optional<DataDocument> DataDocument::fromText(std::string text, std::filesystem::path file, std::string& problem) {
    DataDocument document;
    try {
        document.document_ = OrderedJson::parse(text, nullptr, true, true); // comments allowed, like every data file
    } catch (const OrderedJson::parse_error& error) {
        problem = std::format("{}: not valid JSON: {}", file.generic_string(), error.what());
        return std::nullopt;
    }
    document.file_ = std::move(file);
    document.original_ = std::move(text);
    document.saved_ = document.document_;
    return document;
}

std::optional<DataDocument> DataDocument::open(const std::filesystem::path& file, std::string& problem) {
    std::optional<std::string> text = core::readTextFile(file);
    if (!text) {
        problem = file.generic_string() + ": cannot be opened";
        return std::nullopt;
    }
    return fromText(std::move(*text), file, problem);
}

const OrderedJson* DataDocument::find(const std::string& path) const {
    const std::optional<DocPath> parsed = parsePath(path);
    if (!parsed) return nullptr;
    const OrderedJson* at = &document_;
    for (const PathSegment& segment : *parsed) {
        if (segment.index) {
            if (!at->is_array() || segment.position >= at->size()) return nullptr;
            at = &(*at)[segment.position];
        } else {
            if (!at->is_object() || !at->contains(segment.key)) return nullptr;
            at = &at->at(segment.key);
        }
    }
    return at;
}

void DataDocument::commit(OrderedJson next, std::string what) {
    undo_.push_back(std::move(document_));
    if (undo_.size() > kUndoSteps) undo_.erase(undo_.begin());
    redo_.clear();
    document_ = std::move(next);
    lastEdit_ = std::move(what);
    ++version_;
}

bool DataDocument::set(const std::string& path, OrderedJson value, std::string& problem) {
    const std::optional<DocPath> parsed = parsePath(path);
    if (!parsed) {
        problem = "\"" + path + "\" is not a path";
        return false;
    }
    OrderedJson next = document_;
    if (parsed->empty()) {
        commit(std::move(value), "set the file");
        return true;
    }
    DocPath parentPath = *parsed;
    const PathSegment last = parentPath.back();
    parentPath.pop_back();
    OrderedJson* parent = resolve(next, parentPath);
    if (parent == nullptr) {
        problem = "nothing at " + formatPath(parentPath);
        return false;
    }
    if (last.index) {
        if (!parent->is_array() || last.position >= parent->size()) {
            problem = "no element " + formatPath(*parsed);
            return false;
        }
        if (sameJson((*parent)[last.position], value)) return true; // nothing changes: no step of undo
        (*parent)[last.position] = std::move(value);
    } else {
        if (!parent->is_object()) {
            problem = formatPath(parentPath) + " is not an object";
            return false;
        }
        if (parent->contains(last.key) && sameJson(parent->at(last.key), value)) return true;
        (*parent)[last.key] = std::move(value); // a new key goes to the end
    }
    commit(std::move(next), "set " + path);
    return true;
}

bool DataDocument::insertElement(const std::string& listPath, std::size_t index, OrderedJson value, std::string& problem) {
    const std::optional<DocPath> parsed = parsePath(listPath);
    OrderedJson next = document_;
    OrderedJson* list = parsed ? resolve(next, *parsed) : nullptr;
    if (list == nullptr || !list->is_array()) {
        problem = "\"" + listPath + "\" is not a list";
        return false;
    }
    if (index > list->size()) {
        problem = std::format("{} has {} entries: no place {}", listPath, list->size(), index);
        return false;
    }
    list->insert(list->begin() + static_cast<std::ptrdiff_t>(index), std::move(value));
    commit(std::move(next), std::format("add to {}", listPath));
    return true;
}

bool DataDocument::removeElement(const std::string& listPath, std::size_t index, std::string& problem) {
    const std::optional<DocPath> parsed = parsePath(listPath);
    OrderedJson next = document_;
    OrderedJson* list = parsed ? resolve(next, *parsed) : nullptr;
    if (list == nullptr || !list->is_array() || index >= list->size()) {
        problem = std::format("{} has no entry {}", listPath, index);
        return false;
    }
    list->erase(list->begin() + static_cast<std::ptrdiff_t>(index));
    commit(std::move(next), std::format("remove {}[{}]", listPath, index));
    return true;
}

bool DataDocument::moveElement(const std::string& listPath, std::size_t from, std::size_t to, std::string& problem) {
    const std::optional<DocPath> parsed = parsePath(listPath);
    OrderedJson next = document_;
    OrderedJson* list = parsed ? resolve(next, *parsed) : nullptr;
    if (list == nullptr || !list->is_array() || from >= list->size() || to >= list->size()) {
        problem = std::format("{} cannot move entry {} to {}", listPath, from, to);
        return false;
    }
    if (from == to) return true;
    OrderedJson moved = std::move((*list)[from]);
    list->erase(list->begin() + static_cast<std::ptrdiff_t>(from));
    list->insert(list->begin() + static_cast<std::ptrdiff_t>(to), std::move(moved));
    commit(std::move(next), std::format("move {}[{}]", listPath, from));
    return true;
}

bool DataDocument::addMember(const std::string& objectPath, const std::string& key, OrderedJson value, std::string& problem) {
    const std::optional<DocPath> parsed = parsePath(objectPath);
    OrderedJson next = document_;
    OrderedJson* object = parsed ? resolve(next, *parsed) : nullptr;
    if (object == nullptr || !object->is_object()) {
        problem = "\"" + objectPath + "\" is not an object";
        return false;
    }
    if (object->contains(key)) {
        problem = "\"" + key + "\" is there already";
        return false;
    }
    (*object)[key] = std::move(value);
    commit(std::move(next), "add " + key);
    return true;
}

bool DataDocument::removeMember(const std::string& objectPath, const std::string& key, std::string& problem) {
    const std::optional<DocPath> parsed = parsePath(objectPath);
    OrderedJson next = document_;
    OrderedJson* object = parsed ? resolve(next, *parsed) : nullptr;
    if (object == nullptr || !object->is_object() || !object->contains(key)) {
        problem = "\"" + key + "\" is not there";
        return false;
    }
    object->erase(key);
    commit(std::move(next), "remove " + key);
    return true;
}

bool DataDocument::renameMember(const std::string& objectPath, const std::string& from, const std::string& to, std::string& problem) {
    const std::optional<DocPath> parsed = parsePath(objectPath);
    OrderedJson next = document_;
    OrderedJson* object = parsed ? resolve(next, *parsed) : nullptr;
    if (object == nullptr || !object->is_object() || !object->contains(from)) {
        problem = "\"" + from + "\" is not there";
        return false;
    }
    if (from != to && object->contains(to)) {
        problem = "\"" + to + "\" is there already";
        return false;
    }
    OrderedJson renamed = OrderedJson::object();
    for (auto& item : object->items()) renamed[item.key() == from ? to : item.key()] = item.value();
    *object = std::move(renamed);
    commit(std::move(next), std::format("rename {} to {}", from, to));
    return true;
}

void DataDocument::replaceRoot(OrderedJson document, const std::string& what) { commit(std::move(document), what); }

bool DataDocument::undo() {
    if (undo_.empty()) return false;
    redo_.push_back(std::move(document_));
    document_ = std::move(undo_.back());
    undo_.pop_back();
    ++version_;
    return true;
}

bool DataDocument::redo() {
    if (redo_.empty()) return false;
    undo_.push_back(std::move(document_));
    document_ = std::move(redo_.back());
    redo_.pop_back();
    ++version_;
    return true;
}

std::optional<std::string> DataDocument::save(int backups) {
    const std::string written = text();
    if (std::optional<std::string> problem = core::writeTextFileSafely(file_, written, backups)) return problem;
    original_ = written;
    saved_ = document_;
    return std::nullopt;
}

bool DataDocument::reloadFromDisk(std::string& problem) {
    std::optional<DataDocument> fresh = open(file_, problem);
    if (!fresh) return false;
    *this = std::move(*fresh);
    ++version_;
    return true;
}

} // namespace odysseus::sim
