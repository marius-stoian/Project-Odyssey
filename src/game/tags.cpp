#include "game/tags.h"

#include "sim/data.h"

#include <algorithm>
#include <cctype>
#include <format>
#include <set>

namespace odysseus::game {

namespace {

bool isWord(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '-'; });
}

std::vector<std::string> readWords(const nlohmann::json& entry, const std::filesystem::path& file, const std::string& where, const std::string& field,
                                   std::vector<std::string> derived) {
    if (!entry.contains(field)) return derived;
    const nlohmann::json& list = entry.at(field);
    if (!list.is_array()) throw sim::DataError(file, where + "." + field, "must be a list of words, like [\"a\", \"b\"]");
    std::vector<std::string> out;
    std::set<std::string> seen;
    for (std::size_t i = 0; i < list.size(); ++i) {
        const std::string at = std::format("{}.{}[{}]", where, field, i);
        if (!list[i].is_string() || !isWord(list[i].get<std::string>())) {
            throw sim::DataError(file, at, "must be one word of letters, digits, '-' or '_'");
        }
        const std::string word = list[i].get<std::string>();
        if (!seen.insert(word).second) throw sim::DataError(file, at, "\"" + word + "\" is listed twice");
        out.push_back(word);
    }
    return out;
}

} // namespace

std::vector<std::string> readTags(const nlohmann::json& entry, const std::filesystem::path& file, const std::string& where, std::vector<std::string> derived) {
    return readWords(entry, file, where, "tags", std::move(derived));
}

std::vector<std::string> readStates(const nlohmann::json& entry, const std::filesystem::path& file, const std::string& where, std::vector<std::string> derived) {
    return readWords(entry, file, where, "states", std::move(derived));
}

} // namespace odysseus::game
