// US-180: quest data and runtime. US-181: objectives from world events.
#include "sim/quest_book.h"
#include "sim/quest_data.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <string>

namespace fs = std::filesystem;
namespace rules = odysseus::sim::rules;

namespace {

// The example of the brief (section 4.1), with the reward written in a form the effect language reads.
const char* const kFirstDay = R"json(// The elder's first lesson.
{
  "id": "first-day",
  "title": "The First Day",
  "giver": "none",
  "requires": ["not quest(first-day) == done"],
  "start": "gather",
  "steps": {
    "gather": { "text": "Gather berries for the clan.",
                "objective": "gather berries 3", "marker": "tag:edible",
                "hint": { "after": "120s", "text": "Berry bushes grow by the stream." },
                "next": "eat" },
    "eat":    { "text": "Eat at the clan fire.", "objective": "interact eat-berries", "marker": "object:clan-fire", "next": "fire" },
    "fire":   { "text": "Keep the fire alive.", "objective": "interact tend-fire", "next": "END",
                "branches": [ { "if": "flag(fire-out)", "to": "relight" } ] },
    "relight": { "text": "Light it again.", "objective": "interact tend-fire", "next": "END" }
  },
  "fail": ["hero.dead"],
  "rewards": ["opinion npc hero +10", "give hero flint 2"],
  "journal": "The elder taught me the first things a clan needs."
})json";

class Recorder : public rules::EffectHost {
public:
    std::vector<std::string> log;
    void setState(const rules::ThingRef&, const std::string& state) override { log.push_back("set " + state); }
    void apply(const rules::Effect& effect, int actor, const rules::ThingRef&) override { log.push_back(effect.source + " by " + std::to_string(actor)); }
    int ticksPerDay() const override { return 1000; }
};

// The world: flags and paths set by the test.
class World : public rules::RuleContext {
public:
    std::map<std::string, long long> flags;
    std::map<std::string, long long> paths;
    rules::Value path(const std::string& dotted) const override {
        const auto it = paths.find(dotted);
        return rules::Value::ofNumber(it == paths.end() ? 0 : it->second);
    }
    rules::Value call(const std::string& name, const std::vector<rules::Value>& args) const override {
        if (name == "flag" && args.size() == 1) {
            const auto it = flags.find(args[0].text);
            return rules::Value::ofNumber(it == flags.end() ? 0 : it->second);
        }
        return rules::Value::ofNumber(0);
    }
};

void replaceFirst(std::string& text, const std::string& from, const std::string& to) {
    const std::size_t at = text.find(from);
    REQUIRE(at != std::string::npos);
    text.replace(at, from.size(), to);
}

rules::Quest quest(const std::string& text, const std::string& id = "first-day") {
    rules::LoadReport report;
    auto parsed = rules::parseQuest(text, "quests/" + id + ".json", report, id);
    std::string all;
    for (const auto& e : report.errors) all += e.text() + "; ";
    REQUIRE_MESSAGE(parsed.has_value(), all);
    return *parsed;
}

std::string firstError(const std::string& text, const std::string& id = "x") {
    rules::LoadReport report;
    const auto parsed = rules::parseQuest(text, "quests/" + id + ".json", report, id);
    CHECK_FALSE(parsed.has_value());
    return report.errors.empty() ? std::string() : report.errors.front().text();
}

struct Table {
    rules::QuestBook book;
    World world;
    rules::ActionRunner runner;
    Recorder host;
    std::int64_t now = 100;
    explicit Table(const std::string& text, const std::string& id = "first-day") : book(std::vector<rules::Quest>{quest(text, id)}) {}
    std::vector<rules::QuestChange> update() { return book.update(world, now, 1000, runner, host); }
    void event(rules::QuestObjective::Kind kind, const std::string& subject, int amount = 1) { book.notify({kind, subject, amount, now}); }
};

} // namespace

TEST_CASE("US-180 Load: two steps, a branch and a reward; the quest is available once its prerequisites hold") {
    const rules::Quest q = quest(kFirstDay);
    CHECK(q.id == "first-day");
    CHECK(q.title == "The First Day");
    CHECK(q.steps.size() == 4);
    CHECK(q.steps[0].objective.kind == rules::QuestObjective::Kind::Gather);
    CHECK(q.steps[0].objective.subject == "berries");
    CHECK(q.steps[0].objective.amount == 3);
    CHECK(q.steps[0].hint->afterSeconds == 120);
    CHECK(q.steps[2].branches.size() == 1);
    CHECK(q.rewards.size() == 2);

    // A quest that someone has to give stays Available until it is started.
    std::string given = kFirstDay;
    replaceFirst(given, "\"giver\": \"none\"", "\"giver\": \"role:elder\"");
    Table t(given);
    CHECK(t.book.status("first-day") == rules::QuestStatus::Locked);
    t.update();
    CHECK(t.book.status("first-day") == rules::QuestStatus::Available);
    t.update();
    CHECK(t.book.status("first-day") == rules::QuestStatus::Available);
    CHECK(t.book.start("first-day", t.now));
    CHECK(t.book.status("first-day") == rules::QuestStatus::Active);
    CHECK(t.book.activeStep("first-day") == "gather");
    CHECK_FALSE(t.book.start("first-day", t.now)); // already active
}

TEST_CASE("US-180 A prerequisite keeps a quest locked until it holds") {
    std::string text = kFirstDay;
    replaceFirst(text, "\"not quest(first-day) == done\"", "\"flag(met-elder)\"");
    Table t(text);
    t.update();
    CHECK(t.book.status("first-day") == rules::QuestStatus::Locked);
    t.world.flags["met-elder"] = 1;
    t.update();
    CHECK(t.book.status("first-day") != rules::QuestStatus::Locked);
}

TEST_CASE("US-180 Progress: the objective is met, the next step starts and the reward comes at the end") {
    Table t(kFirstDay);
    t.update(); // available, and started at once: the giver is none
    REQUIRE(t.book.activeStep("first-day") == "gather");
    t.now += 20;
    t.event(rules::QuestObjective::Kind::Gather, "berries", 2);
    t.update();
    CHECK(t.book.activeStep("first-day") == "gather");
    CHECK(t.book.state("first-day")->progress == 2);
    t.now += 20;
    t.event(rules::QuestObjective::Kind::Gather, "berries", 1);
    const auto changes = t.update();
    CHECK(t.book.activeStep("first-day") == "eat");
    REQUIRE(changes.size() == 1);
    CHECK(changes[0].kind == rules::QuestChange::Kind::Step);
    CHECK(t.host.log.empty()); // no reward yet

    t.now += 20;
    t.event(rules::QuestObjective::Kind::Interact, "eat-berries");
    t.update();
    CHECK(t.book.activeStep("first-day") == "fire");
    t.now += 20;
    t.event(rules::QuestObjective::Kind::Interact, "tend-fire");
    t.update();
    CHECK(t.book.status("first-day") == rules::QuestStatus::Done);
    REQUIRE(t.host.log.size() == 2);
    CHECK(t.host.log[1] == "give hero flint 2 by 0");
}

TEST_CASE("US-180 A branch whose condition holds sends the quest to its step") {
    Table t(kFirstDay);
    t.update();
    t.world.flags["fire-out"] = 1;
    t.book.jumpTo("first-day", "fire", t.now);
    t.now += 20;
    t.event(rules::QuestObjective::Kind::Interact, "tend-fire");
    t.update();
    CHECK(t.book.activeStep("first-day") == "relight");
    t.now += 20;
    t.event(rules::QuestObjective::Kind::Interact, "tend-fire");
    t.update();
    CHECK(t.book.status("first-day") == rules::QuestStatus::Done);
}

TEST_CASE("US-180 Error: a step pointing to a missing step names the file and the line") {
    const std::string text = "{\n"
                             "  \"id\": \"x\",\n"
                             "  \"title\": \"X\",\n"
                             "  \"start\": \"a\",\n"
                             "  \"steps\": {\n"
                             "    \"a\": { \"text\": \"A\", \"objective\": \"talk elder\",\n"
                             "             \"next\": \"nowhere\" }\n"
                             "  }\n"
                             "}";
    CHECK(firstError(text) == "quests/x.json:7: unknown step \"nowhere\"");
}

TEST_CASE("US-180 Error: the rest of the folder still loads") {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us180" / "quests";
    fs::remove_all(folder);
    fs::create_directories(folder);
    std::ofstream(folder / "first-day.json") << kFirstDay;
    std::ofstream(folder / "bad.json") << "{ \"id\": \"bad\", \"title\": \"B\", \"start\": \"a\", \"steps\": { \"a\": { \"text\": \"A\", \"objective\": \"talk x\", \"next\": \"b\" } } }";
    std::ofstream(folder / "first-day.quest.layout.json") << "{ \"nodes\": {} }";
    rules::LoadReport report;
    const std::vector<rules::Quest> quests = rules::loadQuests(folder, report);
    CHECK(quests.size() == 1);
    CHECK(quests[0].id == "first-day");
    REQUIRE(report.errors.size() == 1);
    CHECK(report.errors[0].text() == "quests/bad.json:1: unknown step \"b\"");
}

TEST_CASE("US-180 Mistakes in a quest file are named") {
    const auto make = [](const std::string& step, const std::string& extra = "") {
        return "{ \"id\": \"x\", \"title\": \"X\", \"start\": \"a\"," + extra + " \"steps\": { \"a\": " + step + " } }";
    };
    CHECK(firstError(make("{ \"text\": \"A\", \"objective\": \"gather berries many\", \"next\": \"END\" }")).find("not a count") != std::string::npos);
    CHECK(firstError(make("{ \"text\": \"A\", \"objective\": \"dance\", \"next\": \"END\" }")).find("unknown objective") != std::string::npos);
    CHECK(firstError(make("{ \"text\": \"A\", \"next\": \"END\" }")).find("needs an \"objective\"") != std::string::npos);
    CHECK(firstError(make("{ \"text\": \"A\", \"objective\": \"wait 5\", \"next\": \"END\" }")).find("not a time") != std::string::npos);
    CHECK(firstError(make("{ \"text\": \"A\", \"objective\": \"talk x\", \"next\": \"END\", \"colour\": 1 }")).find("unknown field \"colour\"") != std::string::npos);
    CHECK(firstError(make("{ \"text\": \"A\", \"objective\": \"talk x\", \"next\": \"END\" }", " \"requires\": [\"has(\"],")).find("requires") == std::string::npos);
    CHECK(firstError("{ \"id\": \"y\", \"title\": \"X\", \"start\": \"a\", \"steps\": {} }", "x").find("must match the file name") != std::string::npos);
    CHECK(firstError("not json").find("not valid JSON") != std::string::npos);
}

TEST_CASE("US-180 A failing condition fails the quest for good") {
    Table t(kFirstDay);
    t.update();
    REQUIRE(t.book.status("first-day") == rules::QuestStatus::Active);
    t.world.paths["hero.dead"] = 1;
    const auto changes = t.update();
    CHECK(t.book.status("first-day") == rules::QuestStatus::Failed);
    REQUIRE(changes.size() == 1);
    CHECK(changes[0].kind == rules::QuestChange::Kind::Failed);
    t.world.paths["hero.dead"] = 0;
    t.update();
    CHECK(t.book.status("first-day") == rules::QuestStatus::Failed);
    CHECK(t.book.reset("first-day"));
    CHECK(t.book.status("first-day") == rules::QuestStatus::Locked);
}

TEST_CASE("US-180 Round trip: writing a quest and reading it again gives the same quest") {
    const rules::Quest q = quest(kFirstDay);
    const std::string written = rules::writeQuest(q);
    const rules::Quest again = quest(written);
    CHECK(rules::writeQuest(again) == written);
    CHECK(again.steps.size() == q.steps.size());
    CHECK(again.requires_[0].source == q.requires_[0].source);
    CHECK(again.steps[0].hint->text == q.steps[0].hint->text);
    CHECK(again.rewards[1].source == "give hero flint 2");
}

TEST_CASE("US-180 The quest state is saved and loaded; a changed clock does not matter") {
    Table t(kFirstDay);
    t.update();
    t.now += 40;
    t.event(rules::QuestObjective::Kind::Gather, "berries", 2);
    t.update();
    const std::string saved = t.book.save(t.now);

    Table other(kFirstDay);
    other.now = 5000; // a different clock
    CHECK(other.book.load(saved, other.now).empty());
    CHECK(other.book.status("first-day") == rules::QuestStatus::Active);
    CHECK(other.book.state("first-day")->progress == 2);
    CHECK(other.book.state("first-day")->stepStartTick == 5000 - (t.now - t.book.state("first-day")->stepStartTick));
    CHECK(other.book.save(other.now) == saved);

    // Progress of a quest the data no longer has is dropped with a note.
    rules::QuestBook empty;
    CHECK(empty.load(saved, 0).size() == 1);
    CHECK(other.book.load("not json", 0).size() == 1);
}

TEST_CASE("US-180 The same inputs give the same quests") {
    Table a(kFirstDay);
    Table b(kFirstDay);
    for (Table* t : {&a, &b}) {
        t->update();
        for (int i = 0; i < 5; ++i) {
            t->now += 20;
            t->event(rules::QuestObjective::Kind::Gather, "berries", 1);
            t->update();
        }
    }
    CHECK(a.book.hash() == b.book.hash());
    b.now += 20;
    b.event(rules::QuestObjective::Kind::Interact, "eat-berries");
    b.update();
    CHECK(a.book.hash() != b.book.hash());
}

TEST_CASE("US-181 Events count only after the step started, and only the objective's own kind and subject") {
    Table t(kFirstDay);
    t.update();
    const std::int64_t begin = t.now;
    t.book.notify({rules::QuestObjective::Kind::Gather, "berries", 3, begin - 5}); // before the step: ignored
    t.event(rules::QuestObjective::Kind::Gather, "nuts", 3);                      // another item
    t.event(rules::QuestObjective::Kind::Craft, "berries", 3);                    // another kind
    t.update();
    CHECK(t.book.state("first-day")->progress == 0);
    t.now += 1;
    t.event(rules::QuestObjective::Kind::Gather, "berries", 5); // more than needed counts as exactly done
    t.update();
    CHECK(t.book.activeStep("first-day") == "eat");
}

TEST_CASE("US-181 Wait and flag objectives need no event") {
    const std::string text = "{ \"id\": \"w\", \"title\": \"W\", \"start\": \"a\", \"steps\": {"
                             " \"a\": { \"text\": \"Wait.\", \"objective\": \"wait 30s\", \"next\": \"b\" },"
                             " \"b\": { \"text\": \"Flag.\", \"objective\": \"flag signal\", \"next\": \"c\" },"
                             " \"c\": { \"text\": \"A day.\", \"objective\": \"wait 1d\", \"next\": \"END\" } } }";
    Table t(text, "w");
    t.update();
    CHECK(t.book.activeStep("w") == "a");
    t.now += 599;
    t.update();
    CHECK(t.book.activeStep("w") == "a");
    t.now += 1; // 600 ticks = 30 s
    t.update();
    CHECK(t.book.activeStep("w") == "b");
    t.update();
    CHECK(t.book.activeStep("w") == "b");
    t.world.flags["signal"] = 1;
    t.update();
    CHECK(t.book.activeStep("w") == "c");
    t.now += 999;
    t.update();
    CHECK(t.book.status("w") == rules::QuestStatus::Active);
    t.now += 1; // a day is 1000 ticks in this host
    t.update();
    CHECK(t.book.status("w") == rules::QuestStatus::Done);
}

TEST_CASE("US-181 The hint shows after the time without progress") {
    Table t(kFirstDay);
    t.update();
    CHECK_FALSE(t.book.hintDue("first-day", t.now));
    t.now += 119 * 20;
    CHECK_FALSE(t.book.hintDue("first-day", t.now));
    t.now += 20;
    CHECK(t.book.hintDue("first-day", t.now));
    t.event(rules::QuestObjective::Kind::Gather, "berries", 1); // progress resets the wait
    t.update();
    CHECK_FALSE(t.book.hintDue("first-day", t.now));
}

TEST_CASE("US-180 Objective lines are read and explained") {
    CHECK(rules::parseObjective("talk elder").problem.empty());
    CHECK(rules::parseObjective("defeat wolf 2").objective.amount == 2);
    CHECK(rules::parseObjective("wait 2m").objective.waitUnit == rules::DelayUnit::Minutes);
    CHECK_FALSE(rules::parseObjective("").problem.empty());
    CHECK_FALSE(rules::parseObjective("talk").problem.empty());
    CHECK_FALSE(rules::parseObjective("talk a b").problem.empty());
    CHECK_FALSE(rules::parseObjective("gather berries 0").problem.empty());
}

TEST_CASE("US-182 The offer and turn-in words are read, written and kept") {
    std::string text = kFirstDay;
    replaceFirst(text, "\"journal\": \"The elder", "\"offer\": \"Will you help?\", \"turnIn\": \"Well done.\", \"journal\": \"The elder");
    const rules::Quest q = quest(text);
    CHECK(q.offer == "Will you help?");
    CHECK(q.turnIn == "Well done.");
    const rules::Quest again = quest(rules::writeQuest(q));
    CHECK(again.offer == q.offer);
    CHECK(again.turnIn == q.turnIn);
}

TEST_CASE("US-182 quest(id) and step(id) answer in conditions, and the quest verb is a known effect") {
    Table t(kFirstDay);
    t.update();
    const rules::ParsedExpr active = rules::parseExpression("quest(first-day) == active and step(first-day) == gather");
    REQUIRE(active.root != nullptr);
    CHECK(rules::parseEffect("quest complete first-day").problem == std::nullopt);
    CHECK(t.book.status("first-day") == rules::QuestStatus::Active);
    CHECK(t.book.activeStep("first-day") == "gather");
    CHECK(t.book.complete("first-day", t.now, &t.runner, &t.host));
    CHECK(t.book.status("first-day") == rules::QuestStatus::Done);
    CHECK(t.host.log.size() == 2); // the rewards
    CHECK_FALSE(t.book.complete("first-day", t.now, &t.runner, &t.host));
}

TEST_CASE("US-186 The debugger can start, jump, fail, reset and ask why a condition holds") {
    Table t(kFirstDay);
    t.update();
    REQUIRE(t.book.jumpTo("first-day", "eat", t.now));
    CHECK(t.book.activeStep("first-day") == "eat");
    CHECK_FALSE(t.book.jumpTo("first-day", "nowhere", t.now));
    const rules::Quest& q = *t.book.find("first-day");
    CHECK(t.book.holds(t.world, q.requires_[0]));         // not quest(first-day) == done
    t.world.flags["fire-out"] = 1;
    CHECK(t.book.holds(t.world, q.find("fire")->branches[0].condition));
    CHECK_FALSE(t.book.holds(t.world, q.fail[0]));        // hero.dead is 0
    CHECK(t.book.fail("first-day"));
    CHECK(t.book.reset("first-day"));
    CHECK(t.book.status("first-day") == rules::QuestStatus::Locked);
    CHECK(t.book.jumpTo("first-day", "gather", t.now)); // revives it
    CHECK(t.book.status("first-day") == rules::QuestStatus::Active);
}
