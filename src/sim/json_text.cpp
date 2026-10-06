#include "sim/json_text.h"

#include <algorithm>

namespace odysseus::sim {

namespace {

// A small recursive reader that only cares about where things are. It trusts the text to be valid JSON (the real parser has already said so, or will);
// on anything it does not understand it stops and keeps what it has.
class Scanner {
public:
    Scanner(std::string_view text, std::map<std::string, int>& out) : text_(text), out_(out) {}

    void run() { value(""); }

private:
    bool atEnd() const { return i_ >= text_.size(); }

    // Whitespace, // line comments and /* block comments */, counting lines as it goes.
    void skipSpace() {
        while (!atEnd()) {
            const char c = text_[i_];
            if (c == '\n') {
                ++line_;
                ++i_;
            } else if (c == ' ' || c == '\t' || c == '\r') {
                ++i_;
            } else if (c == '/' && i_ + 1 < text_.size() && text_[i_ + 1] == '/') {
                while (!atEnd() && text_[i_] != '\n') ++i_;
            } else if (c == '/' && i_ + 1 < text_.size() && text_[i_ + 1] == '*') {
                i_ += 2;
                while (!atEnd() && !(text_[i_] == '*' && i_ + 1 < text_.size() && text_[i_ + 1] == '/')) {
                    if (text_[i_] == '\n') ++line_;
                    ++i_;
                }
                i_ = std::min(i_ + 2, text_.size());
            } else {
                break;
            }
        }
    }

    // The text of a string, escapes left as written except \" and \\ (member names are plain words, so nothing else matters here).
    bool readString(std::string& into) {
        if (atEnd() || text_[i_] != '"') return false;
        ++i_;
        into.clear();
        while (!atEnd() && text_[i_] != '"') {
            if (text_[i_] == '\\' && i_ + 1 < text_.size()) {
                ++i_;
                if (text_[i_] != '"' && text_[i_] != '\\') into += '\\';
                into += text_[i_];
            } else {
                if (text_[i_] == '\n') ++line_;
                into += text_[i_];
            }
            ++i_;
        }
        if (atEnd()) return false;
        ++i_;
        return true;
    }

    bool value(const std::string& path) {
        skipSpace();
        if (atEnd()) return false;
        out_.emplace(path, line_);
        const char c = text_[i_];
        if (c == '{') return object(path);
        if (c == '[') return array(path);
        if (c == '"') {
            std::string ignored;
            return readString(ignored);
        }
        while (!atEnd() && text_[i_] != ',' && text_[i_] != '}' && text_[i_] != ']' && text_[i_] != ' ' && text_[i_] != '\n' && text_[i_] != '\r' && text_[i_] != '\t' &&
               text_[i_] != '/') {
            ++i_;
        }
        return true;
    }

    bool object(const std::string& path) {
        ++i_; // {
        skipSpace();
        if (!atEnd() && text_[i_] == '}') {
            ++i_;
            return true;
        }
        while (true) {
            skipSpace();
            std::string key;
            const int keyLine = line_;
            if (!readString(key)) return false;
            const std::string child = JsonLines::childPath(path, key);
            out_.emplace(child, keyLine);
            skipSpace();
            if (atEnd() || text_[i_] != ':') return false;
            ++i_;
            if (!value(child)) return false;
            skipSpace();
            if (atEnd()) return false;
            if (text_[i_] == ',') {
                ++i_;
                continue;
            }
            if (text_[i_] == '}') {
                ++i_;
                return true;
            }
            return false;
        }
    }

    bool array(const std::string& path) {
        ++i_; // [
        skipSpace();
        if (!atEnd() && text_[i_] == ']') {
            ++i_;
            return true;
        }
        std::size_t index = 0;
        while (true) {
            if (!value(JsonLines::indexPath(path, index++))) return false;
            skipSpace();
            if (atEnd()) return false;
            if (text_[i_] == ',') {
                ++i_;
                continue;
            }
            if (text_[i_] == ']') {
                ++i_;
                return true;
            }
            return false;
        }
    }

    std::string_view text_;
    std::map<std::string, int>& out_;
    std::size_t i_ = 0;
    int line_ = 1;
};

} // namespace

JsonLines JsonLines::scan(std::string_view text) {
    JsonLines lines;
    Scanner(text, lines.lines_).run();
    return lines;
}

int JsonLines::lineOf(const std::string& path) const {
    std::string probe = path;
    while (true) {
        const auto found = lines_.find(probe);
        if (found != lines_.end()) return found->second;
        if (probe.empty()) return 0;
        // The nearest known parent: cut the last ".member" or "[index]".
        const std::size_t dot = probe.rfind('.');
        const std::size_t bracket = probe.rfind('[');
        const std::size_t cut = dot == std::string::npos ? bracket : (bracket == std::string::npos ? dot : std::max(dot, bracket));
        probe = cut == std::string::npos ? std::string() : probe.substr(0, cut);
    }
}

std::string JsonLines::childPath(const std::string& parent, const std::string& key) {
    if (key.find_first_of(".[]\"") == std::string::npos) return parent.empty() ? key : parent + "." + key;
    // A key with a dot, a bracket or a quote in it (help.json names its fields "npc.sword") is written ["npc.sword"], so the path is unambiguous (data_document.h).
    std::string quoted;
    for (const char c : key) quoted += (c == '"' || c == '\\' ? "\\" : "") + std::string(1, c);
    return parent + "[\"" + quoted + "\"]";
}

std::string JsonLines::indexPath(const std::string& parent, std::size_t index) { return parent + "[" + std::to_string(index) + "]"; }

} // namespace odysseus::sim
