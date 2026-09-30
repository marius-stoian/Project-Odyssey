#pragma once

#include "boundary.h"

#include "calendar.h"

#include <string>
#include <vector>

namespace odysseus::sim {

// One notable event, as a sentence (NA-02: the clan's tapestry).
struct ChronicleEntry {
    Date date;
    int importance = 0; // 0..100; births, deaths and feuds are high, a meal is low
    std::string text;   // "Ura was born to Tok and Maa."
};

class Chronicle {
public:
    void add(const Date& date, int importance, std::string text);

    const std::vector<ChronicleEntry>& entries() const { return entries_; }

private:
    std::vector<ChronicleEntry> entries_;
};

} // namespace odysseus::sim
