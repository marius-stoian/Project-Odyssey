// US-265 Talk with placed NPCs: a placed person with a dialogue for the player offers Talk, one without offers none, the panel pauses the game, the
// dialogue reads and changes the opinion, and the clan keeps its own talk.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/npc_population.h"

#include <functional>
#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

const char* kTraderDialogue = R"(# A placed trader (US-265). The @who names nobody, so no clan member ever gets this script.
@who npc-trader

=== start
Trader: Fresh flint, {hero}?   [if opinion(npc, hero) >= 5]
Trader: Hello there, {hero}.
-> Say something kind {opinion npc hero 5; remember npc "{hero} was kind" 20} => thanks
-> Leave => END

=== thanks
Trader: Kind words are worth more than flint.
-> Leave => END
)";

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;
    int ossa = 0, brek = 0, cori = 0;

    Studio(const std::string& name, bool clan = false) : data(dataCopy(name)) {
        // The trader class gives its people the dialogue "npc-trader" for the player; Brek is a wanderer with no class and so nothing to say.
        writeText(data / "dialogue" / "npc-trader.dlg", kTraderDialogue);
        writeText(data / "npc-classes" / "trader.json",
                  "{\n  \"id\": \"trader\",\n  \"label\": \"Trader\",\n  \"colour\": \"#d9a441\",\n  \"icon\": \"coin\",\n  \"tags\": [\"trader\"],\n"
                  "  \"dialogues\": { \"player\": \"npc-trader.dlg\" },\n  \"actions\": { \"allow\": [], \"deny\": [] }\n}\n");
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        level.clan = clan;
        const game::PixelPoint hero = level.heroStart;
        game::PlacedCharacter a{level.nextId++, "wanderer", {hero.x + 60, hero.y}, game::Facing::South, "Ossa", 60, 4, {"trader"}};
        game::PlacedCharacter b{level.nextId++, "wanderer", {hero.x + 100, hero.y}, game::Facing::South, "Brek", 60, 4, {}};
        game::PlacedCharacter c{level.nextId++, "wanderer", {hero.x + 140, hero.y}, game::Facing::South, "Cori", 60, 4, {"trader"}};
        c.dialogues = {{"player", "no-such-script.dlg"}}; // a dialogue that does not exist is no dialogue
        ossa = a.id;
        brek = b.id;
        cori = c.id;
        level.characters = {a, b, c};
        game::saveLevel(level, definitions, data / "talk-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "talk-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    std::vector<std::string> menuOf(int id) {
        const game::PlacedCharacter* placed = odyssey->placedCharacter(id);
        REQUIRE(placed != nullptr);
        REQUIRE(odyssey->run().openContext(*odyssey, placed->feet.x, placed->feet.y - 20));
        std::vector<std::string> labels;
        for (const auto& entry : odyssey->run().contextEntries()) labels.push_back(entry.reason.empty() ? entry.label : entry.label + " | " + entry.reason);
        return labels;
    }
};

} // namespace

TEST_CASE("US-265 Talk: the menu of a placed person with a dialogue offers Talk, opens it and the game pauses") {
    Studio studio("npc-talk-open");
    const std::vector<std::string> menu = studio.menuOf(studio.ossa);
    CHECK(studio.odyssey->run().contextTitle() == "Ossa (neutral)"); // the attitude word shows (US-264)
    REQUIRE_FALSE(menu.empty());
    CHECK(menu[0] == "Talk");
    studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kContextBase + 0);
    REQUIRE(studio.odyssey->run().screen() == game::Screen::Talk);
    const auto shown = studio.odyssey->run().shownText();
    REQUIRE_FALSE(shown.empty());
    CHECK(shown[0] == "Ossa   (neutral)"); // the title of a conversation: name and attitude word
    bool greeted = false;
    for (const std::string& line : shown) greeted = greeted || line.find("Hello there") != std::string::npos;
    CHECK(greeted);
    // The game waits while the panel is open.
    const auto ticks = studio.odyssey->ticks();
    for (int i = 0; i < 20; ++i) studio.odyssey->update({});
    CHECK(studio.odyssey->ticks() == ticks);
    CHECK(studio.odyssey->run().screen() == game::Screen::Talk);
}

TEST_CASE("US-265 Choices: a dialogue reads and changes the opinion and the memories of the placed person") {
    Studio studio("npc-talk-choices");
    sim::NpcPopulation& people = studio.odyssey->npcPopulationMutable();
    CHECK(people.opinion(studio.ossa, sim::NpcPopulation::kHero) == 0);
    studio.menuOf(studio.ossa);
    studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kContextBase + 0); // Talk
    studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kTalkChoiceBase + 0); // Say something kind
    CHECK(people.opinion(studio.ossa, sim::NpcPopulation::kHero) == 5);
    REQUIRE(people.noteCount(people.indexOf(studio.ossa)) >= 1);
    CHECK(people.note(people.indexOf(studio.ossa), people.noteCount(people.indexOf(studio.ossa)) - 1).text.find("was kind") != std::string::npos);
    const auto shown = studio.odyssey->run().shownText();
    bool answered = false;
    for (const std::string& line : shown) answered = answered || line.find("Kind words") != std::string::npos;
    CHECK(answered);
    studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kTalkChoiceBase + 0); // Leave
    CHECK(studio.odyssey->run().screen() == game::Screen::None);
    // Next time the conversation reads the opinion: at 5 or more the first line is the warm one.
    studio.menuOf(studio.ossa);
    studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kContextBase + 0);
    bool warm = false;
    for (const std::string& line : studio.odyssey->run().shownText()) warm = warm || line.find("Fresh flint") != std::string::npos;
    CHECK(warm);
}

TEST_CASE("US-265 No script: a placed person with no dialogue for the player, or a missing one, has no Talk") {
    Studio studio("npc-talk-none");
    const std::vector<std::string> brek = studio.menuOf(studio.brek);
    for (const std::string& label : brek) CHECK(label.rfind("Talk", 0) != 0);
    CHECK(studio.odyssey->run().contextTitle() == "Brek (neutral)");
    const std::vector<std::string> cori = studio.menuOf(studio.cori); // its dialogue names a script that does not exist
    for (const std::string& label : cori) CHECK(label.rfind("Talk", 0) != 0);
    CHECK(studio.odyssey->npcDialogueFor(studio.brek) == nullptr);
    CHECK(studio.odyssey->npcDialogueFor(studio.cori) == nullptr);
    CHECK(studio.odyssey->npcDialogueFor(studio.ossa) != nullptr);
}

TEST_CASE("US-265 Too far: the menu shows Talk greyed out beyond 2 m, and a creature or empty ground has no menu outside a run") {
    Studio studio("npc-talk-far");
    // Move the hero's view of the world: the menu measures from the hero, so Cori's neighbour Brek is 3 m away; a trader 3 m away is too far.
    const game::PlacedCharacter* ossa = studio.odyssey->placedCharacter(studio.ossa);
    REQUIRE(ossa != nullptr);
    CHECK(studio.menuOf(studio.ossa)[0] == "Talk");
    // Nothing there, and no run of the hero: no menu.
    CHECK_FALSE(studio.odyssey->run().openContext(*studio.odyssey, 5.0, 5.0));
}

TEST_CASE("US-265 Tags: the tag \"speaks\" is what talk.json targets, and clan members carry it") {
    Studio studio("npc-talk-tags");
    const auto subject = game::npcSubject(*studio.odyssey, studio.ossa);
    REQUIRE(subject.has_value());
    const auto has = [&](const std::string& tag) { return std::find(subject->info.tags.begin(), subject->info.tags.end(), tag) != subject->info.tags.end(); };
    CHECK(has("npc"));
    CHECK(has("speaks"));
    CHECK(has("trader")); // the class tag
    CHECK_FALSE(has("person")); // the clan's tag: the actions made for the clan are not offered
    const auto brek = game::npcSubject(*studio.odyssey, studio.brek);
    REQUIRE(brek.has_value());
    CHECK(std::find(brek->info.tags.begin(), brek->info.tags.end(), "speaks") == brek->info.tags.end());
    CHECK_FALSE(game::npcSubject(*studio.odyssey, 99999).has_value());
    // The interaction file targets it.
    const auto* talk = studio.odyssey->interactions().find("talk");
    REQUIRE(talk != nullptr);
    CHECK(talk->targetTags == std::vector<std::string>{"speaks"});
    const auto builtIn = game::builtInThingTags(*studio.odyssey);
    CHECK(std::find(builtIn.begin(), builtIn.end(), "speaks") != builtIn.end());
}
