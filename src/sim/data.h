#pragma once

#include "boundary.h"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace odysseus::sim {

// A problem in a content file. The message always names the file and the field, so a
// typo in assets/data is found in seconds (Charter rule 7, ARC-08).
class DataError : public std::runtime_error {
public:
    DataError(const std::filesystem::path& file, const std::string& field, const std::string& problem);
};

} // namespace odysseus::sim
