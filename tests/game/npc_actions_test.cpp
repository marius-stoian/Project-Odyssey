// US-267 Actions: tags give the defaults, the allow and deny lists fine-tune; an NPC's menu hides what it cannot do for a reason of its own and the Actions
// pop-up (key X) lists every action with what it needs.
#include "camp.h"

#include "game/game_rules.h"
#include "game/npc_class_book.h"
#include "sim/npc_population.h"

#include <functional>
#include <memory>

using namespace camp_support;
namespace sim = odysseus::sim;

namespace {

luna::engine::Intents pressing(luna::engine::Intent intent) {
    luna::engine::Intents intents;
    intents.set(intent, true, true);
    return intents;
}

// A trade action for the traders of the test: it needs the attitude friendly or better (an opinion of 10), the words of the shipped trade file of M9b.
const char* kTrade = R"({
  "id": "trade-test",
  "label": "Trade",
  "actors": ["hero"],
  "target": { "tags": ["trader"] },
  "range": 8,
  "order": 15,
  "requires": [ { "if": "opinion(npc, hero) >= 10", "else": "needs: friendly or better" } ],
  "effects": [ "say \"Let us trade\"" ]
})";

struct Studio {
    fs::path data;
    luna::engine::RecordingRenderer renderer;
    std::unique_ptr<game::OdysseyGame> odyssey;

    Studio(const std::string& name, const std::function<void(game::Level&)>& changeLevel) : data(dataCopy(name)) {
        writeText(data / "interactions" / "trade-test.json", kTrade);
        const game::Definitions definitions = game::loadDefinitions(data);
        game::Level level = game::loadLevel(ODYSSEUS_DEMO_LEVEL, definitions).level;
        level.characters.clear();
        level.pickups.clear();
        changeLevel(level);
        game::saveLevel(level, definitions, data / "actions-level.json");
        odyssey = std::make_unique<game::OdysseyGame>(data, data / "actions-level.json");
        odyssey->setViewScales(1, 1);
        odyssey->start(renderer);
    }
    std::vector<std::string> menuOf(int id) {
        const game::PlacedCharacter* placed = odyssey->placedCharacter(id);
        REQUIRE(placed != nullptr);
        REQUIRE(odyssey->run().openContext(*odyssey, placed->feet.x, placed->feet.y - 20));
        return labels();
    }
    std::vector<std::string> labels() {
        std::vector<std::string> out;
        for (const auto& entry : odyssey->run().contextEntries()) out.push_back(entry.reason.empty() ? entry.label : entry.label + " | " + entry.reason);
        return out;
    }
    std::vector<std::string> popupOf(int id) {
        const auto subject = game::npcSubject(*odyssey, id);
        REQUIRE(subject.has_value());
        REQUIRE(odyssey->run().openActions(*odyssey, *subject));
        return labels();
    }
};

game::PlacedCharacter trader(game::Level& level, const std::string& name, int east) {
    game::PlacedCharacter placed{level.nextId++, "wanderer", {level.heroStart.x + east, level.heroStart.y}, game::Facing::South, name, 60, 4, {"trader"}};
    return placed;
}

bool has(const std::vector<std::string>& entries, const std::string& prefix) {
    return std::any_of(entries.begin(), entries.end(), [&](const std::string& e) { return e.rfind(prefix, 0) == 0; });
}

} // namespace

TEST_CASE("US-267 Hidden: a trader whose trade needs friendly and who is wary of the hero has no Trade in its menu") {
    Studio studio("actions-hidden", [](game::Level& level) {
        game::PlacedCharacter wary = trader(level, "Ossa", 60);
        wary.attitude = "wary";
        game::PlacedCharacter friendly = trader(level, "Brek", 100);
        friendly.attitude = "friendly";
        level.characters = {wary, friendly};
    });
    const auto& characters = studio.odyssey->level().characters;
    const std::vector<std::string> wary = studio.menuOf(characters[0].id);
    CHECK_FALSE(has(wary, "Trade"));
    CHECK(has(wary, "Confront..."));
    CHECK(has(wary, "Actions..."));
    // A friendly trader offers it.
    studio.odyssey->run().close();
    const std::vector<std::string> friendly = studio.menuOf(characters[1].id);
    CHECK(has(friendly, "Trade"));
    CHECK(friendly[0] == "Trade"); // order 15 comes before Confront and Actions
}

TEST_CASE("US-267 Pop-up: the Actions pop-up lists Trade with 'needs: friendly or better' for the wary trader") {
    Studio studio("actions-popup", [](game::Level& level) {
        game::PlacedCharacter wary = trader(level, "Ossa", 60);
        wary.attitude = "wary";
        level.characters = {wary};
    });
    const int ossa = studio.odyssey->level().characters[0].id;
    const std::vector<std::string> popup = studio.popupOf(ossa);
    CHECK(studio.odyssey->run().contextTitle() == "Actions of Ossa (wary)");
    CHECK(has(popup, "Trade | needs: friendly or better"));
    // Every action of the NPC is there: the confront ones too, and the pop-up itself.
    for (const char* label : {"Taunt", "Insult", "Ask for peace", "Antagonise", "De-escalate", "Confront...", "Actions..."}) CHECK_MESSAGE(has(popup, label), label);
    // An unmet action cannot be chosen: it does nothing.
    studio.odyssey->run().setMessage("untouched");
    for (std::size_t i = 0; i < popup.size(); ++i) {
        if (popup[i].rfind("Trade", 0) == 0) studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kContextBase + static_cast<int>(i));
    }
    CHECK(studio.odyssey->run().message() == "untouched");
    // The same NPC after a gift: its opinion rises past the threshold and Trade is no longer greyed out in the pop-up.
    studio.odyssey->run().close();
    studio.odyssey->npcPopulationMutable().event(ossa, sim::NpcPopulation::kHero, "gift"); // wary -45 + 15 = -30: not yet
    CHECK(has(studio.popupOf(ossa), "Trade | needs: friendly or better"));
    studio.odyssey->run().close();
    studio.odyssey->npcPopulationMutable().adjust(ossa, sim::NpcPopulation::kHero, 45);
    const std::vector<std::string> later = studio.popupOf(ossa);
    CHECK(has(later, "Trade"));
    CHECK_FALSE(has(later, "Trade |"));
}

TEST_CASE("US-267 Key: the Actions key opens the pop-up of the nearest NPC") {
    Studio studio("actions-key", [](game::Level& level) {
        game::PlacedCharacter wary = trader(level, "Ossa", 60);
        wary.attitude = "wary";
        level.characters = {wary};
    });
    studio.odyssey->update(pressing(luna::engine::Intent::Actions));
    REQUIRE(studio.odyssey->run().screen() == game::Screen::Context);
    CHECK(studio.odyssey->run().contextTitle() == "Actions of Ossa (wary)");
    CHECK(has(studio.labels(), "Trade | needs: friendly or better"));
    // The right-click entry opens the same pop-up.
    studio.odyssey->run().close();
    const game::PlacedCharacter* placed = studio.odyssey->placedCharacter(studio.odyssey->level().characters[0].id);
    REQUIRE(studio.odyssey->run().openContext(*studio.odyssey, placed->feet.x, placed->feet.y - 20));
    const auto entries = studio.odyssey->run().contextEntries();
    std::size_t at = entries.size();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].label == "Actions...") at = i;
    }
    REQUIRE(at < entries.size());
    studio.odyssey->run().press(*studio.odyssey, game::RunFlow::kContextBase + static_cast<int>(at));
    CHECK(studio.odyssey->run().contextTitle() == "Actions of Ossa (wary)");
}

TEST_CASE("US-267 Allow and deny: the tags give the defaults, the lists fine-tune, and a deny wins") {
    Studio studio("actions-lists", [](game::Level& level) {
        game::PlacedCharacter denied = trader(level, "Denied", 60);
        denied.attitude = "friendly";
        denied.deny = {"trade-test"};                       // a trader that does not trade
        game::PlacedCharacter allowed = {level.nextId++, "wanderer", {level.heroStart.x + 100, level.heroStart.y}, game::Facing::South, "Allowed", 60, 4, {}};
        allowed.attitude = "friendly";
        allowed.allow = {"trade-test"};                     // no trader class, but trade is allowed for it
        game::PlacedCharacter plain = {level.nextId++, "wanderer", {level.heroStart.x + 140, level.heroStart.y}, game::Facing::South, "Plain", 60, 4, {}};
        plain.attitude = "friendly";
        level.characters = {denied, allowed, plain};
    });
    const auto& characters = studio.odyssey->level().characters;
    CHECK_FALSE(has(studio.menuOf(characters[0].id), "Trade"));
    CHECK_FALSE(has(studio.popupOf(characters[0].id), "Trade")); // denied actions are not the NPC's actions: not even in the pop-up
    studio.odyssey->run().close();
    CHECK(has(studio.menuOf(characters[1].id), "Trade"));        // allowed by the list although the tags do not match
    studio.odyssey->run().close();
    CHECK_FALSE(has(studio.menuOf(characters[2].id), "Trade"));  // the tags say trader only
    // The resolved lists are what the rules see.
    const auto subject = game::npcSubject(*studio.odyssey, characters[0].id);
    REQUIRE(subject.has_value());
    CHECK(subject->info.deny == std::vector<std::string>{"trade-test"});
    const auto allowedSubject = game::npcSubject(*studio.odyssey, characters[1].id);
    CHECK(allowedSubject->info.allow == std::vector<std::string>{"trade-test"});
}

TEST_CASE("US-267 Too far: an action that is only out of range stays in the menu, greyed out") {
    Studio studio("actions-far", [](game::Level& level) {
        game::PlacedCharacter far = trader(level, "Far", 32 * 6); // 6 m: beyond the 2 m of Talk, within the 8 m of Trade
        far.attitude = "friendly";
        far.classes = {"trader"};
        far.dialogues = {{"player", "npc-actions-far.dlg"}};
        level.characters = {far};
    });
    const int far = studio.odyssey->level().characters[0].id;
    // No script named "npc-actions-far" exists, so there is no Talk; but Trade (range 8) is offered.
    CHECK(has(studio.menuOf(far), "Trade"));
    studio.odyssey->run().close();
    // Move the same level's trader out of Trade's range: it stays, greyed, with the range reason.
    game::PlacedCharacter* placed = const_cast<game::PlacedCharacter*>(studio.odyssey->placedCharacter(far));
    placed->feet.x = studio.odyssey->level().heroStart.x + 32 * 12;
    studio.odyssey->npcPopulationMutable().move(studio.odyssey->npcPopulation().indexOf(far), placed->feet.x, placed->feet.y); // the grid follows
    CHECK(has(studio.menuOf(far), "Trade | Too far away"));
}
