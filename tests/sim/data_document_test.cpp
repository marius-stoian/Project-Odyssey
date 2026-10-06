#include "core/text.h"
#include "sim/data_document.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <string>

namespace sim = odysseus::sim;
namespace fs = std::filesystem;
using sim::OrderedJson;

namespace {

fs::path copyOf(const std::string& relative, const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us191-doc" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    const fs::path target = folder / fs::path(relative).filename();
    fs::copy_file(fs::path(ODYSSEUS_DATA_DIR) / relative, target);
    return target;
}

int differingLines(const std::string& a, const std::string& b) {
    std::size_t at = 0;
    std::size_t bt = 0;
    int count = 0;
    while (at < a.size() || bt < b.size()) {
        const std::size_t ae = a.find('\n', at);
        const std::size_t be = b.find('\n', bt);
        const std::string left = a.substr(at, ae == std::string::npos ? std::string::npos : ae - at);
        const std::string right = b.substr(bt, be == std::string::npos ? std::string::npos : be - bt);
        count += left != right ? 1 : 0;
        if (ae == std::string::npos || be == std::string::npos) break;
        at = ae + 1;
        bt = be + 1;
    }
    return count;
}

} // namespace

TEST_CASE("US-191 Paths") {
    const auto parsed = sim::parsePath("weapons[2].damage");
    REQUIRE(parsed);
    REQUIRE(parsed->size() == 3);
    CHECK((*parsed)[0].key == "weapons");
    CHECK((*parsed)[1].index);
    CHECK((*parsed)[1].position == 2);
    CHECK((*parsed)[2].key == "damage");
    CHECK(sim::formatPath(*parsed) == "weapons[2].damage");
    // A member with a dot in its name is quoted, and the two forms read each other.
    const auto quoted = sim::parsePath("fields[\"npc.sword\"].purpose");
    REQUIRE(quoted);
    REQUIRE(quoted->size() == 3);
    CHECK((*quoted)[1].key == "npc.sword");
    CHECK(sim::formatPath(*quoted) == "fields[\"npc.sword\"].purpose");
    CHECK(sim::parsePath("")->empty());
    CHECK_FALSE(sim::parsePath("a..b"));
    CHECK_FALSE(sim::parsePath("a[x]"));
    CHECK_FALSE(sim::parsePath("a[1"));
}

TEST_CASE("US-191 Edit and save") {
    const fs::path file = copyOf("weapons.json", "edit");
    const std::string before = *odysseus::core::readTextFile(file);
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(file, problem);
    REQUIRE(document);
    CHECK_FALSE(document->dirty());
    REQUIRE(document->set("weapons[0].damage", 9, problem));
    CHECK(document->dirty());
    CHECK(document->lastEdit() == "set weapons[0].damage");
    CHECK(document->find("weapons[0].damage")->get<int>() == 9);
    CHECK_FALSE(document->save());
    CHECK_FALSE(document->dirty());
    const std::string after = *odysseus::core::readTextFile(file);
    CHECK(differingLines(before, after) == 1); // the damage changed, every other line is as it was
    CHECK(fs::exists(file.string() + ".bak1")); // the old file is kept
    // The same file opened again shows the saved value.
    const std::optional<sim::DataDocument> again = sim::DataDocument::open(file, problem);
    REQUIRE(again);
    CHECK(again->find("weapons[0].damage")->get<int>() == 9);
}

TEST_CASE("US-191 Undo") {
    const fs::path file = copyOf("sim/needs.json", "undo");
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(file, problem);
    REQUIRE(document);
    const OrderedJson original = document->root();
    REQUIRE(document->set("maximum", 90, problem));
    REQUIRE(document->set("dailyDecay.hunger", 31, problem));
    REQUIRE(document->set("dailyDecay.energy", 36, problem));
    REQUIRE(document->addMember("", "note", "my words", problem));
    REQUIRE(document->set("daysAtZeroBeforeDeath.warmth", 4, problem));
    const OrderedJson edited = document->root();
    CHECK(document->canUndo());
    for (int i = 0; i < 5; ++i) CHECK(document->undo());
    CHECK_FALSE(document->undo()); // nothing is left to undo
    CHECK(sim::sameJson(document->root(), original));
    CHECK_FALSE(document->dirty());
    for (int i = 0; i < 5; ++i) CHECK(document->redo());
    CHECK(sim::sameJson(document->root(), edited));
    // A new edit after an undo drops what could have been redone.
    CHECK(document->undo());
    REQUIRE(document->set("maximum", 80, problem));
    CHECK_FALSE(document->canRedo());
}

TEST_CASE("US-191 Lists and members") {
    const fs::path file = copyOf("weapons.json", "lists");
    const std::string before = *odysseus::core::readTextFile(file);
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(file, problem);
    REQUIRE(document);
    const OrderedJson original = document->root();
    const std::size_t count = original["weapons"].size();
    OrderedJson copy = original["weapons"][1];
    copy["name"] = "copy of one";
    REQUIRE(document->insertElement("weapons", count, copy, problem));
    CHECK(document->root()["weapons"].size() == count + 1);
    REQUIRE(document->moveElement("weapons", count, 0, problem));
    CHECK(document->root()["weapons"][0]["name"] == "copy of one");
    REQUIRE(document->renameMember("weapons[0]", "name", "title", problem));
    CHECK(document->root()["weapons"][0].items().begin().key() == "title"); // it keeps its place
    REQUIRE(document->removeMember("weapons[0]", "title", problem));
    REQUIRE(document->removeElement("weapons", 0, problem));
    CHECK(sim::sameJson(document->root(), original));
    CHECK(document->text() == before); // all that was undone by hand: the text is the file again
    // The file text of an added weapon is one more line.
    REQUIRE(document->insertElement("weapons", count, copy, problem));
    CHECK_FALSE(document->save());
    const std::string after = *odysseus::core::readTextFile(file);
    CHECK(after.size() > before.size());
}

TEST_CASE("US-191 Refused edits") {
    const fs::path file = copyOf("sim/needs.json", "refused");
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(file, problem);
    REQUIRE(document);
    const OrderedJson original = document->root();
    CHECK_FALSE(document->set("nothing.here", 1, problem));
    CHECK_FALSE(problem.empty());
    CHECK_FALSE(document->set("dailyDecay[3]", 1, problem));            // an object has no elements
    CHECK_FALSE(document->insertElement("maximum", 0, 1, problem));      // not a list
    CHECK_FALSE(document->removeElement("dailyDecay", 0, problem));
    CHECK_FALSE(document->addMember("dailyDecay", "hunger", 1, problem)); // there already
    CHECK_FALSE(document->removeMember("dailyDecay", "nothing", problem));
    CHECK_FALSE(document->renameMember("dailyDecay", "hunger", "energy", problem));
    CHECK_FALSE(document->set("a..b", 1, problem));
    CHECK(sim::sameJson(document->root(), original));
    CHECK_FALSE(document->dirty());
    CHECK_FALSE(document->canUndo()); // a refused edit is not a step
    std::string notJson;
    CHECK_FALSE(sim::DataDocument::fromText("{ \"a\": ", "x.json", notJson));
    CHECK(notJson.find("not valid JSON") != std::string::npos);
}

TEST_CASE("US-191 Changed outside") {
    const fs::path file = copyOf("sim/needs.json", "outside");
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(file, problem);
    REQUIRE(document);
    REQUIRE(document->set("maximum", 90, problem));
    std::string text = *odysseus::core::readTextFile(file);
    const std::size_t at = text.find("\"mealValue\": 40");
    REQUIRE(at != std::string::npos);
    text.replace(at, 15, "\"mealValue\": 44");
    odysseus::core::writeTextFileSafely(file, text);
    REQUIRE(document->reloadFromDisk(problem));
    CHECK_FALSE(document->dirty());
    CHECK_FALSE(document->canUndo());
    CHECK(document->find("mealValue")->get<int>() == 44);
    CHECK(document->find("maximum")->get<int>() == 100);
}
