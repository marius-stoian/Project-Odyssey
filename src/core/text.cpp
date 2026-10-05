#include "core/text.h"

#include <format>
#include <fstream>
#include <iterator>

namespace odysseus::core {

namespace fs = std::filesystem;

std::optional<std::string> readTextFile(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return std::nullopt;
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::optional<std::string> writeTextFileSafely(const fs::path& file, std::string_view text, int backups) {
    std::error_code ec;
    if (file.has_parent_path()) fs::create_directories(file.parent_path(), ec);
    const fs::path temporary = fs::path(file).concat(".tmp");
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return std::format("{} cannot be written", temporary.generic_string());
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        out.flush();
        if (!out) return std::format("{} could not be written completely (is the disk full?)", temporary.generic_string());
    } // closed here, before the rename
    if (backups > 0) {
        const auto backup = [&](int number) { return fs::path(file).concat(".bak" + std::to_string(number)); };
        fs::remove(backup(backups), ec);
        for (int number = backups - 1; number >= 1; --number) {
            if (fs::exists(backup(number), ec)) fs::rename(backup(number), backup(number + 1), ec);
        }
        if (fs::exists(file, ec)) fs::rename(file, backup(1), ec);
    }
    fs::rename(temporary, file, ec); // the new file appears in one step
    if (ec) {
        std::error_code ignored;
        fs::remove(temporary, ignored);
        return std::format("{} cannot be replaced: {}", file.generic_string(), ec.message());
    }
    return std::nullopt;
}

std::string lowered(std::string text) {
    for (char& c : text) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return text;
}

std::vector<std::string> splitWords(std::string_view text) {
    std::vector<std::string> words;
    const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v'; };
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && space(text[i])) ++i;
        const std::size_t start = i;
        while (i < text.size() && !space(text[i])) ++i;
        if (i > start) words.emplace_back(text.substr(start, i - start));
    }
    return words;
}

std::string joined(const std::vector<std::string>& words, std::string_view separator) {
    std::string out;
    for (const std::string& word : words) {
        if (!out.empty()) out += separator;
        out += word;
    }
    return out;
}

std::string replaceAll(std::string text, std::string_view from, std::string_view to) {
    if (from.empty()) return text;
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) text.replace(at, from.size(), to);
    return text;
}

} // namespace odysseus::core
