#include "game/run_flow.h"

#include "game/game_rules.h"
#include "game/odyssey_game.h"

#include <algorithm>
#include <chrono>
#include <format>

namespace odysseus::game {

namespace {

using luna::engine::Rect;
using luna::engine::UiColor;
using luna::engine::UiPainter;

constexpr int kRowHeight = 12;
constexpr int kTalkWrap = 56; // characters of a line of speech in the conversation panel (the design of M8)

enum ScreenIds {
    kStart = 1, kLive = 2, kBegin = 3, kNewGameButton = 4, kStatsYes = 5, kStatsNo = 6, kTutorialToggle = 7, kPresetBase = 10, kComfortBase = 20, kActivityBase = 100, kOptionBase = 200,
    kTabBase = 300, kResolutionBase = 320, kFullscreen = 330, kVolumeDown = 331, kVolumeUp = 332, kFoundFire = 340, kRitual = 341, kTendFire = 342,
    kApprenticeBase = 350, kRecipeBase = 400, kGiveBase = 500, kGiveLessBase = 520, kWantBase = 540, kWantLessBase = 560, kPayLater = 580, kPropose = 581,
    kAcceptCounter = 582, kPayDebtBase = 600, kActionBase = 700
};

std::vector<std::string> wrapped(const std::string& text, int width) {
    std::vector<std::string> out;
    std::string current;
    std::string word;
    const auto flush = [&] {
        if (!word.empty()) {
            if (!current.empty() && static_cast<int>(current.size() + 1 + word.size()) > width) {
                out.push_back(current);
                current.clear();
            }
            current += (current.empty() ? "" : " ") + word;
            word.clear();
        }
    };
    for (const char c : text) {
        if (c == ' ') flush(); else word += c;
    }
    flush();
    if (!current.empty()) out.push_back(current);
    return out;
}

std::string goodsLine(const sim::HeroData& data, const std::map<std::string, int>& goods) {
    std::string out;
    for (const auto& [id, n] : goods) {
        if (n <= 0) continue;
        out += std::format("{}{} x {}", out.empty() ? "" : ", ", n, data.item(id) != nullptr ? data.item(id)->name : id);
    }
    return out.empty() ? "nothing" : out;
}

std::uint64_t randomSeed() {
    return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()) * 6364136223846793005ULL + 1442695040888963407ULL;
}

} // namespace

void RunFlow::title(const std::string& text) {
    lines_.push_back({text, UiColor::Gold, panel_.x + 8, cursorY_});
    cursorY_ += luna::engine::kLineHeight + 3;
}

void RunFlow::line(const std::string& text, UiColor colour) {
    lines_.push_back({text, colour, panel_.x + 8, cursorY_});
    cursorY_ += luna::engine::kLineHeight;
}

void RunFlow::paragraph(const std::string& text, UiColor colour, int width) {
    for (const std::string& part : wrapped(text, width)) line(part, colour);
}

void RunFlow::button(const std::string& label, int id, bool enabled, bool selected, int x, int width) {
    const int w = width > 0 ? width : UiPainter::textWidth(label) + 10;
    if (x >= 0) cursorX_ = x;
    const int left = cursorX_ >= 0 ? cursorX_ : panel_.x + 8;
    if (left + w > panel_.x + panel_.width - 8 && cursorX_ >= 0) { // a full row: start the next
        cursorX_ = -1;
        cursorY_ += kRowHeight + 2;
        widgets_.push_back({{panel_.x + 8, cursorY_, w, kRowHeight}, label, enabled, selected, id});
        cursorX_ = panel_.x + 8 + w + 3;
        return;
    }
    widgets_.push_back({{left, cursorY_, w, kRowHeight}, label, enabled, selected, id});
    cursorX_ = left + w + 3;
}

void RunFlow::openNewGame() {
    screen_ = Screen::NewGame;
    seedText_.clear();
    preset_ = 0;
    comfort_ = 1;
}

void RunFlow::openFocus() {
    screen_ = Screen::Focus;
    picked_.clear();
}

void RunFlow::openMenu() {
    screen_ = Screen::Menu;
}

// ---- building the screens

void RunFlow::build(OdysseyGame& game) {
    widgets_.clear();
    lines_.clear();
    cursorY_ = panel_.y + 6;
    cursorX_ = -1;
    switch (screen_) {
    case Screen::NewGame: buildNewGame(game); break;
    case Screen::Focus: buildFocus(game); break;
    case Screen::Event: buildEvent(game); break;
    case Screen::Mantle: buildMantle(game); break;
    case Screen::Menu: buildMenu(game); break;
    case Screen::Craft: buildCraft(game); break;
    case Screen::Barter: buildBarter(game); break;
    case Screen::Context: buildContext(); break;
    case Screen::Talk: buildTalk(game); break;
    case Screen::Ended: buildEnded(game); break;
    case Screen::Privacy: buildPrivacy(); break;
    default: break;
    }
    if (!message_.empty() && screen_ != Screen::None) {
        const std::vector<std::string> parts = wrapped(message_, 62);
        int y = panel_.y + panel_.height - 6 - static_cast<int>(parts.size()) * luna::engine::kLineHeight;
        for (const std::string& part : parts) {
            lines_.push_back({part, UiColor::Gold, panel_.x + 8, y});
            y += luna::engine::kLineHeight;
        }
    }
}

void RunFlow::buildNewGame(OdysseyGame& game) {
    const sim::HeroData& data = *game.heroData();
    title("NEW GAME");
    line(std::format("Seed: {}{}   (type digits; empty = random)", seedText_.empty() ? "random" : seedText_, seedText_.size() < 18 ? "_" : ""));
    gap();
    line("Growing Period:", UiColor::Dim);
    for (std::size_t i = 0; i < data.config.presets.size(); ++i) {
        const sim::PresetConfig& p = data.config.presets[i];
        button(std::format("{} (age {})", p.name, p.startAge), kPresetBase + static_cast<int>(i), true, static_cast<int>(i) == preset_);
    }
    newRow();
    cursorY_ += kRowHeight + 6;
    line("Comfort:", UiColor::Dim);
    for (std::size_t i = 0; i < data.config.comforts.size(); ++i) button(data.config.comforts[i].name, kComfortBase + static_cast<int>(i), true, static_cast<int>(i) == comfort_);
    newRow();
    cursorY_ += kRowHeight + 6;
    button(tutorial_ ? "Tutorial: on" : "Tutorial: off", kTutorialToggle, true, tutorial_, panel_.x + 8, 100);
    cursorY_ += kRowHeight + 6;
    button("Start", kStart, true, false, panel_.x + 8, 60);
}

void RunFlow::buildPrivacy() {
    title("SESSION STATISTICS");
    paragraph("May the game keep a small file on this computer with how long you played and the key moments of the session? It helps the makers see how the game plays. The file stays on your computer; the game never sends anything anywhere. You can say no and play exactly the same.");
    gap();
    button("Yes, keep it on my computer", kStatsYes, true, false, panel_.x + 8, 180);
    cursorY_ += kRowHeight + 6;
    button("No, record nothing", kStatsNo, true, false, panel_.x + 8, 180);
}

void RunFlow::buildFocus(OdysseyGame& game) {
    const sim::HeroLife& hero = *game.life();
    title(std::format("A YEAR OF YOUTH: {} is {}", hero.name(), hero.ageYears()));
    line(std::format("Early choices shape you most: this year's imprint is x{:.2f}.", hero.imprintPercent(hero.ageYears()) / 100.0));
    std::string a;
    for (std::size_t i = 0; i < sim::kAffinityCount; ++i) a += std::format("{} {}  ", sim::affinityName(static_cast<sim::Affinity>(i)), hero.affinities()[i]);
    paragraph(a, UiColor::Dim, 66);
    gap();
    line("Choose two activities for the year:");
    const auto& activities = hero.activities();
    for (std::size_t i = 0; i < activities.size(); ++i) {
        const bool picked = std::find(picked_.begin(), picked_.end(), static_cast<int>(i)) != picked_.end();
        button(activities[i].name, kActivityBase + static_cast<int>(i), true, picked, panel_.x + 8, panel_.width - 16);
        cursorX_ = -1;
        cursorY_ += kRowHeight + 2;
    }
    gap();
    button("Live the year", kLive, picked_.size() == 2, false, panel_.x + 8, 90);
}

void RunFlow::buildEvent(OdysseyGame& game) {
    const sim::HeroLife& hero = *game.life();
    const sim::CrossroadsEvent* event = hero.pendingEvent();
    if (event == nullptr) return;
    title(event->title.empty() ? "CROSSROADS" : "CROSSROADS: " + event->title);
    paragraph(event->text);
    gap(6);
    for (std::size_t i = 0; i < event->options.size(); ++i) {
        for (const std::string& part : wrapped(event->options[i].text, 60)) {
            (void)part;
        }
        button(std::format("{}. {}", i + 1, event->options[i].text), kOptionBase + static_cast<int>(i), true, false, panel_.x + 8, panel_.width - 16);
        cursorX_ = -1;
        cursorY_ += kRowHeight + 2;
    }
}

void RunFlow::buildMantle(OdysseyGame& game) {
    const sim::HeroLife& hero = *game.life();
    title(std::format("THE MANTLE: {} is {}", hero.name(), hero.ageYears()));
    std::string names;
    for (const std::string& n : hero.specialtyNames()) names += (names.empty() ? "" : " and ") + n;
    line(std::format("Your specialty: {}", names), UiColor::Gold);
    gap();
    for (std::size_t i = 0; i < sim::kAffinityCount; ++i) line(std::format("{:<14} {:>3}", sim::affinityName(static_cast<sim::Affinity>(i)), hero.affinities()[i]));
    gap();
    paragraph("Your youth is over. From here the days are yours: learn a trade from a master, craft, trade with the rival clans, or keep a sacred fire, until your clan leads the region.", UiColor::Dim);
    gap(6);
    button("Begin", kBegin, true, false, panel_.x + 8, 60);
}

void RunFlow::buildMenu(OdysseyGame& game) {
    sim::HeroLife* hero = game.life();
    const sim::HeroData* data = game.heroData();
    const char* tabs[] = {"Bag", "Skills", "Dominion", "Settings"};
    for (int i = 0; i < 4; ++i) button(tabs[i], kTabBase + i, true, static_cast<int>(tab_) == i);
    button("Close", kClose);
    newRow();
    cursorY_ += kRowHeight + 6;
    if (hero == nullptr || data == nullptr) {
        if (tab_ != MenuTab::Settings) {
            line("No run is in progress. Start a new game.");
            button("New game", kNewGameButton, true, false, panel_.x + 8, 70);
            return;
        }
    }
    switch (tab_) {
    case MenuTab::Bag: {
        title(std::format("{}  age {}", hero->name(), hero->ageYears()));
        if (hero->inventory().empty()) line("Your bag is empty.", UiColor::Dim);
        for (const auto& [id, n] : hero->inventory()) line(std::format("{} x {}", data->item(id) != nullptr ? data->item(id)->name : id, n));
        gap();
        line("Things you made:", UiColor::Dim);
        const auto& crafted = hero->crafted();
        const std::size_t first = crafted.size() > 4 ? crafted.size() - 4 : 0;
        for (std::size_t i = first; i < crafted.size(); ++i) paragraph(hero->describe(crafted[i]), UiColor::Text, 66);
        if (crafted.empty()) line("nothing yet", UiColor::Dim);
        break;
    }
    case MenuTab::Skills: {
        title("Skills and masters");
        for (std::size_t p = 0; p < data->professions.size(); ++p) {
            const int master = hero->masterOf(static_cast<int>(p));
            const int apprentice = hero->apprenticeOf(static_cast<int>(p));
            std::string state = apprentice >= 0 ? std::format("apprentice of {}", hero->world().people()[static_cast<std::size_t>(apprentice)].name) : "";
            line(std::format("{:<14} level {} ({} points) {}", data->professions[p].name, hero->skillLevel(static_cast<int>(p)), hero->skillPoints(static_cast<int>(p)), state));
            if (apprentice < 0 && master >= 0) {
                button(std::format("Ask {} to teach you", hero->world().people()[static_cast<std::size_t>(master)].name), kApprenticeBase + static_cast<int>(p), true, false, panel_.x + 16, 210);
                cursorX_ = -1;
                cursorY_ += kRowHeight + 2;
            }
        }
        break;
    }
    case MenuTab::Dominion: {
        title("Dominion");
        line(std::format("Trade     {:>3}%", hero->tradePercent()));
        line(std::format("Religion  {:>3}%", hero->religionPercent()));
        for (const char* later : {"Military", "Politics", "Technology", "Ideology"}) line(std::format("{:<10} later Ages", later), UiColor::Dim);
        line(std::format("Win at {}% in Trade or Religion, or {}% combined.", data->config.dominion.winPercent, data->config.dominion.combinedWinPercent), UiColor::Dim);
        gap();
        const sim::SacredFire& fire = hero->fire();
        if (fire.founded) {
            line(std::format("Sacred fire {}: {}, {} followers", fire.name, fire.lit ? "burning" : "OUT", hero->followers()), UiColor::Gold);
            button("Hold a ritual", kRitual);
            button("Tend the fire", kTendFire);
        } else {
            const std::string why = hero->foundFireBlockedReason();
            line(std::format("Name your fire: {}_", fireName_.empty() ? "(type)" : fireName_));
            button("Found a sacred fire here", kFoundFire, why.empty() && !fireName_.empty(), false, panel_.x + 8, 150);
            if (!why.empty()) {
                cursorX_ = -1;
                cursorY_ += kRowHeight + 2;
                paragraph(why, UiColor::Dim);
            }
        }
        break;
    }
    case MenuTab::Settings: {
        const GameSettings& s = game.settings();
        title("Settings");
        button(std::format("Full screen: {}", s.fullscreen ? "on" : "off"), kFullscreen, true, false, panel_.x + 8, 130);
        newRow();
        cursorY_ += kRowHeight + 4;
        line("Window size:", UiColor::Dim);
        const int sizes[][2] = {{1280, 720}, {1600, 900}, {1920, 1080}};
        for (int i = 0; i < 3; ++i) button(std::format("{}x{}", sizes[i][0], sizes[i][1]), kResolutionBase + i, true, s.width == sizes[i][0]);
        newRow();
        cursorY_ += kRowHeight + 4;
        line(std::format("Volume: {}", s.volume), UiColor::Text);
        button("-", kVolumeDown, true, false, panel_.x + 8, 20);
        button("+", kVolumeUp, true, false, panel_.x + 32, 20);
        break;
    }
    }
}

void RunFlow::buildCraft(OdysseyGame& game) {
    sim::HeroLife& hero = *game.life();
    const sim::HeroData& data = *game.heroData();
    title(station_ == "fire" ? "AT THE FIRE" : "AT THE KNAPPING STONE");
    int shown = 0;
    for (std::size_t i = 0; i < data.recipes.size(); ++i) {
        const sim::Recipe& recipe = data.recipes[i];
        if (recipe.station != station_) continue;
        std::string need;
        for (const auto& [id, n] : recipe.inputs) need += std::format("{}{} {}", need.empty() ? "" : " + ", n, data.item(id)->name);
        for (const std::string& tool : recipe.tools) need += std::format("{}[{}]", need.empty() ? "" : " + ", data.item(tool)->name);
        const std::string why = hero.craftBlockedReason(recipe);
        button(std::format("Make {}  ({})", recipe.name, need.empty() ? "no materials" : need), kRecipeBase + static_cast<int>(i), why.empty(), false, panel_.x + 8, panel_.width - 16);
        cursorX_ = -1;
        cursorY_ += kRowHeight + 1;
        if (!why.empty()) line("   " + why, UiColor::Red);
        ++shown;
    }
    if (shown == 0) line("Nothing can be made here.", UiColor::Dim);
    gap();
    line("In your bag: " + goodsLine(data, hero.inventory()), UiColor::Dim);
    button("Close", kClose, true, false, panel_.x + 8, 50);
}

void RunFlow::buildBarter(OdysseyGame& game) {
    sim::HeroLife& hero = *game.life();
    const sim::HeroData& data = *game.heroData();
    const auto& rivals = hero.rivals();
    if (rival_ < 0 || rival_ >= static_cast<int>(rivals.size())) return;
    title(std::format("BARTER with {}  ({} people, they think {} of you)", rivals[static_cast<std::size_t>(rival_)].name, rivals[static_cast<std::size_t>(rival_)].population, hero.relation(rival_)));
    line("You give (your bag):", UiColor::Dim);
    int column = 0;
    int index = 0;
    for (const auto& [id, n] : hero.inventory()) {
        const int chosen = give_.contains(id) ? give_[id] : 0;
        button(std::format("{} {}/{}", data.item(id)->name, chosen, n), kGiveBase + index, n > 0, chosen > 0, -1, 96);
        button("-", kGiveLessBase + index, chosen > 0, false, -1, 14);
        if (++column % 2 == 0) newRow(), cursorY_ += kRowHeight + 2;
        ++index;
    }
    newRow();
    cursorY_ += kRowHeight + 4;
    line("You want (their goods):", UiColor::Dim);
    column = 0;
    index = 0;
    for (const sim::Item& item : data.items) {
        if (item.kind == "material" && item.id != "flint") {
            ++index;
            continue; // they trade goods, tools, food and flint
        }
        const int chosen = want_.contains(item.id) ? want_[item.id] : 0;
        button(std::format("{}{} {}", item.name, hero.rivalNeeds(rival_, item.id) ? "*" : "", chosen), kWantBase + index, true, chosen > 0, -1, 96);
        button("-", kWantLessBase + index, chosen > 0, false, -1, 14);
        if (++column % 2 == 0) newRow(), cursorY_ += kRowHeight + 2;
        ++index;
    }
    newRow();
    cursorY_ += kRowHeight + 4;
    button(std::format("Pay later: {}", payLater_ ? "yes (+20%)" : "no"), kPayLater, true, payLater_, panel_.x + 8, 100);
    button("Propose", kPropose, true, false, -1, 60);
    if (counter_) button("Accept their counter", kAcceptCounter, true, false, -1, 120);
    button("Close", kClose, true, false, -1, 44);
    newRow();
    cursorY_ += kRowHeight + 4;
    int shownDebts = 0;
    for (std::size_t i = 0; i < hero.debts().size() && shownDebts < 3; ++i) {
        const sim::Debt& debt = hero.debts()[i];
        if (debt.settled || debt.defaulted) continue;
        std::string owe;
        for (const auto& [id, n] : debt.owe) owe += std::format("{}{} {}", owe.empty() ? "" : ", ", n, data.item(id)->name);
        button(std::format("Pay debt to {}: {} (due day {})", rivals[static_cast<std::size_t>(debt.rival)].name, owe, debt.dueDay + 1), kPayDebtBase + static_cast<int>(i), true, false, panel_.x + 8, panel_.width - 16);
        cursorX_ = -1;
        cursorY_ += kRowHeight + 1;
        ++shownDebts;
    }
    (void)game;
}

void RunFlow::buildContext() {
    title(contextTitle_);
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        button(actions_[i].label, kActionBase + static_cast<int>(i), actions_[i].reason.empty(), false, panel_.x + 8, 200);
        if (!actions_[i].reason.empty()) line("", UiColor::Dim);
        lines_.push_back({actions_[i].reason, UiColor::Red, panel_.x + 214, cursorY_ + 2});
        cursorX_ = -1;
        cursorY_ += kRowHeight + 2;
    }
    button("Cancel", kClose, true, false, panel_.x + 8, 60);
}

void RunFlow::buildEnded(OdysseyGame& game) {
    const sim::HeroLife& hero = *game.life();
    const sim::RunSummary s = hero.summary();
    const char* heading = s.outcome == sim::Outcome::Victory ? "VICTORY" : (s.outcome == sim::Outcome::Defeat ? "DEFEAT" : "THE END");
    title(std::format("{}: {} (age {})", heading, s.heroName, s.ageYears));
    paragraph(s.reason, UiColor::Gold);
    std::string specialty;
    for (const std::string& n : s.specialty) specialty += (specialty.empty() ? "" : " and ") + n;
    line(std::format("Specialty: {}   Trade {}%   Religion {}%", specialty.empty() ? "none" : specialty, s.tradePercent, s.religionPercent), UiColor::Dim);
    gap();
    line("A life in the chronicle:", UiColor::Dim);
    const std::size_t from = s.highlights.size() > 7 ? s.highlights.size() - 7 : 0;
    for (std::size_t i = from; i < s.highlights.size(); ++i) paragraph(s.highlights[i], UiColor::Text, 66);
    gap(6);
    button("New game", kNewGameButton, true, false, panel_.x + 8, 70);
}

// ---- acting

bool RunFlow::openContext(OdysseyGame& game, double wx, double wy) {
    const sim::HeroLife* hero = game.life();
    if (hero == nullptr || game.heroData() == nullptr || hero->phase() != sim::Phase::Free) return false;
    actions_.clear();
    // The thing under the pointer (a clan member, a fire, the stone, a rival camp, a plant), and what the interaction files offer the hero
    // for it (US-152). The menu is made entirely from data: labels, disabled reasons and order come from the files.
    const std::optional<Subject> subject = subjectAt(game, wx, wy);
    if (!subject) return false;
    contextTitle_ = subject->title;
    const GameRuleContext context(game, *subject);
    for (const sim::rules::Offer& offer : game.offersFor(*subject)) {
        const std::string id = offer.interaction->id; // by id: the data may be reloaded (F5) while the menu is open
        actions_.push_back({sim::rules::fillTokens(offer.interaction->label, context), offer.enabled ? std::string() : offer.reason,
                            [id, subject = *subject](OdysseyGame& g) {
                                if (!startInteraction(g, id, subject)) g.run().setMessage("That action is no longer in the data.");
                            }});
    }
    backTo_ = Screen::None;
    screen_ = Screen::Context;
    return true;
}
void RunFlow::setMessage(const std::string& text) { message_ = text; }

void RunFlow::openCraft(const std::string& station) {
    station_ = station;
    screen_ = Screen::Craft;
}

void RunFlow::openTalk(sim::rules::Conversation conversation, Subject subject) {
    conversation_ = std::move(conversation);
    talkSubject_ = std::move(subject);
    screen_ = Screen::Talk;
}

// The panel of a conversation: who, how they feel, what they say, and up to five numbered choices (D-38). Made fresh every tick from the
// simulation, like every screen, so a choice whose condition changed is never shown stale.
void RunFlow::buildTalk(OdysseyGame& game) {
    if (!conversation_ || !talkSubject_ || game.clan() == nullptr || game.life() == nullptr) {
        screen_ = Screen::None;
        return;
    }
    const GameRuleContext context(game, *talkSubject_);
    const sim::World& world = *game.clan();
    const int person = talkSubject_->index;
    std::string mood = "neutral";
    if (person >= 0 && static_cast<std::size_t>(person) < world.people().size()) {
        mood = sim::rules::moodWord(world.opinion(person, game.life()->personId()), world.people()[static_cast<std::size_t>(person)].needs);
    }
    title(std::format("{}   ({})", talkSubject_->name, mood));
    const sim::rules::ConversationView view = conversation_->view(context);
    for (const sim::rules::ConversationLine& said : view.lines) paragraph(said.speaker.empty() ? said.text : said.speaker + ": " + said.text, UiColor::Text, kTalkWrap);
    gap(6);
    for (std::size_t i = 0; i < view.choices.size(); ++i) {
        const sim::rules::ConversationChoice& choice = view.choices[i];
        const std::string label = std::format("{}. {}{}", i + 1, choice.text, choice.enabled || choice.reason.empty() ? "" : "  (" + choice.reason + ")");
        button(label, kTalkChoiceBase + static_cast<int>(i), choice.enabled, false, panel_.x + 8, panel_.width - 16);
        cursorX_ = -1;
        cursorY_ += kRowHeight + 2;
    }
    if (view.choices.empty()) button("Leave", kClose, true, false, panel_.x + 8, 60);
}

void RunFlow::openBarter(int rival) {
    rival_ = rival;
    screen_ = Screen::Barter;
    give_.clear();
    want_.clear();
    counter_.reset();
    payLater_ = false;
}

bool RunFlow::update(OdysseyGame& game, const luna::engine::Intents& intents) {
    if (screen_ == Screen::None) return false;
    const luna::engine::Pointer& pointer = intents.pointer();
    pointerX_ = pointer.x;
    pointerY_ = pointer.y;
    // Typing: the seed, or the name of a sacred fire.
    if (screen_ == Screen::NewGame) {
        for (const char c : intents.text()) {
            if (c >= '0' && c <= '9' && static_cast<int>(seedText_.size()) < seedDigitsMax) seedText_ += c;
        }
        if (intents.pressed(luna::engine::Intent::Erase) && !seedText_.empty()) seedText_.pop_back();
    } else if (screen_ == Screen::Menu && tab_ == MenuTab::Dominion) {
        for (const char c : intents.text()) {
            if (c >= 32 && c < 127 && fireName_.size() < 24) fireName_ += c;
        }
        if (intents.pressed(luna::engine::Intent::Erase) && !fireName_.empty()) fireName_.pop_back();
    }
    build(game);
    // A conversation: keys 1 to 5 pick a choice, Esc walks away (the world has stood still all the while).
    if (screen_ == Screen::Talk) {
        if (intents.pressed(luna::engine::Intent::OpenMenu)) {
            act(game, kClose);
            return screen_ != Screen::None;
        }
        for (int k = 0; k < sim::rules::kMaxVisibleChoices; ++k) {
            const auto slot = static_cast<luna::engine::Intent>(static_cast<int>(luna::engine::Intent::Slot1) + k);
            if (intents.pressed(slot)) {
                act(game, kTalkChoiceBase + k);
                build(game);
                return screen_ != Screen::None;
            }
        }
    }
    if ((screen_ == Screen::Menu || screen_ == Screen::Craft || screen_ == Screen::Barter || screen_ == Screen::Context) && intents.pressed(luna::engine::Intent::OpenMenu)) {
        screen_ = screen_ == Screen::Menu ? Screen::None : Screen::Menu;
        if (screen_ == Screen::Menu) build(game);
        return true;
    }
    if (screen_ == Screen::NewGame && intents.pressed(luna::engine::Intent::Interact)) {
        act(game, kStart);
        return true;
    }
    if (pointer.inside() && pointer.wasPressed(luna::engine::PointerButton::Left)) {
        for (const Widget& widget : widgets_) {
            if (widget.enabled && pointer.x >= widget.area.x && pointer.x < widget.area.x + widget.area.width && pointer.y >= widget.area.y && pointer.y < widget.area.y + widget.area.height) {
                act(game, widget.id);
                build(game); // show the result at once
                break;
            }
        }
    }
    return screen_ != Screen::None;
}

bool RunFlow::press(OdysseyGame& game, int id) {
    build(game);
    for (const Widget& widget : widgets_) {
        if (widget.id == id && widget.enabled) {
            act(game, id);
            build(game);
            return true;
        }
    }
    return false;
}

void RunFlow::act(OdysseyGame& game, int id) {
    sim::HeroLife* hero = game.life();
    const sim::HeroData* data = game.heroData();
    if (id == kClose) {
        screen_ = Screen::None;
        conversation_.reset(); // walking away from a talk: no effects
        message_.clear();
        return;
    }
    if (id == kNewGameButton) {
        openNewGame();
        message_.clear();
        return;
    }
    message_.clear();
    switch (screen_) {
    case Screen::Privacy: {
        if (id == kStatsYes || id == kStatsNo) {
            game.setStatistics(id == kStatsYes);
            screen_ = Screen::NewGame;
        }
        break;
    }
    case Screen::NewGame: {
        if (id == kTutorialToggle) tutorial_ = !tutorial_;
        else if (id >= kPresetBase && id < kPresetBase + 10) preset_ = id - kPresetBase;
        else if (id >= kComfortBase && id < kComfortBase + 10) comfort_ = id - kComfortBase;
        else if (id == kStart) {
            sim::NewGame newGame;
            newGame.seed = seedText_.empty() ? randomSeed() : std::stoull(seedText_);
            newGame.preset = preset_;
            newGame.comfort = comfort_;
            game.startNewRun(newGame, true, tutorial_);
        }
        break;
    }
    case Screen::Focus: {
        if (id >= kActivityBase && id < kActivityBase + 50) {
            const int index = id - kActivityBase;
            const auto at = std::find(picked_.begin(), picked_.end(), index);
            if (at != picked_.end()) picked_.erase(at);
            else {
                if (picked_.size() == 2) picked_.erase(picked_.begin());
                picked_.push_back(index);
            }
        } else if (id == kLive && hero != nullptr && picked_.size() == 2 && hero->chooseFocus(picked_[0], picked_[1])) {
            hero->liveYear();
            game.afterYear();
            picked_.clear();
            if (hero->phase() == sim::Phase::Ended) screen_ = Screen::Ended;
            else if (hero->pendingEvent() != nullptr) screen_ = Screen::Event;
            else if (hero->phase() == sim::Phase::Free) screen_ = Screen::Mantle;
        }
        break;
    }
    case Screen::Event: {
        if (id >= kOptionBase && id < kOptionBase + 5 && hero != nullptr && hero->resolveEvent(id - kOptionBase)) {
            message_ = hero->lastNote();
            screen_ = hero->phase() == sim::Phase::Free ? Screen::Mantle : Screen::Focus;
            picked_.clear();
        }
        break;
    }
    case Screen::Mantle:
        if (id == kBegin) screen_ = Screen::None;
        break;
    case Screen::Menu: {
        if (id >= kTabBase && id < kTabBase + 4) tab_ = static_cast<MenuTab>(id - kTabBase);
        else if (id >= kApprenticeBase && id < kApprenticeBase + 5 && hero != nullptr) message_ = hero->askToApprentice(id - kApprenticeBase).message;
        else if (id == kFoundFire && hero != nullptr) {
            const PixelPoint spot{static_cast<int>(game.hero().feetX()), static_cast<int>(game.hero().feetY())};
            const sim::ActionResult result = hero->foundFire(fireName_, spot.x / kTileSize, spot.y / kTileSize);
            message_ = result.message;
            if (result.ok) fireName_.clear();
        } else if (id == kRitual && hero != nullptr) {
            const double fx = hero->fire().tileX * kTileSize + 16;
            const double fy = hero->fire().tileY * kTileSize + 16;
            message_ = hero->holdRitual(game.attendeesAt(fx, fy, data->config.fire.ritualRadiusTiles)).message;
        } else if (id == kTendFire && hero != nullptr) message_ = hero->tendFire().message;
        else if (id == kFullscreen) { GameSettings s = game.settings(); s.fullscreen = !s.fullscreen; game.applySettings(s); }
        else if (id >= kResolutionBase && id < kResolutionBase + 3) {
            const int sizes[][2] = {{1280, 720}, {1600, 900}, {1920, 1080}};
            GameSettings s = game.settings();
            s.width = sizes[id - kResolutionBase][0];
            s.height = sizes[id - kResolutionBase][1];
            game.applySettings(s);
        } else if (id == kVolumeDown || id == kVolumeUp) {
            GameSettings s = game.settings();
            s.volume = std::clamp(s.volume + (id == kVolumeUp ? 10 : -10), 0, 100);
            game.applySettings(s);
        }
        break;
    }
    case Screen::Craft:
        if (id >= kRecipeBase && id < kRecipeBase + 50 && hero != nullptr && data != nullptr) message_ = hero->craft(data->recipes[static_cast<std::size_t>(id - kRecipeBase)].id).message;
        break;
    case Screen::Barter: {
        if (hero == nullptr || data == nullptr) break;
        if (id >= kGiveBase && id < kGiveBase + 20) {
            int index = 0;
            for (const auto& [item, n] : hero->inventory()) {
                if (index++ == id - kGiveBase && give_[item] < n) ++give_[item];
            }
        } else if (id >= kGiveLessBase && id < kGiveLessBase + 20) {
            int index = 0;
            for (const auto& [item, n] : hero->inventory()) {
                if (index++ == id - kGiveLessBase && give_[item] > 0) --give_[item];
                (void)n;
            }
        } else if (id >= kWantBase && id < kWantBase + 20) {
            ++want_[data->items.at(static_cast<std::size_t>(id - kWantBase)).id];
        } else if (id >= kWantLessBase && id < kWantLessBase + 20) {
            int& n = want_[data->items.at(static_cast<std::size_t>(id - kWantLessBase)).id];
            n = std::max(0, n - 1);
        } else if (id == kPayLater) payLater_ = !payLater_;
        else if (id == kPropose || id == kAcceptCounter) {
            sim::BarterOffer offer;
            if (id == kAcceptCounter && counter_) {
                offer = *counter_;
            } else {
                for (const auto& [item, n] : give_) if (n > 0) offer.give.push_back({item, n});
                for (const auto& [item, n] : want_) if (n > 0) offer.want.push_back({item, n});
                offer.payLater = payLater_;
            }
            const sim::BarterResult result = hero->barter(rival_, offer);
            message_ = result.message;
            counter_ = result.counter;
            if (result.accepted) {
                give_.clear();
                want_.clear();
                counter_.reset();
            }
        } else if (id >= kPayDebtBase && id < kPayDebtBase + 50) message_ = hero->payDebt(static_cast<std::size_t>(id - kPayDebtBase)).message;
        break;
    }
    case Screen::Context: {
        if (id >= kActionBase && id < kActionBase + static_cast<int>(actions_.size()) + kActionBase) {
            const std::size_t index = static_cast<std::size_t>(id - kActionBase);
            if (index < actions_.size() && actions_[index].reason.empty()) {
                const auto run = actions_[index].run;
                screen_ = Screen::None; // the action may open another screen
                run(game);
            }
        }
        break;
    }
    case Screen::Talk: {
        if (conversation_ && talkSubject_ && id >= kTalkChoiceBase && id < kTalkChoiceBase + sim::rules::kMaxVisibleChoices) {
            chooseConversationOption(game, *conversation_, *talkSubject_, id - kTalkChoiceBase);
            if (conversation_->finished()) {
                conversation_.reset();
                screen_ = Screen::None;
            }
        }
        break;
    }
    case Screen::Ended: break;
    default: break;
    }
}

void RunFlow::draw(luna::engine::Renderer& renderer, const luna::engine::Texture& uiSheet) const {
    if (screen_ == Screen::None) return;
    UiPainter painter(renderer, uiSheet);
    painter.fill({0, 0, 480, 270}, UiColor::Shade);
    painter.fill(panel_, UiColor::Dark);
    painter.outline(panel_, UiColor::Gold);
    for (const Line& l : lines_) painter.text(l.x, l.y, l.text, l.colour);
    for (const Widget& w : widgets_) {
        const bool hover = pointerX_ >= w.area.x && pointerX_ < w.area.x + w.area.width && pointerY_ >= w.area.y && pointerY_ < w.area.y + w.area.height;
        painter.fill(w.area, w.selected ? UiColor::Selected : (hover && w.enabled ? UiColor::Hover : UiColor::Panel));
        painter.outline(w.area, w.enabled ? UiColor::Border : UiColor::Dim);
        painter.text(w.area.x + 4, w.area.y + 3, w.label.substr(0, static_cast<std::size_t>(std::max(1, (w.area.width - 6) / luna::engine::kTextAdvance))), w.enabled ? UiColor::Text : UiColor::Dim);
    }
}

} // namespace odysseus::game
