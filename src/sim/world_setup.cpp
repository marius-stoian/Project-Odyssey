#include "world_setup.h"

#include "hero_life.h"
#include "json_data.h"
#include "json_patch.h"
#include "rivals.h"
#include "world.h"

#include <algorithm>
#include <charconv>
#include <format>
#include <set>

namespace odysseus::sim {

namespace {

using nlohmann::json;

bool parseInt(const std::string& text, int& value) {
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return !text.empty() && result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

bool memberNumber(const std::string& text, int& id) { return parseInt(text, id) && id >= 0 && id < 100000; }

std::string trim(std::string text) {
    text.erase(0, text.find_first_not_of(" \t"));
    text.erase(text.find_last_not_of(" \t") + 1);
    return text;
}

std::vector<std::string> split(const std::string& text, char separator) {
    std::vector<std::string> parts;
    std::size_t from = 0;
    while (from <= text.size()) {
        std::size_t to = text.find(separator, from);
        if (to == std::string::npos) to = text.size();
        parts.push_back(trim(text.substr(from, to - from)));
        from = to + 1;
    }
    return parts;
}

// Words separated by spaces or commas.
std::vector<std::string> words(const std::string& text) {
    std::vector<std::string> found;
    std::string word;
    for (const char c : text + " ") {
        if (c == ' ' || c == ',' || c == '\t') {
            if (!word.empty()) found.push_back(word);
            word.clear();
        } else {
            word += c;
        }
    }
    return found;
}

// "key=number" words.
bool keyNumbers(const std::string& text, std::vector<std::pair<std::string, int>>& out, std::string& problem) {
    for (const std::string& word : words(text)) {
        const std::size_t equals = word.find('=');
        int value = 0;
        if (equals == std::string::npos || equals == 0 || !parseInt(word.substr(equals + 1), value)) {
            problem = "'" + word + "': write name=number";
            return false;
        }
        out.push_back({word.substr(0, equals), value});
    }
    return true;
}


} // namespace

// --- Text fields ---

std::string storeText(const ClanSetup& clan) {
    std::string text;
    if (clan.food >= 0) text = std::format("food={}", clan.food);
    for (const auto& [item, count] : clan.items) text += (text.empty() ? "" : " ") + std::format("{}={}", item, count);
    return text;
}

bool setStoreText(ClanSetup& clan, const std::string& text, std::string& problem) {
    std::vector<std::pair<std::string, int>> found;
    if (!keyNumbers(text, found, problem)) return false;
    int food = -1;
    NamedCounts items;
    for (const auto& [key, value] : found) {
        if (value < 0 || value > 100000) {
            problem = "'" + key + "': a count is from 0 to 100000";
            return false;
        }
        if (key == "food") food = value;
        else items.push_back({key, value});
    }
    clan.food = food;
    clan.items = std::move(items);
    return true;
}

std::string debtsText(const ClanSetup& clan) {
    std::string text;
    for (const DebtSetup& debt : clan.debts) {
        std::string line = debt.to + ":";
        for (const auto& [item, count] : debt.goods) line += std::format(" {}={}", item, count);
        line += std::format(" value={} days={}", debt.value, debt.days);
        text += (text.empty() ? "" : "; ") + line;
    }
    return text;
}

bool setDebtsText(ClanSetup& clan, const std::string& text, std::string& problem) {
    std::vector<DebtSetup> debts;
    for (const std::string& part : split(text, ';')) {
        if (part.empty()) continue;
        const std::size_t colon = part.find(':');
        if (colon == std::string::npos || trim(part.substr(0, colon)).empty()) {
            problem = "'" + part + "': write clan: item=count value=number days=number";
            return false;
        }
        DebtSetup debt;
        debt.to = trim(part.substr(0, colon));
        std::vector<std::pair<std::string, int>> found;
        if (!keyNumbers(part.substr(colon + 1), found, problem)) return false;
        for (const auto& [key, value] : found) {
            if (value < 0 || value > 100000) {
                problem = "'" + key + "': a number is from 0 to 100000";
                return false;
            }
            if (key == "value") debt.value = value;
            else if (key == "days") debt.days = std::max(1, value);
            else debt.goods.push_back({key, value});
        }
        debts.push_back(std::move(debt));
    }
    clan.debts = std::move(debts);
    return true;
}

std::string listText(const std::vector<std::string>& names) {
    std::string text;
    for (const std::string& name : names) text += (text.empty() ? "" : ", ") + name;
    return text;
}

std::vector<std::string> parseList(const std::string& text) {
    // Names may hold spaces ("the River Clan"), so a list is separated by commas only when there is one; else by spaces.
    if (text.find(',') != std::string::npos) {
        std::vector<std::string> names;
        for (const std::string& part : split(text, ',')) {
            if (!part.empty()) names.push_back(part);
        }
        return names;
    }
    const std::string one = trim(text);
    return one.empty() ? std::vector<std::string>{} : std::vector<std::string>{one};
}

std::string opinionsText(const PersonSetup& person) {
    std::string text;
    for (const auto& [other, value] : person.opinions) text += (text.empty() ? "" : " ") + std::format("{}={}", other, value);
    return text;
}

bool setOpinionsText(PersonSetup& person, const std::string& text, std::string& problem) {
    std::vector<std::pair<std::string, int>> found;
    if (!keyNumbers(text, found, problem)) return false;
    for (const auto& [other, value] : found) {
        int id = 0;
        if (!memberNumber(other, id)) {
            problem = "'" + other + "' is not a clan member's number";
            return false;
        }
        if (value < -100 || value > 100) {
            problem = "an opinion is from -100 to 100";
            return false;
        }
    }
    person.opinions = std::move(found);
    return true;
}

std::string grudgesText(const PersonSetup& person) {
    std::string text;
    for (const GrudgeSetup& grudge : person.grudges) text += (text.empty() ? "" : "; ") + std::format("{}:{}:{}", grudge.about, grudge.weight, grudge.reason);
    return text;
}

bool setGrudgesText(PersonSetup& person, const std::string& text, std::string& problem) {
    std::vector<GrudgeSetup> grudges;
    for (const std::string& part : split(text, ';')) {
        if (part.empty()) continue;
        const std::size_t first = part.find(':');
        const std::size_t second = first == std::string::npos ? first : part.find(':', first + 1);
        int about = 0;
        int weight = 0;
        if (second == std::string::npos || !memberNumber(trim(part.substr(0, first)), about) || !parseInt(trim(part.substr(first + 1, second - first - 1)), weight)) {
            problem = "'" + part + "': write member:weight:reason";
            return false;
        }
        const std::string reason = trim(part.substr(second + 1));
        if (weight < 1 || weight > 100 || reason.empty()) {
            problem = "a grudge needs a weight from 1 to 100 and a reason";
            return false;
        }
        grudges.push_back({std::to_string(about), weight, reason});
    }
    person.grudges = std::move(grudges);
    return true;
}

std::string kinText(const PersonSetup& person) {
    std::string text;
    const auto add = [&text](const char* label, const std::string& value) {
        if (!value.empty()) text += (text.empty() ? "" : " ") + std::format("{}={}", label, value);
    };
    add("mother", person.mother);
    add("father", person.father);
    add("partner", person.partner);
    return text;
}

bool setKinText(PersonSetup& person, const std::string& text, std::string& problem) {
    std::vector<std::pair<std::string, int>> found;
    if (!keyNumbers(text, found, problem)) return false;
    PersonSetup next = person;
    next.mother.clear();
    next.father.clear();
    next.partner.clear();
    for (const auto& [key, value] : found) {
        int id = 0;
        if (!memberNumber(std::to_string(value), id)) {
            problem = "'" + key + "' is not a clan member's number";
            return false;
        }
        if (key == "mother") next.mother = std::to_string(value);
        else if (key == "father") next.father = std::to_string(value);
        else if (key == "partner") next.partner = std::to_string(value);
        else {
            problem = "kin is mother, father or partner";
            return false;
        }
    }
    person = std::move(next);
    return true;
}

// --- The file ---

nlohmann::ordered_json setupSectionsToJson(const WorldSetup& setup) {
    using nlohmann::ordered_json;
    ordered_json clans = ordered_json::object();
    for (const auto& [name, clan] : setup.clans) {
        if (clan.empty()) continue;
        ordered_json item = ordered_json::object();
        if (!clan.leader.empty()) item["leader"] = clan.leader;
        if (!clan.stance.empty()) item["stance"] = clan.stance;
        if (clan.food >= 0 || !clan.items.empty()) {
            ordered_json store = ordered_json::object();
            if (clan.food >= 0) store["food"] = clan.food;
            for (const auto& [thing, count] : clan.items) store[thing] = count;
            item["store"] = std::move(store);
        }
        if (!clan.debts.empty()) {
            ordered_json debts = ordered_json::array();
            for (const DebtSetup& debt : clan.debts) {
                ordered_json goods = ordered_json::object();
                for (const auto& [thing, count] : debt.goods) goods[thing] = count;
                debts.push_back({{"to", debt.to}, {"goods", std::move(goods)}, {"value", debt.value}, {"days", debt.days}});
            }
            item["debts"] = std::move(debts);
        }
        if (!clan.partners.empty()) item["partners"] = clan.partners;
        if (!clan.members.empty()) item["members"] = clan.members;
        clans[name] = std::move(item);
    }
    ordered_json people = ordered_json::object();
    for (const auto& [id, person] : setup.people) {
        if (person.empty()) continue;
        ordered_json item = ordered_json::object();
        if (!person.mother.empty() || !person.father.empty() || !person.partner.empty()) {
            ordered_json kin = ordered_json::object();
            if (!person.mother.empty()) kin["mother"] = person.mother;
            if (!person.father.empty()) kin["father"] = person.father;
            if (!person.partner.empty()) kin["partner"] = person.partner;
            item["kin"] = std::move(kin);
        }
        if (!person.opinions.empty()) {
            ordered_json opinions = ordered_json::object();
            for (const auto& [other, value] : person.opinions) opinions[other] = value;
            item["opinions"] = std::move(opinions);
        }
        if (!person.grudges.empty()) {
            ordered_json grudges = ordered_json::array();
            for (const GrudgeSetup& grudge : person.grudges) grudges.push_back({{"about", grudge.about}, {"weight", grudge.weight}, {"reason", grudge.reason}});
            item["grudges"] = std::move(grudges);
        }
        people[id] = std::move(item);
    }
    ordered_json sections = ordered_json::object();
    sections["clans"] = std::move(clans);
    sections["people"] = std::move(people);
    return sections;
}

WorldSetup setupFromJson(const json& data, const std::filesystem::path& file) {
    WorldSetup setup;
    const auto counts = [&file](const json& object, const std::string& where) {
        NamedCounts found;
        if (!object.is_object()) throw DataError(file, where, "must be an object of item: count");
        for (const auto& [item, count] : object.items()) {
            if (!count.is_number_integer() || count.get<int>() < 0) throw DataError(file, where + "." + item, "must be a whole number, 0 or more");
            found.push_back({item, count.get<int>()});
        }
        return found;
    };
    if (data.contains("clans")) {
        for (const auto& [name, item] : data.at("clans").items()) {
            const std::string where = "clans." + name;
            ClanSetup clan;
            clan.leader = item.value("leader", std::string());
            clan.stance = item.value("stance", std::string());
            if (item.contains("store")) {
                for (const auto& [thing, count] : counts(item.at("store"), where + ".store")) {
                    if (thing == "food") clan.food = count;
                    else clan.items.push_back({thing, count});
                }
            }
            if (item.contains("debts")) {
                for (const json& entry : item.at("debts")) {
                    DebtSetup debt;
                    debt.to = entry.at("to").get<std::string>();
                    if (entry.contains("goods")) debt.goods = counts(entry.at("goods"), where + ".debts.goods");
                    debt.value = entry.value("value", 0);
                    debt.days = std::max(1, entry.value("days", 10));
                    clan.debts.push_back(std::move(debt));
                }
            }
            if (item.contains("partners")) clan.partners = item.at("partners").get<std::vector<std::string>>();
            if (item.contains("members")) clan.members = item.at("members").get<std::vector<std::string>>();
            setup.clans[name] = std::move(clan);
        }
    }
    if (data.contains("people")) {
        for (const auto& [id, item] : data.at("people").items()) {
            const std::string where = "people." + id;
            int number = 0;
            if (!memberNumber(id, number)) throw DataError(file, where, "must be a clan member's number");
            PersonSetup person;
            if (item.contains("kin")) {
                person.mother = item.at("kin").value("mother", std::string());
                person.father = item.at("kin").value("father", std::string());
                person.partner = item.at("kin").value("partner", std::string());
            }
            if (item.contains("opinions")) {
                for (const auto& [other, count] : item.at("opinions").items()) {
                    int otherNumber = 0;
                    if (!memberNumber(other, otherNumber) || !count.is_number_integer() || count.get<int>() < -100 || count.get<int>() > 100) {
                        throw DataError(file, where + ".opinions." + other, "must be a member number with an opinion from -100 to 100");
                    }
                    person.opinions.push_back({other, count.get<int>()});
                }
            }
            if (item.contains("grudges")) {
                for (const json& entry : item.at("grudges")) {
                    GrudgeSetup grudge;
                    grudge.about = entry.at("about").get<std::string>();
                    grudge.weight = entry.value("weight", 20);
                    grudge.reason = entry.at("reason").get<std::string>();
                    int aboutNumber = 0;
                    if (!memberNumber(grudge.about, aboutNumber) || grudge.weight < 1 || grudge.weight > 100 || grudge.reason.empty()) {
                        throw DataError(file, where + ".grudges", "needs a member number, a weight from 1 to 100 and a reason");
                    }
                    person.grudges.push_back(std::move(grudge));
                }
            }
            setup.people[id] = std::move(person);
        }
    }
    return setup;
}

// --- Consistency ---

std::vector<std::string> setupProblems(const WorldSetup& setup, const RegionEdits& edits) {
    std::vector<std::string> problems;
    std::set<std::string> clanNames = {"player", "the River Clan", "the Stone Clan", "the Wind Clan", "the Ash Clan"}; // the generator's rival names
    for (const PlacedEdit& entry : edits.placed) {
        if (entry.group == EditGroup::Camp && !entry.removal && entry.kind == "rival" && !entry.name.empty()) clanNames.insert(entry.name);
    }
    std::map<std::string, std::string> memberOf;
    for (const auto& [name, clan] : setup.clans) {
        if (!clanNames.contains(name)) problems.push_back(std::format("clan '{}' is not in the world (no such rival camp)", name));
        for (const DebtSetup& debt : clan.debts) {
            if (debt.to == name || !clanNames.contains(debt.to)) problems.push_back(std::format("'{}' owes a debt to '{}', a clan that is not in the world", name, debt.to));
        }
        for (const std::string& partner : clan.partners) {
            if (partner == name || !clanNames.contains(partner)) problems.push_back(std::format("'{}' trades with '{}', a clan that is not in the world", name, partner));
        }
        for (const std::string& member : clan.members) {
            const auto [found, fresh] = memberOf.insert({member, name});
            if (!fresh && found->second != name) problems.push_back(std::format("member {} is in two clans ('{}' and '{}')", member, found->second, name));
        }
    }
    for (const auto& [id, person] : setup.people) {
        for (const auto& [other, value] : person.opinions) {
            if (other == id) problems.push_back(std::format("member {} has an opinion of themself", id));
        }
        for (const GrudgeSetup& grudge : person.grudges) {
            if (grudge.about == id) problems.push_back(std::format("member {} holds a grudge against themself", id));
        }
        if (!person.partner.empty()) {
            if (person.partner == id) {
                problems.push_back(std::format("member {} is their own partner", id));
            } else if (const auto other = setup.people.find(person.partner); other != setup.people.end() && !other->second.partner.empty() && other->second.partner != id) {
                problems.push_back(std::format("member {} names {} as partner, who names {}", id, person.partner, other->second.partner));
            }
        }
        // Kin cycle: walking up the mothers and fathers of the setup must never come back.
        std::set<std::string> seen;
        std::vector<std::string> up = {person.mother, person.father};
        while (!up.empty()) {
            const std::string at = up.back();
            up.pop_back();
            if (at.empty() || !seen.insert(at).second) continue;
            if (at == id) {
                problems.push_back(std::format("member {} is their own ancestor", id));
                break;
            }
            if (const auto parent = setup.people.find(at); parent != setup.people.end()) {
                up.push_back(parent->second.mother);
                up.push_back(parent->second.father);
            }
        }
    }
    return problems;
}

// --- Applying ---

void applyClanSetup(World& world, const WorldSetup& setup, std::vector<std::string>& problems) {
    if (const auto clan = setup.clans.find("player"); clan != setup.clans.end() && clan->second.food >= 0) world.adjustFood(clan->second.food - world.food());
    const auto exists = [&world](const std::string& text, int& id) {
        return memberNumber(text, id) && world.personMutable(id) != nullptr;
    };
    for (const auto& [key, person] : setup.people) { // kin first, so the opinions and grudges find the family in place
        int id = 0;
        if (!exists(key, id)) {
            problems.push_back(std::format("member {} does not exist in this clan", key));
            continue;
        }
        Person& self = *world.personMutable(id);
        int other = 0;
        if (!person.mother.empty() && exists(person.mother, other)) self.mother = other;
        if (!person.father.empty() && exists(person.father, other)) self.father = other;
        if (!person.partner.empty() && exists(person.partner, other) && other != id) {
            self.partner = other;
            world.personMutable(other)->partner = id;
        }
    }
    for (const auto& [key, person] : setup.people) {
        int id = 0;
        if (!exists(key, id)) continue;
        for (const auto& [otherKey, value] : person.opinions) {
            int other = 0;
            if (exists(otherKey, other)) world.setOpinion(id, other, value);
            else problems.push_back(std::format("member {} has an opinion of {}, who does not exist", key, otherKey));
        }
        for (const GrudgeSetup& grudge : person.grudges) {
            int about = 0;
            if (!exists(grudge.about, about) || world.addSetupGrudge(id, about, grudge.weight, grudge.reason) < 0) {
                problems.push_back(std::format("member {} holds a grudge against {}, who does not exist", key, grudge.about));
            }
        }
    }
}

void applyHeroSetup(HeroLife& life, const WorldSetup& setup, std::vector<std::string>& problems) {
    const auto clan = setup.clans.find("player");
    if (clan == setup.clans.end()) return;
    for (const auto& [item, count] : clan->second.items) {
        if (!life.stockItem(item, count)) problems.push_back(std::format("the store holds '{}', which is not an item of the game", item));
    }
    for (const DebtSetup& debt : clan->second.debts) {
        int rival = -1;
        for (std::size_t i = 0; i < life.rivals().size(); ++i) {
            if (life.rivals()[i].name == debt.to) rival = static_cast<int>(i);
        }
        if (rival < 0) {
            problems.push_back(std::format("a debt to '{}': no such rival clan in this game", debt.to));
            continue;
        }
        life.addDebt(rival, debt.goods, debt.value, debt.days);
    }
}

void applyRivalSetup(Rivals& rivals, const WorldSetup& setup, std::vector<std::string>& problems) {
    for (const auto& [name, clan] : setup.clans) {
        if (name == "player" || clan.food < 0) continue;
        const auto found = std::find_if(rivals.clans().begin(), rivals.clans().end(), [&name](const RivalClan& rival) { return rival.name == name; });
        if (found == rivals.clans().end()) {
            problems.push_back(std::format("clan '{}' is not a rival in this game", name));
            continue;
        }
        found->world->adjustFood(clan.food - found->world->food());
    }
}

} // namespace odysseus::sim
