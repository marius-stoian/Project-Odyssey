// US-115 Tell the clan's story in episodes (M2b story engine).
#include "sim/chronicle.h"
#include "sim/episodes.h"
#include "sim/world.h"

#include "story_helpers.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

using odysseus::sim::EventKind;
using odysseus::sim::World;
namespace sim = odysseus::sim;

using namespace story_test;

TEST_CASE("US-115 Episode") {
    // A failed harvest and a thin store: a hard winter, with hunger, thefts and deaths linked to it.
    sim::SimConfig config = realConfig();
    config.story.season.leanAutumnPercent = 100;
    config.story.season.leanForagePercent = 0;
    config.actions.forageDaily = {40, 60, 0, 0}; // spring and summer feed the store; autumn and winter give nothing
    config.actions.gameDaily = 0;
    config.actions.mammothPerMille = 0;
    config.story.episodes.maxPerCentury = 1000; // the limit is tested elsewhere: here every episode is told
    World world(42, config);
    runDays(world, 3 * 28);
    const auto episodes = sim::findEpisodes(world);
    const sim::Episode* winter = nullptr;
    for (const auto& episode : episodes) {
        if (episode.name.rfind("The Hard Winter of year ", 0) == 0) {
            winter = &episode;
            break;
        }
    }
    REQUIRE(winter != nullptr);
    const std::string text = sim::formatEpisode(world, *winter);
    MESSAGE(text);
    // One named paragraph: who, why (how it began), what came of it.
    CHECK(text.rfind(winter->name + ".", 0) == 0);
    CHECK(text.find("It began in ") != std::string::npos);
    CHECK(text.find("The turn came in ") != std::string::npos);
    CHECK(text.find("It ended in ") != std::string::npos);
    CHECK(text.find("Those who lived it: ") != std::string::npos);
    CHECK(text.find(world.chronicle().find(winter->beginning)->text) != std::string::npos); // the cause is told
    CHECK(winter->events.size() >= 3);
    for (const int id : winter->events) {
        CHECK(id >= winter->events.front());
        CHECK(id <= winter->events.back());
    }
    CHECK(winter->beginning == winter->events.front());
    CHECK(winter->end == winter->events.back());
    CHECK(winter->turn > winter->beginning);
    CHECK(winter->turn < winter->end);
    CHECK_FALSE(winter->people.empty());
    CHECK(static_cast<int>(winter->people.size()) <= world.config().story.episodes.maxPeople);
}

TEST_CASE("US-115 Fewer, bigger") {
    World world(7, realConfig());
    const int years = 60;
    runDays(world, years * 28);
    const auto episodes = sim::findEpisodes(world);
    MESSAGE(episodes.size(), " episodes in ", years, " years from ", world.chronicle().entries().size(), " events");
    REQUIRE_FALSE(episodes.empty());
    CHECK(episodes.size() <= 40); // never more than 40 episodes a century
    CHECK(static_cast<int>(episodes.size()) <= (world.config().story.episodes.maxPerCentury * years + 99) / 100);
    int previousBeginning = -1;
    for (const auto& episode : episodes) {
        CHECK(episode.name.rfind("The ", 0) == 0);
        CHECK(static_cast<int>(episode.events.size()) >= world.config().story.episodes.minEvents);
        CHECK(std::is_sorted(episode.events.begin(), episode.events.end()));
        CHECK_FALSE(episode.people.empty());
        CHECK(episode.beginning >= previousBeginning); // told in the order they began
        previousBeginning = episode.beginning;
        const std::string text = sim::formatEpisode(world, episode);
        CHECK(text.find("It began in ") != std::string::npos);
        CHECK(text.find("Those who lived it: ") != std::string::npos);
    }
    // The same world tells the same story.
    const auto again = sim::findEpisodes(world);
    REQUIRE(again.size() == episodes.size());
    for (std::size_t i = 0; i < episodes.size(); ++i) {
        CHECK(again[i].name == episodes[i].name);
        CHECK(again[i].events == episodes[i].events);
    }
}

TEST_CASE("US-115 The story, then the lines with reasons") {
    World world(7, realConfig());
    runDays(world, 40 * 28);
    const auto lines = sim::formatStory(world, sim::kDefaultChronicleThreshold);
    REQUIRE_FALSE(lines.empty());
    std::size_t episodesAt = lines.size();
    std::size_t linesAt = lines.size();
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].rfind("Episodes", 0) == 0) episodesAt = i;
        if (lines[i].rfind("Births, deaths, pairings and feuds", 0) == 0) linesAt = i;
    }
    REQUIRE(episodesAt < linesAt);
    REQUIRE(linesAt < lines.size());
    CHECK(linesAt - episodesAt - 1 >= 1);
    // Episodes come first; then only births, deaths, pairings, partings and feuds, each with its reason.
    bool sawDeath = false;
    for (std::size_t i = linesAt + 1; i < lines.size(); ++i) {
        const bool known = lines[i].find("was born to") != std::string::npos || lines[i].find(" died ") != std::string::npos ||
                           lines[i].find(" was killed") != std::string::npos || lines[i].find("became partners") != std::string::npos ||
                           lines[i].find("parted") != std::string::npos || lines[i].find("feud broke out") != std::string::npos;
        CHECK(known);
        sawDeath = sawDeath || lines[i].find(" died ") != std::string::npos;
    }
    CHECK(sawDeath);
}
