#include "core/text.h"
#include "sim/data_refs.h"
#include "sim/hero_data.h"
#include "sim/interaction.h"
#include "sim/quest_data.h"
#include "sim/schema.h"
#include "sim/schema_index.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace sim = odysseus::sim;
namespace refs = odysseus::sim::refs;
namespace schema = odysseus::sim::schema;
namespace fs = std::filesystem;

namespace {

// A copy of the data folder and the levels folder, so a rename can be written and read back.
struct Folders {
    fs::path root;
    refs::Roots roots;
    explicit Folders(const std::string& name) {
        root = fs::temp_directory_path() / "odysseus-us193" / name;
        fs::remove_all(root);
        fs::create_directories(root);
        fs::copy(ODYSSEUS_DATA_DIR, root / "data", fs::copy_options::recursive);
        fs::copy(fs::path(ODYSSEUS_DATA_DIR).parent_path() / "levels", root / "levels", fs::copy_options::recursive);
        roots = {root / "data", root / "levels"};
    }
    schema::SchemaSet schemas() const { return schema::SchemaSet::load(roots.data / "schemas"); }
    std::string text(const std::string& relative) const { return *odysseus::core::readTextFile(root / relative); }
};

bool hasUse(const std::vector<refs::Use>& uses, const std::string& file, const std::string& kind) {
    return std::any_of(uses.begin(), uses.end(), [&](const refs::Use& use) { return use.file == file && use.kind == kind; });
}

} // namespace

TEST_CASE("US-193 Words") {
    CHECK(refs::replaceWord("has(hero, berries, 1)", "berries", "fruit") == "has(hero, fruit, 1)");
    CHECK(refs::replaceWord("take hero berries 1; opinion npc hero 5", "berries", "fruit") == "take hero fruit 1; opinion npc hero 5");
    // A longer word that contains it is another word; quoted text is prose.
    CHECK(refs::replaceWord("interact eat-berries", "berries", "fruit") == "interact eat-berries");
    CHECK(refs::replaceWord("berries-and-nuts and berries_2 and berries", "berries", "fruit") == "berries-and-nuts and berries_2 and fruit");
    CHECK(refs::replaceWord("remember npc \"{hero} shared berries\" 20; give berries", "berries", "fruit") == "remember npc \"{hero} shared berries\" 20; give fruit");
    // A name with a space is a quoted name.
    CHECK(refs::replaceWord("target.kind == \"grey wolf\"", "grey wolf", "wolf") == "target.kind == \"wolf\"");
    CHECK(refs::replaceWord("a \"grey wolf howls\"", "grey wolf", "wolf") == "a \"grey wolf howls\"");
    CHECK(refs::replaceWord("same", "same", "same") == "same");
    CHECK(refs::validEntryName("iron sword"));
    CHECK(refs::validEntryName("a-b_c 2"));
    CHECK_FALSE(refs::validEntryName(""));
    CHECK_FALSE(refs::validEntryName(" lead"));
    CHECK_FALSE(refs::validEntryName("quote\""));
}

TEST_CASE("US-193 Uses") {
    Folders folders("uses");
    const schema::SchemaSet set = folders.schemas();
    const std::vector<refs::Use> uses = refs::usesOf(folders.roots, set, {"items"}, "berries");
    CHECK(hasUse(uses, "quests/first-day.json", "text"));        // the objective "gather berries 1"
    CHECK(hasUse(uses, "dialogue/elder-fire.dlg", "text"));      // [if has(hero, berries, 1)] and {take hero berries 1; ...}
    CHECK(hasUse(uses, "interactions/give-berries.json", "text"));
    CHECK(hasUse(uses, "levels/npc-test.json", "key"));          // the price of berries in the level's economy
    for (const refs::Use& use : uses) {
        INFO(use.file << " " << use.where);
        CHECK(use.line > 0);
        CHECK(use.kind != "entry"); // the entry itself is not a use
    }
    // An item nothing names has no uses: a plant that no rule or level mentions.
    CHECK(refs::usesOf(folders.roots, set, {"items"}, "no-such-thing").empty());
}

TEST_CASE("US-193 Rename an item") {
    Folders folders("rename");
    const schema::SchemaSet set = folders.schemas();
    const refs::Plan plan = refs::planRename(folders.roots, set, {"items"}, "berries", "red-berries");
    REQUIRE(plan.problem.empty());
    CHECK(hasUse(plan.uses, "hero/items.json", "entry")); // the entry itself
    CHECK(hasUse(plan.uses, "quests/first-day.json", "text"));
    CHECK(hasUse(plan.uses, "dialogue/elder-fire.dlg", "text"));
    CHECK(hasUse(plan.uses, "levels/npc-test.json", "key"));
    CHECK_FALSE(plan.texts.empty());
    // Nothing is written by making the plan.
    CHECK(folders.text("data/hero/items.json").find("\"id\": \"berries\"") != std::string::npos);
    REQUIRE_FALSE(refs::applyPlan(plan));

    CHECK(folders.text("data/hero/items.json").find("\"id\": \"red-berries\"") != std::string::npos);
    CHECK(folders.text("data/quests/first-day.json").find("\"gather red-berries 1\"") != std::string::npos);
    const std::string dialogue = folders.text("data/dialogue/elder-fire.dlg");
    CHECK(dialogue.find("has(hero, red-berries, 1)") != std::string::npos);
    CHECK(dialogue.find("take hero red-berries 1;") != std::string::npos);
    CHECK(dialogue.find("\"{hero} shared berries\"") != std::string::npos); // prose in quotes is not touched
    CHECK(dialogue.find("-> Offer berries") != std::string::npos);          // nor is what a choice says
    CHECK(folders.text("data/interactions/eat-berries.json").find("\"id\": \"eat-berries\"") != std::string::npos); // another name that holds the word
    CHECK(folders.text("levels/npc-test.json").find("\"red-berries\"") != std::string::npos);

    // The whole data folder is still consistent: every file passes its schema and every link names an entry.
    const schema::SchemaSet after = folders.schemas();
    const schema::DataIndex index = schema::buildIndex(folders.roots.data, after);
    std::string report;
    for (const schema::FileIssue& issue : index.issues) report += "\n  " + issue.text();
    for (const schema::FileIssue& issue : index.brokenLinks) report += "\n  " + issue.text();
    INFO("after the rename:" << report);
    CHECK(index.clean());
    CHECK(index.catalogs.at("items").count("red-berries") == 1);
    CHECK(index.catalogs.at("items").count("berries") == 0);
    // The loaders read what was written: the hero data, the rule files and the quests.
    schema::install(std::make_shared<schema::SchemaSet>(after), folders.roots.data);
    CHECK_NOTHROW((void)sim::loadHeroData(folders.roots.data));
    sim::rules::LoadReport load;
    (void)sim::rules::InteractionRegistry::load(folders.roots.data / "interactions", load);
    (void)sim::rules::loadQuests(folders.roots.data / "quests", load);
    schema::uninstall();
    std::string problems;
    for (const sim::rules::Diagnostic& problem : load.errors) problems += "\n  " + problem.text();
    INFO("loading after the rename:" << problems);
    CHECK(load.errors.empty());
}

TEST_CASE("US-193 Rename a plant that levels and rules name") {
    Folders folders("rename-plant");
    const schema::SchemaSet set = folders.schemas();
    // "moss" is a plant: interactions target it by kind. It is in the catalogs plants and kinds.
    const refs::Plan plan = refs::planRename(folders.roots, set, {"plants", "kinds"}, "moss", "green-moss");
    REQUIRE(plan.problem.empty());
    CHECK(hasUse(plan.uses, "plants.json", "entry"));
    CHECK(hasUse(plan.uses, "interactions/pick-flint.json", "value"));
    REQUIRE_FALSE(refs::applyPlan(plan));
    const schema::DataIndex index = schema::buildIndex(folders.roots.data, folders.schemas());
    CHECK(index.clean());
    CHECK(index.catalogs.at("kinds").count("green-moss") == 1);
    CHECK(index.catalogs.at("kinds").count("moss") == 0);
}

TEST_CASE("US-193 A rename that cannot be done says why") {
    Folders folders("refused");
    const schema::SchemaSet set = folders.schemas();
    CHECK(refs::planRename(folders.roots, set, {"items"}, "berries", "flint").problem.find("has an entry called \"flint\" already") != std::string::npos);
    CHECK(refs::planRename(folders.roots, set, {"items"}, "berries", "bad\"name").problem.find("is not a name") != std::string::npos);
    CHECK(refs::planRename(folders.roots, set, {"items"}, "berries", "berries").problem == "the new name is the old one");
    CHECK(refs::planRename(folders.roots, set, {"items"}, "berries", "").problem.find("is not a name") != std::string::npos);
    refs::Plan broken;
    broken.problem = "no";
    CHECK(refs::applyPlan(broken) == std::optional<std::string>("no"));
}

TEST_CASE("US-193 A plan that fails halfway is undone") {
    Folders folders("halfway");
    const schema::SchemaSet set = folders.schemas();
    refs::Plan plan = refs::planRename(folders.roots, set, {"items"}, "berries", "red-berries");
    REQUIRE(plan.problem.empty());
    // One file of the plan cannot be written (its folder is a file): the ones written before it are put back.
    const fs::path blocker = folders.root / "blocked";
    odysseus::core::writeTextFileSafely(blocker, "a file");
    plan.texts[blocker / "inside.json"] = "{}";
    const std::string before = folders.text("data/hero/items.json");
    const std::optional<std::string> problem = refs::applyPlan(plan);
    CHECK(problem.has_value());
    CHECK(folders.text("data/hero/items.json") == before);
}
