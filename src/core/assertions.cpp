#include "core/assertions.h"

#include "core/log.h"

#include <format>

namespace odysseus::core {

void reportAssertionFailure(std::string_view condition, std::string_view message, std::string_view file, int line) {
    logError(std::format("Assertion failed: {} ({}) at {}:{}", condition, message, file, line));
}

} // namespace odysseus::core
