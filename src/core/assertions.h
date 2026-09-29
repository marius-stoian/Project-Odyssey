#pragma once

#include "boundary.h"

#include <string_view>

// Named assertions.h (not assert.h) so it can never be mistaken for the standard <assert.h>.

namespace odysseus::core {

// Writes "Assertion failed: <condition> (<message>) at <file>:<line>" to the log.
// Called by ODYSSEUS_ASSERT; you normally do not call it yourself.
void reportAssertionFailure(std::string_view condition, std::string_view message, std::string_view file, int line);

} // namespace odysseus::core

// Stops the program right here so the debugger shows this very line. A compiler
// intrinsic, not an operating-system call, so it belongs in Core.
#if defined(_MSC_VER)
#define ODYSSEUS_DEBUG_BREAK() __debugbreak()
#else
#define ODYSSEUS_DEBUG_BREAK() __builtin_trap()
#endif

// ODYSSEUS_ASSERT(condition, message): checks an assumption in Debug builds (ADR-015).
// If the condition is false it logs the file and line, then breaks into the debugger.
// It is a macro because only a macro knows the caller's __FILE__ and __LINE__.
// In Release it does nothing; sizeof() mentions the condition without running it, so a
// variable used only in an assert does not trigger an "unused variable" warning.
#if defined(NDEBUG)
#define ODYSSEUS_ASSERT(condition, message) \
    do {                                    \
        (void)sizeof(condition);            \
    } while (false)
#else
#define ODYSSEUS_ASSERT(condition, message)                                                             \
    do {                                                                                                \
        if (!(condition)) {                                                                             \
            ::odysseus::core::reportAssertionFailure(#condition, (message), __FILE__, __LINE__);      \
            ODYSSEUS_DEBUG_BREAK();                                                                     \
        }                                                                                               \
    } while (false)
#endif
