#pragma once

#include "boundary.h"

#include "sim/data_document.h"
#include "sim/schema.h"

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace odysseus::sim::form {

// What a form for a data file is made of (US-191, EDT-02). The Data tab shows a file as a list of entries and, for the chosen entry, a column of rows built from the
// file's schema: a number with its range, a text, a choice, a yes/no, a link to another catalog, a list, a group. This header is the part that needs no screen: it turns
// a document and a schema into rows, and a typed text back into an edit of the document. The Editor draws the rows with Luna widgets and the tests drive them directly.

// One entry of a file's list: an element of one of the lists the file provides a catalog from (a weapon of weapons.json), or the rest of the file.
struct Entry {
    std::string label;   // what the list shows: the name of the weapon, "(file)"
    std::string path;    // "weapons[2]"; empty for the whole file
    std::string group;   // the list it belongs to ("weapons"); empty for the file's own entry
    bool element = false; // an element of a list: it can be copied, renamed, moved and deleted
};

// The entries of a document. A file whose schema provides a catalog from a list at its top (`weapons[].name`) has one entry per element of that list and one entry
// "(file)" for everything else in it; any other file is one entry "(file)".
std::vector<Entry> entriesOf(const OrderedJson& document, const schema::Schema& schema);

// The name of an element of a list as a person calls it: its name, id, label, title or kind; "#3" when it has none.
std::string labelOfElement(const OrderedJson& element, std::size_t index);

enum class RowKind {
    Heading,   // an object (a group of fields); can be folded
    Whole,     // a whole number
    Decimal,   // a number with a fraction
    Text,      // a line of text
    Choice,    // one of a fixed set of words (an enum)
    Reference, // a name of an entry of another catalog (ref)
    Flag,      // true or false
    Words,     // a list of words or numbers, written separated by commas
    ListHeader, // a list of objects: its heading, with Add
    ItemHeader, // one element of a list of objects, or one key of a map of objects: its heading, with Remove and the arrows
    MapHeader, // a map keyed by id: its heading, with Add
    Raw,       // a member the schema does not know, shown as JSON text
};

struct FormRow {
    RowKind kind = RowKind::Text;
    int depth = 0;
    std::string path;                  // where the value stands: "weapons[2].damage"
    std::string label;                 // "damage", or the key of a map entry
    const schema::Node* node = nullptr; // the schema of the value; null for a member the schema does not know
    std::string value;                 // as the field shows it ("5", "iron sword", "true", "a, b")
    std::string error;                 // the validator's words about this value; empty when it is fine
    std::string helpId;                // "data.weapons.damage": the help of the field (schema name, then the path without list positions)
    bool present = true;               // false: the file leaves the field out (the row shows its default); typing a value adds it
    bool required = false;
    bool collapsed = false;            // a heading that is folded
    bool canRemove = false;            // a list element or map key (Remove), or an optional field that is present (reset to default)
    bool canMoveUp = false;
    bool canMoveDown = false;
    bool canAdd = false;               // a list or map heading
    std::vector<std::string> choices;  // Choice rows
    std::string catalog;               // Reference rows
};

// The rows of one entry. `collapsed` holds the paths of the headings that are folded. `issues` are what schema::check said about the whole document (their paths are
// the rows' paths); each row takes the first error that stands at its path.
std::vector<FormRow> buildRows(const OrderedJson& document, const schema::Schema& schema, const std::string& entryPath, const std::set<std::string>& collapsed,
                               const std::vector<schema::Issue>& issues);

// A value written the way a field shows it.
std::string valueText(const OrderedJson& value);

// Turns what was typed into a field into an edit of the document: a whole number is checked against its range, a choice against its words, a flag against true and
// false, a list is split at the commas. Returns false with the reason in `problem` (and changes nothing) when the text does not fit the field.
bool applyText(DataDocument& document, const FormRow& row, const std::string& text, std::string& problem);

// A value for a new entry or a field that is added: the schema's default; or the first choice; or 0 (inside the range), "" and false; an object with its required
// fields; an empty list (with `minItems` entries when it needs them).
OrderedJson defaultValue(const schema::Node& node);

// A name that is not in `taken`: `base`, or `base-2`, `base-3`...
std::string uniqueName(const std::string& base, const std::set<std::string>& taken);

} // namespace odysseus::sim::form
