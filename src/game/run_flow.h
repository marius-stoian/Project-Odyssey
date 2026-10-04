#pragma once

#include "boundary.h"

#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "game/game_rules.h"
#include "luna/engine/ui.h"
#include "sim/conversation.h"
#include "sim/hero_life.h"

#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

class OdysseyGame;

// The screens of a run (M5, D-32): the New Game screen, the yearly Focus choice and Crossroads events of the Growing Period, the
// Mantle summary, the menu (bag, skills, dominion, settings), crafting, bartering, the context menu of a thing, a conversation
// with a clan member (US-161), and the end of the run. Each is a panel of text lines and buttons made fresh every tick from the run's state (immediate mode), so a screen can
// never show anything stale; a click on a button changes the run through the simulation layer's own functions. While a screen
// is open the world stands still.
enum class Screen { None, NewGame, Focus, Event, Mantle, Menu, Craft, Barter, Context, Talk, Ended, Privacy };
enum class MenuTab { Bag, Skills, Dominion, Settings };

class RunFlow {
public:
    Screen screen() const { return screen_; }
    bool modal() const { return screen_ != Screen::None; }

    void openNewGame();
    void openFocus();
    void openPrivacy() { screen_ = Screen::Privacy; }
    bool tutorialOn() const { return tutorial_; }
    void setTutorialOn(bool on) { tutorial_ = on; }
    void openMenu();
    void close() { screen_ = Screen::None; }
    void openMantle() { screen_ = Screen::Mantle; }
    void openEnded() { screen_ = Screen::Ended; }
    void openCraft(const std::string& station);
    void openBarter(int rival);
    // A conversation with a clan member (US-161): the panel shows their name, mood, words and numbered choices; the world waits.
    void openTalk(sim::rules::Conversation conversation, Subject subject);
    const sim::rules::Conversation* conversation() const { return conversation_ ? &*conversation_ : nullptr; }
    void setMessage(const std::string& text);

    // A right click on something in the world at (worldX, worldY): the things there offer their actions in a context menu
    // (US-061). False when there is nothing there to offer anything.
    bool openContext(OdysseyGame& game, double worldX, double worldY);
    // The menu of the confront actions of an NPC (US-266): the interactions that say "menu": "confront". The ordinary menu never lists them.
    bool openConfront(OdysseyGame& game, const Subject& subject);

    // One tick with a screen open: hit-test the buttons, act on a click. Returns true while a screen is open (the world waits).
    bool update(OdysseyGame& game, const luna::engine::Intents& intents);
    void draw(luna::engine::Renderer& renderer, const luna::engine::Texture& uiSheet) const;

    // The open context menu: its heading and its items with the reason each is greyed out (empty = possible). For tests and scripted play;
    // press(game, kActionBase + i) chooses item i.
    struct ContextEntry {
        std::string label;
        std::string reason;
    };
    std::vector<ContextEntry> contextEntries() const {
        std::vector<ContextEntry> entries;
        for (const ContextAction& action : actions_) entries.push_back({action.label, action.reason});
        return entries;
    }
    const std::string& contextTitle() const { return contextTitle_; }
    static constexpr int kContextBase = 700;

    // The text lines the open screen shows now, top to bottom (for tests and scripted play): the heading first.
    std::vector<std::string> shownText() const {
        std::vector<std::string> out;
        for (const Line& l : lines_) out.push_back(l.text);
        return out;
    }

    // What the last action said (shown on the screen and kept for tests).
    const std::string& message() const { return message_; }

    // Ids of the widgets the screen offers now, in order, with their labels (for tests and scripted play).
    struct Widget {
        luna::engine::Rect area;
        std::string label;
        bool enabled = true;
        bool selected = false;
        int id = 0;
    };
    const std::vector<Widget>& widgets() const { return widgets_; }
    // Presses a widget by id as if it had been clicked (tests and scripted play).
    bool press(OdysseyGame& game, int id);
    static constexpr int kClose = 999;
    // The choices of a conversation: widget kTalkChoiceBase + n is the n-th choice shown (keys 1 to 5 press the same widgets).
    static constexpr int kTalkChoiceBase = 800;

    // Screen-specific choices the tests and the game set.
    int seedDigitsMax = 18;

private:
    struct Line {
        std::string text;
        luna::engine::UiColor colour = luna::engine::UiColor::Text;
        int x = 0;
        int y = 0;
    };
    struct ContextAction {
        std::string label;
        std::string reason; // why it is not possible now; empty = possible
        std::function<void(OdysseyGame&)> run;
    };

    bool openMenuFor(OdysseyGame& game, const Subject& subject, bool confront);
    void build(OdysseyGame& game);
    void buildNewGame(OdysseyGame& game);
    void buildFocus(OdysseyGame& game);
    void buildEvent(OdysseyGame& game);
    void buildMantle(OdysseyGame& game);
    void buildMenu(OdysseyGame& game);
    void buildCraft(OdysseyGame& game);
    void buildBarter(OdysseyGame& game);
    void buildContext();
    void buildTalk(OdysseyGame& game);
    void buildEnded(OdysseyGame& game);
    void buildPrivacy();
    void act(OdysseyGame& game, int id);

    // Layout helpers while building.
    void title(const std::string& text);
    void line(const std::string& text, luna::engine::UiColor colour = luna::engine::UiColor::Text);
    void paragraph(const std::string& text, luna::engine::UiColor colour = luna::engine::UiColor::Text, int width = 62);
    void button(const std::string& label, int id, bool enabled = true, bool selected = false, int x = -1, int width = -1);
    void gap(int pixels = 4) { cursorY_ += pixels; }
    void newRow() { cursorX_ = -1; }

    Screen screen_ = Screen::None;
    Screen backTo_ = Screen::None;       // where Craft, Barter and Context return to
    MenuTab tab_ = MenuTab::Bag;
    std::vector<Widget> widgets_;
    std::vector<Line> lines_;
    std::string message_;
    int cursorY_ = 0;
    int cursorX_ = -1;
    static constexpr int kMaxPanelWidth = 600;  // US-233: wide enough for long lines, narrow enough to read
    static constexpr int kMaxPanelHeight = 420;
    static constexpr int kMinPanelHeight = 120;
    int fitChars() const { return (panel_.width - 16) / luna::engine::kTextAdvance; }
    luna::engine::Rect screenArea_{0, 0, 480, 270};
    luna::engine::Rect panel_{30, 10, 420, 250};

    // New game
    std::string seedText_;
    int preset_ = 0;
    int comfort_ = 1;
    bool tutorial_ = true;
    // Focus
    std::vector<int> picked_;
    // Craft and barter
    std::string station_;
    int rival_ = 0;
    sim::BarterOffer offer_;
    std::map<std::string, int> give_;
    std::map<std::string, int> want_;
    bool payLater_ = false;
    std::optional<sim::BarterOffer> counter_;
    // Context
    std::vector<ContextAction> actions_;
    std::string contextTitle_;
    // Talk
    std::optional<sim::rules::Conversation> conversation_;
    std::optional<Subject> talkSubject_;
    // Sacred fire name being typed
    std::string fireName_;
    int pointerX_ = -1;
    int pointerY_ = -1;
};

} // namespace odysseus::game
