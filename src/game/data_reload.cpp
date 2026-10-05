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

// Is the key of a file the root itself, or a file under it?
bool isUnder(const std::string& key, const std::string& prefix) {
    return key == prefix || (key.size() > prefix.size() && key.compare(0, prefix.size(), prefix) == 0 && key[prefix.size()] == '/');
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

std::vector<ReloadOutcome> DataReload::changed(const std::vector<std::filesystem::path>& files) {
    std::vector<ReloadOutcome> outcomes;
    std::vector<std::string> done; // each set once, with the first file that named it
    for (const std::filesystem::path& file : files) {
        const std::vector<std::string> names = setsFor(file);
        const auto live = [&](const std::string& name) {
            return std::any_of(sets_.begin(), sets_.end(), [&](const DataSet& set) { return set.name == name && !set.restartOnly; });
        };
        const bool anyLive = std::any_of(names.begin(), names.end(), live);
        for (const std::string& name : names) {
            if (std::find(done.begin(), done.end(), name) != done.end()) continue;
            for (const DataSet& set : sets_) {
                if (set.name != name || (set.restartOnly && anyLive)) continue;
                done.push_back(name);
                ReloadOutcome outcome = run(set);
                outcome.file = file;
                outcomes.push_back(std::move(outcome));
            }
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

// ---- FileWatcher

namespace {

// Names the watcher never reports: hidden files, an editor's swap and temporary files, and the backups the game keeps itself.
bool ignoredName(const std::string& name) {
    if (name.empty() || name.front() == '.' || name.front() == '~' || name.back() == '~') return true;
    for (const char* ending : {".tmp", ".swp", ".bak", ".bak1", ".bak2", ".bak3"}) {
        const std::string suffix = ending;
        if (name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) return true;
    }
    return false;
}

} // namespace

void FileWatcher::watch(const std::filesystem::path& root) {
    inRound_ = false; // the roots changed: a round in progress starts over
    dir_.reset();
    partial_.clear();
    if (std::find(roots_.begin(), roots_.end(), root) == roots_.end()) roots_.push_back(root);
}

std::map<std::string, FileWatcher::Seen> FileWatcher::scan(const std::filesystem::path& root) const {
    std::map<std::string, Seen> found;
    std::error_code error;
    const auto add = [&](const std::filesystem::path& path, const std::filesystem::file_time_type time, const std::uintmax_t size) {
        if (!ignoredName(path.filename().string())) found[keyOf(path)] = {path, {time, size}};
    };
    if (std::filesystem::is_regular_file(root, error)) {
        add(root, std::filesystem::last_write_time(root, error), std::filesystem::file_size(root, error));
    } else if (std::filesystem::is_directory(root, error)) {
        for (std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::skip_permission_denied, error), end; !error && it != end; it.increment(error)) {
            if (!it->is_regular_file(error)) continue;
            add(it->path(), it->last_write_time(error), it->file_size(error));
        }
    }
    return found;
}

void FileWatcher::snapshot() {
    roundStart_ = -1.0;
    inRound_ = false;
    dir_.reset();
    partial_.clear();
    known_.clear();
    waiting_.clear();
    own_.clear();
    for (const std::filesystem::path& root : roots_) {
        for (const auto& [key, seen] : scan(root)) known_[key] = seen.stamp;
    }
}

std::vector<std::filesystem::path> FileWatcher::poll(double nowSeconds) {
    std::vector<std::filesystem::path> settled;
    if (roots_.empty()) return settled;
    if (nowSeconds < roundStart_) { // the clock went back (a test): start over
        inRound_ = false;
        roundStart_ = -1.0;
    }
    if (!inRound_ && (roundStart_ < 0.0 || nowSeconds - roundStart_ >= kPollSeconds)) { // a new round of looks at every file
        inRound_ = true;
        roundStart_ = nowSeconds;
        rootIndex_ = 0;
        dir_.reset();
        partial_.clear();
    }
    // A tick looks at a handful of files, not all of them: the round takes as many ticks as it needs, and the frame stays cheap (budget 0.5 ms a frame).
    for (int budget = kFilesPerTick; inRound_ && budget > 0;) look(nowSeconds, budget);
    for (auto it = waiting_.begin(); it != waiting_.end();) {
        if (nowSeconds - it->second.since >= kDebounceSeconds) {
            settled.push_back(it->second.path);
            it = waiting_.erase(it);
        } else {
            ++it;
        }
    }
    return settled;
}

// Looks at the next files of the round (each costs one unit of `budget`); when a root is done it is compared with what was known.
void FileWatcher::look(double nowSeconds, int& budget) {
    const std::filesystem::path& root = roots_[rootIndex_];
    std::error_code error;
    const auto finish = [&] {
        compare(root, partial_, nowSeconds);
        partial_.clear();
        dir_.reset();
        if (++rootIndex_ >= roots_.size()) inRound_ = false;
    };
    const auto note = [&](const std::filesystem::path& path, std::filesystem::file_time_type time, std::uintmax_t size) {
        if (!ignoredName(path.filename().string())) partial_[keyOf(path)] = {path, {time, size}};
        --budget;
    };
    if (!dir_) {
        if (std::filesystem::is_regular_file(root, error)) {
            note(root, std::filesystem::last_write_time(root, error), std::filesystem::file_size(root, error));
            finish();
            return;
        }
        if (!std::filesystem::is_directory(root, error)) { // nothing there (yet)
            finish();
            return;
        }
        dir_.emplace(root, std::filesystem::directory_options::skip_permission_denied, error);
        if (error) {
            dir_.reset();
            finish();
            return;
        }
    }
    if (*dir_ == std::filesystem::recursive_directory_iterator()) {
        finish();
        return;
    }
    const std::filesystem::directory_entry& entry = **dir_;
    if (entry.is_regular_file(error)) note(entry.path(), entry.last_write_time(error), entry.file_size(error));
    else --budget; // a folder costs a little too
    dir_->increment(error);
    if (error) finish();
}

// One root, completely looked at: what changed, appeared or went since the last look.
void FileWatcher::compare(const std::filesystem::path& root, const std::map<std::string, Seen>& current, double nowSeconds) {
    const std::string prefix = keyOf(root);
    for (const auto& [key, seen] : current) {
        const auto old = known_.find(key);
        if (old != known_.end() && old->second == seen.stamp) continue;
        known_[key] = seen.stamp;
        if (const auto own = own_.find(key); own != own_.end()) {
            const bool ours = own->second == seen.stamp;
            own_.erase(own);
            if (ours) continue; // the game wrote this one itself: ignored once
        }
        waiting_[key] = {seen.path, nowSeconds}; // a new change restarts the wait: a file written twice settles once
    }
    for (auto it = known_.begin(); it != known_.end();) { // a file of this root that went
        if (!isUnder(it->first, prefix) || current.contains(it->first)) {
            ++it;
            continue;
        }
        waiting_[it->first] = {std::filesystem::path(it->first), nowSeconds};
        own_.erase(it->first);
        it = known_.erase(it);
    }
}

void FileWatcher::noteOwnWrite(const std::filesystem::path& file) {
    std::error_code error;
    const std::filesystem::file_time_type time = std::filesystem::last_write_time(file, error);
    if (error) return;
    const std::uintmax_t size = std::filesystem::file_size(file, error);
    if (error) return;
    own_[keyOf(file)] = {time, size};
}

void FileWatcher::resync(const std::filesystem::path& root) {
    const std::string prefix = keyOf(root);
    const auto under = [&](const std::string& key) { return isUnder(key, prefix); };
    for (auto it = waiting_.begin(); it != waiting_.end();) it = under(it->first) ? waiting_.erase(it) : std::next(it);
    for (auto it = own_.begin(); it != own_.end();) it = under(it->first) ? own_.erase(it) : std::next(it);
    for (auto it = known_.begin(); it != known_.end();) it = under(it->first) ? known_.erase(it) : std::next(it);
    for (const auto& [key, seen] : scan(root)) known_[key] = seen.stamp;
}

} // namespace odysseus::game
