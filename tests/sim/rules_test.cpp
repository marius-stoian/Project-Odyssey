// US-150: the rule language (conditions and effects) and the interaction files.
#include "core/random.h"
#include "sim/interaction.h"
#include "sim/rule_effect.h"
#include "sim/rule_expr.h"
#include "sim/rule_json.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;

namespace {

fs::path dataDir() { return fs::path(ODYSSEUS_DATA_DIR); }
fs::path interactionsDir() { return dataDir() / "interactions"; }
fs::path guideFile() { return dataDir().parent_path().parent_path() / "docs" / "guides" / "interaction-data.md"; }

std::string readAll(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

fs::path freshFolder(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us150" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

// A world made of small tables: paths and function answers are set by the test.
class Table : public rules::RuleContext {
public:
    std::map<std::string, rules::Value> paths;
    std::map<std::string, long long> answers; // "has(berries,2)" -> 1
    mutable int calls = 0;

    rules::Value path(const std::string& dotted) const override {
        const auto it = paths.find(dotted);
        return it == paths.end() ? rules::Value::ofNumber(0) : it->second;
    }
    rules::Value call(const std::string& name, const std::vector<rules::Value>& args) const override {
        ++calls;
        std::string key = name + "(";
        for (std::size_t i = 0; i < args.size(); ++i) key += (i ? "," : "") + (args[i].isText ? args[i].text : std::to_string(args[i].number));
        key += ")";
        const auto it = answers.find(key);
        return rules::Value::ofNumber(it == answers.end() ? 0 : it->second);
    }
};

long long run(const std::string& source, const rules::RuleContext& context) {
    const rules::ParsedExpr parsed = rules::parseExpression(source);
    REQUIRE_MESSAGE(parsed.root != nullptr, source, ": ", (parsed.problem ? parsed.problem->message : std::string("no root")));
    return rules::evaluate(*parsed.root, context).number;
}

std::string problemOf(const std::string& source) {
    const rules::ParsedExpr parsed = rules::parseExpression(source);
    return parsed.problem ? parsed.problem->message : std::string();
}

// The example interaction of the design brief (US-150's "Load" scenario): a range, a duration, two conditions, three effects.
// The shipped gather.json has since been made to do exactly what the game did before (US-152), so this keeps the original here.
const char* const kBriefGather = R"(// Gather from a plant that is ripe.
{
  "id": "gather",
  "label": "Gather {target.name}",
  "note": "The first interaction written as data.",
  "actors": ["hero", "person"],
  "target": { "tags": ["edible", "plant"] },
  "range": 1.5,        // metres
  "duration": 3.0,     // seconds
  "requires": [
    { "if": "season != winter",     "else": "Nothing grows in winter" },
    { "if": "target.state == ripe", "else": "Nothing to pick yet" }
  ],
  "effects": [
    "give actor berries 2",
    "set target.state picked",
    "after 15s set target.state ripe"
  ],
  "npc": { "score": "need(hunger) * 2 + trait(diligent) * 10", "cooldown": 60 }
})";

const char* const kLine12Fixture =
    "{\n"                                              // 1
    "  \"id\": \"gather\",\n"                          // 2
    "  \"label\": \"Gather {target.name}\",\n"         // 3
    "  \"actors\": [\"hero\"],\n"                      // 4
    "  \"target\": { \"tags\": [\"edible\"] },\n"      // 5
    "  \"range\": 1.5,\n"                              // 6
    "  \"duration\": 3,\n"                             // 7
    "  \"requires\": [\n"                              // 8
    "    { \"if\": \"season != winter\", \"else\": \"No\" }\n" // 9
    "  ],\n"                                           // 10
    "  \"effects\": [\n"                               // 11
    "    \"giv actor berries 2\",\n"                   // 12
    "    \"set target.state picked\"\n"                // 13
    "  ]\n"                                            // 14
    "}\n";                                             // 15

} // namespace

TEST_CASE("US-150 Operators and precedence") {
    const Table world;
    CHECK(run("1 + 2 * 3", world) == 7);
    CHECK(run("(1 + 2) * 3", world) == 9);
    CHECK(run("10 - 2 - 3", world) == 5);   // left to right
    CHECK(run("10 / 3", world) == 3);        // whole numbers
    CHECK(run("-3 + 5", world) == 2);
    CHECK(run("2 * -3", world) == -6);
    CHECK(run("+4", world) == 4);
    CHECK(run("7 / 0", world) == 0);         // never a crash
    CHECK(run("1 < 2", world) == 1);
    CHECK(run("2 <= 1", world) == 0);
    CHECK(run("3 >= 3", world) == 1);
    CHECK(run("1 + 1 == 2", world) == 1);    // + binds tighter than ==
    CHECK(run("not 0 and 1", world) == 1);   // not binds tighter than and
    CHECK(run("not (0 and 1)", world) == 1);
    CHECK(run("1 or 0 and 0", world) == 1);  // and binds tighter than or
    CHECK(run("(1 or 0) and 0", world) == 0);
    CHECK(run("not not 5", world) == 1);
}

TEST_CASE("US-150 Words, paths and text") {
    Table world;
    world.paths["season"] = rules::Value::ofText("autumn");
    world.paths["target.state"] = rules::Value::ofText("ripe");
    world.paths["time"] = rules::Value::ofText("night");
    world.paths["distance"] = rules::Value::ofNumber(3);
    CHECK(run("season != winter", world) == 1);
    CHECK(run("season == autumn", world) == 1);
    CHECK(run("target.state == ripe", world) == 1);
    CHECK(run("target.state == \"ripe\"", world) == 1);
    CHECK(run("time == night and distance < 5", world) == 1);
    CHECK(run("wild-berry == wild-berry", world) == 1); // a hyphen inside a word
    CHECK(run("winter == 3", world) == 0);              // different kinds are simply different
    CHECK(run("winter != 3", world) == 1);
    CHECK(run("winter < summer", world) == 0);          // only numbers are ordered
    CHECK(run("winter + 1", world) == 0);
}

TEST_CASE("US-150 Functions reach the world") {
    Table world;
    world.answers["has(berries,2)"] = 1;
    world.answers["has(hero,berries,2)"] = 1;
    world.answers["need(hunger)"] = 80;
    world.answers["skill(hunter)"] = 3;
    world.answers["trait(diligent)"] = 1;
    world.answers["opinion(0,1)"] = -20;
    world.answers["kin(0,1)"] = 1;
    world.answers["flag(met-elder)"] = 1;
    world.answers["tag(0,edible)"] = 1;
    world.paths["npc"] = rules::Value::ofNumber(0);
    world.paths["hero"] = rules::Value::ofNumber(1);
    world.paths["target"] = rules::Value::ofNumber(0);
    CHECK(run("has(berries, 2)", world) == 1);
    CHECK(run("has(hero, berries, 2)", world) == 0); // "hero" is the number 1 here, so the table has no such answer
    CHECK(run("need(hunger) * 2 + trait(diligent) * 10", world) == 170);
    CHECK(run("skill(hunter) >= 3", world) == 1);
    CHECK(run("opinion(npc, hero) >= -20", world) == 1);
    CHECK(run("kin(npc, hero)", world) == 1);
    CHECK(run("flag(met-elder) and tag(target, edible)", world) == 1);
}

TEST_CASE("US-150 and and or stop early") {
    Table world;
    CHECK(run("0 and has(berries, 1)", world) == 0);
    CHECK(world.calls == 0);
    CHECK(run("1 or has(berries, 1)", world) == 1);
    CHECK(world.calls == 0);
    CHECK(run("1 and has(berries, 1)", world) == 0);
    CHECK(world.calls == 1);
}

TEST_CASE("US-150 Big numbers never overflow") {
    const Table world;
    CHECK(run("1000000000 * 1000000000 * 1000000000 * 1000000000", world) > 0);
    CHECK(run("-1000000000 * 1000000000 * 1000000000 * 1000000000", world) < 0);
    CHECK(run("1000000000 + 1000000000 + 1000000000", world) == 3'000'000'000LL);
    CHECK(problemOf("1000000001").find("bigger than") != std::string::npos);
}

TEST_CASE("US-150 Expression mistakes say what is wrong") {
    CHECK(problemOf("hass(berries, 1)") == "unknown function \"hass\"");
    CHECK(problemOf("has(berries)").find("has() takes 2 to 3 argument(s), not 1") != std::string::npos);
    CHECK(problemOf("need()").find("need() takes 1 argument(s), not 0") != std::string::npos);
    CHECK(problemOf("targt.state == ripe").find("unknown name \"targt.state\"") != std::string::npos);
    CHECK(problemOf("1 +").find("ends where a value was expected") != std::string::npos);
    CHECK(problemOf("(1 + 2").find("closing ')'") != std::string::npos);
    CHECK(problemOf("a = b").find("use '=='") != std::string::npos);
    CHECK(problemOf("a ! b").find("use 'not'") != std::string::npos);
    CHECK(problemOf("and 1").find("wrong place") != std::string::npos);
    CHECK(problemOf("1 2").find("unexpected") != std::string::npos);
    CHECK(problemOf("\"open").find("never ends") != std::string::npos);
    CHECK(problemOf("1.5").find("unexpected character '.'") != std::string::npos);
    CHECK(problemOf("").find("empty") != std::string::npos);
    CHECK(problemOf("3s").find("time units") != std::string::npos);
    CHECK(problemOf(std::string(500, '1')).find("longer than") != std::string::npos);
    CHECK(problemOf(std::string(40, '(') + "1" + std::string(40, ')')).find("nested too deeply") != std::string::npos);
    CHECK(problemOf("1 + 1").empty());
}

TEST_CASE("US-150 Every effect verb has a working example") {
    // The one-line example of each verb, taken from the table the guide is checked against.
    const std::map<std::string, std::string> examples = {
        {"give", "give actor berries 2"},
        {"take", "take hero berries 1"},
        {"set", "set target.state picked"},
        {"flag", "flag met-elder"},
        {"quest", "quest start first-day"},
        {"opinion", "opinion npc hero +5"},
        {"remember", "remember npc \"shared berries\" 20"},
        {"start", "start inspect target"},
        {"talk", "talk elder-fire"},
        {"say", "say \"Hello, {hero.name}\""},
        {"fx", "fx leaves"},
        {"sound", "sound pop"},
        {"do", "do give-berries"},
        {"after", "after 15s set target.state ripe"},
        {"chronicle", "chronicle \"{actor.name} shared berries\""},
    };
    for (const rules::VerbInfo& verb : rules::knownVerbs()) {
        CAPTURE(verb.name);
        REQUIRE(examples.count(verb.name) == 1);
        const rules::ParsedEffect parsed = rules::parseEffect(examples.at(verb.name));
        CHECK_MESSAGE(!parsed.problem, (parsed.problem ? parsed.problem->message : std::string()));
        CHECK(parsed.effect.verb == verb.name);
    }
    CHECK(rules::knownVerbs().size() == examples.size());
}

TEST_CASE("US-150 Effect arguments and delays") {
    const rules::ParsedEffect give = rules::parseEffect("give actor berries (1 + skill(gatherer))");
    REQUIRE_FALSE(give.problem);
    CHECK(give.effect.args.size() == 3);
    CHECK(give.effect.argSources[2] == "(1 + skill(gatherer))");

    const rules::ParsedEffect later = rules::parseEffect("after 2m after 10s set target.state ripe");
    REQUIRE_FALSE(later.problem);
    CHECK(later.effect.delayAmount == 2);
    CHECK(later.effect.delayUnit == rules::DelayUnit::Minutes);
    REQUIRE(later.effect.inner != nullptr);
    CHECK(later.effect.inner->delayAmount == 10);
    REQUIRE(later.effect.inner->inner != nullptr);
    CHECK(later.effect.inner->inner->verb == "set");

    const rules::ParsedEffect days = rules::parseEffect("after 1d flag famine");
    REQUIRE_FALSE(days.problem);
    CHECK(days.effect.delayUnit == rules::DelayUnit::Days);
}

TEST_CASE("US-150 Effect mistakes say what is wrong") {
    const auto message = [](const std::string& line) {
        const rules::ParsedEffect parsed = rules::parseEffect(line);
        return parsed.problem ? parsed.problem->message : std::string();
    };
    CHECK(message("giv actor berries 2") == "unknown effect verb \"giv\"");
    CHECK(message("give actor berries").find("give takes 3 argument(s), not 2") != std::string::npos);
    CHECK(message("set picked x").find("something to change") != std::string::npos);
    CHECK(message("set target.state").find("set takes 2") != std::string::npos);
    CHECK(message("give berries berries 2").find("who first") != std::string::npos);
    CHECK(message("opinion npc 5 5").find("two people") != std::string::npos);
    CHECK(message("after soon set target.state ripe").find("is not a time") != std::string::npos);
    CHECK(message("after 15s").find("needs a time and an effect") != std::string::npos);
    CHECK(message("after 15s giv actor berries 2").find("unknown effect verb \"giv\"") != std::string::npos);
    CHECK(message("say \"open").find("never ends") != std::string::npos);
    CHECK(message("give actor (berries 2").find("never closed") != std::string::npos);
    CHECK(message("give actor berries has(2").find("never closed") != std::string::npos);
    CHECK(message("").find("empty") != std::string::npos);
    CHECK(message("give actor berries hass(2)").find("unknown function") != std::string::npos);
    CHECK(message("after 1s after 1s after 1s after 1s after 1s after 1s set target.state x").find("nested too deeply") != std::string::npos);
    CHECK(message("give actor berries 2").empty());
}

TEST_CASE("US-150 The JSON reader knows its lines") {
    const rules::JsonParseResult ok = rules::parseJson("// first note\n{\n  \"a\": 1, /* inline */ \"b\": [true,\n null, \"x\\n\\u0041\"]\n}\n");
    REQUIRE(ok.value.has_value());
    const rules::JsonValue& root = *ok.value;
    REQUIRE(root.isObject());
    CHECK(root.keys[0] == "a");
    CHECK(root.keyLines[0] == 3);
    const rules::JsonValue* b = root.find("b");
    REQUIRE(b != nullptr);
    CHECK(b->items.size() == 3);
    CHECK(b->items[1].line == 4);
    CHECK(b->items[2].text == "x\nA");

    const auto failure = [](const std::string& text) { return rules::parseJson(text); };
    CHECK(failure("{ \"a\": 1\n \"b\": 2 }").errorLine == 2);      // a comma is missing
    CHECK(failure("{ \"a\": 1,\n}").error.find("extra comma") != std::string::npos);
    CHECK(failure("[1,\n 2,\n]").errorLine == 3);
    CHECK(failure("{ \"a\": 1, \"a\": 2 }").error.find("appears twice") != std::string::npos);
    CHECK(failure("{ \"a\": 1e5 }").error.find("exponents") != std::string::npos);
    CHECK(failure("/* never ends").error.find("never ends") != std::string::npos);
    CHECK(failure("\"open\n\"").error.find("next line") != std::string::npos);
    CHECK(failure("{ \"a\": 1 } x").error.find("after the end") != std::string::npos);
    CHECK(failure("").error.find("ends where a value") != std::string::npos);
    CHECK(failure(std::string(100, '[')).error.find("too deeply") != std::string::npos);
}

TEST_CASE("US-150 Decimals are whole thousandths") {
    CHECK(rules::parseMilli("1.5") == 1500);
    CHECK(rules::parseMilli("3") == 3000);
    CHECK(rules::parseMilli("0.25") == 250);
    CHECK(rules::parseMilli("-2.125") == -2125);
    CHECK_FALSE(rules::parseMilli("1.2345").has_value());
    CHECK_FALSE(rules::parseMilli("1.").has_value());
    CHECK_FALSE(rules::parseMilli("abc").has_value());
    CHECK(rules::formatMilli(1500) == "1.5");
    CHECK(rules::formatMilli(3000) == "3");
    CHECK(rules::formatMilli(250) == "0.25");
}

TEST_CASE("US-150 Load: gather.json is registered and offered on edible plants") {
    const fs::path folder = freshFolder("brief-gather") / "interactions";
    fs::create_directories(folder);
    { std::ofstream(folder / "gather.json", std::ios::binary) << kBriefGather; }
    rules::LoadReport report;
    const rules::InteractionRegistry registry = rules::InteractionRegistry::load(folder, report);
    for (const rules::Diagnostic& d : report.errors) MESSAGE(d.text());
    REQUIRE(report.errors.empty());
    REQUIRE(registry.all().size() >= 1);
    const rules::Interaction* gather = registry.find("gather");
    REQUIRE(gather != nullptr);
    CHECK(gather->file == "interactions/gather.json");
    CHECK(gather->label == "Gather {target.name}");
    CHECK(gather->rangeMilli == 1500);
    CHECK(gather->durationMilli == 3000);
    CHECK(gather->requires_.size() == 2);
    CHECK(gather->effects.size() == 3);
    REQUIRE(gather->npc.has_value());
    CHECK(gather->npc->cooldownSeconds == 60);

    const rules::ThingInfo hero{"hero", {"hero", "person"}};
    const rules::ThingInfo bush{"bush", {"edible", "plant"}};
    const rules::ThingInfo rock{"rock", {"mineral"}};

    Table world;
    world.paths["season"] = rules::Value::ofText("summer");
    world.paths["target.state"] = rules::Value::ofText("ripe");

    auto offers = registry.offered(hero, bush, 1000, world);
    REQUIRE(offers.size() >= 1);
    CHECK(offers[0].interaction->id == "gather");
    CHECK(offers[0].enabled);

    CHECK(registry.offered(hero, rock, 1000, world).empty()); // not edible

    offers = registry.offered(hero, bush, 4000, world);
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "Too far away");

    world.paths["target.state"] = rules::Value::ofText("picked");
    offers = registry.offered(hero, bush, 1000, world);
    CHECK_FALSE(offers[0].enabled);
    CHECK(offers[0].reason == "Nothing to pick yet");

    world.paths["target.state"] = rules::Value::ofText("ripe");
    world.paths["season"] = rules::Value::ofText("winter");
    offers = registry.offered(hero, bush, 1000, world);
    CHECK(offers[0].reason == "Nothing grows in winter");

    const rules::ThingInfo deer{"deer", {"animal"}};
    CHECK(registry.offered(deer, bush, 1000, world).empty()); // gather is for the hero and people
}

TEST_CASE("US-150 Error: an unknown verb on line 12, and the rest still loads") {
    const fs::path folder = freshFolder("line12") / "interactions";
    fs::create_directories(folder);
    { std::ofstream(folder / "gather.json", std::ios::binary) << kLine12Fixture; }
    fs::copy_file(interactionsDir() / "gather.json", folder / "gather-ok.json");
    { // the copy's id must match its file name
        std::string text = readAll(folder / "gather-ok.json");
        text.replace(text.find("\"id\": \"gather\""), 14, "\"id\": \"gather-ok\"");
        std::ofstream(folder / "gather-ok.json", std::ios::binary) << text;
    }

    rules::LoadReport report;
    const rules::InteractionRegistry registry = rules::InteractionRegistry::load(folder, report);
    REQUIRE(report.errors.size() == 1);
    CHECK(report.errors[0].text() == "interactions/gather.json:12: unknown effect verb \"giv\"");
    CHECK(report.filesRead == 2);
    CHECK(report.loaded == 1);
    CHECK(registry.find("gather") == nullptr); // the broken file is left out
    CHECK(registry.find("gather-ok") != nullptr);
}

TEST_CASE("US-150 Error: every kind of mistake is named with its line") {
    const auto errorsOf = [](const std::string& text, const std::string& expectedId = "x") {
        rules::LoadReport report;
        const auto result = rules::InteractionRegistry::parse(text, "interactions/x.json", report, expectedId);
        CHECK_FALSE(result.has_value());
        return report.errors;
    };
    const std::string good =
        "{\n \"id\": \"x\",\n \"label\": \"L\",\n \"actors\": [\"hero\"],\n \"target\": { \"tags\": [\"t\"] },\n"
        " \"effects\": [\"fx a\"]\n}";
    {
        rules::LoadReport report;
        CHECK(rules::InteractionRegistry::parse(good, "interactions/x.json", report, "x").has_value());
        CHECK(report.errors.empty());
    }
    const auto replaced = [&](const std::string& from, const std::string& to) {
        std::string text = good;
        text.replace(text.find(from), from.size(), to);
        return text;
    };
    auto e = errorsOf("{ \"id\": \"x\" \n \"label\": 1 }");
    REQUIRE(e.size() == 1);
    CHECK(e[0].line == 2);
    CHECK(e[0].message.find("not valid JSON") == 0);

    e = errorsOf(replaced("\"effects\"", "\"efects\""));
    CHECK(e.size() >= 1);
    bool sawUnknown = false;
    for (const auto& d : e) sawUnknown = sawUnknown || (d.line == 6 && d.message.find("unknown field \"efects\"") != std::string::npos);
    CHECK(sawUnknown);

    e = errorsOf(replaced("\"hero\"", "\"he ro\""));
    REQUIRE(e.size() == 1);
    CHECK(e[0].line == 4);

    e = errorsOf(replaced("{ \"tags\": [\"t\"] }", "{ }"));
    REQUIRE(e.size() == 1);
    CHECK(e[0].message.find("tags or kinds") != std::string::npos);

    e = errorsOf(replaced("\"label\": \"L\"", "\"label\": \"Gather {tragte.name}\""));
    REQUIRE(e.size() == 1);
    CHECK(e[0].message.find("unknown token") != std::string::npos);

    e = errorsOf(replaced("\"effects\": [\"fx a\"]", "\"effects\": [\"fx a\", \"set target.state\"]"));
    REQUIRE(e.size() == 1);
    CHECK(e[0].message.find("set takes 2") != std::string::npos);

    e = errorsOf(replaced("\"effects\": [\"fx a\"]", "\"effects\": [], \"range\": 99"));
    CHECK(e.size() == 2); // empty effects and a range of 99 m
    e = errorsOf(replaced("\"effects\": [\"fx a\"]", "\"effects\": [\"fx a\"], \"requires\": [{ \"if\": \"target.state ==\" }]"));
    REQUIRE(e.size() == 1);
    CHECK(e[0].message.find("ends where a value was expected") != std::string::npos);

    e = errorsOf(good, "other");
    REQUIRE(e.size() == 1);
    CHECK(e[0].message.find("must match the file name \"other\"") != std::string::npos);

    e = errorsOf("[1, 2]");
    REQUIRE(e.size() == 1);
    CHECK(e[0].message.find("one {...} object") != std::string::npos);

    e = errorsOf(replaced("\"effects\": [\"fx a\"]", "\"effects\": [\"fx a\"], \"duration\": \"long\""));
    REQUIRE(e.size() == 1);
    CHECK(e[0].message.find("duration must be a number") != std::string::npos);

    e = errorsOf(replaced("\"effects\": [\"fx a\"]", "\"effects\": [\"fx a\"], \"npc\": { \"score\": \"need(x) +\" }"));
    REQUIRE(e.size() == 1);
}

TEST_CASE("US-150 start must name an interaction, and unknown tags are warned about") {
    const fs::path folder = freshFolder("refs") / "interactions";
    fs::create_directories(folder);
    const auto write = [&](const std::string& id, const std::string& effect, const std::string& tag) {
        std::ofstream(folder / (id + ".json"), std::ios::binary) << "{ \"id\": \"" << id << "\", \"label\": \"L\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"" << tag
                                                                   << "\"] },\n \"effects\": [\"" << effect << "\"] }";
    };
    write("a", "start b target", "edible");
    write("b", "fx x", "edible");
    write("c", "start nowhere target", "flammable");
    rules::LoadOptions options;
    options.knownTags = {"edible"};
    rules::LoadReport report;
    const rules::InteractionRegistry registry = rules::InteractionRegistry::load(folder, report, options);
    REQUIRE(report.errors.size() == 1);
    CHECK(report.errors[0].text().find("interactions/c.json:") == 0);
    CHECK(report.errors[0].message.find("start names \"nowhere\"") != std::string::npos);
    CHECK(registry.find("a") != nullptr);
    CHECK(registry.find("b") != nullptr);
    CHECK(registry.find("c") == nullptr);
    REQUIRE(report.warnings.size() == 1);
    CHECK(report.warnings[0].message.find("unknown tag \"flammable\"") != std::string::npos);
}

TEST_CASE("US-150 Menu order: lower order first, then id") {
    const fs::path folder = freshFolder("order") / "interactions";
    fs::create_directories(folder);
    const auto write = [&](const std::string& id, int order) {
        std::ofstream(folder / (id + ".json"), std::ios::binary) << "{ \"id\": \"" << id << "\", \"label\": \"L\", \"actors\": [\"hero\"], \"target\": { \"tags\": [\"t\"] }, \"order\": "
                                                                   << order << ", \"effects\": [\"fx x\"] }";
    };
    write("b-second", 20);
    write("a-second", 20);
    write("z-first", 5);
    rules::LoadReport report;
    const rules::InteractionRegistry registry = rules::InteractionRegistry::load(folder, report);
    REQUIRE(report.errors.empty());
    REQUIRE(registry.all().size() == 3);
    CHECK(registry.all()[0].id == "z-first");
    CHECK(registry.all()[1].id == "a-second");
    CHECK(registry.all()[2].id == "b-second");
}

TEST_CASE("US-150 Every shipped interaction file loads and survives a load, save, load") {
    rules::LoadReport report;
    const rules::InteractionRegistry registry = rules::InteractionRegistry::load(interactionsDir(), report);
    for (const rules::Diagnostic& d : report.errors) MESSAGE(d.text());
    REQUIRE(report.errors.empty());
    REQUIRE(report.loaded >= 1);
    for (const rules::Interaction& interaction : registry.all()) {
        CAPTURE(interaction.id);
        const std::string written = rules::toJson(interaction);
        rules::LoadReport again;
        const auto reread = rules::InteractionRegistry::parse(written, interaction.file, again, interaction.id);
        for (const rules::Diagnostic& d : again.errors) MESSAGE(d.text());
        REQUIRE(reread.has_value());
        CHECK(rules::toJson(*reread) == written);
        CHECK(reread->rangeMilli == interaction.rangeMilli);
        CHECK(reread->durationMilli == interaction.durationMilli);
        CHECK(reread->effects.size() == interaction.effects.size());
        CHECK(reread->requires_.size() == interaction.requires_.size());
        CHECK(reread->note == interaction.note);
    }
}

TEST_CASE("US-150 Labels fill their tokens") {
    Table world;
    world.paths["target.name"] = rules::Value::ofText("bush");
    world.paths["actor.name"] = rules::Value::ofText("Tok");
    CHECK(rules::fillTokens("Gather {target.name}", world) == "Gather bush");
    CHECK(rules::fillTokens("{actor.name} greets {target.name}!", world) == "Tok greets bush!");
    CHECK(rules::fillTokens("{unknown} stays", world) == "{unknown} stays");
    CHECK(rules::fillTokens("open { brace", world) == "open { brace");
}

TEST_CASE("US-150 Guide: every function and verb is explained with an example") {
    const std::string guide = readAll(guideFile());
    REQUIRE_FALSE(guide.empty());
    for (const rules::FunctionInfo& f : rules::knownFunctions()) {
        CAPTURE(f.name);
        CHECK(guide.find(std::string("`") + f.name + "(") != std::string::npos);
        CHECK(guide.find(f.meaning) != std::string::npos);
    }
    for (const rules::VerbInfo& v : rules::knownVerbs()) {
        CAPTURE(v.name);
        CHECK(guide.find(std::string("`") + v.name + "`") != std::string::npos);
        CHECK(guide.find(v.meaning) != std::string::npos);
    }
    for (const char* name : {"time", "season", "distance"}) {
        CAPTURE(name);
        CHECK(guide.find(std::string("`") + name + "`") != std::string::npos);
    }
}

TEST_CASE("US-150 Damaged files never crash the loader") {
    // 300 deterministic mutations of the shipped file: cut, flip, insert, duplicate. Every one must either load
    // or report at least one error; none may crash or hang.
    const std::string original = readAll(interactionsDir() / "gather.json");
    REQUIRE_FALSE(original.empty());
    odysseus::core::Pcg32 random(150, 1);
    const std::string junk = "{}[]\",:/*\\ \n0123456789abcxyz.+-()";
    int loaded = 0;
    int rejected = 0;
    for (int round = 0; round < 300; ++round) {
        std::string text = original;
        const int edits = 1 + static_cast<int>(random.below(3));
        for (int e = 0; e < edits && !text.empty(); ++e) {
            const std::size_t at = random.below(static_cast<std::uint32_t>(text.size()));
            switch (random.below(4)) {
            case 0: text.erase(at, 1 + random.below(8)); break;
            case 1: text[at] = junk[random.below(static_cast<std::uint32_t>(junk.size()))]; break;
            case 2: text.insert(at, 1, junk[random.below(static_cast<std::uint32_t>(junk.size()))]); break;
            default: text.insert(at, text.substr(at, 1 + random.below(20))); break;
            }
        }
        rules::LoadReport report;
        const auto result = rules::InteractionRegistry::parse(text, "interactions/gather.json", report, "gather");
        if (result) {
            ++loaded;
            CHECK(report.errors.empty());
        } else {
            ++rejected;
            CHECK_FALSE(report.errors.empty());
        }
    }
    CHECK(loaded + rejected == 300);
    CHECK(rejected > 100); // the mutations really did damage the file
}
