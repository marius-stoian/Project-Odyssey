// M5: the hero's run, from the New Game screen to the victory or the grave (US-050..US-073, US-082).
#include "sim/data.h"
#include "sim/hero_data.h"
#include "sim/hero_life.h"
#include "sim/world.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

const sim::HeroData& data() {
    static const sim::HeroData loaded = sim::loadHeroData(ODYSSEUS_DATA_DIR);
    return loaded;
}

sim::SimConfig baseConfig() { return sim::loadSimConfig(ODYSSEUS_DATA_DIR); }

struct Run {
    sim::World world;
    sim::HeroLife life;
    explicit Run(sim::NewGame game = {})
        : world(game.seed, sim::configForComfort(data(), baseConfig(), game.comfort)), life(data(), world, game) {}
    // A day of the clan's life, then the run's own daily work.
    void days(int count) {
        for (int i = 0; i < count; ++i) {
            world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()));
            life.update();
        }
    }
};

sim::NewGame off(std::uint64_t seed = 1) {
    sim::NewGame game;
    game.seed = seed;
    game.preset = 2; // Off
    return game;
}

int activityIndex(const std::string& id) {
    for (std::size_t i = 0; i < data().activities.size(); ++i) {
        if (data().activities[i].id == id) return static_cast<int>(i);
    }
    FAIL("no such activity: " << id);
    return -1;
}

std::string textOf(const fs::path& file) {
    std::ostringstream text;
    text << std::ifstream(file).rdbuf();
    return text.str();
}

fs::path dataCopy(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-m5" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    fs::copy(ODYSSEUS_DATA_DIR, folder, fs::copy_options::recursive);
    return folder;
}

} // namespace

TEST_CASE("US-050 Defaults") {
    Run run;
    CHECK(run.life.preset().name == "Full");                               // the default Growing Period
    CHECK(data().config.comforts.at(static_cast<std::size_t>(run.life.game().comfort)).name == "Balanced");
    CHECK(run.life.ageYears() == 12);                                     // the hero starts at age 12
    CHECK(run.life.phase() == sim::Phase::Growing);
}

TEST_CASE("US-050 Seed") {
    sim::NewGame game;
    game.seed = 1234;
    Run first(game);
    Run second(game);
    CHECK(first.world.hash() == second.world.hash()); // the same clan
    CHECK(first.life.personId() == second.life.personId());
    CHECK(first.life.origin() == second.life.origin());
    CHECK(first.life.affinities() == second.life.affinities());
    sim::NewGame other = game;
    other.seed = 1235;
    Run third(other);
    CHECK((third.world.hash() != first.world.hash() || third.life.affinities() != first.life.affinities()));
}

TEST_CASE("US-050 Preset Off") {
    Run run(off());
    CHECK(run.life.ageYears() == 26);
    CHECK(run.life.phase() == sim::Phase::Free);
    CHECK_FALSE(run.life.specialty().empty());
    // The affinities come from the origin: the family's strong suit leads.
    const auto& origins = data().config.origins;
    const auto origin = std::find_if(origins.begin(), origins.end(), [&](const sim::OriginConfig& o) { return o.name == run.life.origin(); });
    REQUIRE(origin != origins.end());
    for (std::size_t i = 0; i < sim::kAffinityCount; ++i) {
        CHECK(run.life.affinities()[i] >= origin->affinity[i]);
        CHECK(run.life.affinities()[i] <= origin->affinity[i] + 10);
    }
}

TEST_CASE("US-050 Comfort") {
    const sim::SimConfig harsh = sim::configForComfort(data(), baseConfig(), 2);
    const sim::SimConfig gentle = sim::configForComfort(data(), baseConfig(), 0);
    CHECK(harsh.needs.dailyDecay[0] > gentle.needs.dailyDecay[0]);
    CHECK(harsh.clan.startingFood < gentle.clan.startingFood);
}

TEST_CASE("US-053 Curve") {
    Run run;
    CHECK(run.life.imprintPercent(12) == 300);
    CHECK(run.life.imprintPercent(19) == 175);
    CHECK(run.life.imprintPercent(26) == 50);
    CHECK(run.life.imprintPercent(27) == 25); // settled after 26
    CHECK(run.life.imprintPercent(40) == 25);
    CHECK(run.life.imprintPercent(15) < 300);
    CHECK(run.life.imprintPercent(15) > 175);
}

TEST_CASE("US-053 Tunable") {
    const fs::path folder = dataCopy("tunable");
    std::string text = textOf(folder / "hero" / "hero.json");
    const std::string peak = "\"peakPercent\": 300";
    REQUIRE(text.find(peak) != std::string::npos);
    text.replace(text.find(peak), peak.size(), "\"peakPercent\": 400");
    std::ofstream(folder / "hero" / "hero.json", std::ios::trunc) << text;
    const sim::HeroData changed = sim::loadHeroData(folder);
    sim::World world(1, baseConfig());
    sim::HeroLife life(changed, world, sim::NewGame{});
    CHECK(life.imprintPercent(12) == 400); // the new curve, no C++ change
}

TEST_CASE("US-051 Choose and effect") {
    Run run;
    CHECK_FALSE(run.life.chooseFocus(1, 1)); // two different ones
    CHECK_FALSE(run.life.chooseFocus(-1, 2));
    const int fire = activityIndex("fire");
    const int wander = activityIndex("wander");
    const auto before = run.life.affinities();
    const int skillBefore = run.life.skillPoints(3);
    REQUIRE(run.life.chooseFocus(fire, wander));
    run.life.liveYear();
    const int imprint = run.life.imprintPercent(12);
    // "Tend the fire": Fire-keeper +6 and Religion +4, times the imprint of age 12 (x3); "Wander" adds a little to all.
    const auto& a = run.life.affinities();
    CHECK(a[3] == before[3] + (6 * imprint + 99) / 100 + (1 * imprint + 99) / 100);
    CHECK(a[6] == before[6] + (4 * imprint + 99) / 100 + (1 * imprint + 99) / 100);
    CHECK(run.life.skillPoints(3) == skillBefore + (5 * imprint + 99) / 100);
    CHECK(run.life.ageYears() == 13); // a year passed
}

TEST_CASE("US-052 Crossroads") {
    CHECK(data().events.size() >= 10);
    for (const sim::CrossroadsEvent& event : data().events) {
        CAPTURE(event.id);
        CHECK(event.options.size() >= 2);
        CHECK(event.options.size() <= 3);
    }
    Run run;
    int met = 0;
    while (run.life.phase() == sim::Phase::Growing) {
        REQUIRE(run.life.chooseFocus(0, 1));
        const sim::CrossroadsEvent* event = run.life.liveYear();
        if (run.life.phase() == sim::Phase::Ended) break;
        if (event == nullptr) continue;
        ++met; // one event each year
        const auto entriesBefore = run.world.chronicle().entries().size();
        const auto affinityBefore = run.life.affinities();
        REQUIRE(run.life.resolveEvent(0));
        CHECK(run.world.chronicle().entries().size() > entriesBefore); // the chronicle records it
        CHECK(run.world.chronicle().entries().back().text == run.life.lastNote());
        (void)affinityBefore;
    }
    CHECK(met >= 10); // 14 years, one event a year while some match
}

TEST_CASE("US-052 Consequence") {
    Run run;
    REQUIRE(run.life.chooseFocus(0, 1));
    const sim::CrossroadsEvent* event = run.life.liveYear();
    REQUIRE(event != nullptr);
    const auto before = run.life.affinities();
    const sim::CrossroadsOption option = event->options.at(0);
    const int imprint = run.life.imprintPercent(12);
    REQUIRE(run.life.resolveEvent(0));
    for (std::size_t i = 0; i < sim::kAffinityCount; ++i) {
        const int expected = std::clamp(before[i] + (option.affinity[i] > 0 ? (option.affinity[i] * imprint + 99) / 100 : 0), 0, 100);
        if (option.affinity[i] >= 0) CHECK(run.life.affinities()[i] == expected);
    }
}

TEST_CASE("US-054 Mantle") {
    Run run;
    while (run.life.phase() == sim::Phase::Growing) {
        REQUIRE(run.life.chooseFocus(activityIndex("knap"), activityIndex("haggle")));
        if (run.life.liveYear() != nullptr) run.life.resolveEvent(0);
        if (run.life.phase() == sim::Phase::Ended) break;
    }
    if (run.life.phase() == sim::Phase::Free) {
        CHECK(run.life.ageYears() >= 26);
        REQUIRE_FALSE(run.life.specialty().empty());
        CHECK(run.life.specialty()[0] == 2); // a youth spent knapping flint becomes a Flint-knapper
        bool inChronicle = false;
        for (const auto& entry : run.world.chronicle().entries()) inChronicle = inChronicle || entry.text.find("took up the mantle") != std::string::npos;
        CHECK(inChronicle);
    }
}

TEST_CASE("US-054 Tie") {
    Run run(off());
    for (const sim::Affinity affinity : {sim::Affinity::Hunter, sim::Affinity::Gatherer, sim::Affinity::Knapper, sim::Affinity::FireKeeper, sim::Affinity::Shaman}) run.life.setAffinity(affinity, 10);
    run.life.setAffinity(sim::Affinity::Hunter, 50);
    run.life.setAffinity(sim::Affinity::Shaman, 50);
    run.life.mantle();
    CHECK(run.life.specialty() == std::vector<int>{0, 4}); // two equal at the top: both
    run.life.setAffinity(sim::Affinity::Shaman, 10);
    run.life.mantle();
    CHECK(run.life.specialty() == std::vector<int>{0}); // a clear winner: one
}

TEST_CASE("US-060 Professions") {
    REQUIRE(data().professions.size() == 5);
    for (const char* name : {"Hunter", "Gatherer", "Flint-knapper", "Fire-keeper", "Shaman-healer"}) {
        const auto found = std::find_if(data().professions.begin(), data().professions.end(), [&](const sim::Profession& p) { return p.name == name; });
        CHECK(found != data().professions.end());
    }
}

TEST_CASE("US-060 Validation") {
    const fs::path folder = dataCopy("validation");
    std::string text = textOf(folder / "hero" / "professions.json");
    const std::string tool = "\"hammerstone\"";
    REQUIRE(text.find(tool) != std::string::npos);
    text.replace(text.find(tool), tool.size(), "\"no-such-tool\"");
    std::ofstream(folder / "hero" / "professions.json", std::ios::trunc) << text;
    try {
        sim::loadHeroData(folder);
        FAIL("a missing tool was accepted");
    } catch (const sim::DataError& error) {
        const std::string message = error.what();
        CHECK(message.find("professions.json") != std::string::npos);
        CHECK(message.find("Flint-knapper") != std::string::npos);
        CHECK(message.find("no-such-tool") != std::string::npos);
    }
}

TEST_CASE("US-061 Tool-gated") {
    Run run(off());
    const sim::ActionResult blocked = run.life.knapFlint();
    CHECK_FALSE(blocked.ok);
    CHECK(blocked.message == "Needs a hammerstone");
    CHECK(run.life.count("flint") == 0);
    run.life.give("hammerstone", 1);
    CHECK(run.life.knapFlint().ok);
    CHECK(run.life.count("flint") == 2);
    CHECK(run.life.gatherBerries().ok);
    CHECK(run.life.count("berries") == 2);
}

TEST_CASE("US-062 Craft") {
    Run run(off());
    run.life.give("flint", 2);
    run.life.give("hammerstone", 1);
    const sim::Recipe* spearhead = data().recipe("spearhead");
    REQUIRE(spearhead != nullptr);
    CHECK(run.life.craftBlockedReason(*spearhead).empty());
    const int points = run.life.skillPoints(2);
    const sim::ActionResult made = run.life.craft("spearhead");
    CHECK(made.ok);
    CHECK(run.life.count("flint") == 0);       // the flint is consumed
    CHECK(run.life.count("spearhead") == 1);
    CHECK(run.life.count("hammerstone") == 1); // the tool is not
    CHECK(run.life.skillPoints(2) > points);   // a success is experience
    CHECK_FALSE(run.life.craft("spearhead").ok); // nothing left to make it from
    CHECK(run.life.craftBlockedReason(*spearhead).find("Flint") != std::string::npos);
}

TEST_CASE("US-062 Quality") {
    const auto averageQuality = [](int points) {
        Run run(off(7));
        run.life.setSkillPoints(2, points);
        long long total = 0;
        const int crafts = 300;
        for (int i = 0; i < crafts; ++i) {
            run.life.give("flint", 1);
            run.life.give("hammerstone", 1);
            run.life.setSkillPoints(2, points); // keep the skill fixed for the comparison
            REQUIRE(run.life.craft("flint-tool").ok);
        }
        for (const sim::CraftedItem& item : run.life.crafted()) total += item.quality;
        return static_cast<double>(total) / crafts;
    };
    CHECK(averageQuality(50) > averageQuality(10) + 0.5); // skill 5 against skill 1
}

TEST_CASE("US-062 Provenance") {
    Run run(off());
    run.life.give("flint", 2);
    run.life.give("hammerstone", 1);
    REQUIRE(run.life.craft("spearhead").ok);
    REQUIRE(run.life.crafted().size() == 1);
    const std::string text = run.life.describe(run.life.crafted()[0]);
    CHECK(text.find(run.life.name()) != std::string::npos); // who made it
    CHECK(text.find("Flint") != std::string::npos);          // from what
    CHECK(text.find("year") != std::string::npos);            // and when
}

TEST_CASE("US-063 Apprenticeship") {
    Run run(off(3));
    const int profession = 2;
    const int master = run.life.masterOf(profession);
    REQUIRE(master >= 0);
    // Gated: a master who does not know you refuses, and says why.
    run.world.adjustOpinion(master, run.life.personId(), -100);
    const sim::ActionResult refused = run.life.askToApprentice(profession);
    CHECK_FALSE(refused.ok);
    CHECK(refused.message.find("will not teach") != std::string::npos);
    CHECK(run.life.apprenticeOf(profession) < 0);
    // Trust earned: accepted, and the skill grows faster beside the master.
    run.world.adjustOpinion(master, run.life.personId(), 100 + data().config.trustOpinion);
    const sim::ActionResult accepted = run.life.askToApprentice(profession);
    CHECK(accepted.ok);
    CHECK(run.life.apprenticeOf(profession) == master);
    Run alone(off(3));
    const int withMaster = run.life.useSkill(profession, true);
    const int without = alone.life.useSkill(profession, true);
    CHECK(withMaster > without);
    // Every use of a skill gives experience.
    const int before = run.life.skillPoints(0);
    run.life.useSkill(0, true);
    CHECK(run.life.skillPoints(0) > before);
}

TEST_CASE("US-070 Dominion") {
    Run run(off());
    run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    const int trade = run.life.tradePercent();
    CHECK(trade > 0);
    CHECK(trade < 60);
    CHECK(run.life.religionPercent() == 0); // no sacred fire yet
    // A trade changes the percentage.
    run.life.give("fur", 10);
    sim::BarterOffer offer;
    offer.give = {{"fur", 5}};
    offer.want = {{"flint-tool", 3}};
    REQUIRE(run.life.barter(0, offer).accepted);
    CHECK(run.life.tradePercent() > trade);
}

TEST_CASE("US-071 Barter") {
    Run run(off());
    run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    run.life.give("fur", 10);
    sim::BarterOffer offer;
    offer.give = {{"fur", 5}};
    offer.want = {{"flint-tool", 3}};
    const sim::BarterResult accepted = run.life.barter(0, offer); // the River Clan wants furs
    CHECK(accepted.accepted);
    CHECK(run.life.count("fur") == 5);
    CHECK(run.life.count("flint-tool") == 3);
    // Too little offered: a counter, which is then accepted.
    sim::BarterOffer stingy;
    stingy.give = {{"fur", 1}};
    stingy.want = {{"flint-tool", 3}};
    const sim::BarterResult countered = run.life.barter(0, stingy);
    CHECK_FALSE(countered.accepted);
    REQUIRE(countered.counter.has_value());
    CHECK(countered.counter->give.front().second > 1);
    CHECK(run.life.barter(0, *countered.counter).accepted);
    // The hero cannot give what they do not have.
    sim::BarterOffer imaginary;
    imaginary.give = {{"spear", 3}};
    imaginary.want = {{"berries", 1}};
    CHECK_FALSE(run.life.barter(0, imaginary).accepted);
}

TEST_CASE("US-071 Debt and default") {
    Run run(off());
    run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    sim::BarterOffer onCredit;
    onCredit.give = {{"fur", 6}};
    onCredit.want = {{"flint-tool", 3}};
    onCredit.payLater = true;
    const int tradeBefore = run.life.tradePoints();
    REQUIRE(run.life.barter(0, onCredit).accepted);
    CHECK(run.life.count("flint-tool") == 3); // the goods now
    REQUIRE(run.life.debts().size() == 1);    // a debt is recorded
    CHECK(run.life.tradePoints() > tradeBefore); // and counts toward Trade
    const int relationBefore = run.life.relation(0);
    const int tradeAfterDeal = run.life.tradePoints();
    run.days(data().config.trade.dueDays + 3);  // past its due date, unpaid
    CHECK(run.life.debts().front().defaulted);
    CHECK(run.life.relation(0) < relationBefore);
    CHECK(run.life.tradePoints() < tradeAfterDeal);
    // A debt paid in time does not default.
    Run another(off(2));
    another.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    REQUIRE(another.life.barter(0, onCredit).accepted);
    another.life.give("fur", 6);
    CHECK(another.life.payDebt(0).ok);
    another.days(data().config.trade.dueDays + 3);
    CHECK_FALSE(another.life.debts().front().defaulted);
}

TEST_CASE("US-072 Sacred fire") {
    Run run(off(5));
    run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    run.life.setSkillPoints(3, 0);
    // Not everyone may found one.
    const bool shaman = std::find(run.life.specialty().begin(), run.life.specialty().end(), 4) != run.life.specialty().end();
    if (!shaman) {
        CHECK_FALSE(run.life.foundFire("Dawn", 10, 10).ok);
        CHECK_FALSE(run.life.foundFireBlockedReason().empty());
    }
    run.life.setSkillPoints(3, 30); // Fire-keeping level 3
    CHECK(run.life.foundFireBlockedReason().empty());
    REQUIRE(run.life.foundFire("Dawn of the Clan", 10, 10).ok);
    CHECK(run.life.fire().name == "Dawn of the Clan"); // the owner's own name
    CHECK(run.life.fire().lit);
    // A ritual needs 5 at the fire.
    CHECK_FALSE(run.life.holdRitual({1, 2}).ok);
    std::vector<int> attendees;
    for (const sim::Person& person : run.world.people()) {
        if (person.alive && person.id != run.life.personId() && attendees.size() < 6) attendees.push_back(person.id);
    }
    REQUIRE(attendees.size() >= 5);
    REQUIRE(run.life.holdRitual(attendees).ok);
    for (const int id : attendees) CHECK(run.life.faith(id) >= data().config.fire.ritualFaith);
    CHECK(run.life.religionPercent() > 0);
    // Rituals draw listeners from the rivals over time.
    const int followersAfterFirst = run.life.followers();
    for (int i = 0; i < 40; ++i) run.life.holdRitual(attendees);
    CHECK(run.life.followers() > followersAfterFirst);
}

TEST_CASE("US-072 Neglect") {
    Run run(off(5));
    run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    run.life.setSkillPoints(3, 30);
    REQUIRE(run.life.foundFire("Ember", 10, 10).ok);
    std::vector<int> attendees;
    for (const sim::Person& person : run.world.people()) {
        if (person.alive && person.id != run.life.personId() && attendees.size() < 6) attendees.push_back(person.id);
    }
    REQUIRE(run.life.holdRitual(attendees).ok);
    const int faith = run.life.faith(attendees.front());
    run.days(data().config.fire.untendedDaysBeforeOut + 3); // untended
    CHECK_FALSE(run.life.fire().lit);                        // it goes out
    CHECK(run.life.faith(attendees.front()) < faith);        // and followers lose faith
    CHECK_FALSE(run.life.holdRitual(attendees).ok);
    // Tending it again (with wood) lights it.
    run.life.give("wood", 1);
    CHECK(run.life.tendFire().ok);
    CHECK(run.life.fire().lit);
}

TEST_CASE("US-073 Win and lose") {
    SUBCASE("a clan that dominates trade wins") {
        Run run(off());
        run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
        run.life.addTradePoints(100000);
        run.days(1);
        CHECK(run.life.phase() == sim::Phase::Ended);
        CHECK(run.life.outcome() == sim::Outcome::Victory);
        const sim::RunSummary summary = run.life.summary();
        CHECK(summary.tradePercent >= 60);
        CHECK_FALSE(summary.specialty.empty());
        CHECK_FALSE(summary.highlights.empty());
    }
    SUBCASE("a clan below three people loses") {
        Run run(off());
        run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
        int alive = 0;
        for (const sim::Person& person : run.world.people()) {
            if (person.alive && person.id != run.life.personId() && ++alive > 1) run.world.kill(person.id, sim::CauseOfDeath::Illness);
        }
        run.days(1);
        CHECK(run.life.phase() == sim::Phase::Ended);
        CHECK(run.life.outcome() == sim::Outcome::Defeat);
    }
}

TEST_CASE("US-055 Aging and death") {
    Run run(off());
    CHECK(run.life.agingPercent(39) == 0);
    CHECK(run.life.agingPercent(40) == data().config.aging.startPercent);
    CHECK(run.life.agingPercent(50) == data().config.aging.startPercent + 10 * data().config.aging.perYearPercent);
    SUBCASE("death of an adult ends the run with a summary") {
        run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
        run.world.kill(run.life.personId(), sim::CauseOfDeath::Illness);
        run.days(1);
        CHECK(run.life.phase() == sim::Phase::Ended);
        CHECK(run.life.outcome() == sim::Outcome::Died);
        const sim::RunSummary summary = run.life.summary();
        CHECK_FALSE(summary.highlights.empty());
        CHECK_FALSE(summary.specialty.empty());
    }
    SUBCASE("death before the mantle is a loss") {
        Run young;
        REQUIRE(young.life.chooseFocus(0, 1));
        young.world.kill(young.life.personId(), sim::CauseOfDeath::Illness);
        young.life.liveYear();
        CHECK(young.life.phase() == sim::Phase::Ended);
        CHECK(young.life.outcome() == sim::Outcome::Defeat);
    }
}

TEST_CASE("US-055 Energy decay grows with age") {
    Run run(off(9));
    sim::Person* hero = run.world.personMutable(run.life.personId());
    REQUIRE(hero != nullptr);
    hero->ageDays = 50 * run.world.calendar().daysPerYear();
    hero->needs[sim::Need::Energy] = 100;
    run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    const int before = hero->needs[sim::Need::Energy];
    run.days(1);
    // The daily wear plus the extra of age: at 50, 30 percent more than at 39.
    CHECK(run.world.people()[static_cast<std::size_t>(run.life.personId())].needs[sim::Need::Energy] < before);
}

TEST_CASE("US-080 The run is saved") {
    const fs::path file = fs::temp_directory_path() / "odysseus-m5" / "hero.json";
    fs::remove_all(file.parent_path());
    Run run(off(11));
    run.life.setRivals({{"the River Clan", 15}, {"the Stone Clan", 15}});
    run.life.give("flint", 3);
    run.life.setSkillPoints(3, 33);
    REQUIRE(run.life.foundFire("Kept", 4, 5).ok);
    run.life.save(file);
    sim::HeroLife loaded = sim::HeroLife::load(data(), run.world, file);
    CHECK(loaded.personId() == run.life.personId());
    CHECK(loaded.count("flint") == 3);
    CHECK(loaded.skillPoints(3) == 33);
    CHECK(loaded.fire().name == "Kept");
    CHECK(loaded.specialty() == run.life.specialty());
    CHECK(loaded.phase() == run.life.phase());
}

TEST_CASE("US-082 Budget") {
    // 500 simulated people: a tick stays under 10 ms on average (Release builds; Debug builds with their checks are slower).
    sim::SimConfig config = baseConfig();
    config.clan.startingPeople = 500;
    sim::World world(1, config);
    const int ticks = 2 * world.calendar().ticksPerDay();
    double worst = 0.0;
    const auto began = std::chrono::steady_clock::now();
    for (int i = 0; i < ticks; ++i) {
        const auto tickStarted = std::chrono::steady_clock::now();
        world.tick();
        worst = std::max(worst, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - tickStarted).count());
    }
    const double average = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - began).count() / ticks;
    MESSAGE("500 people: average tick " << average << " ms, worst " << worst << " ms");
#ifdef NDEBUG
    CHECK(average < 10.0);
    CHECK(worst < 50.0);
#else
    CHECK(average < 100.0);
#endif
}
