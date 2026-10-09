// US-206 Clans and people inspector: the setup sections of the world file, the checks, and what a game starts with.
#include "sim/data.h"
#include "sim/hero_data.h"
#include "sim/hero_life.h"
#include "sim/person.h"
#include "sim/region.h"
#include "sim/rivals.h"
#include "sim/world.h"
#include "sim/world_file.h"
#include "sim/world_setup.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

sim::RegionConfig regionConfig() { return sim::loadRegionConfig(fs::path(ODYSSEUS_DATA_DIR) / "sim" / "region.json"); }
sim::SimConfig simConfig() { return sim::loadSimConfig(ODYSSEUS_DATA_DIR); }

const sim::HeroData& heroData() {
    static const sim::HeroData loaded = sim::loadHeroData(ODYSSEUS_DATA_DIR);
    return loaded;
}

struct Folder {
    fs::path path;
    Folder() : path(fs::temp_directory_path() / ("odysseus-us206-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))) { fs::create_directories(path); }
    ~Folder() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};

} // namespace

TEST_CASE("US-206 Social: an opinion of -50 and a grudge with a reason are there when the game starts, and the chronicle tells it") {
    sim::World world(1, simConfig());
    sim::World untouched(1, simConfig());
    REQUIRE(world.people().size() >= 3);
    const int before = world.opinion(1, 0);

    sim::WorldSetup setup;
    sim::PersonSetup person;
    std::string problem;
    REQUIRE(sim::setOpinionsText(person, "1=-50", problem));
    REQUIRE(sim::setGrudgesText(person, "1:30:stole the last flint", problem));
    setup.people["0"] = person;
    std::vector<std::string> problems;
    sim::applyClanSetup(world, setup, problems);
    CHECK(problems.empty());

    CHECK(world.opinion(0, 1) == -50); // as written, not added to what was there
    CHECK(world.opinion(1, 0) == before); // only that person changed
    const sim::Grudge* grudge = sim::heaviestGrudge(world.people()[0], 1);
    REQUIRE(grudge != nullptr);
    CHECK(grudge->weight == 30);
    const sim::ChronicleEntry* told = world.chronicle().find(grudge->event);
    REQUIRE(told != nullptr);
    CHECK(told->text.find("stole the last flint") != std::string::npos);
    CHECK(told->text.find(world.people()[0].name) != std::string::npos);
    CHECK(untouched.chronicle().entries().size() < world.chronicle().entries().size()); // the chronicle has the new entry
    CHECK(untouched.opinion(0, 1) != -50);

    // A member who is not in the clan is named, and the rest is still applied.
    sim::PersonSetup stranger;
    REQUIRE(sim::setOpinionsText(stranger, "1=10", problem));
    setup.people["999"] = stranger;
    problems.clear();
    sim::applyClanSetup(world, setup, problems);
    CHECK(problems.size() == 1);
    CHECK(problems[0].find("999") != std::string::npos);
}

TEST_CASE("US-206 Kin: mother, father and partner are set, and a partner names the other back") {
    sim::World world(1, simConfig());
    sim::WorldSetup setup;
    sim::PersonSetup person;
    std::string problem;
    REQUIRE(sim::setKinText(person, "mother=1 father=2 partner=3", problem));
    setup.people["0"] = person;
    std::vector<std::string> problems;
    sim::applyClanSetup(world, setup, problems);
    CHECK(world.people()[0].mother == 1);
    CHECK(world.people()[0].father == 2);
    CHECK(world.people()[0].partner == 3);
    CHECK(world.people()[3].partner == 0);
    CHECK_FALSE(sim::setKinText(person, "cousin=4", problem));
    CHECK_FALSE(sim::setKinText(person, "mother=x", problem));
}

TEST_CASE("US-206 Economy: the store and a debt to a rival are there when the game starts") {
    sim::World world(1, simConfig());
    sim::HeroLife life(heroData(), world, sim::NewGame{});
    life.setRivals({{"the River Clan", 12}, {"the Stone Clan", 14}});
    REQUIRE_FALSE(heroData().items.empty());
    const std::string item = heroData().items.front().id;

    sim::WorldSetup setup;
    sim::ClanSetup clan;
    std::string problem;
    REQUIRE(sim::setStoreText(clan, "food=80 " + item + "=7", problem));
    REQUIRE(sim::setDebtsText(clan, "the Stone Clan: " + item + "=3 value=12 days=10", problem));
    setup.clans["player"] = clan;
    std::vector<std::string> problems;
    sim::applyClanSetup(world, setup, problems);
    sim::applyHeroSetup(life, setup, problems);
    CHECK(problems.empty());
    CHECK(world.food() == 80);
    CHECK(life.inventory().at(item) == 7);
    REQUIRE(life.debts().size() == 1);
    CHECK(life.debts()[0].rival == 1);
    CHECK(life.debts()[0].value == 12);
    CHECK(life.debts()[0].owe.front().second == 3);
    CHECK(life.debts()[0].dueDay == world.date().day + 10);

    // A debt to a clan that is not in the game, and an item that does not exist, are named.
    sim::WorldSetup wrong;
    REQUIRE(sim::setDebtsText(wrong.clans["player"], "the Moon Clan: value=1", problem));
    REQUIRE(sim::setStoreText(wrong.clans["player"], "unobtainium=2", problem));
    problems.clear();
    sim::applyHeroSetup(life, wrong, problems);
    CHECK(problems.size() == 2);
    CHECK(life.debts().size() == 1);
}

TEST_CASE("US-206 Economy: a rival clan's store") {
    sim::Region land(1, regionConfig());
    sim::Rivals rivals(land, land.start(), 1 ^ 0x5151ULL, simConfig());
    sim::WorldSetup setup;
    std::string problem;
    REQUIRE(sim::setStoreText(setup.clans["the River Clan"], "food=77", problem));
    std::vector<std::string> problems;
    sim::applyRivalSetup(rivals, setup, problems);
    CHECK(problems.empty());
    CHECK(rivals.clans()[0].world->food() == 77);
    setup.clans["the Moon Clan"].food = 5;
    problems.clear();
    sim::applyRivalSetup(rivals, setup, problems);
    CHECK(problems.size() == 1);
}

TEST_CASE("US-206 Text fields: the text the owner types and back, and a mistake changes nothing") {
    sim::ClanSetup clan;
    std::string problem;
    REQUIRE(sim::setStoreText(clan, "food=80 flint=20", problem));
    CHECK(clan.food == 80);
    CHECK(sim::storeText(clan) == "food=80 flint=20");
    CHECK_FALSE(sim::setStoreText(clan, "food=lots", problem));
    CHECK_FALSE(problem.empty());
    CHECK(clan.food == 80); // unchanged
    REQUIRE(sim::setDebtsText(clan, "the River Clan: fur=3 value=12 days=10; the Ash Clan: flint=1", problem));
    REQUIRE(clan.debts.size() == 2);
    CHECK(clan.debts[1].days == 10);
    CHECK(sim::debtsText(clan) == "the River Clan: fur=3 value=12 days=10; the Ash Clan: flint=1 value=0 days=10");
    CHECK_FALSE(sim::setDebtsText(clan, "no colon here", problem));
    CHECK(sim::parseList("the River Clan, the Ash Clan") == std::vector<std::string>{"the River Clan", "the Ash Clan"});
    CHECK(sim::parseList("the River Clan") == std::vector<std::string>{"the River Clan"});

    sim::PersonSetup person;
    CHECK_FALSE(sim::setOpinionsText(person, "1=500", problem));
    CHECK_FALSE(sim::setOpinionsText(person, "x=5", problem));
    CHECK_FALSE(sim::setGrudgesText(person, "1:30:", problem)); // a grudge needs its reason
    CHECK_FALSE(sim::setGrudgesText(person, "1:300:why", problem));
    REQUIRE(sim::setGrudgesText(person, "1:30:stole the last flint; 2:5:laughed at the fire", problem));
    CHECK(sim::grudgesText(person) == "1:30:stole the last flint; 2:5:laughed at the fire");
    REQUIRE(sim::setOpinionsText(person, "", problem));
    CHECK(person.opinions.empty());
}

TEST_CASE("US-206 Checks: a clan, debt or partner that is not in the world, a member in two clans, a kin cycle and a partner who does not answer are listed") {
    sim::RegionEdits edits;
    sim::PlacedEdit rival;
    rival.group = sim::EditGroup::Camp;
    rival.kind = "rival";
    rival.name = "the Crow Clan";
    rival.id = "c-0001";
    edits.placed.push_back(rival);

    sim::WorldSetup fine;
    std::string problem;
    REQUIRE(sim::setDebtsText(fine.clans["player"], "the Crow Clan: value=3", problem));
    REQUIRE(sim::setKinText(fine.people["1"], "mother=2", problem));
    CHECK(sim::setupProblems(fine, edits).empty());

    sim::WorldSetup bad;
    bad.clans["the Moon Clan"].stance = "wary";
    REQUIRE(sim::setDebtsText(bad.clans["player"], "the Moon Clan: value=3", problem));
    bad.clans["player"].partners = {"the Crow Clan", "the Sun Clan"};
    bad.clans["player"].members = {"4"};
    bad.clans["the Crow Clan"].members = {"4"};
    REQUIRE(sim::setKinText(bad.people["1"], "mother=2 partner=5", problem));
    REQUIRE(sim::setKinText(bad.people["2"], "mother=1", problem)); // 1 is the mother of 2 and 2 of 1
    REQUIRE(sim::setKinText(bad.people["5"], "partner=6", problem));
    const std::vector<std::string> problems = sim::setupProblems(bad, edits);
    const auto mentions = [&problems](const std::string& text) {
        return std::any_of(problems.begin(), problems.end(), [&text](const std::string& line) { return line.find(text) != std::string::npos; });
    };
    CHECK(mentions("clan 'the Moon Clan' is not in the world"));
    CHECK(mentions("debt to 'the Moon Clan'"));
    CHECK(mentions("trades with 'the Sun Clan'"));
    CHECK(mentions("member 4 is in two clans"));
    CHECK(mentions("own ancestor"));
    CHECK(mentions("names 6 as partner") == false);
    CHECK(mentions("member 1 names 5 as partner")); // 5 names 6, not 1
}

TEST_CASE("US-206 World file: the clans and people sections round-trip and a bad one names the file and the field") {
    Folder folder;
    sim::WorldFile world;
    world.seed = 1;
    std::string problem;
    REQUIRE(sim::setStoreText(world.setup.clans["player"], "food=80 flint=20", problem));
    REQUIRE(sim::setDebtsText(world.setup.clans["player"], "the River Clan: fur=3 value=12 days=7", problem));
    world.setup.clans["player"].leader = "3";
    world.setup.clans["player"].partners = {"the River Clan"};
    world.setup.clans["the Crow Clan"].stance = "wary";
    REQUIRE(sim::setKinText(world.setup.people["0"], "mother=1 partner=2", problem));
    REQUIRE(sim::setOpinionsText(world.setup.people["0"], "1=-50 2=30", problem));
    REQUIRE(sim::setGrudgesText(world.setup.people["0"], "1:30:stole the last flint", problem));

    sim::saveWorld(world, folder.path / "w.json", regionConfig());
    const sim::WorldFile loaded = sim::loadWorld(folder.path / "w.json", regionConfig());
    CHECK(loaded.setup == world.setup);
    CHECK(sim::worldText(loaded, regionConfig()) == sim::worldText(world, regionConfig()));

    std::string text = sim::worldText(world, regionConfig());
    const std::size_t at = text.find("\"-50\"");
    const std::size_t number = text.find("-50");
    REQUIRE(number != std::string::npos);
    (void)at;
    text.replace(number, 3, "-500");
    std::ofstream(folder.path / "bad.json") << text;
    try {
        (void)sim::loadWorld(folder.path / "bad.json", regionConfig());
        FAIL("an opinion of -500 was accepted");
    } catch (const sim::DataError& error) {
        const std::string what = error.what();
        CHECK(what.find("bad.json") != std::string::npos);
        CHECK(what.find("people.0.opinions.1") != std::string::npos);
    }
}
