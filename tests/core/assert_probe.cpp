// us004_assert_probe.exe: fires one false assert on purpose, so the US-004 "Assert"
// scenario can check that a Debug build stops there and logs the file and line.
// Usage: us004_assert_probe <log folder>
#include "core/assertions.h"
#include "core/log.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 2;
    }
    odysseus::core::LogSession session(argv[1]);
    odysseus::core::logInfo("before the assert");
    ODYSSEUS_ASSERT(1 + 1 == 3, "the probe asserts on purpose"); // keep in sync with ODYSSEUS_ASSERT_PROBE_LINE
    odysseus::core::logInfo("after the assert");
    return 0;
}
