// odysseus_headless.exe: runs the simulation without graphics.
// It becomes the console clan simulator in milestone M1.
#include "core/version.h"

#include <iostream>

int main() {
    const std::string_view version = odysseus::core::versionString();
    std::cout << "Project Odyssey headless runner " << version << '\n';
    return 0;
}
