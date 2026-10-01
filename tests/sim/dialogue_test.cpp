// US-160: the .dlg conversation format: parser, errors, canonical writer.
#include "core/random.h"
#include "sim/dialogue_script.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;

namespace {

fs::path dialogueDir() { return fs::path(ODYSSEUS_DATA_DIR) / "dialogue"; }
fs::path guideFile() { return fs::path(ODYSSEUS_DATA_DIR).parent_path().parent_path() / "docs" / "guides" / "dialogue-format.md"; }

std::string readAll(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::optional<rules::DlgScript> parse(const std::string& text, rules::LoadReport& report, const std::string& name = "x") {
    return rules::parseDialogue(text, name, "dialogue/" + name + ".dlg", report);
}

// The messages of a script with mistakes, "line: message".
std::vector<std::string> errorsOf(const std::string& text) {
    rules::LoadReport report;
    const auto script = parse(text, report);
    CHECK_FALSE(script.has_value());
    std::vector<std::string> out;
    for (const auto& d : report.errors) out.push_back(std::to_string(d.line) + ": " + d.message);
    return out;
}

const char* const kAcFixture =
    "# note\n"                         // 1
    "@who elder\n"                     // 2
    "@priority 10\n"                   // 3
    "\n"                               // 4
    "=== start\n"                      // 5
    "Elder: Hello.\n"                  // 6
    "Elder: You walk like a hunter.\n" // 7
    "-> Leave => END\n"                // 8
    "-> Ask about the hunt => hunts\n" // 9
    "\n"                               // 10
    "=== hunt\n"                       // 11
    "Elder: The deer were near the river.\n";

} // namespace

TEST_CASE("US-160 elder-fire.dlg has three nodes, a conditional line and three choices that link to them") {
    rules::LoadReport report;
    const auto script = parse(readAll(dialogueDir() / "elder-fire.dlg"), report, "elder-fire");
    for (const auto& d : report.errors) MESSAGE(d.text());
    REQUIRE(script.has_value());
    CHECK(report.warnings.empty());
    CHECK(script->who == std::vector<std::string>{"elder"});
    CHECK(script->whenSource == "opinion(npc, hero) >= -20");
    CHECK(script->priority == 10);
    REQUIRE(script->headerNotes.size() == 1);
    REQUIRE(script->nodes.size() == 3);
    CHECK(script->startNode() == &script->nodes[0]);

    const rules::DlgNode& start = script->nodes[0];
    CHECK(start.id == "start");
    REQUIRE(start.lines.size() == 2);
    CHECK(start.lines[0].speaker == "Elder");
    CHECK(start.lines[0].text == "The fire is low tonight, {hero}.");
    CHECK(start.lines[0].conditionSource == "time == night"); // the conditional line
    CHECK(start.lines[1].conditionSource.empty());
    REQUIRE(start.choices.size() == 3);
    CHECK(start.choices[0].text == "Ask about the hunt");
    CHECK(start.choices[0].target == "hunt");
    CHECK(script->find("hunt") != nullptr);
    CHECK(start.choices[1].text == "Offer berries");
    CHECK(start.choices[1].conditionSource == "has(hero, berries, 1)");
    CHECK(start.choices[1].elseText == "You have no berries");
    REQUIRE(start.choices[1].effects.size() == 3);
    CHECK(start.choices[1].effects[0].source == "take hero berries 1");
    CHECK(start.choices[1].effects[2].source == "remember npc \"{hero} shared berries\" 20");
    CHECK(start.choices[1].target == "thanks");
    CHECK(start.choices[1].notes.size() == 1); // the note above it is attached to it
    CHECK(start.choices[2].target == "END");
    CHECK(script->nodes[1].lines[0].text == "{smalltalk.hunt}");
    CHECK(script->nodes[1].choices[0].target == "start");
}

TEST_CASE("US-160 A choice to a missing node on line 9 is named exactly") {
    rules::LoadReport report;
    const auto script = rules::parseDialogue(kAcFixture, "elder-fire", "dialogue/elder-fire.dlg", report);
    CHECK_FALSE(script.has_value());
    REQUIRE(report.errors.size() == 1);
    CHECK(report.errors[0].text() == "dialogue/elder-fire.dlg:9: unknown node \"hunts\"");
}

TEST_CASE("US-160 Every shipped .dlg is written back as the same text, notes included") {
    rules::LoadReport library;
    const rules::DialogueLibrary scripts = rules::DialogueLibrary::load(dialogueDir(), library);
    for (const auto& d : library.errors) MESSAGE(d.text());
    REQUIRE(library.errors.empty());
    REQUIRE(library.loaded >= 1);
    for (const fs::directory_entry& entry : fs::directory_iterator(dialogueDir())) {
        if (entry.path().extension() != ".dlg") continue;
        CAPTURE(entry.path().filename().string());
        const std::string text = readAll(entry.path());
        rules::LoadReport report;
        const auto script = parse(text, report, entry.path().stem().string());
        REQUIRE(script.has_value());
        const std::string written = rules::writeDialogue(*script);
        CHECK(written == text);
        rules::LoadReport again;
        const auto reread = parse(written, again, entry.path().stem().string());
        REQUIRE(reread.has_value());
        CHECK(rules::writeDialogue(*reread) == written);
    }
}

TEST_CASE("US-160 The reader is relaxed about spacing and line ends, the writer is exact") {
    const std::string loose =
        "@who   elder   hunter\r\n"
        "@priority 3\r\n"
        "=== start\r\n"
        "   Elder:    Hello there.   [if   time == night   ]\r\n"
        "->   Go on   [if has(hero,berries,1)]   [else Nothing to give]   { take hero berries 1 ;opinion npc hero +5 }   =>   next\r\n"
        "=== next\r\n"
        "Elder: Good.";
    rules::LoadReport report;
    const auto script = parse(loose, report);
    for (const auto& d : report.errors) MESSAGE(d.text());
    REQUIRE(script.has_value());
    CHECK(script->who == std::vector<std::string>{"elder", "hunter"});
    CHECK(script->nodes[0].lines[0].text == "Hello there.");
    CHECK(script->nodes[0].lines[0].conditionSource == "time == night");
    REQUIRE(script->nodes[0].choices.size() == 1);
    CHECK(script->nodes[0].choices[0].text == "Go on");
    REQUIRE(script->nodes[0].choices[0].effects.size() == 2);
    CHECK(script->nodes[0].choices[0].effects[1].source == "opinion npc hero +5");
    CHECK(rules::writeDialogue(*script) ==
          "@who elder hunter\n@priority 3\n\n=== start\nElder: Hello there.   [if time == night]\n"
          "-> Go on [if has(hero,berries,1)] [else Nothing to give] {take hero berries 1; opinion npc hero +5} => next\n\n=== next\nElder: Good.\n");
}

TEST_CASE("US-160 Every kind of mistake is named with its line") {
    const std::string ok = "=== start\nElder: Hi.\n-> Bye => END\n";
    {
        rules::LoadReport report;
        CHECK(parse(ok, report).has_value());
    }
    CHECK(errorsOf("Elder: hi\n") == std::vector<std::string>{"1: this line is before the first node (start one with \"=== start\")", "1: the script has no nodes (start one with \"=== start\")"});
    CHECK(errorsOf("# only a note\n") == std::vector<std::string>{"1: the script has no nodes (start one with \"=== start\")"});
    CHECK(errorsOf("=== start\nElder: Hi.\n-> Bye\n") == std::vector<std::string>{"3: a choice needs \"=> node\" or \"=> END\" at its end"});
    CHECK(errorsOf("=== start\nElder: Hi.\n-> Bye => two words\n") == std::vector<std::string>{"3: after \"=>\" write the name of a node, or END"});
    CHECK(errorsOf("=== start\n=== start\n")[0] == "2: the node \"start\" is defined twice");
    CHECK(errorsOf("=== start\nElder: Hi.\n-> Bye => nowhere\n") == std::vector<std::string>{"3: unknown node \"nowhere\""});
    CHECK(errorsOf("=== two words\nElder: Hi.\n")[0].find("1: a node needs a name") == 0);
    CHECK(errorsOf("@who\n=== start\nElder: Hi.\n")[0].find("1: @who needs a character") == 0);
    CHECK(errorsOf("@when has(\n=== start\nElder: Hi.\n")[0].find("1: ") == 0);
    CHECK(errorsOf("@priority high\n=== start\nElder: Hi.\n")[0].find("1: @priority needs a whole number") == 0);
    CHECK(errorsOf("@mood sad\n=== start\nElder: Hi.\n")[0].find("1: unknown header \"@mood\"") == 0);
    CHECK(errorsOf("=== start\n@who elder\nElder: Hi.\n")[0] == "2: headers (@who, @when...) must come before the first node");
    CHECK(errorsOf("=== start\nElder Hi there\n")[0].find("2: expected \"Speaker: words\"") == 0);
    CHECK(errorsOf("=== start\n-> Bye => END\nElder: Late.\n")[0] == "3: a line the character says must come before the choices of its node");
    CHECK(errorsOf("=== start\nElder: Hi. [if has(]\n")[0].find("2: ") == 0);
    CHECK(errorsOf("=== start\nElder: Hi. [maybe]\n")[0] == "2: a bracket at the end of a line must be [if condition]");
    CHECK(errorsOf("=== start\nElder: Hi {nobody}.\n")[0].find("2: unknown token \"{nobody}\"") == 0);
    CHECK(errorsOf("=== start\nElder: Hi {hero.\n")[0] == "2: a '{' is never closed");
    CHECK(errorsOf("=== start\nElder: Hi.\n-> Bye {giv hero berries 1} => END\n")[0] == "3: unknown effect verb \"giv\"");
    CHECK(errorsOf("=== start\nElder: Hi.\n-> Bye [else Never] => END\n")[0] == "3: [else ...] needs an [if ...] before it");
    CHECK(errorsOf("=== start\nElder: Hi.\n-> Bye [when now] => END\n")[0] == "3: a bracket in a choice must be [if condition] or [else reason]");
    CHECK(errorsOf("=== start\nElder: Hi.\n-> => END\n")[0].find("3: a choice needs its words") == 0);
    CHECK(errorsOf("=== start\nElder: Hi.\n-> A [if 1] [if 2] => END\n")[0] == "3: a choice has one [if ...]");
    CHECK(errorsOf("=== start\nElder: \n")[0] == "2: the character says nothing here");
}

TEST_CASE("US-160 At most five choices fit the panel, and a node nobody can reach is warned about") {
    std::string six = "=== start\nElder: Pick.\n";
    for (int i = 1; i <= 6; ++i) six += "-> Choice " + std::to_string(i) + " => END\n";
    CHECK(errorsOf(six) == std::vector<std::string>{"1: node \"start\" has 6 choices; the panel shows at most 5"});
    std::string five = "=== start\nElder: Pick.\n";
    for (int i = 1; i <= 5; ++i) five += "-> Choice " + std::to_string(i) + " => END\n";
    rules::LoadReport report;
    CHECK(parse(five, report).has_value());

    rules::LoadReport warned;
    const auto script = parse("=== start\nElder: Hi.\n-> Bye => END\n\n=== lost\nElder: Nobody comes here.\n", warned);
    REQUIRE(script.has_value()); // it loads
    REQUIRE(warned.warnings.size() == 1);
    CHECK(warned.warnings[0].text() == "dialogue/x.dlg:5: node \"lost\" is never reached");
}

TEST_CASE("US-160 The conversation begins at start, or at the first node") {
    rules::LoadReport report;
    const auto named = parse("=== other\nA: x\n=== start\nA: y\n", report);
    REQUIRE(named.has_value());
    CHECK(named->startNode()->id == "start");
    const auto plain = parse("=== first\nA: x\n=== second\nA: y\n", report);
    REQUIRE(plain.has_value());
    CHECK(plain->startNode()->id == "first");
}

TEST_CASE("US-160 A folder loads script by script: a bad one is left out and the rest load") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us160" / "dialogue";
    fs::remove_all(folder.parent_path());
    fs::create_directories(folder);
    { std::ofstream(folder / "good.dlg", std::ios::binary) << "=== start\nA: x\n-> Bye => END\n"; }
    { std::ofstream(folder / "bad.dlg", std::ios::binary) << "=== start\nA: x\n-> Bye => nowhere\n"; }
    { std::ofstream(folder / "notes.txt", std::ios::binary) << "not a script"; }
    rules::LoadReport report;
    const rules::DialogueLibrary library = rules::DialogueLibrary::load(folder, report);
    CHECK(report.filesRead == 2);
    CHECK(report.loaded == 1);
    REQUIRE(report.errors.size() == 1);
    CHECK(report.errors[0].text() == "dialogue/bad.dlg:3: unknown node \"nowhere\"");
    CHECK(library.find("good") != nullptr);
    CHECK(library.find("bad") == nullptr);
    rules::LoadReport missing;
    rules::DialogueLibrary::load(folder / "nowhere", missing);
    CHECK(missing.errors.size() == 1);
}

TEST_CASE("US-160 Damaged scripts never crash the parser") {
    const std::string original = readAll(dialogueDir() / "elder-fire.dlg");
    REQUIRE_FALSE(original.empty());
    odysseus::core::Pcg32 random(160, 1);
    const std::string junk = "{}[]=>-@#:;\"\n abcxyz01()";
    int loaded = 0;
    int rejected = 0;
    for (int round = 0; round < 300; ++round) {
        std::string text = original;
        const int edits = 1 + static_cast<int>(random.below(4));
        for (int e = 0; e < edits && !text.empty(); ++e) {
            const std::size_t at = random.below(static_cast<std::uint32_t>(text.size()));
            switch (random.below(4)) {
            case 0: text.erase(at, 1 + random.below(10)); break;
            case 1: text[at] = junk[random.below(static_cast<std::uint32_t>(junk.size()))]; break;
            case 2: text.insert(at, 1, junk[random.below(static_cast<std::uint32_t>(junk.size()))]); break;
            default: text.insert(at, text.substr(at, 1 + random.below(30))); break;
            }
        }
        rules::LoadReport report;
        const auto script = parse(text, report, "elder-fire");
        if (script) {
            ++loaded;
            CHECK(report.errors.empty());
            // Whatever loaded can be written and read again.
            rules::LoadReport again;
            REQUIRE(parse(rules::writeDialogue(*script), again, "elder-fire").has_value());
        } else {
            ++rejected;
            CHECK_FALSE(report.errors.empty());
        }
    }
    CHECK(loaded + rejected == 300);
    CHECK(rejected > 50);
}

TEST_CASE("US-160 Guide: every header, line type and the brief's example are explained") {
    const std::string guide = readAll(guideFile());
    REQUIRE_FALSE(guide.empty());
    for (const char* word : {"@who", "@when", "@priority", "@bark", "@pair", "=== ", "[if ", "[else ", "=> END", "{smalltalk.", "# "}) {
        CAPTURE(word);
        CHECK(guide.find(word) != std::string::npos);
    }
    CHECK(guide.find(readAll(dialogueDir() / "elder-fire.dlg")) != std::string::npos); // the example is the shipped file, whole
}
