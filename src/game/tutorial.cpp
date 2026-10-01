#include "game/tutorial.h"

#include "sim/data.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>

namespace odysseus::game {

using nlohmann::json;

namespace {

constexpr int kClosingSeconds = 10; // the closing line stays this long

std::string textField(const json& object, const char* field, const std::string& file, const std::string& where) {
    if (!object.contains(field) || !object.at(field).is_string() || object.at(field).get<std::string>().empty()) {
        throw sim::DataError(file, where.empty() ? std::string(field) : where + "." + field, "must be text in quotes");
    }
    return object.at(field).get<std::string>();
}

} // namespace

TutorialScript loadTutorial(const std::filesystem::path& file) {
    const std::string name = file.filename().string();
    std::ifstream in(file, std::ios::binary);
    if (!in) throw sim::DataError(name, "", "the file cannot be opened");
    std::stringstream text;
    text << in.rdbuf();
    json data;
    try {
        data = json::parse(text.str());
    } catch (const json::exception& error) {
        throw sim::DataError(name, "", std::string("not valid JSON: ") + error.what());
    }
    TutorialScript script;
    if (!data.contains("hintAfterSeconds") || !data.at("hintAfterSeconds").is_number_integer() || data.at("hintAfterSeconds").get<int>() < 1) {
        throw sim::DataError(name, "hintAfterSeconds", "must be a whole number of seconds, at least 1");
    }
    script.hintAfterSeconds = data.at("hintAfterSeconds").get<int>();
    script.elder = textField(data, "elder", name, "");
    script.done = textField(data, "done", name, "");
    if (!data.contains("steps") || !data.at("steps").is_array() || data.at("steps").empty()) {
        throw sim::DataError(name, "steps", "must be a list with at least one step");
    }
    for (std::size_t i = 0; i < data.at("steps").size(); ++i) {
        const json& step = data.at("steps")[i];
        const std::string where = "steps[" + std::to_string(i) + "]";
        script.steps.push_back({textField(step, "goal", name, where), textField(step, "say", name, where), textField(step, "hint", name, where)});
    }
    return script;
}

void Tutorial::start(const TutorialScript& script) {
    script_ = script;
    active_ = !script_.steps.empty();
    finished_ = false;
    step_ = 0;
    idleTicks_ = 0;
    closingTicks_ = 0;
}

bool Tutorial::notify(const std::string& goal) {
    if (!active_ || finished_ || script_.steps[step_].goal != goal) return false;
    idleTicks_ = 0;
    if (++step_ >= script_.steps.size()) {
        finished_ = true;
        closingTicks_ = 0;
    }
    return true;
}

void Tutorial::tick() {
    if (!active_) return;
    if (finished_) {
        if (++closingTicks_ >= kClosingSeconds * kTicksPerSecond) active_ = false;
        return;
    }
    ++idleTicks_;
}

std::string Tutorial::text() const {
    if (!active_) return {};
    if (finished_) return script_.done;
    return hinting() ? script_.steps[step_].hint : script_.steps[step_].say;
}

} // namespace odysseus::game
