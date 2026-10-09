#pragma once

#include "boundary.h"

#include "region_edits.h"

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace odysseus::sim {

class World;
class HeroLife;
class Rivals;

// The social, economic and political setup of the world file (US-206, design M12 section 10, brief 4.6): the `clans` and `people` sections. Only differences from
// what the game would make are stored. A clan is "player" (the hero's clan) or a rival camp's name; a person is a clan member's number (the people of the placed
// NPCs are set by their own entries and properties, US-204).
using NamedCounts = std::vector<std::pair<std::string, int>>; // an item id and how many, in the order written

struct DebtSetup {
    std::string to;     // a clan of the world (a rival camp's name)
    NamedCounts goods;   // what is owed
    int value = 0;      // what it is worth
    int days = 10;      // days until it falls due
    friend bool operator==(const DebtSetup&, const DebtSetup&) = default;
};

struct ClanSetup {
    std::string leader;               // a clan member's number, or a name
    std::string stance;               // towards the player: a word (friendly, wary, hostile...) kept for the politics of M13
    int food = -1;                    // meals in the store; -1: not set
    NamedCounts items;                 // other things in the store
    std::vector<DebtSetup> debts;     // what this clan owes
    std::vector<std::string> partners; // clans it trades with
    std::vector<std::string> members; // clan members' numbers it claims (a check, not a move)
    bool empty() const { return leader.empty() && stance.empty() && food < 0 && items.empty() && debts.empty() && partners.empty() && members.empty(); }
    friend bool operator==(const ClanSetup&, const ClanSetup&) = default;
};

struct GrudgeSetup {
    std::string about;  // a clan member's number
    int weight = 20;    // how much it counts (opinion lost), 1 to 100
    std::string reason; // told by the chronicle
    friend bool operator==(const GrudgeSetup&, const GrudgeSetup&) = default;
};

struct PersonSetup {
    std::string mother;
    std::string father;
    std::string partner;
    NamedCounts opinions; // another member's number and what this one thinks of them, -100 to 100
    std::vector<GrudgeSetup> grudges;
    bool empty() const { return mother.empty() && father.empty() && partner.empty() && opinions.empty() && grudges.empty(); }
    friend bool operator==(const PersonSetup&, const PersonSetup&) = default;
};

struct WorldSetup {
    std::map<std::string, ClanSetup> clans;
    std::map<std::string, PersonSetup> people;
    bool empty() const { return clans.empty() && people.empty(); }
    friend bool operator==(const WorldSetup&, const WorldSetup&) = default;
};

// The file: the two sections as the world file writes them, and back. A mistake is a DataError naming the file and the field.
nlohmann::ordered_json setupSectionsToJson(const WorldSetup& setup);
WorldSetup setupFromJson(const nlohmann::json& data, const std::filesystem::path& file);

// The Editor's text fields, as the owner types them, and back (an empty text clears; a mistake changes nothing and says what is wrong).
//   store:     "food=80 flint=20"            (food is the meals; the rest are items)
//   debts:     "the River Clan: fur=3 value=12 days=10; ..."
//   list:      names or numbers separated by commas or spaces (partners, members)
//   opinions:  "5=-50 7=20"                  (member number = opinion)
//   grudges:   "5:30:stole the last flint; ..." (member: weight: reason)
//   kin:       "mother=2 father=3 partner=4"
std::string storeText(const ClanSetup& clan);
bool setStoreText(ClanSetup& clan, const std::string& text, std::string& problem);
std::string debtsText(const ClanSetup& clan);
bool setDebtsText(ClanSetup& clan, const std::string& text, std::string& problem);
std::string listText(const std::vector<std::string>& names);
std::vector<std::string> parseList(const std::string& text);
std::string opinionsText(const PersonSetup& person);
bool setOpinionsText(PersonSetup& person, const std::string& text, std::string& problem);
std::string grudgesText(const PersonSetup& person);
bool setGrudgesText(PersonSetup& person, const std::string& text, std::string& problem);
std::string kinText(const PersonSetup& person);
bool setKinText(PersonSetup& person, const std::string& text, std::string& problem);

// What is wrong in the setup, in plain words (never a block; the Editor lists them): a clan that is not in the world, a debt to or a partner that is not a clan of the
// world, a member in two clans, a kin cycle, a partner who does not name them back, a person key that is not a member number. `edits` give the rival camps' names.
std::vector<std::string> setupProblems(const WorldSetup& setup, const RegionEdits& edits);

// Applying the setup when a game starts. Each says what could not be applied in `problems` and applies the rest.
// The clan world: the player's store of meals, and every member set: opinions (as written, not added), kin, and grudges, each told by a chronicle entry that is its reason.
void applyClanSetup(World& world, const WorldSetup& setup, std::vector<std::string>& problems);
// The hero's run: the store's items and the debts to rival clans (a rival is found by its name).
void applyHeroSetup(HeroLife& life, const WorldSetup& setup, std::vector<std::string>& problems);
// The rival clans: the meals in each one's store (the clan is found by its name).
void applyRivalSetup(Rivals& rivals, const WorldSetup& setup, std::vector<std::string>& problems);

} // namespace odysseus::sim
