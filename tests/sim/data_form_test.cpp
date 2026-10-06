#include "core/text.h"
#include "sim/data_form.h"
#include "sim/json_text.h"
#include "sim/schema_index.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <set>
#include <string>

namespace sim = odysseus::sim;
namespace form = odysseus::sim::form;
namespace schema = odysseus::sim::schema;
namespace fs = std::filesystem;
using sim::OrderedJson;

namespace {

const schema::SchemaSet& schemas() {
    static const schema::SchemaSet set = schema::SchemaSet::load(fs::path(ODYSSEUS_DATA_DIR) / "schemas");
    return set;
}

struct Opened {
    sim::DataDocument document;
    const schema::Schema* schema;
};

Opened open(const std::string& relative) {
    std::string problem;
    std::optional<sim::DataDocument> document = sim::DataDocument::open(fs::path(ODYSSEUS_DATA_DIR) / relative, problem);
    REQUIRE_MESSAGE(document.has_value(), problem);
    const schema::Schema* found = schemas().schemaFor(relative);
    REQUIRE(found != nullptr);
    return {std::move(*document), found};
}

std::vector<schema::Issue> issuesOf(const Opened& opened) { return schema::check(*opened.schema->root, opened.document.root()).issues; }

const form::FormRow* rowAt(const std::vector<form::FormRow>& rows, const std::string& path) {
    for (const form::FormRow& row : rows) {
        if (row.path == path) return &row;
    }
    return nullptr;
}

std::vector<form::FormRow> rowsFor(const Opened& opened, const std::string& entry, const std::set<std::string>& collapsed = {}) {
    return form::buildRows(opened.document.root(), *opened.schema, entry, collapsed, issuesOf(opened));
}

} // namespace

TEST_CASE("US-191 Entries") {
    const Opened weapons = open("weapons.json");
    const std::vector<form::Entry> entries = form::entriesOf(weapons.document.root(), *weapons.schema);
    REQUIRE(entries.size() > 10);
    CHECK(entries[0].label == "(file)");
    CHECK(entries[0].path.empty());
    CHECK_FALSE(entries[0].element);
    CHECK(entries[1].label == "iron sword");
    CHECK(entries[1].path == "weapons[0]");
    CHECK(entries[1].group == "weapons");
    CHECK(entries[1].element);
    // A file that provides no list is one entry.
    const Opened needs = open("sim/needs.json");
    CHECK(form::entriesOf(needs.document.root(), *needs.schema).size() == 1);
    // A file of one-per-entry kind (an interaction) is one entry too.
    const Opened gather = open("interactions/gather.json");
    CHECK(form::entriesOf(gather.document.root(), *gather.schema).size() == 1);
    CHECK(form::labelOfElement(OrderedJson::parse(R"({"id":"x"})"), 4) == "x");
    CHECK(form::labelOfElement(OrderedJson::parse(R"({"n":1})"), 4) == "#5");
}

TEST_CASE("US-191 Rows of an entry") {
    const Opened weapons = open("weapons.json");
    const std::vector<form::FormRow> rows = rowsFor(weapons, "weapons[0]");
    const form::FormRow* damage = rowAt(rows, "weapons[0].damage");
    REQUIRE(damage != nullptr);
    CHECK(damage->kind == form::RowKind::Whole);
    CHECK(damage->value == "5");
    CHECK(damage->label == "damage");
    CHECK(damage->helpId == "data.weapons.weapons.damage"); // the help of the field: schema name, then the path without list positions
    CHECK(damage->required);
    const form::FormRow* weaponClass = rowAt(rows, "weapons[0].class");
    REQUIRE(weaponClass != nullptr);
    CHECK(weaponClass->kind == form::RowKind::Choice);
    CHECK(std::find(weaponClass->choices.begin(), weaponClass->choices.end(), "pike") == weaponClass->choices.end());
    CHECK(std::find(weaponClass->choices.begin(), weaponClass->choices.end(), "sword") != weaponClass->choices.end());
    CHECK(rowAt(rows, "weapons[0].speed")->kind == form::RowKind::Decimal);
    CHECK(rowAt(rows, "weapons[0].starter")->kind == form::RowKind::Flag);
    // A field the file leaves out is there, empty: typing into it adds it.
    const form::FormRow* light = rowAt(rows, "weapons[0].light");
    REQUIRE(light != nullptr);
    CHECK(light->kind == form::RowKind::Reference);
    CHECK(light->catalog == "lights");
    CHECK_FALSE(light->present);
    CHECK(light->error.empty()); // optional
    // The file's own entry has the settings, not the weapons.
    const std::vector<form::FormRow> file = rowsFor(weapons, "");
    CHECK(rowAt(file, "classes") != nullptr);
    CHECK(rowAt(file, "classes.bow.launchSpeed") != nullptr);
    CHECK(rowAt(file, "weapons") == nullptr);
}

TEST_CASE("US-191 Edit from the form") {
    Opened weapons = open("weapons.json");
    std::string problem;
    std::vector<form::FormRow> rows = rowsFor(weapons, "weapons[0]");
    const auto row = [&](const std::string& field) { return *rowAt(rows, "weapons[0]." + field); };
    CHECK(form::applyText(weapons.document, row("damage"), "12", problem));
    CHECK(weapons.document.find("weapons[0].damage")->get<int>() == 12);
    // A value out of its range, text that is no number, a word that is no choice: refused with the reason, and nothing changes.
    CHECK_FALSE(form::applyText(weapons.document, row("damage"), "5000", problem));
    CHECK(problem == "must be between 0 and 1000 (is 5000)");
    CHECK_FALSE(form::applyText(weapons.document, row("damage"), "five", problem));
    CHECK(problem == "must be a whole number");
    CHECK_FALSE(form::applyText(weapons.document, row("class"), "pike", problem));
    CHECK(problem.starts_with("must be one of: sword"));
    CHECK(weapons.document.find("weapons[0].damage")->get<int>() == 12);
    CHECK(form::applyText(weapons.document, row("speed"), "2.5", problem));
    CHECK(weapons.document.find("weapons[0].speed")->get<double>() == 2.5);
    CHECK_FALSE(form::applyText(weapons.document, row("speed"), "99", problem));
    CHECK(form::applyText(weapons.document, row("starter"), "false", problem));
    CHECK(weapons.document.find("weapons[0].starter")->get<bool>() == false);
    CHECK(form::applyText(weapons.document, row("class"), "axe", problem));
    // The same text typed again is no edit, so Undo does not fill with steps that changed nothing.
    const std::size_t version = weapons.document.version();
    CHECK(form::applyText(weapons.document, row("damage"), "5000", problem) == false);
    rows = rowsFor(weapons, "weapons[0]");
    CHECK(form::applyText(weapons.document, row("damage"), "12", problem));
    CHECK(weapons.document.version() == version);
    // A field the file leaves out: typing a name adds it; empty text leaves it out.
    CHECK(form::applyText(weapons.document, row("light"), "", problem));
    CHECK(weapons.document.find("weapons[0].light") == nullptr);
    CHECK(form::applyText(weapons.document, row("light"), "campfire", problem));
    CHECK(weapons.document.find("weapons[0].light")->get<std::string>() == "campfire");
    // Undo walks back: light, class, starter, speed, damage.
    for (int i = 0; i < 5; ++i) CHECK(weapons.document.undo());
    CHECK_FALSE(weapons.document.dirty());
}

TEST_CASE("US-191 Groups, lists and maps") {
    const Opened needs = open("sim/needs.json");
    std::vector<form::FormRow> rows = rowsFor(needs, "");
    const form::FormRow* group = rowAt(rows, "dailyDecay");
    REQUIRE(group != nullptr);
    CHECK(group->kind == form::RowKind::Heading);
    CHECK(rowAt(rows, "dailyDecay.hunger") != nullptr);
    CHECK(rowAt(rows, "dailyDecay.hunger")->depth == 1);
    // A folded group shows its heading and none of its fields.
    rows = rowsFor(needs, "", {"dailyDecay"});
    CHECK(rowAt(rows, "dailyDecay")->collapsed);
    CHECK(rowAt(rows, "dailyDecay.hunger") == nullptr);

    // A map keyed by id: a heading that can add, one entry per key.
    const Opened materials = open("materials.json");
    rows = rowsFor(materials, "");
    const form::FormRow* map = rowAt(rows, "materials");
    REQUIRE(map != nullptr);
    CHECK(map->kind == form::RowKind::MapHeader);
    CHECK(map->canAdd);
    const form::FormRow* flint = rowAt(rows, "materials.flint");
    REQUIRE(flint != nullptr);
    CHECK(flint->kind == form::RowKind::ItemHeader);
    CHECK(flint->canRemove);
    CHECK_FALSE(flint->canMoveUp); // the keys of a map keep the order of the file: no arrows
    CHECK_FALSE(flint->canMoveDown);
    CHECK(rowAt(rows, "materials.flint.density") != nullptr);

    // A list of words is one row, a list of objects a heading with an entry each.
    const Opened names = open("sim/names.json");
    const std::vector<form::FormRow> nameRows = rowsFor(names, "");
    const form::FormRow* female = rowAt(nameRows, "female");
    REQUIRE(female != nullptr);
    CHECK(female->kind == form::RowKind::Words);
    CHECK(female->value.find(", ") != std::string::npos);
    const Opened kinds = open("buildings/kinds.json");
    rows = rowsFor(kinds, "kinds[0]");
    const form::FormRow* layout = rowAt(rows, "kinds[0].layout");
    REQUIRE(layout != nullptr);
    CHECK(layout->kind == form::RowKind::ListHeader);
    CHECK(layout->canAdd);
    const form::FormRow* piece = rowAt(rows, "kinds[0].layout[0]");
    REQUIRE(piece != nullptr);
    CHECK(piece->kind == form::RowKind::ItemHeader);
    CHECK(piece->canMoveDown);
    CHECK(rowAt(rows, "kinds[0].layout[0].piece")->kind == form::RowKind::Reference);
}

TEST_CASE("US-191 Words") {
    Opened names = open("sim/names.json");
    std::string problem;
    const std::vector<form::FormRow> rows = rowsFor(names, "");
    CHECK(form::applyText(names.document, *rowAt(rows, "male"), "Ake,  Bor ,Cal", problem));
    CHECK(names.document.root()["male"] == OrderedJson::parse(R"(["Ake","Bor","Cal"])"));
    CHECK(form::applyText(names.document, *rowAt(rows, "male"), "", problem));
    CHECK(names.document.root()["male"].empty());
}

TEST_CASE("US-191 Mistakes in the file show in their row") {
    Opened needs = open("sim/needs.json");
    std::string problem;
    // A value the schema refuses, set behind the form's back (an outside edit).
    needs.document.replaceRoot([&] { OrderedJson root = needs.document.root(); root["maximum"] = 5; return root; }(), "outside");
    const std::vector<form::FormRow> rows = rowsFor(needs, "");
    const form::FormRow* maximum = rowAt(rows, "maximum");
    REQUIRE(maximum != nullptr);
    CHECK(maximum->error == "must be between 10 and 1000 (is 5)");
    CHECK(rowAt(rows, "mealValue")->error.empty());
}

TEST_CASE("US-191 New entries") {
    const Opened weapons = open("weapons.json");
    const schema::Node& item = *weapons.schema->root->property("weapons")->items;
    const OrderedJson fresh = form::defaultValue(item);
    REQUIRE(fresh.is_object());
    for (const std::string& name : item.required) CHECK(fresh.contains(name));
    CHECK_FALSE(fresh.contains("light")); // optional fields are left out
    CHECK(fresh["class"] == "sword");      // the first choice
    CHECK(fresh["damage"] == 0);
    CHECK(fresh["starter"] == false);
    const schema::Report report = schema::check(item, fresh);
    CHECK_FALSE(report.hasErrors());
    CHECK(form::uniqueName("sword", {"axe"}) == "sword");
    CHECK(form::uniqueName("sword", {"sword", "sword-2"}) == "sword-3");
}

TEST_CASE("US-191 Every file has rows") {
    // Every entry of every shipped file builds rows, every row's path leads to its value, every row has a help id, and typing a row's own text back is no edit.
    const fs::path root = ODYSSEUS_DATA_DIR;
    int files = 0;
    int rowCount = 0;
    for (const fs::directory_entry& file : fs::recursive_directory_iterator(root)) {
        if (!file.is_regular_file() || file.path().extension() != ".json") continue;
        const std::string relative = file.path().lexically_relative(root).generic_string();
        const schema::Schema* found = schemas().schemaFor(relative);
        if (found == nullptr) continue; // the schemas themselves and the index
        std::string problem;
        std::optional<sim::DataDocument> document = sim::DataDocument::open(file.path(), problem);
        REQUIRE(document);
        const std::vector<schema::Issue> issues = schema::check(*found->root, document->root()).issues;
        const std::vector<form::Entry> entries = form::entriesOf(document->root(), *found);
        REQUIRE_FALSE(entries.empty());
        ++files;
        for (const form::Entry& entry : entries) {
            const std::vector<form::FormRow> rows = form::buildRows(document->root(), *found, entry.path, {}, issues);
            for (const form::FormRow& row : rows) {
                INFO(relative << " " << row.path);
                ++rowCount;
                CHECK_FALSE(row.helpId.empty());
                if (row.present) CHECK(document->find(row.path) != nullptr);
                const bool editable = row.kind != form::RowKind::Heading && row.kind != form::RowKind::ListHeader && row.kind != form::RowKind::ItemHeader && row.kind != form::RowKind::MapHeader;
                if (!editable || !row.present) continue;
                const std::size_t version = document->version();
                std::string refused;
                CHECK(form::applyText(*document, row, row.value, refused));
                CHECK(refused.empty());
                CHECK(document->version() == version);
            }
        }
        CHECK_FALSE(document->dirty());
    }
    CHECK(files > 150);
    CHECK(rowCount > 3000);
}
