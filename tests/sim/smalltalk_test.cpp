// US-163: generated small talk: the templates file, what people say about what they remember and heard, and variety.
#include "core/random.h"
#include "sim/conversation.h"
#include "sim/save.h"
#include "sim/smalltalk.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;
namespace sim = odysseus::sim;

namespace {

fs::path smalltalkFile() { return fs::path(ODYSSEUS_DATA_DIR) / "dialogue" / "smalltalk.json"; }

rules::SmallTalk shipped() {
    rules::LoadReport report;
    auto data = rules::SmalltalkData::load(smalltalkFile(), "dialogue/smalltalk.json", report);
    for (const auto& d : report.errors) MESSAGE(d.text());
    REQUIRE(data.has_value());
    return rules::SmallTalk(std::move(*data));
}

// The messages a text of smalltalk.json earns, "line: message".
std::vector<std::string> errorsOf(const std::string& text) {
    rules::LoadReport report;
    const auto data = rules::SmalltalkData::parse(text, "dialogue/smalltalk.json", report);
    CHECK_FALSE(data.has_value());
    std::vector<std::string> out;
    for (const auto& d : report.errors) out.push_back(std::to_string(d.line) + ": " + d.message);
    return out;
}

bool has(const std::vector<std::string>& messages, const std::string& part) {
    return std::any_of(messages.begin(), messages.end(), [&](const std::string& m) { return m.find(part) != std::string::npos; });
}

// A valid file with every required topic and three templates each, for the tests that change one thing.
std::string fileWith(const std::string& memoryList) {
    const std::string three = "[\"One.\", \"Two.\", \"Three.\"]";
    return "{\n  \"topics\": {\n    \"memory\": " + memoryList + ",\n    \"people\": " + three + ",\n    \"needs\": " + three + ",\n    \"season\": " + three + ",\n    \"hero\": " + three + "\n  }\n}\n";
}

} // namespace

TEST_CASE("US-163 The shipped small talk loads, has every topic with at least three templates, and fits the guide") {
    rules::LoadReport report;
    const auto data = rules::SmalltalkData::load(smalltalkFile(), "dialogue/smalltalk.json", report);
    for (const auto& d : report.errors) MESSAGE(d.text());
    REQUIRE(data.has_value());
    for (const std::string& topic : rules::requiredSmalltalkTopics()) {
        CAPTURE(topic);
        REQUIRE(data->topic(topic) != nullptr);
        CHECK(data->topic(topic)->size() >= 3);
    }
    CHECK(data->topic("hunt") != nullptr); // the elder's script asks for it
    // Every template is plain and short.
    for (const auto& [name, templates] : data->topics()) {
        for (const auto& entry : templates) CHECK_MESSAGE(entry.text.size() <= 140, name, ": ", entry.text);
    }
}

TEST_CASE("US-163 Mistakes in the file are named with their line") {
    CHECK(has(errorsOf("{ }"), "needs a \"topics\" object"));
    CHECK(has(errorsOf("{ \"topics\": { \"memory\": [\"Only one.\"] } }"), "needs at least 3 templates, it has 1"));
    CHECK(has(errorsOf("{ \"topics\": { \"memory\": [\"a\", \"b\", \"c\"] } }"), "the topic \"people\" is missing"));
    // An unknown token is named, on its own line.
    const auto unknown = errorsOf(fileWith("[\n \"One.\",\n \"Two {nobody}.\",\n \"Three.\"\n]"));
    CHECK(has(unknown, "5: unknown token {nobody}"));
    // A token whose facts live in another topic.
    CHECK(has(errorsOf(fileWith("[\"One.\", \"Two.\", \"A {gossip.who}.\"]")), "{gossip.who} cannot be used in the topic \"memory\""));
    CHECK(has(errorsOf(fileWith("[\"One.\", \"Two.\", { \"text\": \"Three.\", \"mood\": [\"grumpy\"] }]")), "unknown mood \"grumpy\""));
    CHECK(has(errorsOf(fileWith("[\"One.\", \"Two.\", \"" + std::string(141, 'x') + "\"]")), "must be 1 to 140 characters"));
    CHECK(has(errorsOf(fileWith("[\"One.\", \"Two.\", 7]")), "is a quoted sentence or"));
    CHECK(has(errorsOf("{ \"topics\": "), "")); // damaged JSON is an error, not a crash
    // A good file loads.
    rules::LoadReport report;
    CHECK(rules::SmalltalkData::parse(fileWith("[\"One {memory.what}.\", \"Two.\", { \"text\": \"Three.\", \"mood\": [\"warm\"] }]"), "x", report).has_value());
    CHECK(report.errors.empty());
}

TEST_CASE("US-163 Memory: someone who saw a wolf at the fire yesterday mentions the wolf") {
    sim::World world(42, story_test::realConfig());
    world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()) * 3);
    const std::int64_t today = world.date().day;
    sim::Person* person = world.personMutable(4);
    REQUIRE(person != nullptr);
    person->memories.clear();
    person->notes.push_back({"a wolf at the fire", today - 1, -30, false});
    rules::SmallTalk talk = shipped();
    odysseus::core::Pcg32 random(1, 8);
    bool mentioned = false;
    for (int i = 0; i < 10; ++i) {
        const auto said = talk.say(world, 4, 0, random, "memory");
        REQUIRE(said.has_value());
        CHECK(said->topic == "memory");
        mentioned = said->text.find("a wolf at the fire") != std::string::npos;
        CHECK_MESSAGE(mentioned, said->text);
        CHECK(said->text.find('{') == std::string::npos); // no token left unfilled
    }
    // Without a topic asked for, a fresh memory is what they talk about most often.
    rules::SmallTalk other = shipped();
    int aboutTheWolf = 0;
    for (int i = 0; i < 40; ++i) {
        const auto said = other.say(world, 4, 0, random);
        REQUIRE(said.has_value());
        if (said->text.find("wolf") != std::string::npos) ++aboutTheWolf;
    }
    CHECK(aboutTheWolf >= 10);
    // "Yesterday" is said as such.
    rules::SmallTalk when = shipped();
    bool yesterday = false;
    for (int i = 0; i < 30 && !yesterday; ++i) yesterday = when.say(world, 4, 0, random, "memory")->text.find("yesterday") != std::string::npos;
    CHECK(yesterday);
}

TEST_CASE("US-163 Gossip: someone who heard that Bo blamed Ama repeats it and says how they feel") {
    sim::World world(42, story_test::realConfig());
    world.personMutable(2)->name = "Bo";
    world.personMutable(3)->name = "Ama";
    sim::Person* person = world.personMutable(5);
    person->memories.clear();
    person->memories.push_back({2, 3, sim::MemoryKind::Blame, world.date().day, -50, false, true, -1}); // second-hand: heard, not seen
    rules::SmallTalk talk = shipped();
    odysseus::core::Pcg32 random(1, 8);
    for (int i = 0; i < 12; ++i) {
        const auto said = talk.say(world, 5, 0, random, "people");
        REQUIRE(said.has_value());
        CHECK(said->topic == "people");
        CHECK_MESSAGE(said->text.find("Bo blaming Ama") != std::string::npos, said->text);
        CHECK_MESSAGE(said->text.find("uneasy") != std::string::npos, said->text); // -50 is uneasy
    }
    // The feeling follows the memory: a gladly heard thing is said gladly.
    person->memories[0] = {2, 3, sim::MemoryKind::Gift, world.date().day, 70, false, true, -1};
    rules::SmallTalk glad = shipped();
    const auto said = glad.say(world, 5, 0, random, "people");
    REQUIRE(said.has_value());
    CHECK(said->text.find("Bo giving Ama a gift") != std::string::npos);
    CHECK(said->text.find("delighted") != std::string::npos);
    // The hero is "you" and the speaker "me": a gift to the hero is "you".
    person->memories[0] = {2, 0, sim::MemoryKind::Gift, world.date().day, 20, false, true, -1};
    rules::SmallTalk toHero = shipped();
    CHECK(toHero.say(world, 5, 0, random, "people")->text.find("Bo giving you a gift") != std::string::npos);
}

TEST_CASE("US-163 A topic whose facts are missing falls back to the season, and the stream moves the same either way") {
    sim::World world(42, story_test::realConfig());
    sim::Person* person = world.personMutable(6);
    person->memories.clear();
    person->notes.clear();
    rules::SmallTalk talk = shipped();
    odysseus::core::Pcg32 random(1, 8);
    const auto said = talk.say(world, 6, 0, random, "memory");
    REQUIRE(said.has_value());
    CHECK(said->topic == "season");

    // Three numbers are drawn whatever is said, so the stream does not depend on what a person remembers.
    odysseus::core::Pcg32 a(9, 8);
    odysseus::core::Pcg32 b(9, 8);
    rules::SmallTalk first = shipped();
    rules::SmallTalk second = shipped();
    person->notes.push_back({"a wolf at the fire", 0, 0, false});
    first.say(world, 6, 0, a, "memory");
    person->notes.clear();
    second.say(world, 6, 0, b, "memory");
    CHECK(a.state() == b.state());
}

TEST_CASE("US-163 Variety: 50 lines from one seed, no line more than twice") {
    for (const std::uint64_t seed : {1ULL, 7ULL, 42ULL, 12345ULL}) {
        sim::World world(42, story_test::realConfig());
        story_test::runDays(world, 40); // a clan with a past: gifts, thefts, gossip
        rules::SmallTalk talk = shipped();
        odysseus::core::Pcg32 random(seed, 8);
        std::map<std::string, int> counts;
        const int clan = static_cast<int>(world.people().size());
        for (int i = 0; i < 50; ++i) {
            int npc = (i * 7 + 1) % clan; // different people, in a fixed order
            while (!world.people()[static_cast<std::size_t>(npc)].alive || npc == 0) npc = (npc + 1) % clan;
            const auto said = talk.say(world, npc, 0, random);
            REQUIRE(said.has_value());
            CHECK(said->text.find('{') == std::string::npos);
            ++counts[said->text];
            if (seed == 7) MESSAGE("sample ", i + 1, " [", said->topic, "] ", world.people()[static_cast<std::size_t>(npc)].name, ": ", said->text);
        }
        for (const auto& [line, times] : counts) CHECK_MESSAGE(times <= 2, "seed ", seed, ": \"", line, "\" said ", times, " times");
        MESSAGE("seed ", seed, ": ", counts.size(), " different lines in 50");
    }
}

TEST_CASE("US-163 Mood changes the words: a hostile person does not talk like a friend") {
    sim::World world(42, story_test::realConfig());
    rules::SmallTalk talk = shipped();
    odysseus::core::Pcg32 random(3, 8);
    world.adjustOpinion(7, 0, -100);
    std::set<std::string> hostile;
    for (int i = 0; i < 40; ++i) hostile.insert(talk.say(world, 7, 0, random, "hero")->text);
    CHECK(hostile.count("Leave me be.") == 1);
    CHECK(hostile.count("We are glad of you, " + world.people()[0].name + ".") == 0);
    world.adjustOpinion(8, 0, 100);
    std::set<std::string> warm;
    for (int i = 0; i < 40; ++i) warm.insert(talk.say(world, 8, 0, random, "hero")->text);
    CHECK(warm.count("Leave me be.") == 0);
}

TEST_CASE("US-163 {smalltalk.topic} in a script is filled once for each node, and the generated talk has a friendly and a rude answer") {
    rules::LoadReport report;
    const auto script = rules::parseDialogue("=== start\nElder: {smalltalk.hunt}\n-> Again => start\n-> Leave => END\n", "x", "dialogue/x.dlg", report);
    REQUIRE(script.has_value());
    rules::Conversation talk(*script, 0, {1, 3});
    int asked = 0;
    talk.setSmalltalk([&](const std::string& topic) {
        ++asked;
        return "line about " + topic + " " + std::to_string(asked);
    });
    struct Empty : rules::RuleContext {
        rules::Value path(const std::string&) const override { return rules::Value::ofNumber(0); }
        rules::Value call(const std::string&, const std::vector<rules::Value>&) const override { return rules::Value::ofNumber(0); }
    } world;
    CHECK(talk.view(world).lines[0].text == "line about hunt 1");
    CHECK(talk.view(world).lines[0].text == "line about hunt 1"); // the same on the next frame
    CHECK(asked == 1);
    struct Host : rules::EffectHost {
        void setState(const rules::ThingRef&, const std::string&) override {}
        void apply(const rules::Effect&, int, const rules::ThingRef&) override {}
        int ticksPerDay() const override { return 1000; }
    } host;
    rules::ActionRunner runner;
    REQUIRE(talk.choose(0, world, runner, 0, host)); // Again: the node is entered again
    CHECK(talk.view(world).lines[0].text == "line about hunt 2");

    const rules::DlgScript made = rules::smalltalkScript("Tok", "It is spring.");
    rules::Conversation chat(made, 0, {1, 3});
    const auto view = chat.view(world);
    REQUIRE(view.lines.size() == 1);
    CHECK(view.lines[0].speaker == "Tok");
    CHECK(view.lines[0].text == "It is spring.");
    REQUIRE(view.choices.size() == 2);
    CHECK(view.choices[0].text == "Thank you");
    CHECK(view.choices[1].text == "Be quiet");
    CHECK(made.nodes[0].choices[1].effects.size() == 1); // the rude answer costs opinion
    CHECK(made.nodes[0].choices[1].effects[0].source == "opinion npc hero -10");
}

TEST_CASE("US-163 What a person remembers is saved, loaded and part of the world's hash") {
    const fs::path file = fs::temp_directory_path() / "odysseus-us163" / "notes" / "clan.json";
    fs::remove_all(file.parent_path());
    fs::create_directories(file.parent_path());
    sim::World world(42, story_test::realConfig());
    const std::uint64_t plain = world.hash();
    world.personMutable(3)->notes.push_back({"a wolf at the fire", 0, -30, false});
    world.personMutable(3)->notes.push_back({"smoke over the hills", 0, 10, true});
    CHECK(world.hash() != plain); // the world's hash sees it
    const std::uint64_t before = world.hash();
    sim::saveWorld(world, file);
    sim::LoadedWorld loaded = sim::loadWorld(file, story_test::realConfig());
    REQUIRE(loaded.world.people()[3].notes.size() == 2);
    CHECK(loaded.world.people()[3].notes[0].text == "a wolf at the fire");
    CHECK(loaded.world.people()[3].notes[1].secondHand);
    CHECK(loaded.world.hash() == before);
}

TEST_CASE("US-163 Guide: every token, topic and mood of the small talk file is explained") {
    const fs::path guide = fs::path(ODYSSEUS_DATA_DIR).parent_path().parent_path() / "docs" / "guides" / "dialogue-format.md";
    std::ifstream in(guide, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    REQUIRE_FALSE(text.empty());
    for (const std::string& token : rules::smalltalkTokens()) {
        CAPTURE(token);
        CHECK(text.find("`{" + token + "}`") != std::string::npos);
    }
    for (const std::string& topic : rules::requiredSmalltalkTopics()) {
        CAPTURE(topic);
        CHECK(text.find("`" + topic + "`") != std::string::npos);
    }
    for (const char* mood : {"warm", "friendly", "neutral", "wary", "hostile", "hungry", "tired", "cold", "lonely"}) {
        CAPTURE(mood);
        CHECK(text.find(std::string("`") + mood + "`") != std::string::npos);
    }
    CHECK(text.find("smalltalk.json") != std::string::npos);
}
