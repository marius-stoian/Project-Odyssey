#include "play_world.h"

#include <algorithm>
#include <system_error>

namespace odysseus::game {

std::filesystem::path worldsFolder(const std::filesystem::path& dataDirectory) { return (dataDirectory / ".." / "worlds").lexically_normal(); }

std::filesystem::path worldFilePath(const std::filesystem::path& dataDirectory, const std::string& name) { return worldsFolder(dataDirectory) / (name + ".json"); }

std::vector<std::string> worldNames(const std::filesystem::path& dataDirectory) {
    std::vector<std::string> names;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(worldsFolder(dataDirectory), error)) {
        if (entry.is_regular_file(error) && entry.path().extension() == ".json") names.push_back(entry.path().stem().string());
    }
    std::sort(names.begin(), names.end());
    return names;
}

} // namespace odysseus::game
