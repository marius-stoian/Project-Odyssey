#include "sim/data.h"
#include "sim/save.h"
#include "sim/world.h"

#include <doctest/doctest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
namespace sim = odysseus::sim;

namespace {

sim::SimConfig realConfig() {
    return sim::loadSimConfig(ODYSSEUS_DATA_DIR);
}

fs::path freshFolder(const std::string& name) {
    const fs::path folder = fs::temp_directory_path() / "odysseus-us016" / name;
    fs::remove_all(folder);
    fs::create_directories(folder);
    return folder;
}

std::string readAll(const fs::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::stringstream text;
    text << in.rdbuf();
    return text.str();
}

void writeAll(const fs::path& file, const std::string& text) {
    std::ofstream(file, std::ios::binary | std::ios::trunc) << text;
}

void replaceFirst(std::string& text, const std::string& from, const std::string& to) {
    const auto at = text.find(from);
    REQUIRE(at != std::string::npos);
    text.replace(at, from.size(), to);
}

} // namespace

TEST_CASE("US-016 Round trip") {
    const fs::path file = freshFolder("round-trip") / "clan.json";
    sim::World world(42, realConfig());
    world.runTicks(world.calendar().ticksPerYear() * 50);
    const std::uint64_t before = world.hash();
    sim::saveWorld(world, file);

    sim::LoadedWorld loaded = sim::loadWorld(file, realConfig());
    MESSAGE("after 50 years: ", world.population(), " alive, ", world.people().size(), " people ever, ",
            world.chronicle().entries().size(), " chronicle entries, save ", fs::file_size(file) / 1024, " KB");
    CHECK(loaded.loadedFrom == file);
    CHECK(loaded.world.hash() == before); // exactly the same world
    // And it goes on exactly as the original would have: the whole state was saved.
    world.runTicks(world.calendar().ticksPerYear());
    loaded.world.runTicks(world.calendar().ticksPerYear());
    CHECK(loaded.world.hash() == world.hash());
}

TEST_CASE("US-016 Crash-safe") {
    const fs::path folder = freshFolder("crash-safe");
    const fs::path file = folder / "clan.json";
    sim::World world(7, realConfig());
    world.runTicks(world.calendar().ticksPerYear());
    sim::saveWorld(world, file);
    const std::uint64_t firstSave = world.hash();
    world.runTicks(world.calendar().ticksPerYear());
    sim::saveWorld(world, file);
    const std::uint64_t secondSave = world.hash();
    CHECK(fs::exists(sim::backupPath(file, 1))); // the previous save became a backup

    SUBCASE("interrupted while writing: the half-written temporary file is ignored") {
        const std::string whole = readAll(file);
        writeAll(fs::path(file.string() + ".tmp"), whole.substr(0, whole.size() / 2)); // the crash
        const sim::LoadedWorld loaded = sim::loadWorld(file, realConfig());
        CHECK(loaded.loadedFrom == file);
        CHECK(loaded.world.hash() == secondSave);
    }
    SUBCASE("the save itself is cut off halfway: the previous complete save loads") {
        const std::string whole = readAll(file);
        writeAll(file, whole.substr(0, whole.size() / 2));
        const sim::LoadedWorld loaded = sim::loadWorld(file, realConfig());
        CHECK(loaded.loadedFrom == sim::backupPath(file, 1));
        CHECK(loaded.world.hash() == firstSave);
        REQUIRE_FALSE(loaded.notes.empty());
        MESSAGE(loaded.notes.front());
        CHECK(loaded.notes.front().find("skipped clan.json") != std::string::npos);
    }
    SUBCASE("nothing usable at all: a clear error, never a crash") {
        writeAll(file, "{ not json");
        writeAll(sim::backupPath(file, 1), "");
        try {
            (void)sim::loadWorld(file, realConfig());
            FAIL("broken saves were accepted");
        } catch (const sim::DataError& error) {
            MESSAGE(std::string(error.what()));
            CHECK(std::string(error.what()).find("no save could be loaded") != std::string::npos);
        }
    }
}

TEST_CASE("US-016 Three backups are kept") {
    const fs::path file = freshFolder("backups") / "clan.json";
    sim::World world(3, realConfig());
    for (int save = 0; save < 5; ++save) {
        world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()));
        sim::saveWorld(world, file);
    }
    CHECK(fs::exists(file));
    CHECK(fs::exists(sim::backupPath(file, 1)));
    CHECK(fs::exists(sim::backupPath(file, 2)));
    CHECK(fs::exists(sim::backupPath(file, 3)));
    CHECK_FALSE(fs::exists(sim::backupPath(file, 4)));
    CHECK_FALSE(fs::exists(fs::path(file.string() + ".tmp")));
}

TEST_CASE("US-016 Old version") {
    const fs::path folder = freshFolder("old-version");
    const fs::path file = folder / "clan.json";
    sim::World world(42, realConfig());
    world.setDailyLife(false); // the needs-only world of save version 1
    world.runTicks(static_cast<std::uint64_t>(world.calendar().ticksPerDay()) * 2);
    sim::saveWorld(world, file);

    SUBCASE("a version 1 save is upgraded and loads") {
        // Turn the file into what version 1 wrote: no traits, skills, memories, opinions,
        // kinship or life-event fields, and no random streams for the systems added later.
        nlohmann::json save = nlohmann::json::parse(readAll(file));
        save["saveVersion"] = 1;
        for (const char* key : {"hour", "feuds", "mammoths", "lastMammothYear", "storeRanOut", "forageLeft", "gameLeft"}) {
            save.erase(key);
        }
        for (const char* stream : {"decisions", "hunting", "social", "life"}) {
            save["random"].erase(stream);
        }
        for (auto& person : save["people"]) {
            for (const char* key : {"traits", "gatherSkill", "huntSkill", "gatherPractice", "huntPractice", "action", "lastScores",
                                    "lastChosen", "memories", "opinions", "lastGiftDay", "lastTheftDay", "mother", "father",
                                    "partner", "pregnantDays", "childFather", "lastBirthDay"}) {
                person.erase(key);
            }
        }
        writeAll(file, save.dump(1));
        const sim::LoadedWorld loaded = sim::loadWorld(file, realConfig());
        REQUIRE_FALSE(loaded.notes.empty());
        MESSAGE(loaded.notes.front());
        CHECK(loaded.notes.front().find("upgraded from save version 1") != std::string::npos);
        CHECK(loaded.world.people().size() == world.people().size());
        CHECK(loaded.world.people().front().name == world.people().front().name);
        CHECK(loaded.world.people().front().needs.values == world.people().front().needs.values);
        CHECK(loaded.world.people().front().memories.empty()); // version 1 had none
        // The upgraded world lives on.
        sim::World upgraded = loaded.world;
        upgraded.setDailyLife(true);
        upgraded.runTicks(upgraded.calendar().ticksPerYear());
        CHECK(upgraded.population() > 0);
    }
    SUBCASE("a version 2 save (before the story engine) is upgraded and loads") {
        // Turn the file into what version 2 wrote: no event links in the chronicle, no grudges,
        // no event ids in memories, no story stream, no lean-season flag.
        sim::World lived(42, realConfig());
        for (int i = 0; i < 3; ++i) {
            lived.recordTheft(4, 5);
            lived.recordTheft(5, 4);
        }
        lived.runTicks(lived.calendar().ticksPerYear());
        sim::saveWorld(lived, file);
        nlohmann::json save = nlohmann::json::parse(readAll(file));
        save["saveVersion"] = 2;
        save.erase("leanEvent");
        for (auto& feud : save["feuds"]) {
            feud = nlohmann::json::array({feud[0], feud[1]}); // version 2 knew only the two people
        }
        save["random"].erase("story");
        for (auto& person : save["people"]) {
            for (const char* key : {"grudges", "exiled", "health", "healthDays", "healthEvent", "carer", "nursing", "guardian",
                                    "courting", "courtDays", "courtEvent", "courtPauseDay", "master", "apprentice", "teachHunt", "teachEvent"}) {
                person.erase(key);
            }
            for (auto& memory : person["memories"]) {
                memory.erase("event");
            }
        }
        for (auto& entry : save["chronicle"]) {
            for (const char* key : {"kind", "who", "other", "aux", "causes"}) {
                entry.erase(key);
            }
        }
        writeAll(file, save.dump(1));
        const sim::LoadedWorld loaded = sim::loadWorld(file, realConfig());
        REQUIRE_FALSE(loaded.notes.empty());
        CHECK(loaded.notes.front().find("upgraded from save version 2 to 3") != std::string::npos);
        CHECK(loaded.world.feuds() == lived.feuds()); // running feuds survive the upgrade
        REQUIRE(loaded.world.chronicle().entries().size() == lived.chronicle().entries().size());
        for (std::size_t i = 0; i < lived.chronicle().entries().size(); ++i) {
            CHECK(loaded.world.chronicle().entries()[i].text == lived.chronicle().entries()[i].text); // the past is kept word for word
            CHECK(loaded.world.chronicle().entries()[i].kind == sim::EventKind::Note);                // but its links are unknown
        }
        sim::World upgraded = loaded.world;
        upgraded.runTicks(upgraded.calendar().ticksPerYear());
        CHECK(upgraded.population() > 0); // and the upgraded world lives on, with new events linked
    }
    SUBCASE("a save from a newer version explains why it cannot load") {
        std::string text = readAll(file);
        replaceFirst(text, "\"saveVersion\": 3", "\"saveVersion\": 99");
        writeAll(file, text);
        try {
            (void)sim::loadWorld(file, realConfig());
            FAIL("a newer save was accepted");
        } catch (const sim::DataError& error) {
            MESSAGE(std::string(error.what()));
            CHECK(std::string(error.what()).find("newer version of the game") != std::string::npos);
        }
    }
}

TEST_CASE("US-016 A save that does not add up is refused") {
    const fs::path file = freshFolder("inconsistent") / "clan.json";
    sim::World world(42, realConfig());
    sim::saveWorld(world, file);
    std::string text = readAll(file);
    replaceFirst(text, "\"partner\": -1", "\"partner\": 999");
    writeAll(file, text);
    try {
        (void)sim::loadWorld(file, realConfig());
        FAIL("an inconsistent save was accepted");
    } catch (const sim::DataError& error) {
        MESSAGE(std::string(error.what()));
        CHECK(std::string(error.what()).find("does not exist") != std::string::npos);
    }
}
