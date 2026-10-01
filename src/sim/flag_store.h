#pragma once

#include "boundary.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace odysseus::sim::rules {

// The story notes conversations and interactions set (US-164): `flag met-elder` in an effect, `flag(met-elder)` in a condition. A flag is a name with
// a whole number (1 unless a value is given; 0 is the same as never set). Ordered, so saving and hashing never depend on hash order (Charter rule 6).
class FlagStore {
public:
    void set(const std::string& name, int value);
    int get(const std::string& name) const;
    bool empty() const { return flags_.empty(); }
    const std::map<std::string, int>& all() const { return flags_; }
    void clear() { flags_.clear(); }

    // The flags as the text of a JSON object ({"met-elder": 1}), and back. `load` returns the notes of anything it could not read and keeps the rest.
    std::string save() const;
    std::vector<std::string> load(const std::string& text);

    std::uint64_t hash() const;

private:
    std::map<std::string, int> flags_;
};

} // namespace odysseus::sim::rules
