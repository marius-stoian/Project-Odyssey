#pragma once

#include "boundary.h"

#include "game/animals.h"
#include "game/art.h"
#include "game/effect_art.h"
#include "game/editor_history.h"
#include "game/level.h"
#include "game/pickups.h"
#include "game/plants.h"
#include "luna/engine/camera.h"
#include "luna/engine/input.h"
#include "luna/engine/renderer.h"
#include "luna/engine/tile_map.h"
#include "luna/engine/ui.h"

#include "game/npc_class_book.h"
#include "game/npc_marker.h"

#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace odysseus::game {

// The pictures the editor draws with (created by the game at start).
struct EditorTextures {
    luna::engine::Texture tiles;
    luna::engine::Texture heroSheet;
    luna::engine::Texture characters;
    luna::engine::Texture props;
    luna::engine::Texture ui;
    const ArtSet* art = nullptr;
    const WeaponArt* weapons = nullptr; // the weapon icons, for pickups and the weapon palette (US-134)
    const PlantArt* plants = nullptr;   // the plant pictures, for the plant palette and placed plants (US-136)
    const AnimalArt* animals = nullptr; // the animal pictures, for the character palette and placed animals (US-137)
    const EffectArt* effects = nullptr; // the first picture of each effect, for the effect palette and placed effects (US-138)
};

// What a left click on the map does.
enum class EditorTool { Brush, Rectangle, Fill, Eraser, Place, Select, Weapon, Plant, Effect, Light };

const char* toolName(EditorTool tool);

// Editor mode (M2c): the world stands still while the owner looks at and changes the level.
// It edits the game's Level directly; the game rebuilds its play state from it on return.
class Editor {
public:
    Editor(Level& level, const Definitions& definitions, std::filesystem::path levelFile, int viewWidth, int viewHeight);

    // Gives the editor its pictures and builds its panels (the tile palette shows the tiles).
    void setTextures(const EditorTextures& textures);

    // Opens the editor looking at (x, y) in world pixels (where the game camera was).
    void enter(double centreX, double centreY);
    // One tick: panels, shortcuts, panning, and the chosen tool on the map.
    void update(const luna::engine::Intents& intents);
    void render(luna::engine::Renderer& renderer, double alpha) const;

    Level& level() { return level_; }
    const Level& level() const { return level_; }
    // Call after changing the level from outside the editor (the map shown is rebuilt).
    void levelChanged();
    const luna::engine::Camera& camera() const { return camera_; }
    double centreX() const { return centreX_; }
    double centreY() const { return centreY_; }

    EditorTool tool() const { return tool_; }
    void setTool(EditorTool tool) { tool_ = tool; }
    int tile() const { return tile_; }
    void setTile(int tile) { tile_ = tile; }
    bool gridShown() const { return grid_; }
    // The character kind the Place tool puts down (an index into Definitions::characters).
    int kind() const { return kind_; }
    // The character palette shows 12 kinds to a page (the first page is the twelve characters, then the animals).
    static constexpr int kKindsPerPage = 12;
    int kindPage() const { return kindPage_; }
    void setKind(int kind) { kind_ = kind; }
    // Weapon pickups (US-134): the weapons the palette offers, in order (names that are in weapons.json
    // or built in); the Weapon tool places the chosen one.
    void setWeaponPalette(std::vector<std::string> names);
    const std::vector<std::string>& weaponPalette() const { return weaponNames_; }
    int weapon() const { return weapon_; }
    void setWeapon(int weapon) { weapon_ = weapon; }
    // The pickup whose icon covers a screen point, if any.
    std::optional<int> pickupAt(int screenX, int screenY) const;
    // Plants (US-136): the Plant tool places the chosen plant of Definitions::plants at the clicked cell (its feet
    // in the middle of the cell's bottom edge); the palette shows them by picture, 36 to a page.
    // Looping effects (US-138): the Effect tool places the chosen effect of Definitions::loopingEffects where the
    // pointer clicks; in the game it plays in a loop. The Editor shows its first picture.
    int effect() const { return effect_; }
    void setEffect(int effect) { effect_ = effect; }
    std::optional<int> effectAt(int screenX, int screenY) const;
    // Lights (US-247): the Light tool places the chosen kind of Definitions::lightKinds where the pointer clicks; it shines in the game after dark.
    int light() const { return light_; }
    void setLight(int light) { light_ = light; }
    std::optional<int> lightAt(int screenX, int screenY) const;
    // Time-of-day preview (US-247): the level is drawn lit as at this hour (0 to 24). A view only: never saved, never an Undo step. Off by default.
    // The game gives the editor the function that lights a view at an hour; without it there is no preview.
    using LightPreview = std::function<luna::engine::LightFrame(double hour, const luna::engine::Rect& view)>;
    void setLightPreview(LightPreview preview) { lightPreview_ = std::move(preview); }
    std::optional<double> previewHour() const { return previewHour_; }
    luna::engine::Rect timeSliderRect() const; // where the time-of-day slider is, while the preview is on
    void setPreviewHour(std::optional<double> hour);
    int plant() const { return plant_; }
    void setPlant(int plant) { plant_ = plant; }
    int plantPage() const { return plantPage_; }
    static constexpr int kPlantsPerPage = 36;
    // The placed plant whose picture covers a screen point, if any.
    std::optional<int> plantAt(int screenX, int screenY) const;
    // The placed character selected with the Select tool (its id), if any.
    std::optional<int> selected() const { return selected_; }
    void select(std::optional<int> id);
    // Changes one property of the selected character, as one step of Undo.
    void setSelectedName(const std::string& name);
    void setSelectedHp(int hp);
    void setSelectedSwordDamage(int damage);
    // The character whose picture covers a screen point (the one drawn last, on top), if any.
    std::optional<int> characterAt(int screenX, int screenY) const;

    // Level settings (US-126), each one step of Undo.
    void setLevelName(const std::string& name);
    void setLevelSize(int width, int height);  // keeps painted cells; drops characters outside
    void setDefaultGround(int tile);
    void moveHeroStart(PixelPoint feet);
    // The region's economy (US-280, D-54 Q1-Q2): the Economy panel (the Economy button of the Level panel) sets which items are money here, the market's base
    // prices and the goods the region delivers to its traders, each as text "item=number item=number". Each is one step of Undo; a mistake is said in the status
    // line and changes nothing (the function returns false).
    bool economyShown() const { return economyShown_; }
    void showEconomy(bool shown);
    bool setEconomyCurrencies(const std::string& text);
    bool setEconomyPrices(const std::string& text);
    bool setEconomyResources(const std::string& text);
    // The named places of the level (US-290): "market=20,10 grove=30,12/forage/shelter" (tile numbers, tags after slashes), the Places line of the Economy panel; one step of Undo.
    bool setPlaces(const std::string& text);
    // NPC Classes (US-260): the Class button opens a panel with the list of classes and a form for the chosen one. The book is the game's
    // catalog; every change is written to its file at once (Save), so the Editor never holds a class the disk does not.
    void setNpcClasses(NpcClassBook* book) { classBook_ = book; classesStale_ = true; }
    void classesChanged() { classesStale_ = true; } // the catalog was read again (F5)
    bool classesShown() const { return classesShown_; }
    void showClasses(bool shown);
    void newClass();                          // a blank draft; Save writes it
    void selectClass(const std::string& id);  // the draft is a copy of this class
    sim::rules::NpcClass& classDraft() { return classDraft_; }
    bool classDraftIsNew() const { return classNew_; }
    bool saveClass();                         // checks and writes the draft; says why when it cannot
    bool deleteClass();                       // refused, naming the NPCs, while placed NPCs use the class
    // The Kinds tab (US-269): the second tab of the Class panel edits the defaults of one kind (assets/data/npcs/<kind>.json) with the same form as the NPC panel, so a
    // whole kind changes at once. Save writes the file at once; every placed NPC without its own value then follows it, also in play.
    bool kindsTab() const { return kindsTab_; }
    void showKinds(bool shown);                   // opens the Class panel on the Kinds tab (or closes the panel)
    std::vector<std::string> kindNames() const;   // the kinds the owner can edit: every character and animal except the hero
    void selectKind(const std::string& name);     // the draft is a copy of the kind's file (a blank one when it has none)
    sim::rules::NpcKind& kindDraft() { return kindDraft_; }
    void toggleKindClass(const std::string& id);  // the draft has the class or not (no class at all: the field is left out)
    bool saveKind();                              // checks and writes the draft; says why when it cannot
    // The markers (US-269): under every placed NPC that has a class, a ring in its class colour with its icon. Editor only.
    int markerCount() const;
    std::vector<int> markerTextureIds() const; // the texture numbers of the marker pictures made so far (tests find the marker draws by them)
    // The NPC panel (US-268): a placed character with a kind file is an NPC. Everything below changes the selected one, each as one step of Undo, and keeps only
    // the differences from its classes and its kind (a value equal to what it would inherit is not kept). An empty value means "the default".
    void setActionIds(std::vector<std::string> ids) {
        actionIds_ = std::move(ids);
        propertiesStale_ = true;
    }
    bool selectedIsNpc() const;
    void setSelectedClasses(std::vector<std::string> classes);
    void toggleSelectedClass(const std::string& id);
    void setSelectedAttitude(const std::string& word);
    void setSelectedFamily(int family);
    void setSelectedDialogue(const std::string& partner, const std::string& file);
    void setSelectedActionDenied(const std::string& id, bool denied);
    void resetSelectedNpc();
    // The Trade section (US-284, D-54 Q6): six text fields, stock, restock (per day), picks (weighted deliveries a day), weights, wants and rare, each "item=number ..." or a
    // list. For the selected NPC they edit its own trade values (the class and the kind add theirs; a layer can add and change but not remove what a lower one gives, so set a
    // stock or a restock to 0 instead); each change is one step of Undo. For a class draft and a kind draft they edit the draft that Save writes. A mistake is said in the
    // status line, changes nothing and returns false.
    bool setSelectedTrade(const std::string& field, const std::string& text);
    bool setClassTrade(const std::string& field, const std::string& text);
    bool setKindTrade(const std::string& field, const std::string& text);
    // The Schedule form (US-290, D-54 Q9): two text lines, Day and Night, each "06:00 work market; 21:00 sleep home" (time, activity, place; the place is a place of the level or
    // home). An empty Night means the day blocks hold at night too. For the selected NPC it edits its own schedule (one step of Undo per line; the schedule of the highest layer
    // that has one wins, so an NPC's own replaces its kind's and its classes'); for a class or kind draft Save writes it. A mistake is said and changes nothing.
    bool setSelectedSchedule(const std::string& field, const std::string& text);
    bool setClassSchedule(const std::string& field, const std::string& text);
    bool setKindSchedule(const std::string& field, const std::string& text);
    // The Does line (US-291, D-54 Q11): interaction ids separated by spaces, "patrol sing". For a class they are its class actions, for a kind or an NPC its custom actions; an NPC
    // that is idle on duty chooses among them (and the events on offer) by the `npc` score of their files. One step of Undo for an NPC; Save for a class or kind.
    bool setSelectedDoes(const std::string& text);
    bool setClassDoes(const std::string& text);
    bool setKindDoes(const std::string& text);
    // The default actions with each partner type (US-293, D-54 Q14): "Defaults with: animal" and the line "Does" under it name the interaction ids the NPC prefers when it meets that kind
    // of partner (a hunter's animals: hunt). The partner types come from assets/data/sim/partner-types.json plus one class:<id> for every NPC class, so a type added to the file shows
    // here. An empty line removes the type. For an NPC one step of Undo; for a class or kind Save writes it.
    bool setSelectedPartnerActions(const std::string& partnerType, const std::string& text);
    bool setClassPartnerActions(const std::string& partnerType, const std::string& text);
    bool setKindPartnerActions(const std::string& partnerType, const std::string& text);
    // The partner types the dialogue row offers, from data: player, animal, environment and one class:<id> for every class.
    std::vector<std::string> partnerTypes() const;
    bool settingsShown() const { return settingsShown_; }
    void showSettings(bool shown);

    // Levels on disk: the file being edited, the other levels next to it, and switching.
    const std::filesystem::path& levelFile() const { return levelFile_; }
    std::vector<std::filesystem::path> levelFiles() const;
    // Opens another level, or starts a new one. With unsaved changes they only ask first
    // (`asking()`); `answer` then saves, discards or cancels.
    void requestOpen(const std::filesystem::path& file);
    void requestNew();
    enum class Answer { Save, Discard, Cancel };
    bool asking() const { return pending_.has_value(); }
    void answer(Answer answer);

    // Every edit goes through here: applied, and remembered for Undo.
    void run(std::unique_ptr<Command> command);
    bool undo();
    bool redo();
    const History& history() const { return history_; }
    // Saves to the level file (safely, with backups). Returns false and says why when it cannot.
    bool save();
    bool unsaved() const { return unsaved_; }
    const std::string& status() const { return status_; }

    // The map cell under a point of the screen (virtual pixels), if it is on the level.
    std::optional<std::pair<int, int>> cellAt(int screenX, int screenY) const;

    static constexpr int kPanPerTick = 8; // pixels per tick with the keys (160 per second)

private:
    void panTo(double x, double y);
    void say(std::string message);
    void buildPanels();
    bool handlePanels(const luna::engine::UiInput& input);
    void useTool(const luna::engine::Pointer& pointer, bool overPanel);
    void paintAt(int x, int y);
    void finishStroke();
    void usePlaceOrSelect(const luna::engine::Pointer& pointer, bool pressed, bool held, bool released);
    void changeCharacters(const std::string& what, std::vector<PlacedCharacter> after, int nextIdAfter);
    PlacedCharacter* find(int id);
    const PlacedPickup* findPickup(int id) const;
    const PlacedPlant* findPlant(int id) const;
    void changePlants(const std::string& what, std::vector<PlacedPlant> after, int nextIdAfter);
    void buildPlantPalette();
    // The plant palette lists the plants of plants.json, then the world objects of objects.json on their own last page (US-155). Objects are
    // placed, selected, moved and deleted exactly like plants (they are kept in the level's plant list). `plant_` indexes this list.
    std::vector<std::string> plantKinds() const;
    int plantPageCount() const;
    int plantPageFirst(int page) const; // index in plantKinds() of the first kind on a page
    int plantPageSize(int page) const;
    bool onObjectPage(int page) const { return !definitions_.objects.empty() && page == plantPageCount() - 1; }
    const PlacedLight* findLight(int id) const;
    void changeLights(const std::string& what, std::vector<PlacedLight> after, int nextIdAfter);

    const PlacedEffect* findEffect(int id) const;
    void changeEffects(const std::string& what, std::vector<PlacedEffect> after, int nextIdAfter);
    void buildCharacterPalette();
    void changePickups(const std::string& what, std::vector<PlacedPickup> after, int nextIdAfter);
    void removeSelected();
    void buildProperties();
    std::pair<int, int> toWorld(int screenX, int screenY) const;
    void changeLevel(const std::string& what, Level after);
    void buildSettings();
    void buildEconomy();
    bool changeEconomy(const std::string& what, const std::string& text, int minimum, int maximum, sim::ItemCounts sim::RegionEconomy::*table);
    void buildClassPanel();
    void buildKindForm(const luna::engine::Rect& box, int y);
    const luna::engine::Texture& markerTexture(luna::engine::Renderer& renderer, const NpcMarker& marker) const;
    void buildNpcPanel(const PlacedCharacter& shown);
    void buildNpcTradePanel(const PlacedCharacter& shown);
    void addTradeRows(luna::engine::Panel& panel, int left, int width, int& y, const sim::rules::TradeProfile& shown, const std::function<bool(const std::string&, const std::string&)>& set);
    void addScheduleRows(luna::engine::Panel& panel, int left, int width, int& y, const sim::rules::Schedule& shown, const std::function<bool(const std::string&, const std::string&)>& set);
    void addDoesRow(luna::engine::Panel& panel, int left, int width, int& y, const std::vector<std::string>& shown, const std::function<bool(const std::string&)>& set);
    // The two rows of the defaults with a partner type: the type (a click goes to the next) and its actions. `refresh` makes the owner's panel be built again.
    void addPartnerRows(luna::engine::Panel& panel, int left, int width, int& y, const sim::rules::NpcExtras& shown, const std::function<bool(const std::string&, const std::string&)>& set,
                        const std::function<void()>& refresh);
    void changeSelectedNpc(const std::string& what, const std::function<void(PlacedCharacter&)>& change);
    void buildOpenList();
    void buildQuestion();
    void replaceLevel(Level level, std::filesystem::path file, const std::string& what);
    void doPending();

    Level& level_;
    const Definitions& definitions_;
    std::filesystem::path levelFile_;
    int viewWidth_;
    int viewHeight_;
    luna::engine::TileMap map_;
    luna::engine::Camera camera_;
    EditorTextures textures_;
    double centreX_ = 0.0;
    double centreY_ = 0.0;
    bool dragging_ = false;
    int dragX_ = 0;
    int dragY_ = 0;

    EditorTool tool_ = EditorTool::Brush;
    int tile_ = 0;
    bool grid_ = true;
    History history_;
    bool unsaved_ = false;
    std::string status_;
    int statusTicks_ = 0;

    // A brush or eraser stroke in progress: painted as the pointer moves, one Command at the end.
    bool stroking_ = false;
    std::pair<int, int> lastCell_{0, 0};
    std::vector<CellChange> stroke_;
    std::set<std::pair<int, int>> strokeCells_;
    // A rectangle being dragged out: its first corner and the cell under the pointer now.
    std::optional<std::pair<int, int>> rectangleStart_;
    std::pair<int, int> rectangleEnd_{0, 0};
    std::optional<std::pair<int, int>> hover_;

    std::unique_ptr<luna::engine::Panel> toolbar_;
    std::unique_ptr<luna::engine::Panel> palette_;
    std::unique_ptr<luna::engine::Panel> characterPalette_;
    std::unique_ptr<luna::engine::Panel> weaponPalette_;
    std::vector<std::string> weaponNames_;
    std::unique_ptr<luna::engine::Panel> plantPalette_;
    int plant_ = 0;
    int effect_ = 0;
    std::unique_ptr<luna::engine::Panel> effectPalette_;
    int light_ = 0;
    std::unique_ptr<luna::engine::Panel> lightPalette_;
    bool movingLight_ = false;
    std::vector<PlacedLight> movingLightsBefore_;
    std::optional<double> previewHour_;
    bool sliderDragging_ = false;
    LightPreview lightPreview_;
    bool movingEffect_ = false;
    std::vector<PlacedEffect> movingEffectsBefore_;
    int kindPage_ = 0;
    int kindPageWanted_ = 0;
    int plantPage_ = 0;
    int plantPageWanted_ = 0; // a page button sets this; the palette is rebuilt at the start of the next tick
    int weapon_ = 0;
    std::unique_ptr<luna::engine::Panel> properties_;
    int propertiesFor_ = -1;       // the character the properties panel shows (-1: none)
    bool propertiesStale_ = false; // the character changed (undo, redo): show its values again

    int kind_ = 0;
    std::optional<int> selected_;
    // A character being dragged with the Select tool: the list before, and where it was grabbed.
    bool moving_ = false;
    std::vector<PlacedCharacter> movingBefore_;
    // A pickup being dragged with the Select tool: the list before.
    bool movingPickup_ = false;
    std::vector<PlacedPickup> movingPickupsBefore_;
    // A plant being dragged with the Select tool: the list before.
    bool movingPlant_ = false;
    std::vector<PlacedPlant> movingPlantsBefore_;
    bool movingStart_ = false; // the hero start marker is being dragged
    PixelPoint startBefore_;

    std::vector<std::string> actionIds_;           // every interaction of the registry (US-268): the action checkboxes
    std::unique_ptr<luna::engine::Panel> npcPanel_;
    std::unique_ptr<luna::engine::Panel> npcTrade_; // the Trade section of the NPC panel (US-284), beside it
    int partnerIndex_ = 0;                          // which partner type the dialogue row shows
    int defaultsIndex_ = 0;                         // which partner type the defaults row shows (US-293)
    NpcClassBook* classBook_ = nullptr;
    bool classesShown_ = false;
    bool classesStale_ = true;
    bool classNew_ = false;
    sim::rules::NpcClass classDraft_;
    std::string classSelected_;
    std::unique_ptr<luna::engine::Panel> classes_;
    bool kindsTab_ = false;
    std::string kindSelected_;
    sim::rules::NpcKind kindDraft_;
    mutable std::map<std::string, luna::engine::Texture> markerTextures_; // one picture per distinct marker, made when first drawn
    bool settingsShown_ = false;
    bool settingsStale_ = false;
    std::unique_ptr<luna::engine::Panel> settings_;
    bool economyShown_ = false;
    bool economyStale_ = true;
    std::unique_ptr<luna::engine::Panel> economy_;
    std::unique_ptr<luna::engine::Panel> openList_;
    std::unique_ptr<luna::engine::Panel> question_;
    // What waits for an answer about unsaved changes: open this file (or, when empty, a new level).
    std::optional<std::filesystem::path> pending_;
    int grabX_ = 0;
    int grabY_ = 0;
};

} // namespace odysseus::game
