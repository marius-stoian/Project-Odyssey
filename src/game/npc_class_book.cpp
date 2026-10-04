#include "game/npc_class_book.h"

#include "core/log.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <system_error>

namespace odysseus::game {

sim::rules::NpcLayer placedLayer(const PlacedCharacter& placed) {
    sim::rules::NpcLayer layer;
    if (!placed.classes.empty()) layer.classes = placed.classes;
    if (!placed.attitude.empty()) layer.attitude = placed.attitude;
    layer.tags = placed.tags;
    layer.dialogues = placed.dialogues;
    layer.allow = placed.allow;
    layer.deny = placed.deny;
    layer.extras = placed.extras;
    return layer;
}

sim::rules::ResolvedNpc NpcClassBook::resolve(const PlacedCharacter& placed) const {
    const sim::rules::NpcKind* kind = kinds_.find(placed.kind);
    return sim::rules::resolveNpc(catalog_, kind != nullptr ? &kind->layer : nullptr, placedLayer(placed), &partnerDefaults_);
}

NpcClassBook::NpcClassBook(std::filesystem::path folder, std::filesystem::path kindsFolder) : folder_(std::move(folder)), kindsFolder_(std::move(kindsFolder)) {
    catalog_ = sim::rules::NpcClassCatalog::load(folder_, report_);
    if (!kindsFolder_.empty()) kinds_ = sim::rules::NpcKindCatalog::load(kindsFolder_, report_);
    for (const sim::rules::Diagnostic& d : report_.errors) core::logWarning("NPC classes: " + d.text());
    core::logInfo(std::format("NPC classes: {} loaded from {} file(s), {} error(s)", report_.loaded, report_.filesRead, report_.errors.size()));
}

bool NpcClassBook::reload() {
    sim::rules::LoadReport report;
    sim::rules::NpcClassCatalog fresh = sim::rules::NpcClassCatalog::load(folder_, report);
    sim::rules::NpcKindCatalog freshKinds;
    if (!kindsFolder_.empty()) freshKinds = sim::rules::NpcKindCatalog::load(kindsFolder_, report);
    report_ = report; // the latest result is always the one shown
    for (const sim::rules::Diagnostic& d : report.errors) core::logWarning("NPC classes: " + d.text());
    if (!report.errors.empty()) {
        core::logWarning(std::format("NPC classes: reload found {} mistake(s); the last good classes stay in use", report.errors.size()));
        return false;
    }
    catalog_ = std::move(fresh);
    kinds_ = std::move(freshKinds);
    return true;
}

std::optional<std::string> NpcClassBook::save(const sim::rules::NpcClass& npcClass) {
    // The text is checked before it is written, so a mistake never reaches the disk.
    sim::rules::LoadReport check;
    const std::string text = sim::rules::toJson(npcClass);
    if (!sim::rules::NpcClassCatalog::parse(text, "npc-classes/" + npcClass.id + ".json", check, npcClass.id)) {
        return check.errors.empty() ? std::string("the class is not valid") : check.errors.front().text();
    }
    std::error_code ec;
    std::filesystem::create_directories(folder_, ec);
    const std::filesystem::path file = folder_ / (npcClass.id + ".json");
    const std::filesystem::path temporary = std::filesystem::path(file.string() + ".tmp");
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return std::format("{} cannot be written", file.generic_string());
        out << text;
        out.flush();
        if (!out) return std::format("{} could not be written completely (is the disk full?)", file.generic_string());
    }
    std::filesystem::rename(temporary, file, ec);
    if (ec) return std::format("{} cannot be replaced: {}", file.generic_string(), ec.message());
    sim::rules::LoadReport report;
    catalog_ = sim::rules::NpcClassCatalog::load(folder_, report);
    report_ = report;
    return std::nullopt;
}

std::optional<std::string> NpcClassBook::saveKind(const sim::rules::NpcKind& kind) {
    if (kindsFolder_.empty()) return std::string("there is no folder for kind files");
    sim::rules::LoadReport check;
    const std::string text = sim::rules::toJson(kind);
    if (!sim::rules::NpcKindCatalog::parse(text, "npcs/" + kind.kind + ".json", check, kind.kind)) {
        return check.errors.empty() ? std::string("the kind is not valid") : check.errors.front().text();
    }
    std::error_code ec;
    std::filesystem::create_directories(kindsFolder_, ec);
    const std::filesystem::path file = kindsFolder_ / (kind.kind + ".json");
    const std::filesystem::path temporary = std::filesystem::path(file.string() + ".tmp");
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) return std::format("{} cannot be written", file.generic_string());
        out << text;
        out.flush();
        if (!out) return std::format("{} could not be written completely (is the disk full?)", file.generic_string());
    }
    std::filesystem::rename(temporary, file, ec);
    if (ec) return std::format("{} cannot be replaced: {}", file.generic_string(), ec.message());
    sim::rules::LoadReport report;
    kinds_ = sim::rules::NpcKindCatalog::load(kindsFolder_, report);
    return std::nullopt;
}

std::vector<std::string> NpcClassBook::usersOf(const std::string& id, const Level& level) {
    std::vector<std::string> names;
    for (const PlacedCharacter& placed : level.characters) {
        if (std::find(placed.classes.begin(), placed.classes.end(), id) != placed.classes.end()) names.push_back(placed.name);
    }
    return names;
}

std::optional<std::string> NpcClassBook::remove(const std::string& id, const Level& level) {
    const std::vector<std::string> users = usersOf(id, level);
    if (!users.empty()) {
        std::string names;
        for (const std::string& name : users) names += (names.empty() ? "" : ", ") + name;
        return std::format("{} is still used by {}", id, names);
    }
    std::error_code ec;
    std::filesystem::remove(folder_ / (id + ".json"), ec);
    if (ec) return std::format("{} cannot be deleted: {}", id, ec.message());
    sim::rules::LoadReport report;
    catalog_ = sim::rules::NpcClassCatalog::load(folder_, report);
    report_ = report;
    return std::nullopt;
}

} // namespace odysseus::game
