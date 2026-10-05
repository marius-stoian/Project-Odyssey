# Story plans: M7

Per-story plans for milestone M7, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-150](#us-150)
- [US-151](#us-151)
- [US-152](#us-152)
- [US-153](#us-153)
- [US-154](#us-154)
- [US-155](#us-155)
- [US-156](#us-156)

---

<a id="us-150"></a>

## Plan US-150: Interaction data and the rule language

Assembly plan v2.0, prompt S-US-150. Design: [M7 interactions design](M7-interactions-design.md). Traces to INT-01, INT-03, ADR-019, D-34, D-36.

### What was built
| File | What |
|---|---|
| `src/sim/rule_json.{h,cpp}` | Small JSON reader that keeps the line of every value, accepts `//` and `/* */` comments, and reads numbers as text so no floating point is used; `parseMilli` turns `1.5` into 1500 thousandths with whole-number sums |
| `src/sim/rule_expr.{h,cpp}` | The condition and score language: lexer and recursive-descent parser (precedence loosest to tightest: `or`, `and`, `not`, comparison, `+ -`, `* /`, unary minus), an evaluator over a `RuleContext` that the Game or a test implements, saturating whole-number arithmetic, and the function table (`has`, `need`, `skill`, `trait`, `opinion`, `kin`, `flag`, `tag`) |
| `src/sim/rule_effect.{h,cpp}` | Effect lines: 13 verbs with argument counts and checks (`give`, `take`, `set`, `flag`, `opinion`, `remember`, `start`, `talk`, `say`, `fx`, `sound`, `after`, `chronicle`); `after 15s ...` carries its inner effect and delay |
| `src/sim/interaction.{h,cpp}` | The `Interaction` record and the registry that loads a folder (a bad file is skipped, errors listed as `file:line: message`), matching by actor kind or tag and target tags with `Too far away` and `requires` reasons, `start` and unknown-tag checks, and the canonical writer `toJson` |
| `assets/data/interactions/gather.json` | The first interaction written as data (range, duration, two conditions, three effects, an npc score) |
| `docs/guides/interaction-data.md` | The owner's guide: every field, the sum language, every function and verb with a one-line meaning and an example |
| `src/game/odyssey_game.{h,cpp}` | Loads the folder at start, logs each mistake and a summary line, exposes `interactions()` |
| `tests/sim/rules_test.cpp`, `tests/game/interactions_test.cpp` | see below |

Technical choices (Dominus): the code lives flat as `src/sim/rule_*.{h,cpp}` and `interaction.*` because the layer check requires every header to include its layer's own `boundary.h` and has no sub-folder rule; the design document's `src/sim/rules/` became that. The language is plain whole numbers and words (no floats) so it can run in the deterministic simulation. The JSON reader is our own, 250 lines, because nlohmann cannot say which line a value came from.

### Tests
| Scenario | Test |
|---|---|
| Load | `US-150 Load: gather.json is registered and offered on edible plants` (fields, offers, "Too far away", "Nothing to pick yet", "Nothing grows in winter", not offered on rocks or to deer), `US-150 The game registers gather at start and offers it on edible plants` (all 31 edible plants of the catalog, none of the others) |
| Error | `US-150 Error: an unknown verb on line 12, and the rest still loads` (exactly `interactions/gather.json:12: unknown effect verb "giv"`, the second file loads), `US-150 Error: every kind of mistake is named with its line`, `US-150 start must name an interaction, and unknown tags are warned about`, `US-150 Expression mistakes say what is wrong`, `US-150 Effect mistakes say what is wrong`, `US-150 The JSON reader knows its lines` |
| Guide | `US-150 Guide: every function and verb is explained with an example` reads the guide and checks that each function and verb of the code appears with its meaning |
| Language | `US-150 Operators and precedence`, `Words, paths and text`, `Functions reach the world`, `and and or stop early`, `Big numbers never overflow`, `Every effect verb has a working example`, `Effect arguments and delays`, `Decimals are whole thousandths` |
| Files | `US-150 Every shipped interaction file loads and survives a load, save, load`, `Menu order`, `Labels fill their tokens` |
| Damage | `US-150 Damaged files never crash the loader`: 300 deterministic mutations of gather.json must each load or report an error |

### Manual checks (owner)
1. In `assets/data/interactions/gather.json`, replace `give` in the first effect by `giv` and start `odysseus.exe`: the log (and later the F5 panel) shows `Interactions: interactions/gather.json:<line>: unknown effect verb "giv"` and the game still starts.
2. Put it back. The log shows `Interactions: 1 loaded from 1 file(s), 0 error(s)`.
3. Read the function and verb tables in `docs/guides/interaction-data.md`.

---

<a id="us-151"></a>

## Plan US-151: Tags and smart objects

Assembly plan v2.0, prompt S-US-151. Design: [M7 interactions design](M7-interactions-design.md), sections 2 and 4. Traces to INT-01, INT-02, GD-04, D-34, D-36.

### What was built
| File | What |
|---|---|
| `src/game/tags.{h,cpp}` | `readTags` and `readStates`: the optional `"tags"` and `"states"` lists of a catalog entry, validated (words, no repeats), errors name file and field; when absent the loader's derived list is used |
| `src/game/catalogs.{h,cpp}` | `PlantDef`, `AnimalDef`, `WeaponDef` carry `tags` (plants also `states`); tags are derived from the old fields when not written; `Catalogs::knownTags()` |
| `src/game/level.{h,cpp}` | `CharacterKindDef::tags` for characters and animal kinds |
| `src/game/plants.h` | `WorldPlant::state`: starts as the plant's first state (ripe); a regrown plant is ripe again |
| `src/game/game_rules.{h,cpp}` | `GameRuleContext`: the real game as the rule language sees it (`actor.name`, `target.name/kind/state/inspect`, `season`, `time`, `distance`, `has`, `tag`) |
| `src/game/odyssey_game.{h,cpp}` | `plantThing`, `plantOffers` (what the hero may do to a plant now, with reasons), `setPlantState`, `knownTags`; interactions load with the known tags, so an unknown tag is a warning |
| `src/game/run_flow.cpp` | the plant context menu lists the interaction files' offers (Gather, Inspect) with the data's labels and reasons; Knap, Pick up by hand and Chop wood are still code until US-152 |
| `assets/data/interactions/inspect.json` | Inspect, for every plant, from anywhere in view, menu position last |
| `docs/guides/interaction-data.md` | new section "Tags and states" with the table of derived tags and states |
| `tests/game/tags_test.cpp` | see below |

Technical choices (Dominus): a plant gets the `edible` tag only when it can be gathered today (edible and walk-through); the 15 solid fruit-bearing ones get `fruit-bearing` instead, so every plant's menu stays exactly as it was (Chop wood, not Gather). Until the action runner (US-153) exists, the menu runs Gather and Inspect with the code that did it before, found by interaction id; any other data interaction shows a message that no code runs it yet.

### Tests
| Scenario | Test |
|---|---|
| Advertise | `US-151 A new plant kind tagged edible and plant is offered Gather and Inspect with no code change` (a mango added only to a copy of plants.json; a clover next to it offers Inspect only; a far mango has Gather greyed out with "Too far away") |
| State | `US-151 A plant in state picked shows Gather disabled with its reason` ("Nothing to pick yet", Inspect still on) |
| Unknown tag | `US-151 An interaction aimed at a tag no catalog uses gets a warning naming file and tag` (`interactions/warm.json:1: unknown tag "hearth"`); the shipped files give no warning |
| Catalogs | `US-151 Catalog entries get tags, and written tags replace them` (every plant, animal, weapon and character kind), `US-151 Bad tags and states are named with their file and field` |
| Earlier | `US-150 The game registers gather at start and offers it on edible plants` now uses the catalog's own tags |

### Manual checks (owner)
1. Add to the top of the `plants` list in `assets/data/plants.json`: `{"name":"mango","frame":"bush","size":"tall","blocks":false,"edible":false,"inspect":"Sweet and heavy.","tags":["edible","plant"],"states":["ripe","picked"]},` (a bush picture stands in). Place a mango with the Editor (F2), start the game, begin a run and right-click it: Gather and Inspect are offered.
2. Change the mango's `"tags"` to `["plant"]`: only Inspect is offered. Remove the mango again.
3. Copy `gather.json` to `warm.json`, change its id to `warm` and its target tag to `hearth`: the log warns `unknown tag "hearth"` and the game still starts.

---

<a id="us-152"></a>

## Plan US-152: The context menu from data

Assembly plan v2.0, prompt S-US-152. Design: [M7 interactions design](M7-interactions-design.md), section 9. Traces to INT-01, INT-03, D-34.

### What was built
| File | What |
|---|---|
| `src/game/game_rules.{h,cpp}` | `Subject`: one thing the hero may act on (a clan member, the knapping stone, the clan's fire, the sacred fire, a rival camp, a plant), found by `subjectAt` in the order the old menu looked; each has a kind, tags and the point distance is measured to. `GameRuleContext` answers `target.name/kind/state/inspect`, `season`, `time`, `distance`, `has`, `tag` and `flag(sacred-fire)` for it |
| `src/game/builtin_actions.cpp` | the 14 built-in actions (`gather`, `knap`, `pick-flint`, `chop`, `inspect`, `talk`, `give-berries`, `ask-to-teach`, `open-craft`, `eat-berries`, `tend-camp-fire`, `tend-sacred-fire`, `hold-ritual`, `open-barter`): the code the old menu ran for each item, moved here unchanged; `runInteraction` carries out a chosen interaction's `do` and `say` effects |
| `src/game/run_flow.cpp` | `RunFlow::openContext` is now about 20 lines: find the subject, ask the registry what is offered, list it (label, greyed-out reason and order all from the file); a click looks the interaction up by id (data may be reloaded while the menu is open) |
| `src/sim/rule_effect.cpp`, `interaction.{h,cpp}` | new effect verb `do`; `LoadOptions::knownBuiltins`: a `do` naming an action the game does not have is a load error |
| `assets/data/interactions/*.json` | 20 files: talk, give-berries, five ask-to-teach-*, craft-at-fire, eat-berries, tend-fire, tend-sacred-fire-from-camp, craft-at-stone, tend-sacred-fire, hold-ritual, barter, gather, knap, pick-flint, chop, inspect |
| `docs/guides/interaction-data.md` | the `do` verb, the table of built-in actions, the tags and names of the game's own things |
| `tests/game/menu_test.cpp` | see below |

### How "behaviour must not change" was kept
- Same labels, order and greyed-out reasons ("Too far away" first, then the item's own reason), and same distances (2 m for people, fires, the stone and plants; 4 m for the sacred fire; 3 m for a rival camp; the sacred-fire item at the clan's fire has no distance limit, as before).
- `gather.json` does what the game did: instant, reach 2 m, label "Gather". The brief's example (3 s, winter rule, 1.5 m, "Gather {target.name}") is kept as a US-150 test fixture. The winter rule and 3-second duration return in US-153 and the owner's design decision about winter (see Milestone-59).
- "Ask to teach you ..." is offered only while the clan member is the hero's master of that profession and has no apprentice, as before: the game gives such a person the tag `teaches-<profession>`, and each ask-to-teach file targets that tag.

### Tests
| Scenario | Test |
|---|---|
| Same actions | `US-152 The same actions are offered as before: a person, the fire and the knapping stone` (titles, labels, order and reasons for each; no menu on empty ground), `US-152 The sacred fire and its menu` |
| Same behaviour | `US-152 Choosing an item does what it did before` (Craft at the fire opens the craft screen, Eat berries uses a berry, Tend the fire speaks, a greyed-out item does nothing, Talk and Give berries act on the clan member) |
| Data change | `US-152 The owner renames Tend the fire in tend-fire.json and F5 shows it` |
| Validation | `US-152 A do naming a built-in action the game does not have is an error`, `US-152 Every action the old menu had is now a file` (and every built-in is used by some file) |
| Regression | every existing test, unchanged except where noted below |

Tests that had to change: the US-150 `Load` test and the US-156 reload tests used the shipped `gather.json` as the brief's example (3 s, two conditions, 1.5 m). The shipped file now keeps today's behaviour, so US-150 keeps the brief's example as an embedded fixture and US-156 edits the new values (range 2 to 3, `do gather` to `giv gather`). The `US-150`/`US-151` game tests count the gatherable plants with the catalog's own tags. No M5 or M6 test changed.

### Manual checks (owner)
1. Start the game, begin a run (Off preset), right-click a clan member, the clan's fire, the knapping stone: the menus are the ones you know.
2. In `assets/data/interactions/tend-fire.json` change `"label": "Tend the fire"` to `"Feed the flames"`, save, press F5, right-click the clan's fire: the item has the new name.
3. Change a `do` in a file to a misspelling (`do tend-camp-fires`), press F5: the red panel names the file and line.

---

<a id="us-153"></a>

## Plan US-153: Timed actions and world state

Assembly plan v2.0, prompt S-US-153. Design: [M7 interactions design](M7-interactions-design.md), sections 5 and 6. Traces to INT-04, D-36, D-37.

### What was built
| File | What |
|---|---|
| `src/sim/action_runner.{h,cpp}` | `ActionRunner`: starts interactions for an actor on a target (an instant one acts at once, a timed one runs for its duration), `cancel` (nothing happens), `tick` (waiting effects in the order they were due and made, then finished actions), `progress` 0..100 for the ring, `savePending`/`loadPending` for effects still waiting, `hash` for the determinism check. Whole ticks (20 a second), no floating point; ordering by tick, then creation order, then actor id |
| `src/sim/action_runner.h` | `EffectHost`: what the runner asks the game to do (`setState`, `apply` for every other verb, `ticksPerDay`); `ThingRef`: a thing by kind and stable id, never by pointer |
| `src/game/builtin_actions.cpp` | `GameEffectHost` (set state of a plant, `do`, `say`, `give`/`take` on the hero's bag), `startInteraction`, `tickInteractions`; the built-in `gather` became `gather-berries` (berries, skill, message, tutorial step) because what happens to the plant is now written in the file |
| `src/game/game_rules.{h,cpp}` | `refOf`/`subjectFor`: a Subject as a stable ref and back |
| `src/game/plants.h` | `WorldPlant::hidden()`/`present()`: a plant in any state but its first is hidden until it is back to it (D-37) |
| `src/game/odyssey_game.{h,cpp}` | the runner and its clock (play ticks, stopped while a screen is open), the interrupt rule (moving or attacking stops the hero's action), the ring of twelve dots over the target, hidden plants cannot be seen, clicked, inspected or hit, `things.json` saved with the autosave and read with the load |
| `assets/data/interactions/gather.json` | 3 s, `do gather-berries`, `set target.state picked`, `after 15s set target.state ripe`; no winter rule (D-37) |
| `docs/guides/interaction-data.md` | section "Timed actions and things that change" |
| `tests/sim/runner_test.cpp` (9 cases), `tests/game/menu_test.cpp` (5 new cases) | see below |

Technical choices (Dominus): the saved state is a second file, `things.json`, written beside `clan.json` and `hero.json` with the same safe write and backups, instead of a new section and a bump of the clan save version; the clan save (version 3) is untouched and plant states and timers are saved by the layer that owns them. Waiting effects are saved relative to "now" (`in` ticks), so a loaded game does not care what its clock says. Running actions are not saved (their effects have not happened, so only work in progress is lost).

### Tests
| Scenario | Test |
|---|---|
| Progress | `US-153 Gather takes three seconds under a ring, then gives berries and hides the plant until it is ripe again` (progress 50 at 1.5 s, no berries one tick before the end, +2 at 3.0 s, the plant is hidden and unclickable, back and clickable 15 s later; the ring adds draw calls while it fills), `US-153 A timed interaction acts when its time is over, not before, and shows its progress`, `US-153 A short duration still takes at least one tick` |
| Interrupt | `US-153 Moving or attacking stops a timed action and it gives nothing` (both intents), `US-153 A stopped action gives nothing, and the actor is free again`, `US-153 Standing still lets the action finish; opening a screen pauses it` |
| Saved | `US-153 A picked bush and a lit fire are still there after a save and a load` (plant still hidden, sacred fire still founded and lit, berries kept, ripe 10 s after loading), `US-153 Waiting effects survive a save and a load, whatever the clock says`, `US-153 Things that never changed save nothing, and a damaged things file is reported` |
| Order and data | `US-153 after waits for its time, in the order the effects were made` (s, m and d), `US-153 An actor does one thing at a time; two actors may work side by side`, `US-153 An interaction taken out of the data while it runs does nothing when it ends`, `US-153 The same inputs give the same runner state` |

Existing tests that changed: the US-156 reload tests edit the renamed built-in (`do gather` became `do gather-berries`). No M5 or M6 test changed.

### Manual checks (owner)
1. Begin a run, walk next to a ripe wheat plant, right-click it, choose Gather: a ring of dots fills over it for 3 seconds, you get 2 berries, the plant vanishes, and 15 seconds later it is back in the same spot.
2. Start Gather again and walk away (or attack) before the ring is full: the dots stop, no berries, the plant stays.
3. Pick a plant, wait for the day to end (autosave) or press the menu's save, close the game, start it and load: the plant is still gone and comes back about when it would have.
4. In `assets/data/interactions/gather.json`, change `"duration": 3` to `10`, press F5 and gather again: the job now takes 10 seconds.

---

<a id="us-154"></a>

## Plan US-154: NPCs and animals use interactions

Assembly plan v2.0, prompt S-US-154. Design: [M7 interactions design](M7-interactions-design.md), sections 7 and 9. Traces to INT-05, SDC-02, D-36.

### What was built
| File | What |
|---|---|
| `src/sim/npc_chooser.{h,cpp}` | `pickBest` (the best score that reaches a minimum; ties broken by one draw of a seeded stream, drawn every time so the stream moves the same whatever happens) and `CooldownTable` (an actor's resting interactions, ordered containers, with a hash) |
| `src/game/npc_life.{h,cpp}` | `NpcLife`: once a second an idle clan member or animal looks at what is near, scores every interaction it may do with the file's `npc.score`, skips resting ones, picks the best, walks there (people through the clan view, animals step by step over the map) and starts it with the same runner as the hero. A hostile within 6 m makes them drop what they were doing (D-36); `fleeFrom` is what `do flee` does. Whole numbers, ordered containers |
| `src/game/game_rules.{h,cpp}` | `ActorRef` (hero, person, animal) with runner numbers 0, 1000 + person, 100000 + character id; `animalSubject` and `heroSubject` (tags `armed` and `moving` while true); the rule context now knows who acts: `need(...)` is how much is missing, `trait(...)`, `distance` from the actor, `actor.name` |
| `src/game/builtin_actions.cpp` | `startInteractionFor` (any actor), NPC versions of `do gather-berries` (they eat: hunger +30), `do graze`, `do flee`, and `fx` |
| `src/game/clan_view.{h,cpp}` | errands: a person walks where an interaction takes them instead of where their hour's action would |
| `src/game/odyssey_game.{h,cpp}` | the NPC tick after the clan's tick; `harmPerson`; accessors for the ground, the enemies and the placed animals |
| `src/sim/world.{h,cpp}` | `drainPersonNeed` (a hazard: the opposite of `satisfyPersonNeed`) |
| `assets/data/interactions/` | `graze`, `flee-predator`, `flee-armed-hero`, `flee-moving-hero`; the four small grasses carry the tag `grass` in `plants.json` |
| `docs/guides/interaction-data.md` | section "Clan members and animals act on their own", the table of actors, the new built-ins, the new meaning of `need(...)` |
| `tests/sim/runner_test.cpp`, `tests/game/npc_test.cpp`, `tests/game/camp.h` | see below; the camp helper of the menu tests moved into a shared header |

Technical choices (Dominus): the level's harmless animals (deer, rabbits) are placed characters that used to only stand and be drawn; they now walk, graze and flee, with their exact position kept in `NpcLife` and the whole-pixel position written back to the level data for drawing. Fighting monsters (wolf, goblin) are the threats and do not act yet (they stand as before). `need(...)` changed from "how full" to "how much is missing" so that `need(hunger) * 2` in the brief's example means what it says; the guide and function table say so. Clan members and animals are not saved mid-errand: after loading everyone is idle and looks around again.

### Tests
| Scenario | Test |
|---|---|
| Hungry | `US-154 A hungry clan member walks to a ripe plant and gathers it, and the plant shows as picked` (made hungry with `harmPerson`; walks, works under the same runner, the plant is picked and hidden, they stand at it, hunger rises, the plant ripens again and they go back to their day), `US-154 Someone who is not hungry leaves the bush alone` |
| Animals | `US-154 A deer grazes on grass and runs from a wolf` (walks to the grass and grazes; a wolf placed 3 m away makes it flee and gain more than 3 m), `US-154 Prey flee a hero who holds a weapon or runs at them, but not one standing still with empty hands` |
| Danger | `US-154 Danger drops what a clan member was doing` (a goblin beside a walker: they stop, take nothing, and do not set off again) |
| Deterministic | `US-154 The same seed and the same inputs give the same world after thousands of ticks` (two games with 4 plants, 3 animals and hungry people: clan hash, runner hash, NPC hash, every plant state and every animal position are equal; 10,000 ticks in Release, 3,000 in Debug) |
| The chooser | `US-154 The best score wins, below the minimum nothing is chosen`, `US-154 Ties are broken by the seeded stream, the same way every time`, `US-154 An interaction an actor just did rests for its cooldown` |

### Manual checks (owner)
1. Begin a run, place a ripe wheat plant near the camp in the Editor, and watch a hungry person walk to it, work under the ring, and the plant disappear and come back. (In a fresh game people are not yet hungry: after a day or two of game time someone will go.)
2. Place a deer and some grass (`grass`, `clover`) with the Editor: the deer wanders to the grass and grazes. Place a wolf near it: the deer runs away.
3. Walk toward the deer with a weapon in hand: it runs. Stand still with empty hands: it carries on grazing.
4. In `graze.json` change `"cooldown": 10` to `2`, press F5: the deer grazes more often.

---

<a id="us-155"></a>

## Plan US-155: Age 1 world objects

Assembly plan v2.0, prompt S-US-155. Design: [M7 interactions design](M7-interactions-design.md), section 10. Traces to INT-02, INT-04, D-34, D-35.

### What was built
| File | What |
|---|---|
| `assets/data/objects.json` | the seven objects (D-35): fire pit, knapping stone, food store, shelter, flint nodule, water source, sleeping furs, each with tags, states, a picture name and an inspect line |
| `src/game/catalogs.{h,cpp}` | `PlantDef::object`; `loadCatalogs` reads objects.json into the plant catalog (validated: file and field named, a name that is a plant is refused, tags and states as for plants) |
| `src/game/object_art.{h,cpp}` | programmer art: one 32 x 32 picture per object drawn by code into one texture page; an unknown picture name draws a grey block |
| `src/game/level.{h,cpp}`, `plants.h` | `Definitions::objects`; `hasPlant` accepts objects; an object is never hidden by its states |
| `src/game/editor.{h,cpp}` | the plant palette lists the plants, then the objects on a last page of their own ("6/6 objects"); placing, selecting, moving, deleting and undo are the plant ones |
| `src/sim/world.{h,cpp}` | `World::satisfyPersonNeed`: outside help for a need (capped, whole numbers, the dead ignored) |
| `src/game/builtin_actions.cpp`, `odyssey_game.*` | built-in `restore <need> <amount>` and `warm-nearby <tiles> <amount>`, the effect verb `fx`, `OdysseyGame::helpPerson` |
| `assets/data/interactions/` | 9 files: `light-fire`, `put-in-store`, `take-from-store`, `rest-in-shelter`, `sleep-on-furs`, `drink`, `chip-flint`, `pick-nodule-flakes`, `inspect-object` (the knapping stone object reuses `craft-at-stone`) |
| `docs/guides/interaction-data.md` | the objects.json section: fields, the shipped objects and their actions |
| `tests/game/menu_test.cpp`, `tests/sim/needs_test.cpp` | see below |

Technical choice (Dominus, codex issue CI-008): objects ride on the plant machinery, so no level format change or migration was needed. Objects are saved in the level's plant list under their own names, and the existing plant tests are the regression baseline.

### Tests
| Scenario | Test |
|---|---|
| Place | `US-155 The Editor places a fire pit and it is saved in the level` (placed through the plant tool, saved, loaded again, undone) |
| Use | `US-155 Light a fire in a fire pit with a fire drill, and it warms people nearby` (greyed out without a drill, 3 s, state burning, "The fire warms N people.", two more warmings and the going-out waiting as timers, not offered again while it burns) |
| Data | `US-155 A new object kind added to objects.json shows up with its tags, and mistakes are named`, `US-155 objects.json has the seven objects with tags and states` |
| The others | `US-155 The other objects have their actions` (store in and out, flint nodule, furs, water, the stone's Craft), `US-155 Outside help raises a need, capped, and ignores the dead and the unknown` |

Existing tests that changed: `US-130 Cut` counts the plants that are not objects (153; the catalog now also holds the 7 objects), `US-136 Editor` (the palette has one more page for the objects: the last page is 5, not 4), `US-151 Catalog entries get tags` (an object is tagged `object`, not `plant`). No M5 or M6 test changed.

### Manual checks (owner)
1. Press F2, choose the plant tool, press the `>` arrow until the page says `6/6 objects`, choose the fire pit, click the map, press Ctrl+S. Press F1: the pit is there.
2. Begin a run, give yourself a fire drill (craft one at the fire), stand next to the pit, right-click it, choose Light fire: three seconds, then "The fire warms N people." and the clan near it warms up.
3. Add a new object to `assets/data/objects.json` (copy the shelter, change the name), restart the game: it is on the objects page of the palette.

---

<a id="us-156"></a>

## Plan US-156: Hot reload (F5) and the validation panel

Assembly plan v2.0, prompt S-US-156. Design: [M7 interactions design](M7-interactions-design.md), section 8. Traces to INT-03.

### What was built
| File | What |
|---|---|
| `src/luna/platform/events.h`, `sdl_events.cpp`, `src/luna/engine/input.{h,cpp}` | the F5 key and the new intent `Reload` (ARC-03: the game reads the intent, never the key) |
| `src/game/odyssey_game.{h,cpp}` | `reloadInteractions()`: reads the folder into a registry on the side; only a clean one replaces the data in use (all or nothing); the latest report always feeds the panel; `interactionPanelOpen()`; the time is kept and logged |
| `src/game/odyssey_game.cpp` | `drawInteractionPanel`: at the top of the screen, in Game and Editor mode, `file:line: message` for up to 8 mistakes (then "...and N more (see the log)"); it also shows at start when a file is left out, and closes by itself after a clean reload |
| `tests/game/tags_test.cpp` | 4 cases, below |
| `docs/codex-issues.md` | CI-007: catalogs and dialogue are not part of this reload |

Technical choices (Dominus): the swap happens at the start of the tick that sees F5, before anything uses the data, so nothing reads a half-replaced registry (the game is single-threaded, ADR-007). A reload with any mistake keeps the whole old registry: a half-new world is harder to reason about than "nothing changed". Running actions that point at a removed interaction are cancelled by the action runner when it exists (US-153); today nothing runs from data yet.

### Tests
| Scenario | Test |
|---|---|
| Reload | `US-156 F5 takes an edited file within a second` (an edited range is live after the Reload intent in under 1000 ms; a new file appears) |
| Bad file | `US-156 A bad file lists file:line: message and the last good data stays; fixing it closes the panel` (`unknown effect verb "giv"`, the old data stays whole although the same file also had a good change, the panel adds draw calls, the panel is gone after the fix) |
| Fix | same test; and `US-156 A syntax error is reported with its line and a missing folder never crashes` |
| Editor | `US-156 F5 works in the Editor and the panel shows there too` |

### Manual checks (owner)
1. Start the game, open `assets/data/interactions/gather.json` in Notepad, change `"range": 1.5` to `"range": 3`, save, press F5 in the game: the log says `Interactions reloaded: ... in N ms` (well under a second).
2. Replace `give` in the first effect by `giv`, save, press F5: a red panel at the top says `Interaction files: 1 mistake(s). Fix them, then press F5.` and `interactions/gather.json:<line>: unknown effect verb "giv"`; the game keeps working with the old data.
3. Fix the word, press F5: the panel closes. Press F2 for the Editor and try the same: the panel shows there too.
