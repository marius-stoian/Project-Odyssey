#include "sim/rule_json.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <format>

namespace odysseus::sim::rules {

std::string Diagnostic::text() const {
    return std::format("{}:{}: {}", file, line, message);
}

const JsonValue* JsonValue::find(std::string_view key) const {
    if (kind != Kind::Object) return nullptr;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == key) return &items[i];
    }
    return nullptr;
}

namespace {

constexpr int kMaxDepth = 48; // a file nested deeper than this is a mistake, and must not overflow the stack

struct Failure {
    int line;
    std::string message;
};

class Reader {
public:
    explicit Reader(std::string_view text) : text_(text) {}

    JsonValue document() {
        skip();
        JsonValue value = readValue(0);
        skip();
        if (pos_ < text_.size()) fail("unexpected text after the end of the data");
        return value;
    }

private:
    std::string_view text_;
    std::size_t pos_ = 0;
    int line_ = 1;

    [[noreturn]] void fail(const std::string& message) const { throw Failure{line_, message}; }

    bool atEnd() const { return pos_ >= text_.size(); }
    char peek() const { return atEnd() ? '\0' : text_[pos_]; }

    char advance() {
        const char c = text_[pos_++];
        if (c == '\n') ++line_;
        return c;
    }

    // White space and comments.
    void skip() {
        while (!atEnd()) {
            const char c = peek();
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                advance();
            } else if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '/') {
                while (!atEnd() && peek() != '\n') advance();
            } else if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '*') {
                const int startLine = line_;
                advance();
                advance();
                bool closed = false;
                while (!atEnd()) {
                    if (peek() == '*' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '/') {
                        advance();
                        advance();
                        closed = true;
                        break;
                    }
                    advance();
                }
                if (!closed) throw Failure{startLine, "this /* comment never ends"};
            } else {
                break;
            }
        }
    }

    JsonValue readValue(int depth) {
        if (depth > kMaxDepth) fail("the data is nested too deeply");
        JsonValue value;
        value.line = line_;
        const char c = peek();
        if (c == '{') {
            readObject(value, depth);
        } else if (c == '[') {
            readArray(value, depth);
        } else if (c == '"') {
            value.kind = JsonValue::Kind::String;
            value.text = readString();
        } else if (c == '-' || (c >= '0' && c <= '9')) {
            value.kind = JsonValue::Kind::Number;
            value.text = readNumber();
        } else if (matchWord("true")) {
            value.kind = JsonValue::Kind::Bool;
            value.boolean = true;
        } else if (matchWord("false")) {
            value.kind = JsonValue::Kind::Bool;
        } else if (matchWord("null")) {
            value.kind = JsonValue::Kind::Null;
        } else if (atEnd()) {
            fail("the data ends where a value was expected");
        } else {
            fail(std::format("expected a value, found '{}'", c));
        }
        return value;
    }

    bool matchWord(std::string_view word) {
        if (text_.substr(pos_, word.size()) != word) return false;
        pos_ += word.size();
        return true;
    }

    void readObject(JsonValue& value, int depth) {
        value.kind = JsonValue::Kind::Object;
        advance(); // {
        skip();
        if (peek() == '}') {
            advance();
            return;
        }
        while (true) {
            if (peek() != '"') fail("expected a field name in quotes");
            const int keyLine = line_;
            std::string key = readString();
            if (std::find(value.keys.begin(), value.keys.end(), key) != value.keys.end()) {
                throw Failure{keyLine, std::format("the field \"{}\" appears twice", key)};
            }
            skip();
            if (peek() != ':') fail(std::format("expected ':' after \"{}\"", key));
            advance();
            skip();
            value.keys.push_back(std::move(key));
            value.keyLines.push_back(keyLine);
            value.items.push_back(readValue(depth + 1));
            skip();
            if (peek() == ',') {
                advance();
                skip();
                if (peek() == '}') fail("an extra comma before '}'");
            } else if (peek() == '}') {
                advance();
                return;
            } else {
                fail("expected ',' or '}' (is a comma missing on the line above?)");
            }
        }
    }

    void readArray(JsonValue& value, int depth) {
        value.kind = JsonValue::Kind::Array;
        advance(); // [
        skip();
        if (peek() == ']') {
            advance();
            return;
        }
        while (true) {
            value.items.push_back(readValue(depth + 1));
            skip();
            if (peek() == ',') {
                advance();
                skip();
                if (peek() == ']') fail("an extra comma before ']'");
            } else if (peek() == ']') {
                advance();
                return;
            } else {
                fail("expected ',' or ']' (is a comma missing on the line above?)");
            }
        }
    }

    std::string readString() {
        const int startLine = line_;
        advance(); // opening quote
        std::string out;
        while (true) {
            if (atEnd()) throw Failure{startLine, "this string never ends"};
            const char c = advance();
            if (c == '"') return out;
            if (c == '\n') throw Failure{startLine, "a string cannot continue on the next line"};
            if (c != '\\') {
                out += c;
                continue;
            }
            if (atEnd()) throw Failure{startLine, "this string never ends"};
            const char e = advance();
            switch (e) {
            case '"': out += '"'; break;
            case '\\': out += '\\'; break;
            case '/': out += '/'; break;
            case 'b': out += '\b'; break;
            case 'f': out += '\f'; break;
            case 'n': out += '\n'; break;
            case 'r': out += '\r'; break;
            case 't': out += '\t'; break;
            case 'u': appendUnicode(out); break;
            default: fail(std::format("unknown escape '\\{}'", e));
            }
        }
    }

    void appendUnicode(std::string& out) {
        std::uint32_t code = 0;
        for (int i = 0; i < 4; ++i) {
            if (atEnd() || !std::isxdigit(static_cast<unsigned char>(peek()))) fail("\\u needs four hexadecimal digits");
            const char h = advance();
            code = code * 16 + static_cast<std::uint32_t>(h <= '9' ? h - '0' : (std::tolower(static_cast<unsigned char>(h)) - 'a' + 10));
        }
        if (code < 0x80) {
            out += static_cast<char>(code);
        } else if (code < 0x800) {
            out += static_cast<char>(0xC0 | (code >> 6));
            out += static_cast<char>(0x80 | (code & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (code >> 12));
            out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (code & 0x3F));
        }
    }

    std::string readNumber() {
        std::string out;
        if (peek() == '-') out += advance();
        if (!std::isdigit(static_cast<unsigned char>(peek()))) fail("a number needs digits");
        while (std::isdigit(static_cast<unsigned char>(peek()))) out += advance();
        if (peek() == '.') {
            out += advance();
            if (!std::isdigit(static_cast<unsigned char>(peek()))) fail("a number needs digits after the point");
            while (std::isdigit(static_cast<unsigned char>(peek()))) out += advance();
        }
        if (peek() == 'e' || peek() == 'E') fail("exponents are not supported; write the number out");
        return out;
    }
};

} // namespace

JsonParseResult parseJson(std::string_view text) {
    JsonParseResult result;
    try {
        result.value = Reader(text).document();
    } catch (const Failure& failure) {
        result.errorLine = failure.line;
        result.error = failure.message;
    }
    return result;
}

std::optional<long long> parseMilli(std::string_view text) {
    if (text.empty()) return std::nullopt;
    std::size_t i = 0;
    bool negative = false;
    if (text[0] == '-') {
        negative = true;
        i = 1;
    }
    long long whole = 0;
    std::size_t digits = 0;
    while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
        whole = whole * 10 + (text[i] - '0');
        if (whole > 1'000'000'000) return std::nullopt;
        ++i;
        ++digits;
    }
    if (digits == 0) return std::nullopt;
    long long fraction = 0;
    int fractionDigits = 0;
    if (i < text.size() && text[i] == '.') {
        ++i;
        while (i < text.size() && std::isdigit(static_cast<unsigned char>(text[i]))) {
            if (fractionDigits == 3) return std::nullopt;
            fraction = fraction * 10 + (text[i] - '0');
            ++fractionDigits;
            ++i;
        }
        if (fractionDigits == 0) return std::nullopt;
    }
    if (i != text.size()) return std::nullopt;
    for (; fractionDigits < 3; ++fractionDigits) fraction *= 10;
    const long long value = whole * 1000 + fraction;
    return negative ? -value : value;
}

std::string formatMilli(long long milli) {
    const bool negative = milli < 0;
    const long long a = negative ? -milli : milli;
    std::string out = std::format("{}", a / 1000);
    if (a % 1000 != 0) {
        std::string frac = std::format("{:03}", a % 1000);
        while (frac.back() == '0') frac.pop_back();
        out += "." + frac;
    }
    return negative ? "-" + out : out;
}

std::string quoteJson(std::string_view text) {
    std::string out = "\"";
    for (const char c : text) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                out += std::format("\\u{:04x}", static_cast<unsigned>(static_cast<unsigned char>(c)));
            } else {
                out += c;
            }
        }
    }
    out += '"';
    return out;
}

} // namespace odysseus::sim::rules
