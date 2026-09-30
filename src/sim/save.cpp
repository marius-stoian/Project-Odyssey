#include "sim/save.h"

#include "sim/json_data.h"

#include <format>
#include <fstream>
#include <sstream>
#include <system_error>

namespace odysseus::sim {

namespace {

using nlohmann::json;

json saveRandom(const core::Pcg32& random) {
    return json{{"state", random.state()}, {"increment", random.increment()}};
}

void loadRandom(core::Pcg32& random, const json& value) {
    random.restore(value.at("state").get<std::uint64_t>(), value.at("increment").get<std::uint64_t>());
}

json saveMemory(const Memory& memory) {
    return json{{"subject", memory.subject}, {"object", memory.object},   {"kind", static_cast<int>(memory.kind)},
                {"day", memory.day},         {"feeling", memory.feeling}, {"major", memory.major},
                {"secondHand", memory.secondHand}, {"event", memory.event}};
}

json saveGrudge(const Grudge& grudge) {
    return json{{"about", grudge.about}, {"event", grudge.event}, {"weight", grudge.weight}};
}

Grudge loadGrudge(const json& value) {
    Grudge grudge;
    grudge.about = value.at("about").get<int>();
    grudge.event = value.at("event").get<int>();
    grudge.weight = value.at("weight").get<int>();
    return grudge;
}

Memory loadMemory(const json& value) {
    Memory memory;
    memory.subject = value.at("subject").get<int>();
    memory.object = value.at("object").get<int>();
    memory.kind = static_cast<MemoryKind>(value.at("kind").get<int>());
    memory.day = value.at("day").get<std::int64_t>();
    memory.feeling = value.at("feeling").get<int>();
    memory.major = value.at("major").get<bool>();
    memory.secondHand = value.at("secondHand").get<bool>();
    memory.event = value.at("event").get<int>();
    return memory;
}

json savePerson(const Person& p) {
    json memories = json::array();
    for (const Memory& memory : p.memories) {
        memories.push_back(saveMemory(memory));
    }
    json grudges = json::array();
    for (const Grudge& grudge : p.grudges) {
        grudges.push_back(saveGrudge(grudge));
    }
    return json{{"id", p.id},
                {"name", p.name},
                {"sex", static_cast<int>(p.sex)},
                {"ageDays", p.ageDays},
                {"alive", p.alive},
                {"causeOfDeath", static_cast<int>(p.causeOfDeath)},
                {"exiled", p.exiled},
                {"health", static_cast<int>(p.health)},
                {"healthDays", p.healthDays},
                {"healthEvent", p.healthEvent},
                {"carer", p.carer},
                {"nursing", p.nursing},
                {"guardian", p.guardian},
                {"needs", p.needs.values},
                {"daysAtZeroHunger", p.daysAtZeroHunger},
                {"daysAtZeroWarmth", p.daysAtZeroWarmth},
                {"traits", p.traits},
                {"gatherSkill", p.gatherSkill},
                {"huntSkill", p.huntSkill},
                {"gatherPractice", p.gatherPractice},
                {"huntPractice", p.huntPractice},
                {"action", static_cast<int>(p.action)},
                {"lastScores", p.lastDecision.scores},
                {"lastChosen", static_cast<int>(p.lastDecision.chosen)},
                {"memories", memories},
                {"opinions", p.opinions},
                {"grudges", grudges},
                {"lastGiftDay", p.lastGiftDay},
                {"lastTheftDay", p.lastTheftDay},
                {"mother", p.mother},
                {"father", p.father},
                {"partner", p.partner},
                {"courting", p.courting},
                {"courtDays", p.courtDays},
                {"courtEvent", p.courtEvent},
                {"courtPauseDay", p.courtPauseDay},
                {"master", p.master},
                {"apprentice", p.apprentice},
                {"teachHunt", p.teachHunt},
                {"teachEvent", p.teachEvent},
                {"pregnantDays", p.pregnantDays},
                {"childFather", p.childFather},
                {"lastBirthDay", p.lastBirthDay}};
}

Person loadPerson(const json& value) {
    Person p;
    p.id = value.at("id").get<int>();
    p.name = value.at("name").get<std::string>();
    p.sex = static_cast<Sex>(value.at("sex").get<int>());
    p.ageDays = value.at("ageDays").get<int>();
    p.alive = value.at("alive").get<bool>();
    p.causeOfDeath = static_cast<CauseOfDeath>(value.at("causeOfDeath").get<int>());
    p.exiled = value.at("exiled").get<bool>();
    p.health = static_cast<Health>(value.at("health").get<int>());
    p.healthDays = value.at("healthDays").get<int>();
    p.healthEvent = value.at("healthEvent").get<int>();
    p.carer = value.at("carer").get<int>();
    p.nursing = value.at("nursing").get<int>();
    p.guardian = value.at("guardian").get<int>();
    p.needs.values = value.at("needs").get<std::array<int, kNeedCount>>();
    p.daysAtZeroHunger = value.at("daysAtZeroHunger").get<int>();
    p.daysAtZeroWarmth = value.at("daysAtZeroWarmth").get<int>();
    p.traits = value.at("traits").get<std::uint8_t>();
    p.gatherSkill = value.at("gatherSkill").get<int>();
    p.huntSkill = value.at("huntSkill").get<int>();
    p.gatherPractice = value.at("gatherPractice").get<int>();
    p.huntPractice = value.at("huntPractice").get<int>();
    p.action = static_cast<Action>(value.at("action").get<int>());
    p.lastDecision.scores = value.at("lastScores").get<std::array<int, kActionCount>>();
    p.lastDecision.chosen = static_cast<Action>(value.at("lastChosen").get<int>());
    for (const json& memory : value.at("memories")) {
        p.memories.push_back(loadMemory(memory));
    }
    p.opinions = value.at("opinions").get<std::vector<int>>();
    for (const json& grudge : value.at("grudges")) {
        p.grudges.push_back(loadGrudge(grudge));
    }
    p.lastGiftDay = value.at("lastGiftDay").get<std::int64_t>();
    p.lastTheftDay = value.at("lastTheftDay").get<std::int64_t>();
    p.mother = value.at("mother").get<int>();
    p.father = value.at("father").get<int>();
    p.partner = value.at("partner").get<int>();
    p.courting = value.at("courting").get<int>();
    p.courtDays = value.at("courtDays").get<int>();
    p.courtEvent = value.at("courtEvent").get<int>();
    p.courtPauseDay = value.at("courtPauseDay").get<std::int64_t>();
    p.master = value.at("master").get<int>();
    p.apprentice = value.at("apprentice").get<int>();
    p.teachHunt = value.at("teachHunt").get<bool>();
    p.teachEvent = value.at("teachEvent").get<int>();
    p.pregnantDays = value.at("pregnantDays").get<int>();
    p.childFather = value.at("childFather").get<int>();
    p.lastBirthDay = value.at("lastBirthDay").get<std::int64_t>();
    return p;
}

// Version 1 -> 2: the needs-only world had no traits, skills, memories, opinions or kinship,
// and no random streams for decisions, hunting, gossip or family life. They start fresh,
// exactly as a new world would have them.
void upgradeFrom1(json& save) {
    const std::uint64_t seed = save.at("seed").get<std::uint64_t>();
    const std::size_t count = save.at("people").size();
    for (json& person : save.at("people")) {
        person["traits"] = 0;
        person["gatherSkill"] = 10;
        person["huntSkill"] = 10;
        person["gatherPractice"] = 0;
        person["huntPractice"] = 0;
        person["action"] = static_cast<int>(Action::Rest);
        person["lastScores"] = std::array<int, kActionCount>{};
        person["lastChosen"] = static_cast<int>(Action::Rest);
        person["memories"] = json::array();
        person["opinions"] = std::vector<int>(count, 0);
        person["lastGiftDay"] = -1;
        person["lastTheftDay"] = -1;
        person["mother"] = -1;
        person["father"] = -1;
        person["partner"] = -1;
        person["pregnantDays"] = 0;
        person["childFather"] = -1;
        person["lastBirthDay"] = -1;
    }
    for (const auto& [name, stream] : {std::pair{"decisions", Stream::Decisions}, std::pair{"hunting", Stream::Hunting},
                                       std::pair{"social", Stream::Social}, std::pair{"life", Stream::Life}}) {
        save["random"][name] = saveRandom(core::Pcg32(seed, static_cast<std::uint64_t>(stream)));
    }
    save["hour"] = 1;
    save["feuds"] = json::array();
    save["mammoths"] = 0;
    save["lastMammothYear"] = 0;
    save["storeRanOut"] = false;
    save["forageLeft"] = 0;
    save["gameLeft"] = 0;
    save["saveVersion"] = 2;
}

// Version 2 -> 3 (M2b story engine): chronicle entries get an id's worth of links (they are
// free-text notes with no known causes), memories know no event, nobody holds a grudge yet,
// and the story's random stream starts fresh, as a new world's would.
void upgradeFrom2(json& save) {
    const std::uint64_t seed = save.at("seed").get<std::uint64_t>();
    for (json& person : save.at("people")) {
        person["grudges"] = json::array();
        person["exiled"] = false;
        person["health"] = static_cast<int>(Health::Well);
        person["healthDays"] = 0;
        person["healthEvent"] = -1;
        person["carer"] = -1;
        person["nursing"] = -1;
        person["guardian"] = -1;
        person["courting"] = -1;
        person["courtDays"] = 0;
        person["courtEvent"] = -1;
        person["courtPauseDay"] = -1;
        person["master"] = -1;
        person["apprentice"] = -1;
        person["teachHunt"] = false;
        person["teachEvent"] = -1;
        for (json& memory : person.at("memories")) {
            memory["event"] = -1;
        }
    }
    for (json& entry : save.at("chronicle")) {
        entry["kind"] = static_cast<int>(EventKind::Note);
        entry["who"] = -1;
        entry["other"] = -1;
        entry["aux"] = -1;
        entry["causes"] = json::array();
    }
    save["random"]["story"] = saveRandom(core::Pcg32(seed, static_cast<std::uint64_t>(Stream::Story)));
    save["leanEvent"] = -1;
    for (json& feud : save.at("feuds")) {
        feud.push_back(-1); // the event that started it: unknown
        feud.push_back(0);  // since day: long ago
        feud.push_back(-1); // revenge: never
    }
    save["saveVersion"] = 3;
}

// Reads one file into a world, or throws DataError naming the file and the problem.
World readSave(const std::filesystem::path& file, const SimConfig& config, std::vector<std::string>& notes);

} // namespace

// The one place allowed to see the World's private state: the save format.
struct WorldArchive {
    static json toJson(const World& world) {
        json people = json::array();
        for (const Person& person : world.people_) {
            people.push_back(savePerson(person));
        }
        json chronicle = json::array();
        for (const ChronicleEntry& entry : world.chronicle_.entries()) {
            chronicle.push_back(json{{"year", entry.date.year},
                                     {"season", static_cast<int>(entry.date.season)},
                                     {"dayOfSeason", entry.date.dayOfSeason},
                                     {"day", entry.date.day},
                                     {"importance", entry.importance},
                                     {"text", entry.text},
                                     {"kind", static_cast<int>(entry.kind)},
                                     {"who", entry.who},
                                     {"other", entry.other},
                                     {"aux", entry.aux},
                                     {"causes", entry.causes}});
        }
        json feuds = json::array();
        for (const auto& feud : world.feuds_) {
            feuds.push_back(json::array({feud.a, feud.b, feud.event, feud.sinceDay, feud.lastRevengeDay}));
        }
        return json{{"saveVersion", kSaveVersion},
                    {"game", "Project Odyssey"},
                    {"seed", world.seed_},
                    {"ticks", world.ticks_},
                    {"random",
                     {{"weather", saveRandom(world.weather_)},
                      {"people", saveRandom(world.peopleRandom_)},
                      {"decisions", saveRandom(world.decisionRandom_)},
                      {"hunting", saveRandom(world.huntRandom_)},
                      {"social", saveRandom(world.socialRandom_)},
                      {"life", saveRandom(world.lifeRandom_)},
                      {"story", saveRandom(world.storyRandom_)}}},
                    {"leanEvent", world.leanEvent_},
                    {"temperature", world.temperature_},
                    {"hour", world.hour_},
                    {"dailyLife", world.dailyLife_},
                    {"food", world.food_},
                    {"feuds", feuds},
                    {"mammoths", world.mammoths_},
                    {"lastMammothYear", world.lastMammothYear_},
                    {"storeRanOut", world.storeRanOut_},
                    {"forageLeft", world.forageLeft_},
                    {"gameLeft", world.gameLeft_},
                    {"people", people},
                    {"chronicle", chronicle}};
    }

    static World fromJson(const json& save, const SimConfig& config) {
        World world(save.at("seed").get<std::uint64_t>(), config);
        world.ticks_ = save.at("ticks").get<std::uint64_t>();
        const json& random = save.at("random");
        loadRandom(world.weather_, random.at("weather"));
        loadRandom(world.peopleRandom_, random.at("people"));
        loadRandom(world.decisionRandom_, random.at("decisions"));
        loadRandom(world.huntRandom_, random.at("hunting"));
        loadRandom(world.socialRandom_, random.at("social"));
        loadRandom(world.lifeRandom_, random.at("life"));
        loadRandom(world.storyRandom_, random.at("story"));
        world.leanEvent_ = save.at("leanEvent").get<int>();
        world.temperature_ = save.at("temperature").get<int>();
        world.hour_ = save.at("hour").get<int>();
        world.dailyLife_ = save.value("dailyLife", true);
        world.food_ = save.at("food").get<int>();
        world.feuds_.clear();
        for (const json& pair : save.at("feuds")) {
            world.feuds_.push_back({pair.at(0).get<int>(), pair.at(1).get<int>(), pair.at(2).get<int>(),
                                    pair.at(3).get<std::int64_t>(), pair.at(4).get<std::int64_t>()});
        }
        world.mammoths_ = save.at("mammoths").get<int>();
        world.lastMammothYear_ = save.at("lastMammothYear").get<int>();
        world.storeRanOut_ = save.at("storeRanOut").get<bool>();
        world.forageLeft_ = save.at("forageLeft").get<int>();
        world.gameLeft_ = save.at("gameLeft").get<int>();
        world.people_.clear();
        for (const json& person : save.at("people")) {
            world.people_.push_back(loadPerson(person));
        }
        world.chronicle_ = Chronicle();
        for (const json& entry : save.at("chronicle")) {
            Date date;
            date.year = entry.at("year").get<int>();
            date.season = static_cast<Season>(entry.at("season").get<int>());
            date.dayOfSeason = entry.at("dayOfSeason").get<int>();
            date.day = entry.at("day").get<std::int64_t>();
            world.chronicle_.record(date, entry.at("importance").get<int>(), static_cast<EventKind>(entry.at("kind").get<int>()),
                                    entry.at("who").get<int>(), entry.at("other").get<int>(), entry.at("aux").get<int>(),
                                    entry.at("causes").get<std::vector<int>>(), entry.at("text").get<std::string>());
        }
        return world;
    }

    // A save that parses but does not add up (hand-edited, or damaged in a way JSON cannot
    // see) must never reach the simulation, which trusts its own indices.
    static std::string problemIn(const World& world) {
        const auto count = static_cast<int>(world.people_.size());
        const auto events = static_cast<int>(world.chronicle_.entries().size());
        auto validId = [count](int id) { return id >= -1 && id < count; };
        for (int i = 0; i < count; ++i) {
            const Person& p = world.people_[static_cast<std::size_t>(i)];
            if (p.id != i) {
                return std::format("people[{}].id is {}", i, p.id);
            }
            if (static_cast<int>(p.opinions.size()) != count) {
                return std::format("people[{}].opinions has {} entries for {} people", i, p.opinions.size(), count);
            }
            if (p.healthEvent >= events) {
                return std::format("people[{}].healthEvent names an event that does not exist", i);
            }
            if (!validId(p.master) || !validId(p.apprentice) || p.teachEvent >= events) {
                return std::format("people[{}].master names a person or event that does not exist", i);
            }
            if (!validId(p.courting) || p.courtEvent >= events) {
                return std::format("people[{}].courting names a person or event that does not exist", i);
            }
            if (!validId(p.carer) || !validId(p.nursing) || !validId(p.guardian)) {
                return std::format("people[{}] names a carer, patient or guardian who does not exist", i);
            }
            if (!validId(p.mother) || !validId(p.father) || !validId(p.partner) || !validId(p.childFather)) {
                return std::format("people[{}] names a person who does not exist", i);
            }
            for (const Memory& memory : p.memories) {
                if (!validId(memory.subject) || !validId(memory.object)) {
                    return std::format("people[{}].memories names a person who does not exist", i);
                }
                if (memory.event >= events) {
                    return std::format("people[{}].memories names an event that does not exist", i);
                }
            }
            for (const Grudge& grudge : p.grudges) {
                if (!validId(grudge.about) || grudge.event < -1 || grudge.event >= events) {
                    return std::format("people[{}].grudges names a person or event that does not exist", i);
                }
            }
        }
        for (const ChronicleEntry& entry : world.chronicle_.entries()) {
            if (!validId(entry.who) || !validId(entry.other) || !validId(entry.aux)) {
                return std::format("chronicle entry {} names a person who does not exist", entry.id);
            }
            for (const int cause : entry.causes) {
                if (cause < 0 || cause >= entry.id) {
                    return std::format("chronicle entry {} names a cause that did not happen before it", entry.id);
                }
            }
        }
        if (world.leanEvent_ >= events) {
            return "leanEvent names an event that does not exist";
        }
        for (const auto& feud : world.feuds_) {
            if (feud.a < 0 || feud.b < 0 || feud.a >= count || feud.b >= count || feud.event >= events) {
                return "feuds names a person or event that does not exist";
            }
        }
        if (world.hour_ < 1 || world.hour_ > kHoursPerDay) {
            return std::format("hour is {}", world.hour_);
        }
        return {};
    }
};

namespace {

World readSave(const std::filesystem::path& file, const SimConfig& config, std::vector<std::string>& notes) {
    json save = readJsonFile(file); // a missing file or broken JSON (a half-written save) throws here
    if (!save.is_object() || !save.contains("saveVersion") || !save.at("saveVersion").is_number_integer()) {
        throw DataError(file, "saveVersion", "is missing: this is not a Project Odyssey save");
    }
    const int version = save.at("saveVersion").get<int>();
    if (version > kSaveVersion) {
        throw DataError(file, "saveVersion",
                        std::format("is {}: the save was made by a newer version of the game (this one reads up to {})", version, kSaveVersion));
    }
    if (version < 1) {
        throw DataError(file, "saveVersion", std::format("is {}: no such save version", version));
    }
    if (version < kSaveVersion) {
        // One step at a time: 1 -> 2 -> 3, so each upgrade only has to know its own change.
        if (version == 1) {
            upgradeFrom1(save);
        }
        upgradeFrom2(save);
        notes.push_back(std::format("{}: upgraded from save version {} to {}", file.filename().string(), version, kSaveVersion));
    }
    try {
        World world = WorldArchive::fromJson(save, config);
        if (const std::string problem = WorldArchive::problemIn(world); !problem.empty()) {
            throw DataError(file, "(contents)", "is damaged: " + problem);
        }
        return world;
    } catch (const json::exception& error) {
        // A field is missing or has the wrong type: say which file, and what the library found.
        throw DataError(file, "(contents)", std::string("is damaged: ") + error.what());
    }
}

} // namespace

std::filesystem::path backupPath(const std::filesystem::path& file, int number) {
    return std::filesystem::path(file.string() + ".bak" + std::to_string(number));
}

void saveWorld(const World& world, const std::filesystem::path& file) {
    namespace fs = std::filesystem;
    const fs::path temporary = fs::path(file.string() + ".tmp");
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        if (!out) {
            throw DataError(temporary, "(file)", "cannot be written");
        }
        out << WorldArchive::toJson(world).dump(1);
        out.flush();
        if (!out) {
            throw DataError(temporary, "(file)", "could not be written completely (is the disk full?)");
        }
    } // the file is closed here, before it is renamed
    // Keep the last complete saves: .bak3 is dropped, .bak2 -> .bak3, .bak1 -> .bak2, save -> .bak1.
    std::error_code ignored;
    fs::remove(backupPath(file, kSaveBackups), ignored);
    for (int number = kSaveBackups - 1; number >= 1; --number) {
        if (fs::exists(backupPath(file, number))) {
            fs::rename(backupPath(file, number), backupPath(file, number + 1));
        }
    }
    if (fs::exists(file)) {
        fs::rename(file, backupPath(file, 1));
    }
    fs::rename(temporary, file); // the new save appears in one step
}

LoadedWorld loadWorld(const std::filesystem::path& file, const SimConfig& config) {
    std::vector<std::string> notes;
    std::vector<std::filesystem::path> candidates{file};
    for (int number = 1; number <= kSaveBackups; ++number) {
        candidates.push_back(backupPath(file, number));
    }
    std::string problems;
    for (const auto& candidate : candidates) {
        if (!std::filesystem::exists(candidate)) {
            continue;
        }
        try {
            World world = readSave(candidate, config, notes);
            return LoadedWorld{std::move(world), candidate, notes};
        } catch (const DataError& error) {
            // A newer-version save is not "damaged": trying an older backup would silently lose progress.
            if (std::string(error.what()).find("newer version") != std::string::npos) {
                throw;
            }
            notes.push_back(std::format("skipped {}: {}", candidate.filename().string(), error.what()));
            problems += std::string("\n  ") + error.what();
        }
    }
    throw DataError(file, "(file)", problems.empty() ? "no save found" : "no save could be loaded:" + problems);
}

} // namespace odysseus::sim
