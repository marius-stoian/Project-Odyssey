// odysseus.exe: the game. For now it only proves the build works;
// the window and game loop arrive in US-020.
#include "core/version.h"

#include <iostream>

int main() {
    const std::string_view version = odysseus::core::versionString();
    std::cout << "Project Odyssey " << version << '\n';
    return 0;
}
