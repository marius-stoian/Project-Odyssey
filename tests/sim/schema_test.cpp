#include "core/text.h"
#include "sim/building_data.h"
#include "sim/calendar.h"
#include "sim/hero_data.h"
#include "sim/interaction.h"
#include "sim/json_data.h"
#include "sim/json_text.h"
#include "sim/needs.h"
#include "sim/npc_class.h"
#include "sim/npc_events.h"
#include "sim/npc_kind.h"
#include "sim/npc_schedule.h"
#include "sim/opinion.h"
#include "sim/partner_types.h"
#include "sim/quest_data.h"
#include "sim/region.h"
#include "sim/schema.h"
#include "sim/schema_index.h"
#include "sim/smalltalk.h"
#include "sim/trade_market.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <string>

namespace sim = odysseus::sim;
namespace schema = odysseus::sim::schema;
namespace fs = std::filesystem;
using nlohmann::json;

namespace {

// A schema made from text, for the small cases.
schema::NodePtr nodeOf(const std::string& text) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us190";
    fs::create_directories(folder);
    odysseus::core::writeTextFileSafely(folder / "index.json", R"({"files":[{"match":"x.json","schema":"x"}]})");
    odysseus::core::writeTextFileSafely(folder / "x.schema.json", text);
    const schema::SchemaSet set = schema::SchemaSet::load(folder);
    return set.schema("x")->root;
}

schema::Report checkText(const schema::Node& root, const std::string& text) {
    const json data = json::parse(text, nullptr, true, true);
    const sim::JsonLines lines = sim::JsonLines::scan(text);
    return schema::check(root, data, &lines);
}

bool says(const schema::Report& report, const std::string& words) {
    return std::any_of(report.issues.begin(), report.issues.end(), [&](const schema::Issue& issue) { return (issue.path + ": " + issue.message).find(words) != std::string::npos; });
}

fs::path dataCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us190-data" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy(ODYSSEUS_DATA_DIR, folder, fs::copy_options::recursive);
    return folder;
}

const fs::path& dataFolder() {
    static const fs::path folder = ODYSSEUS_DATA_DIR;
    return folder;
}

const schema::SchemaSet& shipped() {
    static const schema::SchemaSet set = schema::SchemaSet::load(dataFolder() / "schemas");
    return set;
}

} // namespace

TEST_CASE("US-190 Lines") {
    const std::string text = "// a comment\n{\n  \"a\": 1,\n  /* block\n comment */\n  \"list\": [\n    {\"x\": 1},\n    {\"x\": 2}\n  ],\n  \"deep\": {\"inner\": {\n    \"leaf\": true}}\n}\n";
    const sim::JsonLines lines = sim::JsonLines::scan(text);
    CHECK(lines.lineOf("a") == 3);
    CHECK(lines.lineOf("list") == 6);
    CHECK(lines.lineOf("list[0].x") == 7);
    CHECK(lines.lineOf("list[1].x") == 8);
    CHECK(lines.lineOf("deep.inner.leaf") == 11);
    CHECK(lines.lineOf("list[5].x") == 6);    // an unknown path falls back to the nearest known parent
    CHECK(lines.lineOf("nothing.here") == 2);  // only the whole document is known: its first line
}

TEST_CASE("US-190 Subset") {
    const schema::NodePtr root = nodeOf(R"({
      "type": "object", "required": ["name"],
      "properties": {
        "name": {"type": "string", "maxLength": 5},
        "count": {"type": "integer", "minimum": 1, "maximum": 9},
        "speed": {"type": "number", "minimum": 0.5},
        "kind": {"type": "string", "enum": ["a", "b"]},
        "colour": {"type": "string", "format": "colour"},
        "list": {"type": "array", "minItems": 1, "maxItems": 2, "items": {"type": "string", "ref": "catalog:things"}},
        "table": {"type": "object", "additionalProperties": {"type": "integer", "maximum": 3}}
      }})");
    CHECK_FALSE(checkText(*root, R"({"name": "ok", "count": 5})").hasErrors());
    CHECK(says(checkText(*root, R"({"count": 5})"), "name: is missing"));
    CHECK(says(checkText(*root, R"({"name": "toolong"})"), "name: is too long"));
    CHECK(says(checkText(*root, R"({"name": "x", "count": 0})"), "count: must be between 1 and 9 (is 0)"));
    CHECK(says(checkText(*root, R"({"name": "x", "count": 2.5})"), "count: must be a whole number (is a number)"));
    CHECK(says(checkText(*root, R"({"name": "x", "speed": 0.1})"), "speed: must be at least 0.5 (is 0.1)"));
    CHECK(says(checkText(*root, R"({"name": "x", "kind": "c"})"), "kind: must be one of: a, b (is c)"));
    CHECK(says(checkText(*root, R"({"name": "x", "colour": "red"})"), "colour: must be a colour such as #3a9a4c"));
    CHECK(says(checkText(*root, R"({"name": "x", "list": []})"), "list: needs at least 1 entries"));
    CHECK(says(checkText(*root, R"({"name": "x", "table": {"a": 1, "b": 7}})"), "table.b: must be at most 3 (is 7)"));
    CHECK(says(checkText(*root, R"({"name": 5})"), "name: must be text (is a number)"));

    // A field the schema does not know is a warning (a typo), never an error; note fields are always welcome.
    const schema::Report typo = checkText(*root, R"({"name": "x", "colur": 1})");
    CHECK_FALSE(typo.hasErrors());
    CHECK(says(typo, "colur: is not a field this file knows"));
    CHECK(checkText(*root, R"({"name": "x", "note": "hello", "_why": 1, "// c": 2})").issues.empty());

    // A link is recorded with its path and line; the catalog check happens in the index.
    const schema::Report links = checkText(*root, "{\"name\": \"x\",\n \"list\": [\"axe\", \"bow\"]}");
    REQUIRE(links.refs.size() == 2);
    CHECK(links.refs[1].catalog == "things");
    CHECK(links.refs[1].value == "bow");
    CHECK(links.refs[1].path == "list[1]");
    CHECK(links.refs[1].line == 2);
}

TEST_CASE("US-190 Provided") {
    const json data = json::parse(R"({"items": [{"id": "a"}, {"id": "b", "tags": ["x", "y"]}], "materials": {"flint": {}, "wood": {}}, "tags": ["p", "q"]})");
    const auto ids = schema::collectProvided(data, "items[].id");
    REQUIRE(ids.size() == 2);
    CHECK(ids[1].value == "b");
    CHECK(ids[1].path == "items[1].id");
    CHECK(schema::collectProvided(data, "items[].tags[]").size() == 2);
    const auto keys = schema::collectProvided(data, "materials.*");
    REQUIRE(keys.size() == 2);
    CHECK(keys[0].value == "flint");
    CHECK(schema::collectProvided(data, "tags").size() == 2);
    CHECK(schema::collectProvided(data, "nothing[].here").empty());
}

TEST_CASE("US-190 Globs") {
    CHECK(schema::globMatches("weapons.json", "weapons.json"));
    CHECK(schema::globMatches("interactions/*.json", "interactions/gather.json"));
    CHECK_FALSE(schema::globMatches("interactions/*.json", "interactions/sub/gather.json"));
    CHECK(schema::globMatches("interactions/defaults-*.json", "interactions/defaults-animal.json"));
    CHECK_FALSE(schema::globMatches("interactions/defaults-*.json", "interactions/gather.json"));
    CHECK_FALSE(schema::globMatches("a.json", "b.json"));
}

TEST_CASE("US-190 Coverage") {
    // Every file under assets/data/ has a schema and passes it; every link names an entry of a catalog.
    const schema::DataIndex index = schema::buildIndex(dataFolder(), shipped());
    std::string report;
    for (const schema::FileIssue& issue : index.issues) report += "\n  " + issue.text();
    for (const schema::FileIssue& issue : index.brokenLinks) report += "\n  " + issue.text();
    INFO("problems:" << report);
    CHECK(index.files.size() > 150);
    CHECK(index.issues.empty());
    CHECK(index.brokenLinks.empty());
    CHECK(index.clean());
    CHECK(index.catalogs.at("weapons").count("iron sword") == 1);
    CHECK(index.catalogs.at("interactions").count("gather") == 1);
    CHECK(index.catalogs.at("dialogues").count("greet-elder") == 1);
}

TEST_CASE("US-190 Help text") {
    // The help of every field of a form comes from its schema (K-M11 step 1): a field without a description is a mistake.
    std::string missing;
    std::function<void(const schema::Node&, const std::string&, const std::string&)> walk = [&](const schema::Node& node, const std::string& schemaName, const std::string& path) {
        if (!path.empty() && node.description.empty()) missing += "\n  " + schemaName + ": " + path;
        for (const schema::Property& property : node.properties) walk(*property.node, schemaName, path.empty() ? property.name : path + "." + property.name);
        if (node.items) walk(*node.items, schemaName, path + "[]");
        if (node.additional) walk(*node.additional, schemaName, path + ".*");
    };
    for (const schema::Schema& entry : shipped().schemas()) walk(*entry.root, entry.name, "");
    INFO("fields without a description:" << missing);
    CHECK(missing.empty());
}

TEST_CASE("US-190 Errors") {
    // A value out of its range stops the load and the message names file, line, field and the allowed range.
    const fs::path folder = dataCopy("errors");
    const fs::path needs = folder / "sim" / "needs.json";
    std::string text = *odysseus::core::readTextFile(needs);
    const std::size_t at = text.find("\"maximum\"");
    REQUIRE(at != std::string::npos);
    text.replace(at, text.find_first_of(",\n", at) - at, "\"maximum\": 5");
    odysseus::core::writeTextFileSafely(needs, text);
    schema::install(std::make_shared<schema::SchemaSet>(schema::SchemaSet::load(folder / "schemas")), folder);
    std::string message;
    try {
        (void)sim::readJsonFile(needs);
    } catch (const sim::DataError& error) {
        message = error.what();
    }
    schema::uninstall();
    INFO(message);
    CHECK(message.find("needs.json:") != std::string::npos);
    const std::size_t afterName = message.find("needs.json:");
    REQUIRE(afterName != std::string::npos);
    CHECK(std::isdigit(static_cast<unsigned char>(message[afterName + 11])) != 0); // a line number follows the file name
    CHECK(message.find("maximum") != std::string::npos);
    CHECK(message.find("must be between 10 and 1000 (is 5)") != std::string::npos);
}

TEST_CASE("US-190 Links") {
    // A recipe naming an item no catalog has is listed with its file and line.
    const fs::path folder = dataCopy("links");
    const fs::path recipes = folder / "hero" / "recipes.json";
    std::string text = *odysseus::core::readTextFile(recipes);
    const std::size_t at = text.find("\"output\": \"spear\"");
    REQUIRE(at != std::string::npos);
    text.replace(at, 17, "\"output\": \"no-such-item\"");
    odysseus::core::writeTextFileSafely(recipes, text);
    const schema::SchemaSet set = schema::SchemaSet::load(folder / "schemas");
    const schema::DataIndex index = schema::buildIndex(folder, set);
    REQUIRE_FALSE(index.brokenLinks.empty());
    const schema::FileIssue& broken = index.brokenLinks.front();
    CHECK(broken.file == "hero/recipes.json");
    CHECK(broken.issue.message.find("no-such-item") != std::string::npos);
    CHECK(broken.issue.message.find("catalog \"items\"") != std::string::npos);
    CHECK(broken.issue.line > 0);
    CHECK(broken.issue.path.find("output") != std::string::npos);
    CHECK_FALSE(index.clean());
}

TEST_CASE("US-190 Loaders") {
    // With the schemas installed every shipped file loads through its loader exactly as before, and every loader has the schema's eyes on it.
    schema::install(std::make_shared<schema::SchemaSet>(shipped()), dataFolder());
    const fs::path data = dataFolder();
    const auto none = [](const sim::rules::LoadReport& report) {
        std::string text;
        for (const sim::rules::Diagnostic& problem : report.errors) text += "\n  " + problem.text();
        return text;
    };
    CHECK_NOTHROW((void)sim::loadSimConfig(data));
    CHECK_NOTHROW((void)sim::loadHeroData(data));
    CHECK_NOTHROW((void)sim::loadCalendarConfig(data / "sim" / "calendar.json"));
    CHECK_NOTHROW((void)sim::loadNeedsConfig(data / "sim" / "needs.json"));
    CHECK_NOTHROW((void)sim::loadOpinionConfig(data / "sim" / "opinions.json"));
    CHECK_NOTHROW((void)sim::loadPriceConfig(data / "sim" / "trade.json"));
    CHECK_NOTHROW((void)sim::rules::loadScheduleConfig(data / "sim" / "schedule.json"));
    CHECK_NOTHROW((void)sim::loadEventCatalog(data / "sim" / "events.json"));
    CHECK_NOTHROW((void)sim::loadRegionConfig(data / "sim" / "region.json"));
    CHECK_NOTHROW((void)sim::rules::loadPartnerTypes(data / "sim" / "partner-types.json"));
    {
        sim::rules::LoadReport report;
        const sim::rules::InteractionRegistry registry = sim::rules::InteractionRegistry::load(data / "interactions", report);
        INFO("interactions:" << none(report));
        CHECK(report.errors.empty());
        CHECK(registry.all().size() > 50);
    }
    {
        sim::rules::LoadReport report;
        (void)sim::rules::NpcClassCatalog::load(data / "npc-classes", report);
        (void)sim::rules::NpcKindCatalog::load(data / "npcs", report);
        (void)sim::rules::loadPartnerDefaults(data / "interactions", report);
        (void)sim::rules::loadQuests(data / "quests", report);
        (void)sim::rules::SmalltalkData::load(data / "dialogue" / "smalltalk.json", "dialogue/smalltalk.json", report);
        (void)sim::buildings::BuildingData::load(data / "buildings", report);
        INFO("rule files:" << none(report));
        CHECK(report.errors.empty());
    }
    schema::uninstall();
}

TEST_CASE("US-190 Rule files") {
    // The rule files report a schema mistake as "file:line: field: problem" and are left out, like any other mistake of theirs.
    const fs::path folder = dataCopy("rules");
    const fs::path gather = folder / "interactions" / "gather.json";
    std::string text = *odysseus::core::readTextFile(gather);
    const std::size_t at = text.find("\"range\": 2");
    REQUIRE(at != std::string::npos);
    text.replace(at, 10, "\"range\": \"far\"");
    odysseus::core::writeTextFileSafely(gather, text);
    schema::install(std::make_shared<schema::SchemaSet>(schema::SchemaSet::load(folder / "schemas")), folder);
    sim::rules::LoadReport report;
    const sim::rules::InteractionRegistry registry = sim::rules::InteractionRegistry::load(folder / "interactions", report);
    schema::uninstall();
    REQUIRE_FALSE(report.errors.empty());
    CHECK(report.errors.front().file == "interactions/gather.json");
    CHECK(report.errors.front().line > 0);
    CHECK(report.errors.front().message.find("range: must be a number (is text)") != std::string::npos);
    CHECK(registry.find("gather") == nullptr); // the file with the mistake is not loaded
}

TEST_CASE("US-190 Drift") {
    // The member names each loader reads and the fields its schema describes must agree (ADR-020).
    const fs::path repo = dataFolder().parent_path().parent_path();
    const std::vector<schema::DriftProblem> problems = schema::checkDrift(shipped(), repo);
    std::string report;
    for (const schema::DriftProblem& problem : problems) report += "\n  " + (problem.schema.empty() ? std::string() : problem.schema + ": ") + problem.message;
    INFO("drift:" << report);
    CHECK(problems.empty());
}

TEST_CASE("US-190 Function bodies") {
    const std::string source = "int other() { return data.at(\"outside\"); }\n"
                               "Config Loader::load(const std::string& file) const {\n  if (x) { data.at(\"inside\"); }\n  const char* brace = \"}\";\n  return data.at(\"also\");\n}\n"
                               "void call() { load(file); }\n";
    const std::string body = schema::functionBody(source, "load");
    CHECK(body.find("inside") != std::string::npos);
    CHECK(body.find("also") != std::string::npos);
    CHECK(body.find("outside") == std::string::npos);
    CHECK(schema::functionBody(source, "missing").empty());
}

TEST_CASE("US-190 Names read by a loader") {
    const std::string source = "const auto& a = data.at(\"alpha\"); if (data.contains(\"beta\")) {} x = data[\"gamma\"]; int d = requireInt(data, file, \"delta\", 0, 9);\n"
                               "// data.at(\"commented\")\nconst std::string e = entry.value(\"epsilon\", std::string()); log(\"not a key\");";
    const std::set<std::string> keys = schema::keysReadBy(source);
    CHECK(keys.count("alpha") == 1);
    CHECK(keys.count("beta") == 1);
    CHECK(keys.count("gamma") == 1);
    CHECK(keys.count("delta") == 1);
    CHECK(keys.count("epsilon") == 1);
    CHECK(keys.count("commented") == 0);
    CHECK(keys.count("not a key") == 0);
}
