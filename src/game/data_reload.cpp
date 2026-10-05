#include "game/data_reload.h"

#include <algorithm>
#include <chrono>

namespace odysseus::game {

namespace {

// The path as one comparable text: absolute, lexically normalised, forward slashes, no trailing slash.
std::string keyOf(const std::filesystem::path& path) {
    std::error_code error;
    std::filesystem::path full = std::filesystem::absolute(path, error);
    if (error) full = path;
    std::string text = full.lexically_normal().generic_string();
    while (text.size() > 1 && text.back() == '/') text.pop_back();
    return text;
}

bool watches(const std::filesystem::path& watched, const std::string& fileKey) {
    const std::string key = keyOf(watched);
    if (fileKey == key) return true;
    return fileKey.size() > key.size() && fileKey.compare(0, key.size(), key) == 0 && fileKey[key.size()] == '/'; // a file under a watched folder
}

} // namespace

void DataReload::add(DataSet set) { sets_.push_back(std::move(set)); }

std::vector<std::string> DataReload::setsFor(const std::filesystem::path& file) const {
    const std::string fileKey = keyOf(file);
    std::vector<std::string> names;
    for (const DataSet& set : sets_) {
        if (std::any_of(set.watch.begin(), set.watch.end(), [&](const std::filesystem::path& watched) { return watches(watched, fileKey); })) names.push_back(set.name);
    }
    return names;
}

ReloadOutcome DataReload::run(const DataSet& set) {
    ReloadOutcome outcome{set.name, {}};
    const auto started = std::chrono::steady_clock::now();
    if (set.reload) outcome.result = set.reload();
    outcome.result.milliseconds = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    const auto at = std::find_if(last_.begin(), last_.end(), [&](const ReloadOutcome& old) { return old.set == set.name; });
    if (at == last_.end()) last_.push_back(outcome);
    else *at = outcome;
    return outcome;
}

std::vector<ReloadOutcome> DataReload::changed(const std::filesystem::path& file) {
    std::vector<ReloadOutcome> outcomes;
    const std::vector<std::string> names = setsFor(file);
    const auto live = [&](const std::string& name) {
        return std::any_of(sets_.begin(), sets_.end(), [&](const DataSet& set) { return set.name == name && !set.restartOnly; });
    };
    const bool anyLive = std::any_of(names.begin(), names.end(), live);
    for (const std::string& name : names) {
        for (const DataSet& set : sets_) {
            if (set.name != name) continue;
            if (set.restartOnly && anyLive) continue; // a file a live set reads is not "next start" just because a folder says so
            ReloadOutcome outcome = run(set);
            outcome.file = file;
            outcomes.push_back(std::move(outcome));
        }
    }
    return outcomes;
}

std::vector<ReloadOutcome> DataReload::reload(const std::string& name) {
    std::vector<ReloadOutcome> outcomes;
    for (const DataSet& set : sets_) {
        if (set.name == name) outcomes.push_back(run(set));
    }
    return outcomes;
}

std::vector<ReloadOutcome> DataReload::reloadAll() {
    std::vector<ReloadOutcome> outcomes;
    for (const DataSet& set : sets_) {
        if (!set.restartOnly) outcomes.push_back(run(set));
    }
    return outcomes;
}

const ReloadOutcome* DataReload::last(const std::string& name) const {
    const auto at = std::find_if(last_.begin(), last_.end(), [&](const ReloadOutcome& outcome) { return outcome.set == name; });
    return at == last_.end() ? nullptr : &*at;
}

} // namespace odysseus::game
