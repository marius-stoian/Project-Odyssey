#include "sim/building_data.h"

#include "core/text.h"

#include "sim/npc_class.h"
#include "sim/rule_json.h"

#include <algorithm>
#include <format>
#include <set>

namespace odysseus::sim::buildings {

using rules::JsonValue;
using rules::LoadReport;
using rules::parseJson;
using rules::quoteJson;

const char* pieceTypeName(PieceType type) {
    switch (type) {
    case PieceType::Wall: return "wall";
    case PieceType::Floor: return "floor";
    case PieceType::Roof: return "roof";
    case PieceType::Door: return "door";
    case PieceType::Window: return "window";
    case PieceType::Post: return "post";
    case PieceType::Fence: return "fence";
    }
    return "wall";
}

std::optional<PieceType> pieceTypeFromName(std::string_view name) {
    for (const PieceType type : {PieceType::Wall, PieceType::Floor, PieceType::Roof, PieceType::Door, PieceType::Window, PieceType::Post, PieceType::Fence}) {
        if (name == pieceTypeName(type)) return type;
    }
    return std::nullopt;
}

Layer layerOf(PieceType type) {
    if (type == PieceType::Floor) return Layer::Floor;
    if (type == PieceType::Roof) return Layer::Roof;
    return Layer::Wall;
}

bool blocksWalking(PieceType type) { return type == PieceType::Wall || type == PieceType::Post || type == PieceType::Fence; }
bool closesRoom(PieceType type) { return type == PieceType::Wall || type == PieceType::Door || type == PieceType::Window || type == PieceType::Fence; }

const char* interiorModeName(InteriorMode mode) { return mode == InteriorMode::Map ? "map" : "fade"; }
std::optional<InteriorMode> interiorModeFromName(std::string_view name) {
    if (name == "fade") return InteriorMode::Fade;
    if (name == "map") return InteriorMode::Map;
    return std::nullopt;
}

bool isUse(std::string_view word) {
    return std::ranges::any_of(kUses, [&](const char* use) { return word == use; });
}

namespace {

class Parser {
public:
    Parser(const std::string& name, LoadReport& report) : name_(name), report_(report) {}

    void error(int line, const std::string& message) { report_.errors.push_back({name_, line, message}); }
    std::size_t errors() const { return report_.errors.size(); }

    // A whole number field in [lo, hi]; `fallback` when absent.
    int number(const JsonValue& object, const char* key, int lo, int hi, int fallback) {
        const JsonValue* value = object.find(key);
        if (value == nullptr) return fallback;
        const auto milli = value->isNumber() ? rules::parseMilli(value->text) : std::nullopt;
        if (!milli || *milli % 1000 != 0 || *milli / 1000 < lo || *milli / 1000 > hi) {
            error(value->line, std::format("{} must be a whole number from {} to {}", key, lo, hi));
            return fallback;
        }
        return static_cast<int>(*milli / 1000);
    }
    // Seconds with up to three decimals, kept as thousandths.
    int seconds(const JsonValue& object, const char* key, int fallback) {
        const JsonValue* value = object.find(key);
        if (value == nullptr) return fallback;
        const auto milli = value->isNumber() ? rules::parseMilli(value->text) : std::nullopt;
        if (!milli || *milli < 0 || *milli > 36000000) {
            error(value->line, std::format("{} must be a number of seconds, 0 or more", key));
            return fallback;
        }
        return static_cast<int>(*milli);
    }
    std::string text(const JsonValue& object, const char* key, bool required) {
        const JsonValue* value = object.find(key);
        if (value == nullptr || !value->isString() || value->text.empty()) {
            if (required || value != nullptr) error(value != nullptr ? value->line : object.line, std::format("{} must be a name in quotes", key));
            return {};
        }
        return value->text;
    }
    int colour(const JsonValue& object, const char* key, int fallback) {
        const JsonValue* value = object.find(key);
        if (value == nullptr) return fallback;
        const auto parsed = value->isString() ? rules::parseColour(value->text) : std::nullopt;
        if (!parsed) {
            error(value->line, std::format("{} must be a colour like \"#8b6b3e\"", key));
            return fallback;
        }
        return *parsed;
    }
    ItemCounts counts(const JsonValue& object, const char* key, const std::set<std::string>& knownItems) {
        ItemCounts out;
        const JsonValue* value = object.find(key);
        if (value == nullptr) return out;
        if (!value->isObject()) {
            error(value->line, std::format("{} must be {{\"item\": count, ...}}", key));
            return out;
        }
        for (std::size_t i = 0; i < value->keys.size(); ++i) {
            const JsonValue& count = value->items[i];
            const auto milli = count.isNumber() ? rules::parseMilli(count.text) : std::nullopt;
            if (!milli || *milli % 1000 != 0 || *milli < 1000 || *milli > 1000000) {
                error(count.line, std::format("{}.{} must be a whole number from 1 to 1000", key, value->keys[i]));
            } else if (!knownItems.empty() && knownItems.count(value->keys[i]) == 0) {
                error(value->keyLines[i], std::format("{}.{}: no item of that name (items: hero/items.json)", key, value->keys[i]));
            } else {
                out[value->keys[i]] = static_cast<int>(*milli / 1000);
            }
        }
        return out;
    }
    void unknownFields(const JsonValue& object, const std::set<std::string>& known, const std::string& knownText) {
        for (std::size_t i = 0; i < object.keys.size(); ++i) {
            if (known.count(object.keys[i]) == 0) error(object.keyLines[i], std::format("unknown field \"{}\" (known: {})", object.keys[i], knownText));
        }
    }

private:
    const std::string& name_;
    LoadReport& report_;
};

std::optional<PieceDef> parsePiece(Parser& p, const JsonValue& entry, const std::map<std::string, MaterialFire>& materials, const std::set<std::string>& knownItems) {
    const std::size_t before = p.errors();
    if (!entry.isObject()) {
        p.error(entry.line, "a piece must be a {...} object");
        return std::nullopt;
    }
    p.unknownFields(entry, {"id", "label", "type", "material", "hp", "buildSeconds", "cost", "colour", "height"}, "id, label, type, material, hp, buildSeconds, cost, colour, height");
    PieceDef piece;
    piece.id = p.text(entry, "id", true);
    piece.label = p.text(entry, "label", false);
    if (piece.label.empty()) piece.label = piece.id;
    if (const JsonValue* type = entry.find("type"); type != nullptr && type->isString()) {
        if (const auto found = pieceTypeFromName(type->text)) piece.type = *found;
        else p.error(type->line, "type must be one of: wall, floor, roof, door, window, post, fence");
    } else {
        p.error(type != nullptr ? type->line : entry.line, "type must be one of: wall, floor, roof, door, window, post, fence");
    }
    piece.material = p.text(entry, "material", true);
    if (!piece.material.empty() && materials.count(piece.material) == 0) {
        const JsonValue* value = entry.find("material");
        p.error(value != nullptr ? value->line : entry.line, std::format("material \"{}\" is not in the \"materials\" table of pieces.json", piece.material));
    }
    piece.hp = p.number(entry, "hp", 1, 100000, 100);
    piece.buildMilli = p.seconds(entry, "buildSeconds", 3000);
    piece.cost = p.counts(entry, "cost", knownItems);
    piece.colour = p.colour(entry, "colour", piece.colour);
    piece.heightMilli = static_cast<int>(p.seconds(entry, "height", 2000));
    if (p.errors() != before) return std::nullopt;
    return piece;
}

const std::vector<std::string>& seasonWords() {
    static const std::vector<std::string> words(std::begin(kSeasonWords), std::end(kSeasonWords));
    return words;
}

} // namespace


namespace {

std::optional<KindDef> parseKindValue(const BuildingData& data, const JsonValue& entry, const std::string& name, LoadReport& report, bool prefab,
                                      const std::string& expectedId, const std::set<std::string>& knownItems) {
    Parser p(name, report);
    const std::size_t before = p.errors();
    if (!entry.isObject()) {
        p.error(entry.line, "a kind must be a {...} object");
        return std::nullopt;
    }
    p.unknownFields(entry, {"id", "label", "footprint", "layout", "cost", "buildSeconds", "interior", "interiorLevel", "uses", "light", "owner", "wear", "buildable", "known", "colour", "note", "capacity"},
                    "id, label, footprint, layout, cost, buildSeconds, interior, interiorLevel, uses, light, owner, wear, buildable, known, colour, note, capacity");
    KindDef kind;
    kind.prefab = prefab;
    kind.file = name;
    kind.id = p.text(entry, "id", true);
    if (!expectedId.empty() && !kind.id.empty() && kind.id != expectedId) {
        const JsonValue* id = entry.find("id");
        p.error(id != nullptr ? id->line : entry.line, std::format("id \"{}\" must match the file name \"{}\"", kind.id, expectedId));
    }
    kind.label = p.text(entry, "label", false);
    if (kind.label.empty()) kind.label = kind.id;
    if (const JsonValue* footprint = entry.find("footprint")) {
        const auto cell = [&](const JsonValue& v) -> int {
            const auto milli = v.isNumber() ? rules::parseMilli(v.text) : std::nullopt;
            return milli && *milli % 1000 == 0 && *milli >= 1000 && *milli <= 32000 ? static_cast<int>(*milli / 1000) : 0;
        };
        if (!footprint->isArray() || footprint->items.size() != 2 || cell(footprint->items[0]) == 0 || cell(footprint->items[1]) == 0) {
            p.error(footprint->line, "footprint must be [width, height], whole numbers from 1 to 32");
        } else {
            kind.width = cell(footprint->items[0]);
            kind.height = cell(footprint->items[1]);
        }
    } else {
        p.error(entry.line, "footprint is missing: [width, height]");
    }
    if (const JsonValue* layout = entry.find("layout"); layout != nullptr && layout->isArray() && !layout->items.empty()) {
        std::set<std::pair<int, int>> used[kLayerCount];
        for (const JsonValue& item : layout->items) {
            if (!item.isObject()) {
                p.error(item.line, "a layout entry must be {\"piece\": \"id\", \"x\": 0, \"y\": 0}");
                continue;
            }
            p.unknownFields(item, {"piece", "x", "y"}, "piece, x, y");
            LayoutPiece lp;
            lp.piece = p.text(item, "piece", true);
            lp.x = p.number(item, "x", 0, 31, 0);
            lp.y = p.number(item, "y", 0, 31, 0);
            const PieceDef* def = lp.piece.empty() ? nullptr : data.piece(lp.piece);
            if (!lp.piece.empty() && def == nullptr) {
                p.error(item.find("piece")->line, std::format("no piece named \"{}\" (pieces.json)", lp.piece));
            } else if (def != nullptr) {
                if (lp.x >= kind.width || lp.y >= kind.height) {
                    p.error(item.line, std::format("piece \"{}\" at {}, {} lies outside the {} x {} footprint", lp.piece, lp.x, lp.y, kind.width, kind.height));
                } else if (!used[static_cast<int>(layerOf(def->type))].insert({lp.x, lp.y}).second) {
                    p.error(item.line, std::format("two pieces of the same layer at {}, {}", lp.x, lp.y));
                }
                kind.layout.push_back(lp);
            }
        }
    } else {
        p.error(layout != nullptr ? layout->line : entry.line, "layout must be a list of {\"piece\", \"x\", \"y\"} with at least one piece");
    }
    kind.cost = p.counts(entry, "cost", knownItems);
    if (entry.find("cost") == nullptr) kind.cost = layoutCost(data, kind.layout);
    kind.buildMilli = entry.find("buildSeconds") != nullptr ? p.seconds(entry, "buildSeconds", 0) : layoutBuildMilli(data, kind.layout);
    if (const JsonValue* interior = entry.find("interior")) {
        const auto mode = interior->isString() ? interiorModeFromName(interior->text) : std::nullopt;
        if (!mode) p.error(interior->line, "interior must be \"fade\" or \"map\"");
        else kind.interior = *mode;
    }
    kind.interiorLevel = p.text(entry, "interiorLevel", false);
    if (kind.interior == InteriorMode::Map && kind.interiorLevel.empty()) {
        p.error(entry.find("interior") != nullptr ? entry.find("interior")->line : entry.line, "interior \"map\" needs interiorLevel: the name of the level file the door leads into");
    }
    if (const JsonValue* uses = entry.find("uses")) {
        if (!uses->isArray()) {
            p.error(uses->line, "uses must be a list: shelter, sleep, store, work");
        } else {
            for (const JsonValue& use : uses->items) {
                if (!use.isString() || !isUse(use.text)) p.error(use.line, "a use must be one of: shelter, sleep, store, work");
                else if (std::ranges::find(kind.uses, use.text) == kind.uses.end()) kind.uses.push_back(use.text);
            }
        }
    }
    kind.light = p.text(entry, "light", false);
    if (const JsonValue* owner = entry.find("owner")) {
        if (!owner->isString() || (owner->text != "none" && owner->text != "clan" && owner->text != "person")) p.error(owner->line, "owner must be \"none\", \"clan\" or \"person\"");
        else kind.owner = owner->text;
    }
    if (const JsonValue* wear = entry.find("wear")) {
        if (!wear->isObject()) {
            p.error(wear->line, "wear must be {\"winter\": 2, ...}: hp each piece loses when that season ends");
        } else {
            for (std::size_t i = 0; i < wear->keys.size(); ++i) {
                const JsonValue& amount = wear->items[i];
                const auto milli = amount.isNumber() ? rules::parseMilli(amount.text) : std::nullopt;
                if (std::ranges::find(seasonWords(), wear->keys[i]) == seasonWords().end()) p.error(wear->keyLines[i], "wear names a season: spring, summer, autumn or winter");
                else if (!milli || *milli % 1000 != 0 || *milli < 0 || *milli > 1000000) p.error(amount.line, "wear amounts are whole numbers, 0 or more");
                else kind.wear[wear->keys[i]] = static_cast<int>(*milli / 1000);
            }
        }
    }
    for (const char* flag : {"buildable", "known"}) {
        if (const JsonValue* value = entry.find(flag)) {
            if (value->kind != JsonValue::Kind::Bool) p.error(value->line, std::format("{} must be true or false", flag));
            else (std::string_view(flag) == "buildable" ? kind.buildable : kind.known) = value->boolean;
        }
    }
    kind.capacity = p.number(entry, "capacity", 0, 1000, 0); // meals of the clan's store a storage kind keeps at half the spoilage (US-257)
    kind.colour = p.colour(entry, "colour", kind.colour);
    kind.note = p.text(entry, "note", false);
    if (p.errors() != before) return std::nullopt;
    return kind;
}

void parsePieces(std::vector<PieceDef>& pieces, std::map<std::string, MaterialFire>& materials, int& maxRoomCells, std::string_view text, const std::string& name,
                 LoadReport& report, const std::set<std::string>& knownItems) {
    const rules::JsonParseResult parsed = parseJson(text);
    if (!parsed.value) {
        report.errors.push_back({name, parsed.errorLine, parsed.error});
        return;
    }
    if (const std::vector<rules::Diagnostic> mistakes = rules::schemaDiagnostics(name, text); !mistakes.empty()) { // the schema (US-190)
        report.errors.insert(report.errors.end(), mistakes.begin(), mistakes.end());
        return;
    }
    Parser p(name, report);
    const JsonValue& root = *parsed.value;
    if (!root.isObject()) {
        p.error(root.line, "the file must hold one {...} object with \"materials\" and \"pieces\"");
        return;
    }
    p.unknownFields(root, {"materials", "maxRoomCells", "pieces"}, "materials, maxRoomCells, pieces");
    if (const JsonValue* table = root.find("materials")) {
        if (!table->isObject()) {
            p.error(table->line, "materials must be {\"wood\": {\"flammability\": 60, \"burnSeconds\": 40}, ...}");
        } else {
            for (std::size_t i = 0; i < table->keys.size(); ++i) {
                const JsonValue& entry = table->items[i];
                if (!entry.isObject()) {
                    p.error(entry.line, "a material must be {\"flammability\": percent, \"burnSeconds\": seconds}");
                    continue;
                }
                p.unknownFields(entry, {"flammability", "burnSeconds"}, "flammability, burnSeconds");
                MaterialFire fire;
                fire.flammability = p.number(entry, "flammability", 0, 100, 0);
                fire.burnSeconds = p.number(entry, "burnSeconds", 0, 100000, 0);
                materials[table->keys[i]] = fire;
            }
        }
    } else {
        p.error(root.line, "\"materials\" is missing");
    }
    maxRoomCells = p.number(root, "maxRoomCells", 1, 1024, maxRoomCells);
    const JsonValue* list = root.find("pieces");
    if (list == nullptr || !list->isArray()) {
        p.error(list != nullptr ? list->line : root.line, "\"pieces\" must be a list of pieces");
        return;
    }
    std::set<std::string> seen;
    for (const JsonValue& entry : list->items) {
        if (auto piece = parsePiece(p, entry, materials, knownItems)) {
            if (!seen.insert(piece->id).second) p.error(entry.line, std::format("two pieces are called \"{}\"", piece->id));
            else pieces.push_back(std::move(*piece));
        }
    }
}

// Reads kinds.json text into `data`; used by the loader and by parse() for tests.
void parseKinds(BuildingData& data, std::vector<KindDef>& kinds, std::string_view text, LoadReport& report, const std::set<std::string>& knownItems) {
    const std::string name = "buildings/kinds.json";
    const rules::JsonParseResult parsed = parseJson(text);
    if (!parsed.value) {
        report.errors.push_back({name, parsed.errorLine, parsed.error});
        return;
    }
    if (const std::vector<rules::Diagnostic> mistakes = rules::schemaDiagnostics(name, text); !mistakes.empty()) { // the schema (US-190)
        report.errors.insert(report.errors.end(), mistakes.begin(), mistakes.end());
        return;
    }
    const JsonValue* list = parsed.value->isObject() ? parsed.value->find("kinds") : nullptr;
    if (list == nullptr || !list->isArray()) {
        report.errors.push_back({name, parsed.value->line, "the file must be {\"kinds\": [ ... ]}"});
        return;
    }
    std::set<std::string> seen;
    for (const JsonValue& entry : list->items) {
        if (auto kind = parseKindValue(data, entry, name, report, false, {}, knownItems)) {
            if (!seen.insert(kind->id).second) report.errors.push_back({name, entry.line, std::format("two kinds are called \"{}\"", kind->id)});
            else kinds.push_back(std::move(*kind));
        }
    }
}

} // namespace

std::optional<KindDef> BuildingData::parseKind(std::string_view text, const std::string& name, LoadReport& report, bool prefab, const std::string& expectedId) const {
    const rules::JsonParseResult parsed = parseJson(text);
    if (!parsed.value) {
        report.errors.push_back({name, parsed.errorLine, parsed.error});
        return std::nullopt;
    }
    if (prefab) { // a prefab file has its own schema (US-190); a kind inside kinds.json is checked with the whole file
        if (const std::vector<rules::Diagnostic> mistakes = rules::schemaDiagnostics(name, text); !mistakes.empty()) {
            report.errors.insert(report.errors.end(), mistakes.begin(), mistakes.end());
            return std::nullopt;
        }
    }
    return parseKindValue(*this, *parsed.value, name, report, prefab, expectedId, {});
}

BuildingData BuildingData::parse(std::string_view piecesText, std::string_view kindsText, LoadReport& report) {
    BuildingData data;
    parsePieces(data.pieces_, data.materials_, data.maxRoomCells_, piecesText, "buildings/pieces.json", report, {});
    std::sort(data.pieces_.begin(), data.pieces_.end(), [](const PieceDef& a, const PieceDef& b) { return a.id < b.id; });
    std::vector<KindDef> kinds;
    parseKinds(data, kinds, kindsText, report, {});
    data.kinds_ = std::move(kinds);
    report.loaded += static_cast<int>(data.pieces_.size() + data.kinds_.size());
    return data;
}

BuildingData BuildingData::load(const std::filesystem::path& folder, LoadReport& report, const std::set<std::string>& knownItems) {
    BuildingData data;
    std::error_code ec;
    if (!std::filesystem::is_directory(folder, ec)) return data;
    const std::optional<std::string> piecesText = core::readTextFile(folder / "pieces.json");
    ++report.filesRead;
    if (!piecesText) {
        report.errors.push_back({"buildings/pieces.json", 0, "the file cannot be read"});
        return data;
    }
    parsePieces(data.pieces_, data.materials_, data.maxRoomCells_, *piecesText, "buildings/pieces.json", report, knownItems);
    std::sort(data.pieces_.begin(), data.pieces_.end(), [](const PieceDef& a, const PieceDef& b) { return a.id < b.id; });
    const std::optional<std::string> kindsText = core::readTextFile(folder / "kinds.json");
    ++report.filesRead;
    std::vector<KindDef> kinds;
    if (!kindsText) report.errors.push_back({"buildings/kinds.json", 0, "the file cannot be read"});
    else parseKinds(data, kinds, *kindsText, report, knownItems);
    std::set<std::string> seen;
    for (const KindDef& kind : kinds) seen.insert(kind.id);
    data.kinds_ = std::move(kinds);
    std::vector<std::filesystem::path> prefabs;
    for (const auto& entry : std::filesystem::directory_iterator(folder / "prefabs", ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") prefabs.push_back(entry.path());
    }
    std::sort(prefabs.begin(), prefabs.end());
    for (const std::filesystem::path& file : prefabs) {
        ++report.filesRead;
        const std::string name = "buildings/prefabs/" + file.filename().generic_string();
        const std::optional<std::string> text = core::readTextFile(file);
        if (!text) {
            report.errors.push_back({name, 0, "the file cannot be read"});
            continue;
        }
        const rules::JsonParseResult parsed = parseJson(*text);
        if (!parsed.value) {
            report.errors.push_back({name, parsed.errorLine, parsed.error});
            continue;
        }
        if (const std::vector<rules::Diagnostic> mistakes = rules::schemaDiagnostics(name, *text); !mistakes.empty()) { // the schema (US-190)
            report.errors.insert(report.errors.end(), mistakes.begin(), mistakes.end());
            continue;
        }
        if (auto kind = parseKindValue(data, *parsed.value, name, report, true, file.stem().string(), knownItems)) {
            if (!seen.insert(kind->id).second) report.errors.push_back({name, parsed.value->line, std::format("a kind called \"{}\" exists already (kinds.json or another prefab)", kind->id)});
            else data.kinds_.push_back(std::move(*kind));
        }
    }
    report.loaded += static_cast<int>(data.pieces_.size() + data.kinds_.size());
    return data;
}

const PieceDef* BuildingData::piece(std::string_view id) const {
    const auto found = std::lower_bound(pieces_.begin(), pieces_.end(), id, [](const PieceDef& p, std::string_view key) { return p.id < key; });
    return found != pieces_.end() && found->id == id ? &*found : nullptr;
}

const KindDef* BuildingData::kind(std::string_view id) const {
    for (const KindDef& k : kinds_) {
        if (k.id == id) return &k;
    }
    return nullptr;
}

const MaterialFire* BuildingData::material(std::string_view name) const {
    const auto found = materials_.find(std::string(name));
    return found == materials_.end() ? nullptr : &found->second;
}

void BuildingData::addKind(KindDef kind) {
    for (KindDef& existing : kinds_) {
        if (existing.id == kind.id) {
            existing = std::move(kind);
            return;
        }
    }
    kinds_.push_back(std::move(kind));
}

Cell turnedSize(int width, int height, int turns) { return turns % 2 == 0 ? Cell{width, height} : Cell{height, width}; }

Cell turnedOffset(int x, int y, int width, int height, int turns) {
    switch (((turns % 4) + 4) % 4) {
    case 1: return {height - 1 - y, x};
    case 2: return {width - 1 - x, height - 1 - y};
    case 3: return {y, width - 1 - x};
    default: return {x, y};
    }
}

ItemCounts layoutCost(const BuildingData& data, const std::vector<LayoutPiece>& layout) {
    ItemCounts total;
    for (const LayoutPiece& lp : layout) {
        if (const PieceDef* piece = data.piece(lp.piece)) {
            for (const auto& [item, count] : piece->cost) total[item] += count;
        }
    }
    return total;
}

int layoutBuildMilli(const BuildingData& data, const std::vector<LayoutPiece>& layout) {
    long long total = 0;
    for (const LayoutPiece& lp : layout) {
        if (const PieceDef* piece = data.piece(lp.piece)) total += piece->buildMilli;
    }
    return static_cast<int>(std::min<long long>(total, 36000000));
}

std::string toJson(const KindDef& k) {
    const auto strings = [](const std::vector<std::string>& values) {
        std::string out = "[";
        for (std::size_t i = 0; i < values.size(); ++i) out += (i != 0 ? ", " : "") + quoteJson(values[i]);
        return out + "]";
    };
    std::string out = "{\n";
    out += std::format("  \"id\": {},\n  \"label\": {},\n  \"footprint\": [{}, {}],\n", quoteJson(k.id), quoteJson(k.label), k.width, k.height);
    out += "  \"layout\": [\n";
    for (std::size_t i = 0; i < k.layout.size(); ++i) {
        out += std::format("    {{ \"piece\": {}, \"x\": {}, \"y\": {} }}{}\n", quoteJson(k.layout[i].piece), k.layout[i].x, k.layout[i].y, i + 1 < k.layout.size() ? "," : "");
    }
    out += "  ],\n";
    std::string cost = "{";
    bool first = true;
    for (const auto& [item, count] : k.cost) {
        cost += std::format("{} {}: {}", first ? "" : ",", quoteJson(item), count);
        first = false;
    }
    out += std::format("  \"cost\": {} }},\n  \"buildSeconds\": {},\n  \"interior\": {}", cost, rules::formatMilli(k.buildMilli), quoteJson(interiorModeName(k.interior)));
    if (!k.interiorLevel.empty()) out += std::format(",\n  \"interiorLevel\": {}", quoteJson(k.interiorLevel));
    out += std::format(",\n  \"uses\": {},\n  \"owner\": {}", strings(k.uses), quoteJson(k.owner));
    if (!k.light.empty()) out += std::format(",\n  \"light\": {}", quoteJson(k.light));
    if (!k.wear.empty()) {
        out += ",\n  \"wear\": {";
        bool firstWear = true;
        for (const char* season : kSeasonWords) {
            if (const auto found = k.wear.find(season); found != k.wear.end()) {
                out += std::format("{} \"{}\": {}", firstWear ? "" : ",", season, found->second);
                firstWear = false;
            }
        }
        out += " }";
    }
    out += std::format(",\n  \"buildable\": {},\n  \"known\": {},\n  \"colour\": {}", k.buildable ? "true" : "false", k.known ? "true" : "false", quoteJson(rules::formatColour(k.colour)));
    if (k.capacity > 0) out += std::format(",\n  \"capacity\": {}", k.capacity);
    if (!k.note.empty()) out += std::format(",\n  \"note\": {}", quoteJson(k.note));
    out += "\n}\n";
    return out;
}

} // namespace odysseus::sim::buildings
