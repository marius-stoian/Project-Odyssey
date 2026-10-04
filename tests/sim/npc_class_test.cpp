// US-260 NPC Classes: the class files of assets/data/npc-classes, their checks and the canonical text the Editor saves.
#include "sim/npc_class.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;

namespace {

fs::path classesDir() { return fs::path(ODYSSEUS_DATA_DIR) / "npc-classes"; }

void write(const fs::path& file, const std::string& text) {
    fs::create_directories(file.parent_path());
    std::ofstream(file, std::ios::binary) << text;
}

const char* kHealer = R"({
  "id": "healer",
  "label": "Healer",
  "colour": "#3a9a4c",
  "icon": "cross",
  "tags": ["healer"],
  "dialogues": { "player": "healer-greet.dlg", "class:guard": "healer-guard.dlg" },
  "actions": { "allow": ["talk"], "deny": ["barter"] }
})";

} // namespace

TEST_CASE("US-260 Shipped classes: the test cast loads without a mistake") {
    rules::LoadReport report;
    const rules::NpcClassCatalog catalog = rules::NpcClassCatalog::load(classesDir(), report);
    for (const auto& error : report.errors) FAIL(error.text());
    for (const char* id : {"trader", "talker", "hunter", "elder", "guard", "monster", "animal"}) {
        const rules::NpcClass* found = catalog.find(id);
        REQUIRE_MESSAGE(found != nullptr, id);
        CHECK(found->file == std::string("npc-classes/") + id + ".json");
        CHECK_FALSE(found->icon.empty());
        CHECK_FALSE(found->tags.empty());
        // Load, save, load again gives the same class.
        rules::LoadReport again;
        const auto reread = rules::NpcClassCatalog::parse(rules::toJson(*found), found->file, again, id);
        REQUIRE(reread.has_value());
        CHECK(*reread == *found);
    }
    CHECK(catalog.all().size() >= 7);
}

TEST_CASE("US-260 Format: colour, icon and canonical text round-trip") {
    rules::LoadReport report;
    const auto healer = rules::NpcClassCatalog::parse(kHealer, "npc-classes/healer.json", report, "healer");
    REQUIRE(healer.has_value());
    CHECK(report.errors.empty());
    CHECK(healer->colour == 0x3A9A4C);
    CHECK(rules::formatColour(healer->colour) == "#3a9a4c");
    CHECK(healer->dialogues.size() == 2);
    CHECK(healer->dialogues[1].first == "class:guard");
    CHECK(healer->deny == std::vector<std::string>{"barter"});
    const std::string text = rules::toJson(*healer);
    rules::LoadReport again;
    const auto reread = rules::NpcClassCatalog::parse(text, "npc-classes/healer.json", again, "healer");
    REQUIRE(reread.has_value());
    CHECK(*reread == *healer);
    CHECK(rules::toJson(*reread) == text); // the same text every time
    CHECK(text.find("\"id\"") < text.find("\"label\""));
    CHECK(text.find("\"label\"") < text.find("\"colour\""));
    CHECK(text.find("\"tags\"") < text.find("\"actions\""));

    CHECK(rules::parseColour("#FFa000") == 0xFFA000);
    CHECK_FALSE(rules::parseColour("FFa000").has_value());
    CHECK_FALSE(rules::parseColour("#12345").has_value());
    CHECK_FALSE(rules::parseColour("#12345g").has_value());
    CHECK(rules::validPartnerType("player"));
    CHECK(rules::validPartnerType("class:guard"));
    CHECK_FALSE(rules::validPartnerType("class:"));
    CHECK_FALSE(rules::validPartnerType("robot"));
    CHECK(rules::npcIconNames().size() == 24);
}

TEST_CASE("US-260 Mistakes: file, line and field are named, and the other classes load") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us260" / "npc-classes";
    fs::remove_all(folder.parent_path());
    write(folder / "healer.json", kHealer);
    write(folder / "badicon.json", "{\n  \"id\": \"badicon\",\n  \"label\": \"Bad\",\n  \"colour\": \"#ffffff\",\n  \"icon\": \"banana\",\n  \"tags\": []\n}\n");
    rules::LoadReport report;
    const rules::NpcClassCatalog catalog = rules::NpcClassCatalog::load(folder, report);
    REQUIRE(report.errors.size() == 1);
    CHECK(report.errors[0].file == "npc-classes/badicon.json");
    CHECK(report.errors[0].line == 5);
    CHECK(report.errors[0].message.find("icon") != std::string::npos);
    CHECK(report.errors[0].text().find("npc-classes/badicon.json:5:") == 0);
    CHECK(catalog.find("badicon") == nullptr);
    CHECK(catalog.find("healer") != nullptr); // the others load

    const auto problem = [&](const std::string& from, const std::string& to) {
        std::string text = kHealer;
        const auto at = text.find(from);
        REQUIRE_MESSAGE(at != std::string::npos, from);
        text.replace(at, from.size(), to);
        rules::LoadReport r;
        rules::NpcClassCatalog::parse(text, "npc-classes/healer.json", r, "healer");
        return r.errors.empty() ? std::string() : r.errors.front().text();
    };
    CHECK(problem("#3a9a4c", "green").find("colour") != std::string::npos);
    CHECK(problem("\"id\": \"healer\"", "\"id\": \"medic\"").find("must match the file name") != std::string::npos);
    CHECK(problem("\"id\": \"healer\"", "\"id\": \"Heal Er\"").find("lower-case") != std::string::npos);
    CHECK(problem("\"player\"", "\"robot\"").find("partner type") != std::string::npos);
    CHECK(problem("healer-greet.dlg", "healer-greet.txt").find(".dlg") != std::string::npos);
    CHECK(problem("\"deny\"", "\"forbid\"").find("forbid") != std::string::npos);
    CHECK(problem("\"tags\": [\"healer\"]", "\"tags\": \"healer\"").find("tags") != std::string::npos);
    CHECK(problem("\"label\": \"Healer\",", "").find("label") != std::string::npos);
    CHECK(problem("\"healer\"],", "\"healer\"], \"moods\": 3,").find("moods") != std::string::npos);

    // A missing folder is no error: there are simply no classes.
    rules::LoadReport none;
    CHECK(rules::NpcClassCatalog::load(folder / "nowhere", none).all().empty());
    CHECK(none.errors.empty());
}
