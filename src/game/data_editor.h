#pragma once

#include "boundary.h"

#include "game/editor_help.h"
#include "luna/engine/input.h"
#include "luna/engine/ui.h"
#include "sim/data_document.h"
#include "sim/data_form.h"
#include "sim/data_refs.h"
#include "sim/quick_check.h"
#include "sim/schema.h"
#include "sim/schema_index.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace odysseus::game {

// The Data tab of the Editor (US-191, EDT-02): any data file of assets/data/ opens as forms built from its schema. Folders, files and the entries of the open file are
// lists on the left; the chosen entry's fields are the form on the right: a number with its range, a choice, a yes or no, a link to another catalog with its names
// offered, a list, a group. Every field shows its help (the schema's description and example) when the pointer rests on it, and a value that breaks a rule is
// refused with the reason. Edits go through a `sim::DataDocument`, so Ctrl+Z and Ctrl+Y walk back and forth, and Ctrl+S writes the file by patching the text it
// was read from: only what changed changes, notes and comments stay. After a save the game reads the file again (`saved`). It takes the whole screen like the graph
// editor; Esc comes back. Everything the screen does is also a function below, so the tests drive it the way a hand would.
class DataEditor {
public:
    using Say = std::function<void(const std::string&)>;
    DataEditor(int viewWidth, int viewHeight, Say say);

    // The data folder (assets/data) and what to do after a file was written. The schemas are read from `<folder>/schemas` unless the game installed them already.
    void setFolder(std::filesystem::path dataFolder, std::function<void(const std::filesystem::path&)> saved = {});
    // The help the fields show; the entries made from the schemas are given to it.
    void setHelp(EditorHelp* help);

    bool shown() const { return shown_; }
    void show(bool shown);
    bool typing() const;

    // Every data file that has a schema, below the data folder with forward slashes ("sim/needs.json").
    std::vector<std::string> files() const;
    // Opens a file. Refused, with the reason said, when the open file has unsaved edits.
    bool open(const std::string& relative);
    // Chooses what the form shows: an entry path of entries(): "weapons[2]", or "" for the file's own settings.
    bool selectEntry(const std::string& path);
    // Types `text` into the field at `path` and presses Enter: a value that does not fit is refused (`problem()` says why) and nothing changes.
    bool setField(const std::string& path, const std::string& text);
    bool toggleFold(const std::string& path);
    // The buttons of a list or map heading and of an entry: Add, Remove (or reset to default), Up and Down.
    bool add(const std::string& path);
    bool remove(const std::string& path);
    bool move(const std::string& path, int steps);
    bool undo();
    bool redo();
    bool save();
    // Adds a new entry to the list the open file keeps its entries in (a new weapon in weapons.json) and shows it; it is a step of undo like any edit.
    bool newEntry();
    // Entity actions (US-193) on the chosen entry of a catalog. Copy makes a new entry beside it, named "<name> copy". Rename and Delete first ask: the places that use the
    // entry are listed (`question()`), and nothing changes until `confirm()`; Esc or `cancel()` leaves everything as it was. A rename writes every file that names the
    // entry at once (the data files, the levels, the dialogues) and is refused while the open file has unsaved changes; a delete is an edit of the open file, undone with
    // Ctrl+Z, and waits for the confirmation only when the entry is still used.
    struct Question {
        enum class Kind { None, Rename, Delete, Quick };
        Kind kind = Kind::None;
        std::string title;
        std::vector<std::string> lines; // "file:line: where"
        std::string name;               // the new name (a rename)
    };
    bool copyEntry();
    bool beginRename(const std::string& newName);
    bool beginDelete();
    bool confirm();
    void cancel();
    const Question& question() const { return question_; }
    // The Quick check (US-194): the clan simulation for 20 years on the saved data and the game's seed, a slice of days at every update so the screen stays alive.
    // The summary (population, deaths by cause, episodes) shows beside the run before it, which is kept for this session only; Esc stops a run.
    void setQuickSeed(std::function<std::uint64_t()> seed) { quickSeed_ = std::move(seed); }
    bool startQuickCheck();
    bool quickRunning() const { return check_.has_value(); }
    // The interactions that name the chosen entry's kind in their targets (the Interactions button opens the first one in the interaction graph).
    std::vector<std::string> interactionsOfEntry() const;
    void setOpenInteraction(std::function<void(const std::string&)> open) { openInteraction_ = std::move(open); }
    // The tags a kind carries in the game, derived ones included (a plant that blocks walking is "solid"): the game knows them, the file may not write them.
    void setTagsOf(std::function<std::vector<std::string>(const std::string&)> tagsOf) { tagsOf_ = std::move(tagsOf); }
    // The open file was changed on disk by something else: read it again unless it has unsaved edits (then it only says so). True when it was read again.
    bool changedOnDisk(const std::filesystem::path& file);

    const std::string& openFile() const { return openFile_; }
    const sim::DataDocument* document() const { return document_ ? &*document_ : nullptr; }
    const std::vector<sim::form::Entry>& entries() const { return entries_; }
    const std::vector<sim::form::FormRow>& rows() const { return rows_; }
    const std::string& entryPath() const { return entryPath_; }
    const std::string& problem() const { return problem_; } // why the last edit was refused
    bool dirty() const { return document_ && document_->dirty(); }
    const sim::schema::Schema* schema() const { return schema_; }
    const luna::engine::Panel& panel() const { return *panel_; } // the widgets as they stand (tests find a field and click it)
    // The names of a catalog, from the data folder (the picker of a link field offers them).
    std::vector<std::string> catalogNames(const std::string& catalog) const;
    // The help entries the schemas give (id "data.<schema>.<path>"), for EditorHelp.
    static std::map<std::string, EditorHelp::Entry> helpFromSchemas(const sim::schema::SchemaSet& set);

    void update(const luna::engine::Intents& intents);
    void draw(luna::engine::UiPainter& painter) const;
    void drawOverlay(luna::engine::UiPainter& painter) const;

private:
    bool loadSchemas();
    void installHelp();
    void refreshIndex();
    void refreshRows();
    void rebuild();
    void buildForm(luna::engine::Panel& panel);
    void addRow(luna::engine::Panel& panel, const sim::form::FormRow& row, int x, int y, int right);
    std::string tipFor(const sim::form::FormRow& row) const;
    const sim::form::FormRow* rowAt(const std::string& path) const;
    void refuse(const std::string& message);
    void markStale() { stale_ = true; }
    int visibleRows() const;
    struct EntryInfo {
        bool valid = false;
        std::string group;
        std::size_t index = 0;
        std::string field; // the member that names the entry ("name", "id")
        std::string name;
        std::set<std::string> catalogs; // everything the entry is the name of (a plant is in plants and kinds)
    };
    EntryInfo describeEntry() const;
    sim::refs::Roots roots() const { return {folder_, folder_.parent_path() / "levels"}; }
    void buildQuestion(luna::engine::Panel& panel);
    void stepQuickCheck();
    void openRenameDialog();

    int viewWidth_;
    int viewHeight_;
    Say say_;
    std::function<void(const std::filesystem::path&)> saved_;
    std::filesystem::path folder_;
    EditorHelp* help_ = nullptr;
    std::shared_ptr<const sim::schema::SchemaSet> set_;
    bool shown_ = false;
    bool stale_ = true;

    std::string folderChosen_;                  // the folder list's choice ("" is the files at the top of assets/data)
    std::string openFile_;
    std::optional<sim::DataDocument> document_;
    const sim::schema::Schema* schema_ = nullptr;
    std::vector<sim::form::Entry> entries_;
    std::string entryPath_;
    std::string search_;
    std::set<std::string> collapsed_;
    std::vector<sim::form::FormRow> rows_;
    std::vector<sim::schema::Issue> issues_;
    std::size_t rowsFor_ = static_cast<std::size_t>(-1); // the document version the rows were made from
    int scroll_ = 0;
    std::string problem_;
    int problemTicks_ = 0;
    bool discardWarned_ = false;
    bool closeRequested_ = false;
    Question question_;
    sim::refs::Plan plan_;                    // a rename that waits for its confirmation
    std::function<void(const std::string&)> openInteraction_;
    std::function<std::uint64_t()> quickSeed_; // the seed of the game in play (42 when the game gives none)
    std::optional<sim::QuickCheck> check_;      // the run that is going on
    std::optional<sim::QuickSummary> lastCheck_; // the run before
    std::function<std::vector<std::string>(const std::string&)> tagsOf_;
    sim::schema::DataIndex index_;
    std::unique_ptr<luna::engine::Panel> panel_;
};

} // namespace odysseus::game
