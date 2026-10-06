#pragma once

#include "boundary.h"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace odysseus::sim {

using OrderedJson = nlohmann::ordered_json;

// Writing an edited data file without rewriting it (US-191, D-60 Q5). The Data tab edits a document (the parsed file) and must save it so that only what the
// owner changed changes in the file: the other lines, the comments and the spacing stay exactly as they were. So the writer does not print the document; it
// takes the text the document came from and patches it.
//
// `original` is the file's text, `before` its parsed document (parsed with comments allowed) and `after` the edited one. A value that changed is replaced
// where it stands; a list or object that gained or lost entries is built again in the style of the one it replaces (one line or one entry to a line, the
// same indentation, the same spacing after colons and commas); everything else is copied byte for byte. With `before == after` the result is `original`.
std::string patchJsonText(std::string_view original, const OrderedJson& before, const OrderedJson& after);

// A whole document in the house style, for a file that has no text to patch (a new file): objects one member to a line, a list or object that fits in
// `width` letters on one line, two spaces of indentation, a newline at the end.
std::string writeJsonText(const OrderedJson& document, int width = 120);

// The two documents are the same: the same values, and in objects the same members in the same order (the order is part of what the owner sees).
bool sameJson(const OrderedJson& a, const OrderedJson& b);

// The comment lines (starting with //) before the first value of a file, with their line breaks; empty when there are none.
std::string leadingComments(std::string_view text);

} // namespace odysseus::sim
