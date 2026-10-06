#include "sim/json_patch.h"

#include <algorithm>
#include <optional>
#include <vector>

namespace odysseus::sim {

namespace {

// ---- A map of the text: where every value stands, so the text between values can be kept as it is.

struct TextNode {
    enum class Kind { Scalar, Array, Object };
    Kind kind = Kind::Scalar;
    std::size_t begin = 0; // the value's own text: a container from its bracket to its closing bracket
    std::size_t end = 0;
    std::vector<TextNode> children;  // the elements of an array, or the values of an object's members
    std::vector<std::size_t> keyBegin; // an object's members: where each key's opening quote stands...
    std::vector<std::size_t> keyEnd;   // ...and the offset just after its closing quote
    std::vector<std::string> keys;     // the keys, decoded
};

class TextParser {
public:
    explicit TextParser(std::string_view text) : s_(text) {}

    std::optional<TextNode> parse() {
        skipSpace();
        TextNode root;
        if (!value(root)) return std::nullopt;
        skipSpace();
        if (i_ != s_.size()) return std::nullopt;
        return root;
    }

private:
    bool atEnd() const { return i_ >= s_.size(); }

    // Whitespace and comments (// to the end of the line, /* ... */).
    void skipSpace() {
        while (!atEnd()) {
            const char c = s_[i_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                ++i_;
            } else if (c == '/' && i_ + 1 < s_.size() && s_[i_ + 1] == '/') {
                while (!atEnd() && s_[i_] != '\n') ++i_;
            } else if (c == '/' && i_ + 1 < s_.size() && s_[i_ + 1] == '*') {
                i_ += 2;
                while (i_ + 1 < s_.size() && !(s_[i_] == '*' && s_[i_ + 1] == '/')) ++i_;
                i_ = std::min(i_ + 2, s_.size());
            } else {
                break;
            }
        }
    }

    bool string() {
        if (atEnd() || s_[i_] != '"') return false;
        ++i_;
        while (!atEnd() && s_[i_] != '"') i_ += s_[i_] == '\\' ? 2 : 1;
        if (atEnd()) return false;
        ++i_;
        return true;
    }

    bool value(TextNode& node) {
        if (atEnd()) return false;
        node.begin = i_;
        const char c = s_[i_];
        bool ok = true;
        if (c == '{') ok = object(node);
        else if (c == '[') ok = array(node);
        else if (c == '"') ok = string();
        else {
            while (!atEnd() && s_[i_] != ',' && s_[i_] != '}' && s_[i_] != ']' && s_[i_] != ' ' && s_[i_] != '\t' && s_[i_] != '\r' && s_[i_] != '\n' && s_[i_] != '/') ++i_;
            ok = i_ > node.begin;
        }
        node.end = i_;
        return ok;
    }

    bool object(TextNode& node) {
        node.kind = TextNode::Kind::Object;
        ++i_;
        skipSpace();
        if (!atEnd() && s_[i_] == '}') {
            ++i_;
            return true;
        }
        while (true) {
            skipSpace();
            const std::size_t keyBegin = i_;
            if (!string()) return false;
            const std::string raw(s_.substr(keyBegin, i_ - keyBegin));
            const OrderedJson decoded = OrderedJson::parse(raw, nullptr, false);
            if (!decoded.is_string()) return false;
            node.keys.push_back(decoded.get<std::string>());
            node.keyBegin.push_back(keyBegin);
            node.keyEnd.push_back(i_);
            skipSpace();
            if (atEnd() || s_[i_] != ':') return false;
            ++i_;
            skipSpace();
            node.children.emplace_back();
            if (!value(node.children.back())) return false;
            skipSpace();
            if (atEnd()) return false;
            if (s_[i_] == ',') {
                ++i_;
                continue;
            }
            if (s_[i_] == '}') {
                ++i_;
                return true;
            }
            return false;
        }
    }

    bool array(TextNode& node) {
        node.kind = TextNode::Kind::Array;
        ++i_;
        skipSpace();
        if (!atEnd() && s_[i_] == ']') {
            ++i_;
            return true;
        }
        while (true) {
            skipSpace();
            node.children.emplace_back();
            if (!value(node.children.back())) return false;
            skipSpace();
            if (atEnd()) return false;
            if (s_[i_] == ',') {
                ++i_;
                continue;
            }
            if (s_[i_] == ']') {
                ++i_;
                return true;
            }
            return false;
        }
    }

    std::string_view s_;
    std::size_t i_ = 0;
};

// ---- How a container is written, found out from the text it came from, so a rebuilt one looks like its neighbours.

struct Style {
    bool multiLine = false;  // one entry to a line
    std::string memberPad;   // the indentation of those lines
    std::string closePad;    // the indentation of the closing bracket's line
    bool colonSpace = true;  // "key": value, not "key":value
    bool commaSpace = true;  // [1, 2], not [1,2]
    bool braceSpace = false; // { "a": 1 }, not {"a": 1}
    std::string eol = "\n";
    int width = 120;         // a new container that fits in this many letters stays on one line
    bool keepSingle = false; // the neighbours are each on one line, however long: a new one is too
};

class Patcher {
public:
    explicit Patcher(std::string_view text) : text_(text) {}

    std::string patched(const TextNode& n, const OrderedJson& before, const OrderedJson& after) const {
        if (sameJson(before, after)) return span(n.begin, n.end);
        if (n.kind == TextNode::Kind::Object && before.is_object() && after.is_object()) {
            return keysOf(before) == keysOf(after) ? inPlace(n, before, after) : rebuildObject(n, before, after);
        }
        if (n.kind == TextNode::Kind::Array && before.is_array() && after.is_array()) {
            return before.size() == after.size() ? inPlace(n, before, after) : rebuildArray(n, before, after);
        }
        // A scalar that changed, or a value that became another kind of value: written fresh where it stands.
        return fresh(after, styleOf(n), padOfLine(n.begin), false);
    }

    // A new value in the given style, with its own line breaks indented by `pad`.
    std::string fresh(const OrderedJson& value, const Style& style, const std::string& pad, bool forceMulti) const {
        if (!value.is_structured() || value.empty()) return single(value, style);
        if (!forceMulti) {
            const std::string one = single(value, style);
            if (style.keepSingle || pad.size() + one.size() <= static_cast<std::size_t>(style.width)) return one;
        }
        const std::string inner = pad + "  ";
        std::vector<std::string> entries;
        if (value.is_array()) {
            for (const OrderedJson& element : value) entries.push_back(inner + fresh(element, style, inner, false));
        } else {
            for (const auto& [key, member] : value.items()) entries.push_back(inner + OrderedJson(key).dump() + (style.colonSpace ? ": " : ":") + fresh(member, style, inner, false));
        }
        std::string out = value.is_array() ? "[" : "{";
        out += style.eol;
        for (std::size_t i = 0; i < entries.size(); ++i) out += entries[i] + (i + 1 < entries.size() ? "," : "") + style.eol;
        return out + pad + (value.is_array() ? "]" : "}");
    }

private:
    static std::vector<std::string> keysOf(const OrderedJson& object) {
        std::vector<std::string> keys;
        for (const auto& item : object.items()) keys.push_back(item.key());
        return keys;
    }

    std::string span(std::size_t begin, std::size_t end) const { return std::string(text_.substr(begin, end - begin)); }

    // The whitespace at the start of the line the offset stands on.
    std::string padOfLine(std::size_t offset) const {
        std::size_t start = offset;
        while (start > 0 && text_[start - 1] != '\n') --start;
        std::size_t stop = start;
        while (stop < text_.size() && (text_[stop] == ' ' || text_[stop] == '\t')) ++stop;
        return span(start, stop);
    }

    bool multiLineSpan(const TextNode& n) const {
        const std::size_t first = text_.find('\n', n.begin);
        return first != std::string_view::npos && first < n.end;
    }

    std::string single(const OrderedJson& value, const Style& style) const {
        if (!value.is_structured()) return value.dump();
        if (value.empty()) return value.is_array() ? "[]" : "{}";
        std::string out = value.is_array() ? "[" : (style.braceSpace ? "{ " : "{");
        bool first = true;
        for (const auto& item : value.items()) {
            if (!first) out += style.commaSpace ? ", " : ",";
            first = false;
            if (value.is_object()) out += OrderedJson(item.key()).dump() + (style.colonSpace ? ": " : ":");
            out += single(item.value(), style);
        }
        return out + (value.is_array() ? "]" : (style.braceSpace ? " }" : "}"));
    }

    Style styleOf(const TextNode& n) const {
        Style style;
        if (text_.find("\r\n") != std::string_view::npos) style.eol = "\r\n";
        if (n.kind == TextNode::Kind::Scalar) return style;
        style.multiLine = multiLineSpan(n);
        style.closePad = padOfLine(n.end - 1);
        if (!n.children.empty()) {
            const std::size_t first = n.kind == TextNode::Kind::Object ? n.keyBegin.front() : n.children.front().begin;
            style.memberPad = padOfLine(first);
        } else {
            style.memberPad = padOfLine(n.begin) + "  ";
        }
        // Spacing: an object tells it itself; an array of objects tells it through its first object.
        const TextNode* object = n.kind == TextNode::Kind::Object ? &n : nullptr;
        if (object == nullptr) {
            for (const TextNode& child : n.children) {
                if (child.kind == TextNode::Kind::Object) {
                    object = &child;
                    break;
                }
            }
        }
        if (object != nullptr && !object->children.empty()) {
            const std::string between = span(object->keyEnd.front(), object->children.front().begin);
            style.colonSpace = between.find(' ') != std::string::npos;
            style.braceSpace = object->begin + 1 < text_.size() && text_[object->begin + 1] == ' ';
        }
        if (n.children.size() >= 2) {
            const std::string between = span(n.children[0].end, n.children[1].begin);
            style.commaSpace = !style.multiLine ? between.find(' ') != std::string::npos : style.commaSpace;
        }
        return style;
    }

    // Same members in the same order, or the same number of elements: the text between the children is kept and each child is patched where it stands.
    std::string inPlace(const TextNode& n, const OrderedJson& before, const OrderedJson& after) const {
        std::vector<const OrderedJson*> was;
        std::vector<const OrderedJson*> now;
        if (before.is_object()) {
            for (const auto& item : before.items()) was.push_back(&item.value());
            for (const auto& item : after.items()) now.push_back(&item.value());
        } else {
            for (const OrderedJson& element : before) was.push_back(&element);
            for (const OrderedJson& element : after) now.push_back(&element);
        }
        if (n.children.empty()) return span(n.begin, n.end);
        std::string out = span(n.begin, n.children.front().begin);
        for (std::size_t i = 0; i < n.children.size(); ++i) {
            out += patched(n.children[i], *was[i], *now[i]);
            const std::size_t next = i + 1 < n.children.size() ? n.children[i + 1].begin : n.end; // the text between: the comma, the next key and its colon
            out += span(n.children[i].end, next);
        }
        return out;
    }

    std::string join(const std::vector<std::string>& entries, const Style& style, bool array, const std::string& openPad) const {
        const std::string open = array ? "[" : (style.braceSpace && !style.multiLine ? "{ " : "{");
        const std::string close = array ? "]" : (style.braceSpace && !style.multiLine ? " }" : "}");
        if (entries.empty()) return array ? "[]" : "{}";
        std::string out = open;
        if (style.multiLine) {
            out += style.eol;
            for (std::size_t i = 0; i < entries.size(); ++i) out += style.memberPad + entries[i] + (i + 1 < entries.size() ? "," : "") + style.eol;
            return out + openPad + close;
        }
        for (std::size_t i = 0; i < entries.size(); ++i) out += (i != 0 ? (style.commaSpace ? ", " : ",") : "") + entries[i];
        return out + close;
    }

    std::string rebuildObject(const TextNode& n, const OrderedJson& before, const OrderedJson& after) const {
        if (n.children.empty()) return fresh(after, styleOf(n), padOfLine(n.begin), false); // it was {}: new members are written as a new value
        const Style style = styleOf(n);
        const std::string closePad = style.multiLine ? style.closePad : padOfLine(n.begin);
        std::vector<std::string> entries;
        for (const auto& item : after.items()) {
            const auto found = std::find(n.keys.begin(), n.keys.end(), item.key());
            std::string key;
            std::string value;
            if (found != n.keys.end() && before.contains(item.key())) {
                const std::size_t j = static_cast<std::size_t>(found - n.keys.begin());
                key = span(n.keyBegin[j], n.keyEnd[j]);
                value = patched(n.children[j], before.at(item.key()), item.value());
            } else {
                key = OrderedJson(item.key()).dump();
                value = fresh(item.value(), style, style.multiLine ? style.memberPad : closePad, false);
            }
            entries.push_back(key + (style.colonSpace ? ": " : ":") + value);
        }
        return join(entries, style, false, closePad);
    }

    // Which element of `before` each element of `after` is: the longest run of equal elements in order. The others are new.
    static std::vector<int> align(const OrderedJson& before, const OrderedJson& after) {
        const std::size_t m = before.size();
        const std::size_t k = after.size();
        std::vector<std::vector<int>> length(m + 1, std::vector<int>(k + 1, 0));
        for (std::size_t i = m; i-- > 0;) {
            for (std::size_t j = k; j-- > 0;) {
                length[i][j] = sameJson(before[i], after[j]) ? length[i + 1][j + 1] + 1 : std::max(length[i + 1][j], length[i][j + 1]);
            }
        }
        std::vector<int> match(k, -1);
        std::size_t i = 0;
        std::size_t j = 0;
        while (i < m && j < k) {
            if (sameJson(before[i], after[j])) {
                match[j++] = static_cast<int>(i++);
            } else if (length[i + 1][j] >= length[i][j + 1]) {
                ++i;
            } else {
                ++j;
            }
        }
        return match;
    }

    std::string rebuildArray(const TextNode& n, const OrderedJson& before, const OrderedJson& after) const {
        if (n.children.empty()) return fresh(after, styleOf(n), padOfLine(n.begin), false); // it was []: new elements are written as a new value
        const Style style = styleOf(n);
        const std::string closePad = style.multiLine ? style.closePad : padOfLine(n.begin);
        const std::vector<int> match = align(before, after);
        const bool siblingsMulti = std::any_of(n.children.begin(), n.children.end(), [&](const TextNode& child) { return multiLineSpan(child); });
        Style itemStyle = style;
        itemStyle.keepSingle = !siblingsMulti && std::any_of(n.children.begin(), n.children.end(), [](const TextNode& child) { return child.kind != TextNode::Kind::Scalar; });
        std::vector<std::string> entries;
        for (std::size_t j = 0; j < after.size(); ++j) {
            if (match[j] >= 0) entries.push_back(span(n.children[static_cast<std::size_t>(match[j])].begin, n.children[static_cast<std::size_t>(match[j])].end));
            else entries.push_back(fresh(after[j], itemStyle, style.multiLine ? style.memberPad : closePad, siblingsMulti && after[j].is_structured()));
        }
        return join(entries, style, true, closePad);
    }

    std::string_view text_;
};

} // namespace

bool sameJson(const OrderedJson& a, const OrderedJson& b) {
    if (a.is_number() && b.is_number()) {
        // 2 and 2.0 are the same number but not the same text: the owner wrote one of them.
        if (a.is_number_float() != b.is_number_float()) return false;
        return a.is_number_float() ? a.get<double>() == b.get<double>() : a.get<long long>() == b.get<long long>();
    }
    if (a.type() != b.type()) return false;
    if (a.is_object()) {
        if (a.size() != b.size()) return false;
        auto left = a.items().begin();
        auto right = b.items().begin();
        for (; left != a.items().end(); ++left, ++right) {
            if (left.key() != right.key() || !sameJson(left.value(), right.value())) return false;
        }
        return true;
    }
    if (a.is_array()) {
        if (a.size() != b.size()) return false;
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (!sameJson(a[i], b[i])) return false;
        }
        return true;
    }
    return a == b;
}

std::string leadingComments(std::string_view text) {
    std::size_t at = 0;
    std::size_t end = 0;
    while (at < text.size()) {
        const std::size_t newline = text.find('\n', at);
        const std::size_t lineEnd = newline == std::string_view::npos ? text.size() : newline + 1;
        std::string_view line = text.substr(at, lineEnd - at);
        while (!line.empty() && (line.front() == ' ' || line.front() == '\t')) line.remove_prefix(1);
        if (line.starts_with("//")) {
            end = lineEnd;
        } else if (!line.empty() && line.front() != '\r' && line.front() != '\n') {
            break;
        }
        at = lineEnd;
    }
    return std::string(text.substr(0, end));
}

std::string writeJsonText(const OrderedJson& document, int width) {
    Style style;
    style.width = width;
    return Patcher("").fresh(document, style, "", true) + "\n";
}

std::string patchJsonText(std::string_view original, const OrderedJson& before, const OrderedJson& after) {
    const std::optional<TextNode> root = TextParser(original).parse();
    if (!root) return leadingComments(original) + writeJsonText(after); // the text cannot be patched (it is not JSON): write the document
    const Patcher patcher(original);
    return std::string(original.substr(0, root->begin)) + patcher.patched(*root, before, after) + std::string(original.substr(root->end));
}

} // namespace odysseus::sim
