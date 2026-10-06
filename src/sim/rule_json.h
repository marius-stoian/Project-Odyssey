#pragma once

#include "boundary.h"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace odysseus::sim::rules {

// A problem in a rules file, always "file:line: message" (Charter rule 7, INT-03).
struct Diagnostic {
    std::string file; // as the owner knows it, e.g. "interactions/gather.json"
    int line = 0;
    std::string message;

    std::string text() const;
};

// What the schema of a data file (US-190) finds wrong in it: type, range and choice mistakes, as diagnostics with the line of the field. `name` is the file as
// the owner knows it below assets/data ("interactions/gather.json"). Empty when no schemas are installed or the file has none. Warnings (an unknown field) are
// left to the loader, which reads its own fields and says so itself.
std::vector<Diagnostic> schemaDiagnostics(const std::string& name, std::string_view text);

// A small JSON reader for the rule files. It does what nlohmann cannot: it remembers the line of every
// value, so an error can say "gather.json:12". It also accepts // and /* */ comments, so the owner can
// explain things inside the files. Numbers keep the text they were written with (no floating point).
struct JsonValue {
    enum class Kind { Null, Bool, Number, String, Array, Object };

    Kind kind = Kind::Null;
    int line = 1;
    bool boolean = false;
    std::string text;               // the string, or the number as written ("1.5")
    std::vector<JsonValue> items;   // the elements of an array, or the values of an object
    std::vector<std::string> keys;  // object only: the names, in file order, matching `items`
    std::vector<int> keyLines;      // object only: the line of each name

    const JsonValue* find(std::string_view key) const; // object only; nullptr when absent
    bool isString() const { return kind == Kind::String; }
    bool isNumber() const { return kind == Kind::Number; }
    bool isArray() const { return kind == Kind::Array; }
    bool isObject() const { return kind == Kind::Object; }
};

struct JsonParseResult {
    std::optional<JsonValue> value;
    int errorLine = 0;
    std::string error; // empty when the text parsed
};

// Reads one JSON document (with comments). Never throws; a mistake comes back as error + errorLine.
JsonParseResult parseJson(std::string_view text);

// "1.5" -> 1500, "3" -> 3000, "0.25" -> 250: a decimal as thousandths, with whole-number arithmetic only
// (Charter rule 6). Empty when the text is not a plain decimal with at most three digits after the point.
std::optional<long long> parseMilli(std::string_view text);

// The other way, shortest form: 1500 -> "1.5", 3000 -> "3".
std::string formatMilli(long long milli);

// A JSON string literal for `text` (quotes, escapes).
std::string quoteJson(std::string_view text);

} // namespace odysseus::sim::rules
