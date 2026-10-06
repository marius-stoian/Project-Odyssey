#pragma once

#include "boundary.h"

#include <map>
#include <string>
#include <string_view>

namespace odysseus::sim {

// Where each value of a JSON text stands (US-190). The parsed document has no positions, so an error such as "weapons[2].damage must be
// between 1 and 999" could not say which line to fix. This scans the same text once (comments allowed, like the data files) and remembers the
// line of every member and element under its path. Paths are written the way the schema validator writes them: "weapons[2].damage".
class JsonLines {
public:
    // Scans `text`. After a syntax mistake it keeps what it saw up to there; the parser reports the mistake itself.
    static JsonLines scan(std::string_view text);

    // The line (counting from 1) of the value at `path`; the line of its nearest known parent when the path is not known, 0 when there is none.
    int lineOf(const std::string& path) const;

    static std::string childPath(const std::string& parent, const std::string& key);
    static std::string indexPath(const std::string& parent, std::size_t index);

private:
    std::map<std::string, int> lines_;
};

} // namespace odysseus::sim
