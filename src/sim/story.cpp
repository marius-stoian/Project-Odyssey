#include "sim/story.h"

#include "sim/json_data.h"

namespace odysseus::sim {

StoryConfig loadStoryConfig(const std::filesystem::path& file) {
    const nlohmann::json json = readJsonFile(file);
    StoryConfig config;
    config.season.leanAutumnPercent = requireInt(json, file, "season", "leanAutumnPercent", 0, 100);
    config.season.leanForagePercent = requireInt(json, file, "season", "leanForagePercent", 0, 100);
    config.causes.theftWindowDays = requireInt(json, file, "causes", "theftWindowDays", 1, 3650);
    config.causes.starvationWindowDays = requireInt(json, file, "causes", "starvationWindowDays", 1, 3650);
    config.causes.maxCauses = requireInt(json, file, "causes", "maxCauses", 1, 20);
    config.causes.grudgeLimit = requireInt(json, file, "causes", "grudgeLimit", 1, 200);
    return config;
}

} // namespace odysseus::sim
