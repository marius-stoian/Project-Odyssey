#pragma once

#include "boundary.h"

#include "game/dialogue_graph.h"
#include "game/editor_history.h"
#include "game/interaction_graph.h"
#include "sim/graph_check.h"
#include "luna/engine/input.h"
#include "luna/engine/node_graph.h"
#include "luna/engine/ui.h"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::game {

// The graph editor of the Editor (M9, D-56): a full-screen canvas over the map with a file list, an "add card" bar and a side panel for the
// chosen card. US-171 opens `.dlg` conversations in it; the interaction graphs (US-172) use the same canvas.
// Every edit of a graph is a Command in the Editor's one History (Q19), through `record`.
class GraphEditor {
public:
    using Record = std::function<void(std::unique_ptr<Command>)>; // remember a command that is already applied
    using Say = std::function<void(const std::string&)>;          // a line for the Editor's status bar
    enum class Kind { Dialogue, Interaction }; // what the editor shows: conversations (`.dlg`) or interactions (`.json`)

    GraphEditor(int viewWidth, int viewHeight, Record record, Say say);

    // The folders the files live in, and what to call after a file was written (the game reads the data again, like F5).
    void setFolders(std::filesystem::path dialogueFolder, std::filesystem::path interactionFolder, std::function<void()> saved);

    bool shown() const { return shown_; }
    void show(bool shown);
    void showKind(Kind kind); // opens the editor on conversations or on interactions
    Kind kind() const { return kind_; }
    // Opens `assets/data/dialogue/<name>.dlg` with its layout sidecar. False (and says why) when the file has mistakes.
    bool open(const std::string& name);
    const std::string& openName() const { return current_ != nullptr ? current_->name : empty_; }
    std::vector<std::string> files() const; // the files of the shown kind (names without the extension)
    std::vector<std::string> dialogueNames() const; // the .dlg names, whatever kind is shown: the pick list of a placed character's script
    // Writes the open conversation and its layout. Refuses (and says why) with a card that cannot be written, and, once, when the file changed on
    // disk since it was opened (press Save again to overwrite).
    bool save();
    bool dirty() const;
    // The check (US-175, D-56 Q17): what the open file names that does not exist, nodes nobody reaches, dead ends. It runs again whenever the graph changed;
    // the list under the canvas shows it, and a click selects the card it points to. `catalog` is what the game knows (items, tags, built-in actions).
    struct Problem {
        bool error = false;
        std::string key; // the card, by its key in the file; empty: the whole file
        std::string text;
    };
    void setCatalog(sim::rules::GraphCatalog catalog) {
        catalog_ = std::move(catalog);
        checked_.reset();
    }
    const std::vector<Problem>& findings() const { return findings_; }
    void recheck();
    bool pickProblem(std::size_t index); // select (and show) the card the finding points to; false for a finding about the whole file
    // After Undo or Redo outside: selection and side panel are looked at again.
    void refresh();

    // One tick: keys, the toolbar, the canvas, the side panel.
    void update(const luna::engine::Intents& intents);
    void draw(luna::engine::UiPainter& painter) const;
    void drawOverlay(luna::engine::UiPainter& painter) const;
    bool typing() const;

    // For the panel and for tests: every change is one step of Undo.
    luna::engine::NodeGraph* graph() { return current_ != nullptr ? current_->graph.get() : nullptr; }
    luna::engine::NodeGraphView* view() { return view_.get(); }
    int addCard(const std::string& type); // a new empty card in the middle of the canvas, selected; returns its id
    bool setCardField(int card, std::size_t index, const std::string& value);
    bool addCardField(int card);          // one more line in an effect or comment card
    bool removeCardField(int card);       // the last line of an effect or comment card
    sim::rules::DlgScript& header() { return current_->header; } // @who and the rest: kept with the file, not part of Undo
    const std::vector<std::string>& problems() const { return problems_; } // from the last save or check, "error: ..." and "warning: ..."
    luna::engine::Rect canvasBounds() const { return canvas_; }
    luna::engine::Rect problemListBounds() const { return problemList_->bounds; }

private:
    struct Doc {
        std::string name;
        std::shared_ptr<luna::engine::NodeGraph> graph = std::make_shared<luna::engine::NodeGraph>();
        sim::rules::DlgScript header;
        luna::engine::NodeGraph saved; // the graph as loaded or last saved, to know whether it changed
        std::string savedHeader;
        Kind kind = Kind::Dialogue;
        std::string leading; // the comment lines at the top of an interaction file, kept
        std::filesystem::file_time_type loadedAt{};
        bool existed = false;
    };

    void buildChrome();
    void buildPanel();
    int selectedCard() const; // the id of the one selected card, or 0
    void edit(const std::string& name, const std::function<void(luna::engine::NodeGraph&)>& change);
    void onViewEdit(const std::string& name, const luna::engine::NodeGraph& before, const luna::engine::NodeGraph& after);
    std::filesystem::path fileOf(const std::string& name) const; // in the folder of the shown kind
    static std::string docKey(Kind kind, const std::string& name) { return (kind == Kind::Dialogue ? "d:" : "i:") + name; }
    bool openDialogue(const std::string& name);
    bool openInteraction(const std::string& name);
    bool saveDialogue();
    bool saveInteraction();
    void bindView();
    std::string errorsNote() const;
    std::string headerText(const sim::rules::DlgScript& script) const;

    int viewWidth_;
    int viewHeight_;
    Record record_;
    Say say_;
    std::filesystem::path folder_;            // dialogue
    std::filesystem::path interactionFolder_;
    std::function<void()> saved_;
    bool shown_ = false;
    std::string empty_;

    std::map<std::string, Doc> docs_;
    Kind kind_ = Kind::Dialogue;
    Doc* current_ = nullptr;
    std::unique_ptr<luna::engine::NodeGraphView> view_;
    luna::engine::Rect canvas_{};
    std::unique_ptr<luna::engine::Panel> chrome_; // the bar and the file list
    std::unique_ptr<luna::engine::Panel> panel_;  // the side panel of the chosen card (or of the file)
    std::string panelKey_;
    sim::rules::GraphCatalog catalog_;
    std::vector<Problem> findings_;
    std::optional<luna::engine::NodeGraph> checked_; // the graph the findings were made from
    std::unique_ptr<luna::engine::ListBox> problemList_;
    std::string panelTitle_;
    std::vector<std::string> problems_;
    bool overwriteArmed_ = false;
    int nodeCounter_ = 1;
    bool rebuild_ = false; // the bar, the list and the side panel are made again at the start of the next tick, never from inside one of their own buttons
};

} // namespace odysseus::game
