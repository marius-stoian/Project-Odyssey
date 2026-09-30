#include "sim/episodes.h"

#include "sim/world.h"

#include <algorithm>
#include <array>
#include <format>
#include <map>
#include <numeric>

namespace odysseus::sim {

namespace {

// What an event is mostly about. An episode takes the theme that weighs most.
enum class Theme { Hardship, Feud, Hunt, Sickness, Love, Teaching, None, Count };
constexpr int kThemes = static_cast<int>(Theme::None);

Theme themeOf(const ChronicleEntry& entry, const std::vector<Person>& people) {
    switch (entry.kind) {
    case EventKind::Lean:
    case EventKind::StoreEmpty:
    case EventKind::Sharing: return Theme::Hardship;
    case EventKind::Theft:
    case EventKind::Feud:
    case EventKind::Peace:
    case EventKind::Quarrel:
    case EventKind::Blame:
    case EventKind::Revenge:
    case EventKind::Exile: return Theme::Feud;
    case EventKind::HuntParty:
    case EventKind::Hero:
    case EventKind::Coward:
    case EventKind::Rescue:
    case EventKind::Mammoth: return Theme::Hunt;
    case EventKind::Sickness:
    case EventKind::Injury:
    case EventKind::Recovery:
    case EventKind::Nursing: return Theme::Sickness;
    case EventKind::Courtship:
    case EventKind::Jealousy:
    case EventKind::Rejection:
    case EventKind::Pairing:
    case EventKind::Parting: return Theme::Love;
    case EventKind::Apprentice:
    case EventKind::Graduation: return Theme::Teaching;
    case EventKind::Death:
        switch (people[static_cast<std::size_t>(entry.who)].causeOfDeath) {
        case CauseOfDeath::Starvation:
        case CauseOfDeath::Cold: return Theme::Hardship;
        case CauseOfDeath::Fight: return Theme::Feud;
        case CauseOfDeath::Hunting: return Theme::Hunt;
        case CauseOfDeath::Wound:
        case CauseOfDeath::Illness: return Theme::Sickness;
        default: return Theme::None;
        }
    default: return Theme::None;
    }
}

// A hard season: hunger deaths or an empty store, and everything around them that season.
bool marksHardSeason(const ChronicleEntry& entry, const std::vector<Person>& people) {
    return entry.kind == EventKind::StoreEmpty ||
           (entry.kind == EventKind::Death && themeOf(entry, people) == Theme::Hardship);
}

bool joinsHardSeason(const ChronicleEntry& entry, const std::vector<Person>& people) {
    return marksHardSeason(entry, people) || entry.kind == EventKind::Lean || entry.kind == EventKind::Sharing ||
           entry.kind == EventKind::Nursing || entry.kind == EventKind::Sickness;
}

// Union-find over chronicle ids: events that belong together end up with one root.
struct Groups {
    std::vector<int> parent;
    explicit Groups(std::size_t count) : parent(count) { std::iota(parent.begin(), parent.end(), 0); }
    int root(int id) {
        while (parent[static_cast<std::size_t>(id)] != id) {
            parent[static_cast<std::size_t>(id)] = parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(id)])];
            id = parent[static_cast<std::size_t>(id)];
        }
        return id;
    }
    void join(int a, int b) {
        const int ra = root(a);
        const int rb = root(b);
        if (ra != rb) parent[static_cast<std::size_t>(std::max(ra, rb))] = std::min(ra, rb); // the root is the oldest event
    }
};

std::string when(const ChronicleEntry& entry) {
    return std::format("{}, year {}", seasonName(entry.date.season), entry.date.year);
}

} // namespace

std::vector<Episode> findEpisodes(const World& world) {
    const EpisodeStory& config = world.config().story.episodes;
    const auto& entries = world.chronicle().entries();
    const auto& people = world.people();
    if (entries.empty()) {
        return {};
    }
    auto counts = [&](const ChronicleEntry& e) { return e.importance >= config.minEventImportance; };

    // 1. Link events: an event and its causes belong together; so do the events of a hard season.
    Groups groups(entries.size());
    std::map<int, std::vector<int>> hardSeasons; // year * 4 + season -> the hardship events of that season
    for (const ChronicleEntry& entry : entries) {
        if (!counts(entry)) continue;
        for (const int cause : entry.causes) {
            if (counts(entries[static_cast<std::size_t>(cause)])) groups.join(entry.id, cause);
        }
        if (joinsHardSeason(entry, people)) {
            hardSeasons[entry.date.year * 4 + static_cast<int>(entry.date.season)].push_back(entry.id);
        }
    }
    for (const auto& [season, ids] : hardSeasons) {
        const bool hard = std::any_of(ids.begin(), ids.end(), [&](int id) { return marksHardSeason(entries[static_cast<std::size_t>(id)], people); });
        if (!hard) continue;
        for (const int id : ids) groups.join(ids.front(), id);
    }

    // 2. Collect the groups that are big and important enough.
    std::map<int, std::vector<int>> members; // root -> events, oldest first
    for (const ChronicleEntry& entry : entries) {
        if (counts(entry)) members[groups.root(entry.id)].push_back(entry.id);
    }
    std::vector<Episode> found;
    for (const auto& [root, ids] : members) {
        if (static_cast<int>(ids.size()) < config.minEvents) continue;
        int strongest = 0;
        for (const int id : ids) strongest = std::max(strongest, entries[static_cast<std::size_t>(id)].importance);
        if (strongest < config.minImportance) continue;

        Episode episode;
        episode.events = ids;
        episode.beginning = ids.front();
        episode.end = ids.back();
        // The turn: the most important event between the beginning and the end (the earliest of equals).
        episode.turn = ids[1];
        for (std::size_t i = 1; i + 1 < ids.size(); ++i) {
            if (entries[static_cast<std::size_t>(ids[i])].importance > entries[static_cast<std::size_t>(episode.turn)].importance) episode.turn = ids[i];
        }
        // The people: most involved first (ties: the lower id).
        std::map<int, int> involved;
        std::array<int, kThemes> weight{};
        int deaths = 0;
        for (const int id : ids) {
            const ChronicleEntry& entry = entries[static_cast<std::size_t>(id)];
            for (const int person : {entry.who, entry.other, entry.aux}) {
                if (person >= 0) ++involved[person];
            }
            const Theme theme = themeOf(entry, people);
            if (theme != Theme::None) weight[static_cast<std::size_t>(theme)] += entry.importance;
            deaths += entry.kind == EventKind::Death ? 1 : 0;
            episode.score += entry.importance;
        }
        episode.score += 20 * deaths + 10 * static_cast<int>(involved.size());
        std::vector<std::pair<int, int>> ranked(involved.begin(), involved.end()); // (person, count), by id
        std::stable_sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
        for (std::size_t i = 0; i < ranked.size() && static_cast<int>(i) < config.maxPeople; ++i) {
            episode.people.push_back(ranked[i].first);
        }

        // The name: from the theme that weighs most (ties: the order of the list).
        const auto theme = static_cast<Theme>(std::max_element(weight.begin(), weight.end()) - weight.begin());
        auto first = [&](auto&& wanted) -> const ChronicleEntry* {
            for (const int id : ids) {
                if (wanted(entries[static_cast<std::size_t>(id)])) return &entries[static_cast<std::size_t>(id)];
            }
            return nullptr;
        };
        auto nameOf = [&](int id) { return id >= 0 ? people[static_cast<std::size_t>(id)].name : std::string("someone"); };
        const ChronicleEntry& start = entries[static_cast<std::size_t>(episode.beginning)];
        std::string name = std::format("The Story of {}", nameOf(episode.people.front()));
        switch (theme) {
        case Theme::Hardship: {
            const ChronicleEntry* hard = first([&](const ChronicleEntry& e) { return marksHardSeason(e, people); });
            const ChronicleEntry& anchor = hard != nullptr ? *hard : start;
            switch (anchor.date.season) {
            case Season::Winter: name = std::format("The Hard Winter of year {}", anchor.date.year); break;
            case Season::Autumn: name = std::format("The Lean Autumn of year {}", anchor.date.year); break;
            case Season::Spring: name = std::format("The Hungry Spring of year {}", anchor.date.year); break;
            case Season::Summer: name = std::format("The Dry Summer of year {}", anchor.date.year); break;
            }
            break;
        }
        case Theme::Feud: {
            if (const ChronicleEntry* revenge = first([](const ChronicleEntry& e) { return e.kind == EventKind::Revenge || e.kind == EventKind::Exile; })) {
                name = std::format("The Vengeance of {}", nameOf(revenge->who));
            } else if (const ChronicleEntry* feud = first([](const ChronicleEntry& e) { return e.kind == EventKind::Feud; })) {
                name = std::format("The Feud of {} and {}", nameOf(feud->who), nameOf(feud->other));
            } else if (const ChronicleEntry* blame = first([](const ChronicleEntry& e) { return e.kind == EventKind::Blame; })) {
                name = std::format("The Grief of {}", nameOf(blame->who));
            } else if (const ChronicleEntry* quarrel = first([](const ChronicleEntry& e) { return e.kind == EventKind::Quarrel; })) {
                name = std::format("The Quarrel of {} and {}", nameOf(quarrel->who), nameOf(quarrel->other));
            }
            break;
        }
        case Theme::Hunt: {
            const ChronicleEntry* party = first([](const ChronicleEntry& e) { return e.kind == EventKind::HuntParty; });
            name = std::format("The Great Hunt of year {}", (party != nullptr ? *party : start).date.year);
            break;
        }
        case Theme::Sickness: {
            const ChronicleEntry* sick = first([](const ChronicleEntry& e) { return e.kind == EventKind::Sickness || e.kind == EventKind::Injury; });
            name = std::format("The Sickness of year {}", (sick != nullptr ? *sick : start).date.year);
            break;
        }
        case Theme::Love: {
            if (const ChronicleEntry* parting = first([](const ChronicleEntry& e) { return e.kind == EventKind::Parting; })) {
                name = std::format("The Parting of {} and {}", nameOf(parting->who), nameOf(parting->other));
            } else if (const ChronicleEntry* rival = first([](const ChronicleEntry& e) { return e.kind == EventKind::Jealousy; })) {
                name = std::format("The Rivals for {}", nameOf(rival->aux));
            } else if (const ChronicleEntry* court = first([](const ChronicleEntry& e) { return e.kind == EventKind::Courtship; })) {
                name = std::format("The Courtship of {}", nameOf(court->who));
            }
            break;
        }
        case Theme::Teaching: {
            if (const ChronicleEntry* lesson = first([](const ChronicleEntry& e) { return e.kind == EventKind::Apprentice; })) {
                name = std::format("The Apprenticeship of {}", nameOf(lesson->other));
            }
            break;
        }
        default: break;
        }
        episode.name = name;
        found.push_back(std::move(episode));
    }

    // 3. Only the best are told: at most maxPerCentury for every hundred years of the chronicle.
    const int years = static_cast<int>(std::max<std::int64_t>(1, world.date().day / world.calendar().daysPerYear()));
    const auto limit = static_cast<std::size_t>(std::max(1, (config.maxPerCentury * years + 99) / 100));
    std::stable_sort(found.begin(), found.end(), [](const Episode& a, const Episode& b) { return a.score != b.score ? a.score > b.score : a.beginning < b.beginning; });
    if (found.size() > limit) {
        found.resize(limit);
    }
    std::sort(found.begin(), found.end(), [](const Episode& a, const Episode& b) { return a.beginning < b.beginning; });
    return found;
}

std::string formatEpisode(const World& world, const Episode& episode) {
    const auto& entries = world.chronicle().entries();
    auto at = [&](int id) -> const ChronicleEntry& { return entries[static_cast<std::size_t>(id)]; };
    std::vector<std::string> names;
    for (const int person : episode.people) {
        names.push_back(world.people()[static_cast<std::size_t>(person)].name);
    }
    return std::format("{}. It began in {}: {} The turn came in {}: {} It ended in {}: {} Those who lived it: {}.", episode.name,
                       when(at(episode.beginning)), at(episode.beginning).text, when(at(episode.turn)), at(episode.turn).text,
                       when(at(episode.end)), at(episode.end).text, joinNames(names));
}

std::vector<std::string> formatStory(const World& world, int threshold) {
    std::vector<std::string> lines;
    const auto episodes = findEpisodes(world);
    lines.push_back(std::format("The story of the clan, {} years", std::max(1, world.chronicle().entries().empty() ? 1 : world.chronicle().entries().back().date.year)));
    lines.push_back(std::format("Episodes ({}):", episodes.size()));
    for (const Episode& episode : episodes) {
        lines.push_back("  " + formatEpisode(world, episode));
    }
    lines.push_back("Births, deaths, pairings and feuds, with their reasons:");
    for (const ChronicleEntry& entry : world.chronicle().entries()) {
        const bool told = entry.kind == EventKind::Birth || entry.kind == EventKind::Death || entry.kind == EventKind::Pairing ||
                          entry.kind == EventKind::Parting || entry.kind == EventKind::Feud;
        if (told && entry.importance >= threshold) {
            lines.push_back("  " + formatEntry(entry));
        }
    }
    return lines;
}

} // namespace odysseus::sim
