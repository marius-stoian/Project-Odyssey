#include "sim/partner_types.h"

#include "sim/data.h"
#include "sim/economy.h"
#include "sim/json_data.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>

namespace odysseus::sim::rules {

namespace {

std::vector<std::string>& registered() {
    static std::vector<std::string> types = {"player", "animal", "environment"};
    return types;
}

} // namespace

const std::vector<std::string>& partnerTypeNames() { return registered(); }

void setPartnerTypes(const std::vector<std::string>& types) {
    std::vector<std::string> out = {"player", "animal", "environment"};
    for (const std::string& type : types) {
        if (std::find(out.begin(), out.end(), type) == out.end()) out.push_back(type);
    }
    registered() = out;
}

bool validPartnerType(const std::string& type) {
    const std::vector<std::string>& known = registered();
    if (std::find(known.begin(), known.end(), type) != known.end()) return true;
    return type.size() > 6 && type.compare(0, 6, "class:") == 0 && validItemId(type.substr(6));
}

std::vector<std::string> loadPartnerTypes(const std::filesystem::path& file) {
    using nlohmann::json;
    const json data = readJsonFile(file);
    if (requireInt(data, file, "version", 1, 1) != 1) throw DataError(file, "version", "must be 1");
    std::vector<std::string> types = {"player", "animal", "environment"};
    if (!data.contains("types") || !data.at("types").is_array()) throw DataError(file, "types", "must be a list of partner type words");
    for (const json& type : data.at("types")) {
        if (!type.is_string() || !validItemId(type.get<std::string>())) throw DataError(file, "types", "must hold words in quotes (lower-case letters, digits and -)");
        const std::string word = type.get<std::string>();
        if (word == "class") throw DataError(file, "types", "\"class\" is the default for every NPC class and cannot be a type: the classes come as class:<id>");
        if (std::find(types.begin(), types.end(), word) == types.end()) types.push_back(word);
    }
    return types;
}

PartnerDefaults loadPartnerDefaults(const std::filesystem::path& folder, LoadReport& report) {
    PartnerDefaults defaults;
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) return defaults;
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        const std::string name = entry.path().filename().string();
        if (entry.is_regular_file() && entry.path().extension() == ".json" && name.rfind("defaults-", 0) == 0) files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const std::filesystem::path& file : files) {
        ++report.filesRead;
        const std::string name = folder.filename().generic_string() + "/" + file.filename().generic_string();
        std::ifstream in(file, std::ios::binary);
        if (!in) {
            report.errors.push_back({name, 0, "the file cannot be read"});
            continue;
        }
        std::stringstream text;
        text << in.rdbuf();
        const JsonParseResult parsed = parseJson(text.str());
        if (!parsed.value || !parsed.value->isObject()) {
            report.errors.push_back({name, parsed.value ? parsed.value->line : parsed.errorLine, parsed.value ? "the file must hold one {...} object" : parsed.error});
            continue;
        }
        if (const std::vector<Diagnostic> mistakes = schemaDiagnostics(name, text.str()); !mistakes.empty()) { // the schema (US-190)
            report.errors.insert(report.errors.end(), mistakes.begin(), mistakes.end());
            continue;
        }
        const JsonValue& root = *parsed.value;
        const std::size_t before = report.errors.size();
        for (std::size_t i = 0; i < root.keys.size(); ++i) {
            if (root.keys[i] != "partnerType" && root.keys[i] != "actions") report.errors.push_back({name, root.keyLines[i], "unknown field \"" + root.keys[i] + "\" (known: partnerType, actions)"});
        }
        const JsonValue* type = root.find("partnerType");
        const std::string expected = file.stem().string().substr(std::string("defaults-").size());
        if (type == nullptr || !type->isString() || type->text.empty()) {
            report.errors.push_back({name, type != nullptr ? type->line : root.line, "partnerType must be the partner type this file is for, in quotes"});
        } else if (type->text != expected) {
            report.errors.push_back({name, type->line, "partnerType \"" + type->text + "\" must match the file name \"" + expected + "\""});
        }
        std::vector<std::string> actions;
        if (const JsonValue* list = root.find("actions")) {
            if (!list->isArray()) {
                report.errors.push_back({name, list->line, "actions must be a list of interaction ids"});
            } else {
                for (const JsonValue& item : list->items) {
                    if (!item.isString() || !validItemId(item.text)) report.errors.push_back({name, item.line, "actions must hold interaction ids in quotes"});
                    else actions.push_back(item.text);
                }
            }
        } else {
            report.errors.push_back({name, root.line, "missing field \"actions\""});
        }
        if (report.errors.size() != before) continue;
        defaults.actions[expected] = actions;
        ++report.loaded;
    }
    return defaults;
}

} // namespace odysseus::sim::rules
