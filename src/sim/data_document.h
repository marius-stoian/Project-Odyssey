#pragma once

#include "boundary.h"

#include "sim/json_patch.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace odysseus::sim {

// Where a value stands in a document: "weapons[2].damage" is member "weapons", its third element, its member "damage". A member name with a dot, a bracket or a
// quote in it is written as ["npc.sword"], so `fields["npc.sword"].purpose` is unambiguous. The schema validator writes its issue paths the same way.
struct PathSegment {
    bool index = false;     // an element of a list (position) rather than a member of an object (key)
    std::size_t position = 0;
    std::string key;
};
using DocPath = std::vector<PathSegment>;

// A path from its text. Nothing when the text is not a path.
std::optional<DocPath> parsePath(const std::string& text);
std::string formatPath(const DocPath& path);

// One data file open for editing (US-191): the parsed document, the edits made to it, and the way back (EDT-02). Edits go through the functions below, each one a
// step of undo; `save` writes the file by patching the text it was read from, so only what changed changes (json_patch.h). It knows nothing of screens: the Data
// tab and the tests use it the same way.
class DataDocument {
public:
    // Reads a file. Nothing, with the reason in `problem`, when it cannot be read or is not JSON.
    static std::optional<DataDocument> open(const std::filesystem::path& file, std::string& problem);
    // The same from text already in memory (a test, or a file that does not exist yet).
    static std::optional<DataDocument> fromText(std::string text, std::filesystem::path file, std::string& problem);

    const std::filesystem::path& file() const { return file_; }
    const OrderedJson& root() const { return document_; }
    // The value at a path, or nullptr.
    const OrderedJson* find(const std::string& path) const;

    // Edits. Each returns false and says why in `problem` (and changes nothing) when it cannot be done.
    bool set(const std::string& path, OrderedJson value, std::string& problem);                                  // an existing value, or a new member of an object
    bool insertElement(const std::string& listPath, std::size_t index, OrderedJson value, std::string& problem); // index == size appends
    bool removeElement(const std::string& listPath, std::size_t index, std::string& problem);
    bool moveElement(const std::string& listPath, std::size_t from, std::size_t to, std::string& problem);      // the element ends up at position `to`
    bool addMember(const std::string& objectPath, const std::string& key, OrderedJson value, std::string& problem); // at the end; the key must be new
    bool removeMember(const std::string& objectPath, const std::string& key, std::string& problem);
    bool renameMember(const std::string& objectPath, const std::string& from, const std::string& to, std::string& problem); // keeps its place; `to` must be new
    // Replaces the whole document (a rename that touched many places, a form that rebuilt it): one step of undo.
    void replaceRoot(OrderedJson document, const std::string& what);

    // The way back. Every edit since the file was opened can be undone, up to kUndoSteps.
    static constexpr std::size_t kUndoSteps = 200;
    bool canUndo() const { return !undo_.empty(); }
    bool canRedo() const { return !redo_.empty(); }
    bool undo();
    bool redo();
    const std::string& lastEdit() const { return lastEdit_; } // "set weapons[2].damage", for the status line

    // The document differs from what is on disk.
    bool dirty() const { return !sameJson(document_, saved_); }
    // Counts edits, undo and redo included: a view that remembers it knows when to build itself again.
    std::size_t version() const { return version_; }

    // The text of the file as it would be written now: the text it was read from, patched.
    std::string text() const { return patchJsonText(original_, saved_, document_); }
    // Writes the file (a temporary file renamed over it, `backups` older copies kept) and takes it as the new original. Returns what went wrong, nothing when it worked.
    std::optional<std::string> save(int backups = 3);
    // The file was changed outside and has been read again: the document, the text and the way back start from the new file.
    bool reloadFromDisk(std::string& problem);

private:
    void commit(OrderedJson next, std::string what);

    std::filesystem::path file_;
    std::string original_;    // the text on disk (or as last saved)
    OrderedJson saved_;       // its document
    OrderedJson document_;    // the document being edited
    std::vector<OrderedJson> undo_;
    std::vector<OrderedJson> redo_;
    std::string lastEdit_;
    std::size_t version_ = 0;
};

} // namespace odysseus::sim
