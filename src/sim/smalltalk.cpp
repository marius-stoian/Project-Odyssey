#include "sim/smalltalk.h"

#include "sim/conversation.h"
#include "sim/rule_json.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <format>
#include <iterator>
#include <set>

namespace odysseus::sim::rules {

namespace {

constexpr std::size_t kMaxLineLength = 140;      // plain and short (D-38): a template longer than this is a mistake
constexpr std::size_t kMinTemplates = 3;         // per topic (the design)
constexpr std::size_t kRecentPerPerson = 3;      // a person does not say the same thing three times running
constexpr std::size_t kWindow = 50;              // nobody says the same line more than twice in this many
constexpr int kMostTwiceInWindow = 2;

const std::vector<std::string>& moodWords() {
    static const std::vector<std::string> words = {"warm", "friendly", "neutral", "wary", "hostile", "hungry", "tired", "cold", "lonely"};
    return words;
}

// Which topic may use which facts. Every topic may use the common tokens.
const std::vector<std::string>& commonTokens() {
    static const std::vector<std::string> tokens = {"season", "hero", "npc", "npc.name"};
    return tokens;
}
const std::vector<std::string>& memoryTokens() {
    static const std::vector<std::string> tokens = {"memory.what", "memory.who", "memory.when"};
    return tokens;
}
const std::vector<std::string>& gossipTokens() {
    static const std::vector<std::string> tokens = {"gossip.who", "gossip.about", "gossip.what", "gossip.feeling"};
    return tokens;
}
const std::vector<std::string>& needTokens() {
    static const std::vector<std::string> tokens = {"need.name"};
    return tokens;
}

bool contains(const std::vector<std::string>& list, const std::string& word) { return std::find(list.begin(), list.end(), word) != list.end(); }

bool tokenAllowedIn(const std::string& topic, const std::string& token) {
    if (contains(commonTokens(), token)) return true;
    if (topic == "memory") return contains(memoryTokens(), token);
    if (topic == "people") return contains(gossipTokens(), token);
    if (topic == "needs") return contains(needTokens(), token);
    return false;
}

std::string lowered(std::string text) {
    for (char& c : text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return text;
}

std::string replaceAll(std::string text, const std::string& from, const std::string& to) {
    for (std::size_t at = text.find(from); at != std::string::npos; at = text.find(from, at + to.size())) text.replace(at, from.size(), to);
    return text;
}

} // namespace

const std::vector<std::string>& requiredSmalltalkTopics() {
    static const std::vector<std::string> topics = {"memory", "people", "needs", "season", "hero"};
    return topics;
}

const std::vector<std::string>& smalltalkTokens() {
    static const std::vector<std::string> tokens = [] {
        std::vector<std::string> all = commonTokens();
        for (const auto* group : {&memoryTokens(), &gossipTokens(), &needTokens()}) all.insert(all.end(), group->begin(), group->end());
        return all;
    }();
    return tokens;
}

// ---- reading smalltalk.json

std::optional<SmalltalkData> SmalltalkData::parse(std::string_view text, const std::string& shownName, LoadReport& report) {
    const std::size_t errorsBefore = report.errors.size();
    const auto fail = [&](int line, const std::string& message) { report.errors.push_back({shownName, line, message}); };
    const JsonParseResult parsed = parseJson(text);
    if (!parsed.value) {
        fail(parsed.errorLine, parsed.error);
        return std::nullopt;
    }
    const JsonValue& root = *parsed.value;
    const JsonValue* topics = root.isObject() ? root.find("topics") : nullptr;
    if (topics == nullptr || !topics->isObject()) {
        fail(root.line, "the file needs a \"topics\" object: { \"topics\": { \"memory\": [ ... ], ... } }");
        return std::nullopt;
    }
    SmalltalkData data;
    for (std::size_t t = 0; t < topics->keys.size(); ++t) {
        const std::string& name = topics->keys[t];
        const JsonValue& list = topics->items[t];
        if (!list.isArray()) {
            fail(list.line, std::format("topic \"{}\" must be a list of templates", name));
            continue;
        }
        std::vector<SmalltalkTemplate> templates;
        for (const JsonValue& item : list.items) {
            SmalltalkTemplate entry;
            entry.line = item.line;
            const JsonValue* words = item.isString() ? &item : (item.isObject() ? item.find("text") : nullptr);
            if (words == nullptr || !words->isString()) {
                fail(item.line, std::format("a template of \"{}\" is a quoted sentence or {{ \"text\": \"...\", \"mood\": [\"warm\"] }}", name));
                continue;
            }
            entry.text = words->text;
            if (entry.text.empty() || entry.text.size() > kMaxLineLength) {
                fail(words->line, std::format("a template must be 1 to {} characters (plain and short), this one is {}", kMaxLineLength, entry.text.size()));
                continue;
            }
            // Every {token} must exist, and its facts must exist in this topic.
            bool tokensOk = true;
            for (std::size_t at = entry.text.find('{'); at != std::string::npos; at = entry.text.find('{', at + 1)) {
                const std::size_t close = entry.text.find('}', at);
                if (close == std::string::npos) {
                    fail(words->line, "a { is never closed");
                    tokensOk = false;
                    break;
                }
                const std::string token = entry.text.substr(at + 1, close - at - 1);
                if (!contains(smalltalkTokens(), token)) {
                    fail(words->line, std::format("unknown token {{{}}} (the tokens are: {})", token, [] {
                        std::string all;
                        for (const std::string& known : smalltalkTokens()) all += (all.empty() ? "" : ", ") + known;
                        return all;
                    }()));
                    tokensOk = false;
                } else if (!tokenAllowedIn(name, token)) {
                    fail(words->line, std::format("{{{}}} cannot be used in the topic \"{}\": its facts only exist in another topic", token, name));
                    tokensOk = false;
                }
            }
            if (item.isObject()) {
                if (const JsonValue* moods = item.find("mood")) {
                    if (!moods->isArray()) {
                        fail(moods->line, "\"mood\" is a list of mood words");
                        tokensOk = false;
                    } else {
                        for (const JsonValue& mood : moods->items) {
                            if (!mood.isString() || !contains(moodWords(), mood.text)) {
                                fail(mood.line, std::format("unknown mood \"{}\" (the moods are: warm, friendly, neutral, wary, hostile, hungry, tired, cold, lonely)", mood.text));
                                tokensOk = false;
                            } else {
                                entry.moods.push_back(mood.text);
                            }
                        }
                    }
                }
            }
            if (tokensOk) templates.push_back(std::move(entry));
        }
        if (list.items.size() < kMinTemplates) fail(list.line, std::format("topic \"{}\" needs at least {} templates, it has {}", name, kMinTemplates, list.items.size()));
        data.topics_[name] = std::move(templates);
    }
    for (const std::string& required : requiredSmalltalkTopics()) {
        if (topics->find(required) == nullptr) fail(topics->line, std::format("the topic \"{}\" is missing", required));
    }
    if (report.errors.size() != errorsBefore) return std::nullopt;
    return data;
}

std::optional<SmalltalkData> SmalltalkData::load(const std::filesystem::path& file, const std::string& shownName, LoadReport& report) {
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        report.errors.push_back({shownName, 0, "the file cannot be read"});
        return std::nullopt;
    }
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return parse(text, shownName, report);
}

const std::vector<SmalltalkTemplate>* SmalltalkData::topic(const std::string& name) const {
    const auto found = topics_.find(name);
    return found == topics_.end() ? nullptr : &found->second;
}

// ---- the facts

namespace {

// How a person is named in what someone says to the hero: "you" for the hero, "me" for the speaker, else their name.
std::string nameIn(const World& world, int id, int npc, int hero) {
    if (id == hero) return "you";
    if (id == npc) return "me";
    if (id < 0 || static_cast<std::size_t>(id) >= world.people().size()) return "someone";
    return world.people()[static_cast<std::size_t>(id)].name;
}

// What happened, as a plain phrase that follows "about" or "that": "Bo blaming Ama", "you giving me a gift".
std::string phraseOf(const World& world, const Memory& memory, int npc, int hero) {
    const std::string s = nameIn(world, memory.subject, npc, hero);
    const std::string o = nameIn(world, memory.object, npc, hero);
    switch (memory.kind) {
    case MemoryKind::Gift: return std::format("{} giving {} a gift", s, o);
    case MemoryKind::Theft: return std::format("{} stealing from the store", s);
    case MemoryKind::Death: return std::format("{} dying", s);
    case MemoryKind::Quarrel: return std::format("{} quarrelling with {}", s, o);
    case MemoryKind::Blame: return std::format("{} blaming {}", s, o);
    case MemoryKind::Fight: return std::format("{} fighting {}", s, o);
    case MemoryKind::Nursing: return std::format("{} nursing {}", s, o);
    case MemoryKind::Sharing: return std::format("{} sharing food with {}", s, o);
    case MemoryKind::Rescue: return std::format("{} saving {}", s, o);
    case MemoryKind::Rejection: return std::format("{} turning {} down", s, o);
    case MemoryKind::Heroism: return std::format("{} doing something brave", s);
    case MemoryKind::Cowardice: return std::format("{} running away", s);
    default: return std::format("{} and {}", s, o);
    }
}

std::string whenWord(std::int64_t today, std::int64_t day) {
    const std::int64_t ago = today - day;
    if (ago <= 0) return "today";
    if (ago == 1) return "yesterday";
    if (ago < 14) return std::format("{} days ago", ago);
    return "a long time ago";
}

// How the feeling of a memory reads aloud: the word the person uses for how it sits with them.
std::string feelingWord(int feeling) {
    if (feeling <= -60) return "angry";
    if (feeling <= -20) return "uneasy";
    if (feeling < 20) return "unsure";
    if (feeling < 60) return "glad";
    return "delighted";
}

// One thing a person could talk about: the facts that fill {memory.*} or {gossip.*}.
struct Heard {
    std::int64_t day = 0;
    std::map<std::string, std::string> tokens;
};

bool newerFirst(const Heard& a, const Heard& b) { return a.day > b.day; }

} // namespace

std::optional<SaidLine> SmallTalk::say(const World& world, int npc, int hero, core::Pcg32& random, const std::string& requested) {
    // Three draws, always, in this order: the topic, the fact, the template.
    const std::uint32_t topicDraw = random.next();
    const std::uint32_t factDraw = random.next();
    const std::uint32_t templateDraw = random.next();
    if (!data_ || npc < 0 || static_cast<std::size_t>(npc) >= world.people().size()) return std::nullopt;
    const Person& person = world.people()[static_cast<std::size_t>(npc)];
    const std::int64_t today = world.date().day;

    // What they could talk about, newest first.
    std::vector<Heard> memories;
    std::vector<Heard> gossip;
    for (const Memory& memory : person.memories) {
        Heard heard;
        heard.day = memory.day;
        heard.tokens["memory.what"] = phraseOf(world, memory, npc, hero);
        heard.tokens["memory.who"] = nameIn(world, memory.subject, npc, hero);
        heard.tokens["memory.when"] = whenWord(today, memory.day);
        if (!memory.secondHand) {
            memories.push_back(heard);
        } else {
            Heard rumour;
            rumour.day = memory.day;
            rumour.tokens["gossip.who"] = nameIn(world, memory.subject, npc, hero);
            rumour.tokens["gossip.about"] = nameIn(world, memory.object, npc, hero);
            rumour.tokens["gossip.what"] = phraseOf(world, memory, npc, hero);
            rumour.tokens["gossip.feeling"] = feelingWord(memory.feeling);
            gossip.push_back(rumour);
        }
    }
    for (const MemoryNote& note : person.notes) {
        Heard heard;
        heard.day = note.day;
        heard.tokens["memory.what"] = note.clause ? "the day " + note.text : note.text; // "the day Voll shared berries"
        heard.tokens["memory.who"] = "someone";
        heard.tokens["memory.when"] = whenWord(today, note.day);
        (note.secondHand ? gossip : memories).push_back(heard); // a heard note is a rumour with no names: it tells its text
        if (note.secondHand) {
            gossip.back().tokens = {{"gossip.who", "someone"}, {"gossip.about", "someone"}, {"gossip.what", note.clause ? "the day " + note.text : note.text}, {"gossip.feeling", feelingWord(note.feeling)}};
        }
    }
    std::stable_sort(memories.begin(), memories.end(), newerFirst);
    std::stable_sort(gossip.begin(), gossip.end(), newerFirst);

    int lowestNeed = 100;
    std::size_t lowest = 0;
    for (std::size_t n = 0; n < kNeedCount; ++n) {
        if (person.needs.values[n] < lowestNeed) {
            lowestNeed = person.needs.values[n];
            lowest = n;
        }
    }
    static const char* const kNeedWords[] = {"food", "rest", "warmth", "company"}; // Hunger, Energy, Warmth, Social

    // The topic: asked for, or chosen by weights from the state.
    std::string topic = requested;
    const bool available = [&] {
        if (topic == "memory") return !memories.empty();
        if (topic == "people") return !gossip.empty();
        if (topic == "needs") return lowestNeed < 60;
        return data_->topic(topic) != nullptr;
    }();
    if (topic.empty()) {
        std::vector<std::pair<std::string, int>> weights;
        if (!memories.empty()) weights.push_back({"memory", today - memories.front().day <= 3 ? 6 : 2});
        if (!gossip.empty()) weights.push_back({"people", today - gossip.front().day <= 10 ? 4 : 3});
        if (lowestNeed < 60) weights.push_back({"needs", lowestNeed < 25 ? 8 : (lowestNeed < 45 ? 4 : 2)});
        weights.push_back({"season", 1});
        weights.push_back({"hero", 2});
        int total = 0;
        for (const auto& [name, weight] : weights) total += weight;
        int pick = static_cast<int>(topicDraw % static_cast<std::uint32_t>(total));
        for (const auto& [name, weight] : weights) {
            if (pick < weight) {
                topic = name;
                break;
            }
            pick -= weight;
        }
    } else if (!available) {
        topic = "season"; // asked for a memory they do not have: talk about the weather
    }
    const std::vector<SmalltalkTemplate>* templates = data_->topic(topic);
    if (templates == nullptr || templates->empty()) return std::nullopt;

    // The facts of the line.
    std::map<std::string, std::string> tokens;
    tokens["season"] = lowered(seasonName(world.date().season));
    tokens["hero"] = hero >= 0 && static_cast<std::size_t>(hero) < world.people().size() ? world.people()[static_cast<std::size_t>(hero)].name : "you";
    tokens["npc"] = person.name;
    tokens["npc.name"] = person.name;
    tokens["need.name"] = kNeedWords[lowest];
    const std::vector<Heard>& pool = topic == "memory" ? memories : gossip;
    if ((topic == "memory" || topic == "people") && !pool.empty()) {
        const Heard& chosen = pool[factDraw % std::min<std::size_t>(3, pool.size())]; // one of the three newest
        tokens.insert(chosen.tokens.begin(), chosen.tokens.end());
    }

    // The templates for how they feel, then the lines that are not tired out.
    const std::string mood = moodWord(world.opinion(npc, hero), person.needs);
    std::vector<const SmalltalkTemplate*> fitting;
    for (const SmalltalkTemplate& entry : *templates) {
        if (entry.moods.empty() || contains(entry.moods, mood)) fitting.push_back(&entry);
    }
    if (fitting.empty()) {
        for (const SmalltalkTemplate& entry : *templates) fitting.push_back(&entry);
    }
    const auto fill = [&](const SmalltalkTemplate& entry) {
        std::string text = entry.text;
        for (const auto& [name, value] : tokens) text = replaceAll(text, "{" + name + "}", value);
        return text;
    };
    const std::deque<std::string>& mine = lastByPerson_[npc];
    const auto timesInWindow = [&](const std::string& text) { return static_cast<int>(std::count(window_.begin(), window_.end(), text)); };
    std::vector<std::string> allowed;
    for (const SmalltalkTemplate* entry : fitting) {
        const std::string text = fill(*entry);
        if (std::find(mine.begin(), mine.end(), text) == mine.end() && timesInWindow(text) < kMostTwiceInWindow) allowed.push_back(text);
    }
    if (allowed.empty()) { // everything fitting is tired out: say what was said longest ago rather than nothing
        for (const SmalltalkTemplate* entry : fitting) {
            const std::string text = fill(*entry);
            if (std::find(mine.begin(), mine.end(), text) == mine.end()) allowed.push_back(text);
        }
    }
    if (allowed.empty()) {
        for (const SmalltalkTemplate* entry : fitting) allowed.push_back(fill(*entry));
    }
    std::string text = allowed[templateDraw % allowed.size()];
    std::deque<std::string>& remembered = lastByPerson_[npc];
    remembered.push_back(text);
    while (remembered.size() > kRecentPerPerson) remembered.pop_front();
    window_.push_back(text);
    while (window_.size() > kWindow) window_.pop_front();
    return SaidLine{topic, std::move(text)};
}

void SmallTalk::clear() {
    lastByPerson_.clear();
    window_.clear();
}

} // namespace odysseus::sim::rules
