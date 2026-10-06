#include "core/text.h"
#include "sim/json_patch.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace sim = odysseus::sim;
namespace fs = std::filesystem;
using sim::OrderedJson;

namespace {

struct DataFile {
    std::string name; // below assets/data, forward slashes
    std::string text;
    OrderedJson doc;
};

// Every JSON file of assets/data (the schemas included): the corpus the patcher must survive.
const std::vector<DataFile>& corpus() {
    static const std::vector<DataFile> files = [] {
        std::vector<DataFile> found;
        const fs::path root = ODYSSEUS_DATA_DIR;
        for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root)) {
            if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
            DataFile file;
            file.name = entry.path().lexically_relative(root).generic_string();
            file.text = *odysseus::core::readTextFile(entry.path());
            file.doc = OrderedJson::parse(file.text, nullptr, true, true);
            found.push_back(std::move(file));
        }
        std::sort(found.begin(), found.end(), [](const DataFile& a, const DataFile& b) { return a.name < b.name; });
        return found;
    }();
    return files;
}

OrderedJson parseText(const std::string& text) { return OrderedJson::parse(text, nullptr, true, true); }

std::vector<std::string> linesOf(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t at = 0;
    while (at <= text.size()) {
        const std::size_t end = text.find('\n', at);
        lines.push_back(text.substr(at, end == std::string::npos ? std::string::npos : end - at));
        if (end == std::string::npos) break;
        at = end + 1;
    }
    return lines;
}

int differingLines(const std::string& a, const std::string& b) {
    const std::vector<std::string> left = linesOf(a);
    const std::vector<std::string> right = linesOf(b);
    if (left.size() != right.size()) return -1;
    int count = 0;
    for (std::size_t i = 0; i < left.size(); ++i) count += left[i] != right[i] ? 1 : 0;
    return count;
}

// Runs `change` on a copy of every file's document, patches the file's text with it and checks the text parses back to the changed document.
// A document the change left alone is skipped. Returns how many files were checked.
int overCorpus(const std::function<void(OrderedJson&)>& change) {
    int checked = 0;
    for (const DataFile& file : corpus()) {
        OrderedJson edited = file.doc;
        change(edited);
        if (sim::sameJson(edited, file.doc)) continue;
        const std::string patched = sim::patchJsonText(file.text, file.doc, edited);
        INFO(file.name);
        OrderedJson again;
        REQUIRE_NOTHROW(again = parseText(patched));
        CHECK(sim::sameJson(again, edited));
        ++checked;
    }
    return checked;
}

void everyValue(OrderedJson& value, const std::function<void(OrderedJson&)>& visit) {
    visit(value);
    if (value.is_object()) {
        for (auto& item : value.items()) everyValue(item.value(), visit);
    } else if (value.is_array()) {
        for (OrderedJson& element : value) everyValue(element, visit);
    }
}

} // namespace

TEST_CASE("US-191 Patch keeps the text") {
    // With nothing changed the patched text is the file, byte for byte: every shipped file, comments and spacing included.
    for (const DataFile& file : corpus()) {
        INFO(file.name);
        CHECK(sim::patchJsonText(file.text, file.doc, file.doc) == file.text);
    }
    CHECK(corpus().size() > 150);
}

TEST_CASE("US-191 Patch one value") {
    const DataFile* weapons = nullptr;
    for (const DataFile& file : corpus()) {
        if (file.name == "weapons.json") weapons = &file;
    }
    REQUIRE(weapons != nullptr);
    OrderedJson edited = weapons->doc;
    edited["weapons"][0]["damage"] = 9;
    const std::string patched = sim::patchJsonText(weapons->text, weapons->doc, edited);
    CHECK(differingLines(weapons->text, patched) == 1); // one line of the file changed
    CHECK(patched.find("\"damage\":9,") != std::string::npos);   // in the style of the line: no space after the colon
    CHECK(sim::sameJson(parseText(patched), edited));
    // The same edit undone gives the file back.
    CHECK(sim::patchJsonText(patched, edited, weapons->doc) == weapons->text);
}

TEST_CASE("US-191 Patch keeps comments and notes") {
    const std::string text = "// The first lines of the file.\n// More.\n{\n  // about hp\n  \"hp\": 10, // inline\n  \"note\": \"my own words\",\n  \"list\": [1, 2, 3]\n}\n";
    const OrderedJson before = parseText(text);
    OrderedJson after = before;
    after["hp"] = 12;
    const std::string patched = sim::patchJsonText(text, before, after);
    CHECK(patched == "// The first lines of the file.\n// More.\n{\n  // about hp\n  \"hp\": 12, // inline\n  \"note\": \"my own words\",\n  \"list\": [1, 2, 3]\n}\n");
    // A list that changes size is written again in its own style; the rest still stands as it was.
    after["list"].push_back(4);
    CHECK(sim::patchJsonText(text, before, after) == "// The first lines of the file.\n// More.\n{\n  // about hp\n  \"hp\": 12, // inline\n  \"note\": \"my own words\",\n  \"list\": [1, 2, 3, 4]\n}\n");
    CHECK(sim::leadingComments(text) == "// The first lines of the file.\n// More.\n");
    CHECK(sim::leadingComments("{\n}\n").empty());
}

TEST_CASE("US-191 Patch adds and removes entries") {
    const DataFile* weapons = nullptr;
    const DataFile* needs = nullptr;
    for (const DataFile& file : corpus()) {
        if (file.name == "weapons.json") weapons = &file;
        if (file.name == "sim/needs.json") needs = &file;
    }
    REQUIRE(weapons != nullptr);
    REQUIRE(needs != nullptr);

    // A new weapon is written as the others are: one object on a line, no space after the colon.
    OrderedJson grown = weapons->doc;
    OrderedJson copy = grown["weapons"][0];
    copy["name"] = "new sword";
    grown["weapons"].push_back(copy);
    const std::string withNew = sim::patchJsonText(weapons->text, weapons->doc, grown);
    CHECK(withNew.find("{\"name\":\"new sword\",") != std::string::npos);
    CHECK(sim::sameJson(parseText(withNew), grown));
    CHECK(linesOf(withNew).size() == linesOf(weapons->text).size() + 1);
    CHECK(sim::patchJsonText(withNew, grown, weapons->doc) == weapons->text); // and taking it out again gives the file back

    // A new member of an object that has one member to a line gets its own line, with the same indentation.
    OrderedJson noted = needs->doc;
    noted["note"] = "tuned by hand";
    const std::string withNote = sim::patchJsonText(needs->text, needs->doc, noted);
    CHECK(withNote.find("  \"note\": \"tuned by hand\"") != std::string::npos);
    CHECK(linesOf(withNote).size() == linesOf(needs->text).size() + 1);
    CHECK(sim::sameJson(parseText(withNote), noted));
    CHECK(sim::patchJsonText(withNote, noted, needs->doc) == needs->text);

    // An empty list gets entries and a list loses them all.
    const std::string empty = "{\n  \"tags\": [],\n  \"items\": [\"a\", \"b\"]\n}\n";
    const OrderedJson before = parseText(empty);
    OrderedJson after = before;
    after["tags"].push_back("x");
    after["items"] = OrderedJson::array();
    CHECK(sim::patchJsonText(empty, before, after) == "{\n  \"tags\": [\"x\"],\n  \"items\": []\n}\n");
}

TEST_CASE("US-191 Patch a whole document") {
    OrderedJson document = OrderedJson::object();
    document["a"] = 1;
    document["list"] = OrderedJson::array({1, 2, 3});
    document["obj"] = OrderedJson::object();
    document["obj"]["x"] = "y";
    CHECK(sim::writeJsonText(document) == "{\n  \"a\": 1,\n  \"list\": [1, 2, 3],\n  \"obj\": {\"x\": \"y\"}\n}\n");
    // Too long for one line: one entry to a line.
    OrderedJson wide = OrderedJson::object();
    for (int i = 0; i < 6; ++i) wide["list"].push_back("a fairly long sentence number " + std::to_string(i) + " that fills the line");
    const std::string text = sim::writeJsonText(wide, 60);
    CHECK(linesOf(text).size() > 6);
    CHECK(sim::sameJson(parseText(text), wide));
    // A text that is not JSON cannot be patched: the document is written instead, under the comments it had.
    CHECK(sim::patchJsonText("// c\nnot json", OrderedJson(), document).starts_with("// c\n{"));
}

TEST_CASE("US-191 Patch every value of every file") {
    // Every number up, every string longer, every flag flipped: each file still parses to exactly the edited document.
    const int checked = overCorpus([](OrderedJson& root) {
        everyValue(root, [](OrderedJson& value) {
            if (value.is_number_integer()) value = value.get<long long>() + 1;
            else if (value.is_number_float()) value = value.get<double>() + 0.5;
            else if (value.is_string()) value = value.get<std::string>() + "x";
            else if (value.is_boolean()) value = !value.get<bool>();
        });
    });
    CHECK(checked > 150);
}

TEST_CASE("US-191 Patch lists that grow and shrink") {
    const auto onEveryArray = [](const std::function<void(OrderedJson&)>& edit) {
        return overCorpus([&](OrderedJson& root) {
            everyValue(root, [&](OrderedJson& value) {
                if (value.is_array()) edit(value);
            });
        });
    };
    CHECK(onEveryArray([](OrderedJson& list) { if (!list.empty()) list.push_back(list.front()); }) > 50);   // one more at the end
    CHECK(onEveryArray([](OrderedJson& list) { if (!list.empty()) list.erase(list.begin()); }) > 50);        // the first one gone
    CHECK(onEveryArray([](OrderedJson& list) { if (list.size() > 1) list.insert(list.begin() + 1, list.front()); }) > 20); // one in the middle
    CHECK(onEveryArray([](OrderedJson& list) { if (!list.empty()) { OrderedJson first = list.front(); list.clear(); list.push_back(first); } }) > 0); // all but one gone
}

TEST_CASE("US-191 Patch objects that gain and lose members") {
    const auto onEveryObject = [](const std::function<void(OrderedJson&)>& edit) {
        return overCorpus([&](OrderedJson& root) {
            everyValue(root, [&](OrderedJson& value) {
                if (value.is_object()) edit(value);
            });
        });
    };
    CHECK(onEveryObject([](OrderedJson& object) { object["zz-new"] = 1; }) > 150);                    // a member at the end
    CHECK(onEveryObject([](OrderedJson& object) { if (!object.empty()) object.erase(object.items().begin().key()); }) > 100); // the first one gone
    CHECK(onEveryObject([](OrderedJson& object) { if (object.size() > 1) { OrderedJson copy = OrderedJson::object(); for (auto& item : object.items()) copy[item.key()] = item.value(); object.clear(); auto it = copy.items().begin(); ++it; for (; it != copy.items().end(); ++it) object[it.key()] = it.value(); object[copy.items().begin().key()] = copy.items().begin().value(); } }) > 100); // the first moved to the end
}
