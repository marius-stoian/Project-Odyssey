# Beta test plan after M9 (Dominus, 2026-10-05)

The owner runs this by hand on the PC after X-M9 is green and `m9-done` is tagged. It covers everything built through M9 (M0 to M9; M10 onward is not built). Each row says what to do and what you should see. Controls and file names come from `README.md` and `docs/guides/`; if a guide and the game disagree, that is a finding.

## 1. Purpose and rules

- Find what the 27 automated test groups cannot: how it looks, how it feels, GPU behaviour, long play, and design mistakes.
- Close the open owner rows of the gates: M8c rows 7 and 8, M8d, M8e, M9a row 7, M9b row 6, M9c rows 6 and 7, M6 row 10 (playtesters).
- Three passes: **A. Smoke** (30 min: can the game be played at all), **B. Feature walk** (about 6 hours over several days, sections 4.1 to 4.14), **C. Soak and stress** (overnight, section 5).
- Out of scope: quests, data editors, world editing, politics, technology (M10 to M14), mobile.

## 2. Preparation

| Step | Do | Done when |
|---|---|---|
| P1 | `git checkout main`, confirm tag `m9-done`, CI green on main | `git describe --tags` names `m9-done` |
| P2 | Release build: `cmake --preset windows-x64-release`, then `cmake --build --preset windows-x64-release` | zero warnings, `odysseus.exe` exists |
| P3 | Also keep the Debug build (ASan finds crashes the Release build hides) | `windows-x64-debug` builds |
| P4 | Back up `%APPDATA%\Project Odyssey\Odysseus\` (saves, settings, logs), then delete `settings.json` to test first launch | folder copied |
| P5 | Back up `assets/levels/` and `assets/data/`; the Editor writes there | copy exists, so a bad edit never costs real data |
| P6 | Create the bug sheet (section 7) and a screenshot folder `docs/evidence/beta-1/` | ready |
| P7 | Close other heavy programs; note your PC specs and GPU driver version | noted in the sheet |

Every run writes a log to `%APPDATA%\Project Odyssey\Odysseus\logs\` (last five kept). Attach the newest one to each bug. `--screenshot file.bmp --quit-after N` saves a frame; the F3 overlay shows frame time.

## 3. Pass A: smoke (do first, stop on any fail)

| # | Check | Expect |
|---|---|---|
| A1 | Run `odysseus.exe` | window opens within 3 s, no error box, a hero stands in the valley |
| A2 | Walk with WASD, then arrows | hero moves in 8 directions, animates, stops when you stop |
| A3 | Press F2, then F1 | Editor opens and the world stops; F1 starts play again |
| A4 | Close with the window button | clean exit, a new log file exists |
| A5 | Run `odysseus_headless.exe --seed 7 --years 100` | finishes, prints a report, no crash |
| A6 | Run `ctest --preset windows-x64-release` | 27 of 27 pass |

## 4. Pass B: feature walk

Result column: P pass, F fail, N not as designed, - skipped. Write the bug id in the sheet for any F or N.

### 4.1 Install, launch, settings, safety nets (M5, M6, M8b)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 1.1 | First launch | Delete `settings.json`, start the game | Privacy screen asks about local statistics once; defaults 1280x720, Whole, zoom 2x, UI 1x, lighting Medium, volume 80 |
| 1.2 | Statistics opt-in | Choose "agree", play 5 minutes, quit | stats stay local, nothing sent (check no network traffic) |
| 1.3 | Settings screen | Open Settings, switch Windowed, Borderless, Exclusive | applies at once, survives restart |
| 1.4 | Window sizes | 1280x720, 1600x900, 1920x1080, 2560x1440 | picture stays 960x540 virtual, no stretch glitch |
| 1.5 | Scaling | Whole, then Fill | Whole has black bars at whole multiples; Fill fits the area |
| 1.6 | Camera zoom | Keys `+` and `-`, mouse wheel | 2x shows 15 x 8.4 tiles, 1x shows 30 x 17 |
| 1.7 | UI scale | Settings, UI 2x | panels, font and hotbar double and stay readable at every size |
| 1.8 | Damaged settings | Corrupt `settings.json` by hand, start | defaults restored and the problem reported, no crash |
| 1.9 | Renderer | Normal start; then force the fallback if the guide says how | GPU path looks the same as the SDL fallback |
| 1.10 | Crash log | Debug build only: trigger a known assert if one is documented | log names file and line; last save intact |
| 1.11 | Zip package | Build the CPack zip, unzip into a clean folder, run | starts without Visual Studio or dev tools |

### 4.2 New game, hero and tutorial (M5, M6)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 2.1 | New Game | Enter a seed, pick Growing Period preset and Comfort | the region generates; same seed gives the same world twice |
| 2.2 | Growing Period | Play through childhood and the crossroads | mantle and specialty choices work and show consequences |
| 2.3 | Tutorial | Follow the elder through the first day | steps advance, hints appear when stuck |
| 2.4 | Skip tutorial | Skip it at the start and mid-way | no stuck state, no missing items |
| 2.5 | Controls | WASD, arrows, mouse aim, gamepad stick and D-pad | hero faces 8 directions, gamepad works fully |
| 2.6 | Death and restart | Let monsters kill the hero | restart at START marker (editor levels) or the run ends cleanly |

### 4.3 Combat and items (M2d to M4)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 3.1 | Pickups and hotbar | Walk over weapon pickups (`range.json` or `demo.json`) | go to first free slot of 9; "Hotbar full" flashes at 9; keys 1 to 9 and Shift choose |
| 3.2 | Weapons | Use sword, axe, whip, bow, crossbow, thrown, staff (E, Space, Enter) | each attacks differently; bolts fly on arcs |
| 3.3 | Elements | Fire, ice, poison, lightning, void weapons on monsters | distinct effects, no stuck status |
| 3.4 | Monsters | Fight goblin, wolf, troll, ghost, others | red `!` warning, half-second delay, damage only within 1.5 m, falls at 0 HP |
| 3.5 | Animals | Meet harmless and enemy animals | the 20 enemy animals bite back, the rest cannot be hit |
| 3.6 | HP and healing | Eat edible plants | +10 HP per edible plant, HP shown top left |

### 4.4 World, plants, weather, time (M3, M4, M8c)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 4.1 | Plants | Approach, E on a plant with empty hands (or right click) | name and line for 3 s; hit destroys it; regrows after 15 s |
| 4.2 | Region generation | Several seeds | biomes, resources, two rival clans each time, no impossible spawns |
| 4.3 | Calendar | Watch a day pass; skip through four seasons | summer 15 h days, winter 8 h; seasons change plants and light |
| 4.4 | Weather | Wait, or `--weather "steady rain"`, `--seed 7` | drizzle to storm fade in over 3 s; same seed gives the same sequence |
| 4.5 | World objects | Right-click fire pit, knapping stone, food store, shelter, flint, water, furs | each offers its actions; timed actions show a ring, can be interrupted |
| 4.6 | Save and load | Autosave at day end, quit, load with `--load` | the world returns exactly; delete the newest save, game falls back to a backup |

### 4.5 Professions, crafting, trade and religion pillars (M5)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 5.1 | Five professions | Hunter, gatherer, flint-knapper, fire-keeper, shaman-healer | each has skill, tools and actions; apprenticeship teaches |
| 5.2 | Crafting | At the fire and at a stone; knap flint | recipes work, quality differs by skill |
| 5.3 | Gather, chop, hunt | Berry bush, tree, deer | items arrive, tools wear if designed to |
| 5.4 | Barter and debts | Trade with the rival clan | counters, debts and reputation record; Trade pillar grows |
| 5.5 | Sacred fire and ritual | Tend the fire, hold a ritual | followers gather; Religion pillar grows |
| 5.6 | Win and lose | Play toward leading the region; also fail on purpose | clear end screens; the run can be restarted |

### 4.6 Clan simulation (M2, M2b to M2d)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 6.1 | Needs and AI | Watch a clan member for a day | eats, sleeps, warms up, socialises; no one freezes in place |
| 6.2 | Inspect people | Hover panel; F12 developer tools | shows needs, mood, memories, relations |
| 6.3 | Memory and gossip | Cause an event near people, wait | others learn it and repeat it |
| 6.4 | Quarrels, feuds, revenge | Run 5 in-game years | every death and feud has a recorded cause |
| 6.5 | Courtship, rivals, parting | Watch pairing | courtship events, jealousy, parting appear in the chronicle |
| 6.6 | Care | Make someone ill | others nurse, share food, teach the young, hunt together |
| 6.7 | Chronicle and episodes | Headless `--chronicle`, in-game story view | readable, consistent, no raw ids; **judge the story: is it a story?** (Kill Gate question) |
| 6.8 | Population | 100-year headless runs on 10 seeds | population stays viable, no instant extinction |
| 6.9 | Determinism | Same seed twice | identical chronicle |

### 4.7 Lighting and rendering (M8b, M8c)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 7.1 | Day and night | Watch dawn, noon, dusk, midnight | smooth colour change; screenshot a time-lapse sheet (closes M8c row 8) |
| 7.2 | Sun and moon | Moon phases, shadows from both | light direction follows the body; eclipses darken as designed |
| 7.3 | Fires and torches | Night near fire pit and torches | warm glow, flicker, shadows of nearby objects |
| 7.4 | Weather light | Rain, storm | scene dims and tints; lightning flashes |
| 7.5 | Normal maps | Walk round a lit fire at night | sprites shade from the light side, no seams |
| 7.6 | Lighting quality | Low, Medium, High | Low: flat sprites and no fire shadows; Medium and High: normal maps and fire shadows |
| 7.7 | Light limit | Place more than 64 lights in one view (Editor) | no crash; the nearest lights win |
| 7.8 | Frame rate | F3 overlay at 1080p High with 500 people | 60 FPS target; record the table for Low, Medium, High (closes M8c row 7) |

### 4.8 Interactions and F5 hot reload (M7)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 8.1 | Context menu | Right-click things, people, animals | menu comes from data, shows only what is available |
| 8.2 | Rules | Try an action that fails its rule | the greyed line says why |
| 8.3 | Timed actions and saved state | Start gather, save and load mid-way | ring resumes; picked bush regrows after its delay |
| 8.4 | NPCs and animals use the same actions | Watch them gather and hunt | same results as the hero |
| 8.5 | F5 reload | Edit `assets/data/interactions/*.json`, press F5 | live within 1 s |
| 8.6 | Validation panel | Introduce a typo | the panel names file, line and field; the old data stays; fixing it clears the panel |

### 4.9 Dialogue and speech (M8)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 9.1 | Talk | Right-click a clan member, Talk | game pauses, panel shows name, mood word, words, up to 5 numbered choices |
| 9.2 | Choices | Click or press 1 to 5; Esc leaves | greyed choices show the `[else]` reason |
| 9.3 | Written scripts | Talk to the elder (`elder-fire.dlg`) at different opinion, time, season | first lines change with opinion (guarded below 10) |
| 9.4 | Effects | Give a berry; take items; opinion changes | bag and opinion update; effects match the guide table |
| 9.5 | Small talk | Talk to someone without a script | lines come from their memories, needs, season, hero; mood decides the template |
| 9.6 | Bubbles | Stand among friendly people | at most one greeting a minute each, at most two bubbles at once |
| 9.7 | Remembered | Say something, talk again later | they remember; the chronicle records the marked ones |
| 9.8 | Clan talks to clan | Watch two clan members | greet, quarrel, comfort in bubbles |

### 4.10 NPC foundation (M9a) on `npc-test.json`

Run `odysseus.exe --level assets/levels/npc-test.json`. Walk the table in `docs/guides/npc-data.md` ("The test level and its walk-through").

| # | Feature | How to check | Expect |
|---|---|---|---|
| 10.1 | Tala, trader | Right-click | title reads `Tala (neutral)`; praise raises opinion; X shows greyed actions and the reason |
| 10.2 | Harn, wary hunter | Talk kindly (+10), then mock (-10) | the attitude word in the title follows opinion |
| 10.3 | Vell, elder | Talk at opinion below and above 10 | different first lines |
| 10.4 | Gur, guard, Confront | Stand next to him, press **C** | his opinion falls; nearby friends hear it and react |
| 10.5 | Actions pop-up | Press **X** near any NPC | lists what can be done and why some are greyed |
| 10.6 | Deer and goblin | Approach each | deer cannot be talked to; goblin attacks (hostile) |
| 10.7 | Change in Editor | F2, Class and Kinds: make `goblin` neutral, F1 | goblin no longer attacks |
| 10.8 | NPC panel | Select an NPC: name, classes, attitude, opinions, partner types, script | edits apply on F1; Undo works |
| 10.9 | Kinds tab and markers | Look at rings and icons under each NPC | each NPC with a class shows ring and icon; click opens its panel |
| 10.10 | Own kind, own class | Create a class and a kind in the Editor, place one | follows precedence: NPC, then kind, then classes |
| 10.11 | 100,000 people | Run the load test from the guide; also place hundreds in the Editor | detail falls with distance; no stutter near the hero |
| 10.12 | Screenshots | Kinds tab, markers, test level | saved in `docs/evidence/beta-1/` (closes M9a row 7) |

### 4.11 Trade economy (M9b)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 11.1 | Trade screen | Right-click Tala, Trade | bag left, stock with prices right, balance bar, `Money here: Shells = 1` |
| 11.2 | Barter | Give berries (wanted), take flint | bar fills, Deal works, goods move |
| 11.3 | Currency | Pick up shells, trade, pay from balance | change comes back as shells |
| 11.4 | Haggle | Haggle once with Harn | a chance and a roll; a second try the same day is greyed out |
| 11.5 | Reputation and rare goods | Ask Harn about rare goods unfriendly, then friendly | refused with a reason first; offered (spearhead) once he is friendly |
| 11.6 | Stock and restock | Wait one in-game day, trade with Tala again | one more flint, plus a random delivery |
| 11.7 | Prices | Buy out a good, wait | price rises with scarcity, drifts, reputation applies last |
| 11.8 | Editor trade panel | Select Tala, edit stock, wants, rare goods | sits below the NPC panel; changes follow in play |
| 11.9 | Economy panel | Level, Economy: Money, Prices, Goods lines | `shells=1 gold=10` style input works; bad input is named |
| 11.10 | Screenshots | Trade screen and Editor Trade panel | saved (closes M9b row 6) |

### 4.12 NPC life (M9c)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 12.1 | Places | Economy panel Places line `market=20,10 grove=30,12/forage/shelter` | places exist; mistakes are named |
| 12.2 | Schedules | Watch Tala through a day and night | work at the market from 06:00, sleep at home from 21:00; interruptions resume |
| 12.3 | Schedule layers | Give NPC, kind and class each a schedule | the highest layer wins as a whole |
| 12.4 | Actions from class, custom, events | Use the Editor schedule/action panels | each source adds actions; no quest hook |
| 12.5 | NPCs act on each other | Watch them over a day: talk, trade, give, confront, fight | far persons act by a daily roll, near ones visibly |
| 12.6 | Defaults by partner type | NPC meets animal, environment, another NPC | `defaults` files supply sensible actions |
| 12.7 | Living test level | Watch one full day | the walk-through in the guide holds |
| 12.8 | Screenshots | Living level | saved (closes M9c row 7) |
| 12.9 | Soak | See section 5 | closes M9c row 6 |

### 4.13 Buildings (M8d, M8e)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 13.1 | Build menu | Press **B**: Buildings and Pieces | known blueprints only (hut, windbreak); ghost follows pointer, **R** turns, red when blocked, Esc or right click cancels |
| 13.2 | Blueprint and build | Place, Bring materials, Build (4 s), Cancel | materials delivered and dropped on cancel; free materials without a run |
| 13.3 | Rooms and roofs | Wall in a room with door and roof | walls and fences block walking; the roof fades over the hero's room |
| 13.4 | Interiors | Enter a building with `interior: map` (Go into) | interior level loads, `exit` place returns to the outside world unchanged |
| 13.5 | Clan builds | Leave blueprints, wait | idle clan members bring up to 2 of each material and build |
| 13.6 | Rival clans | Wait a season | rivals build by season (lists, not map cells) |
| 13.7 | Raids and fire | Rivals at war, fire weapons | raids at season start, fire spreads, repair, put out, fire gives light |
| 13.8 | Owners and storage | Check dawn assignment, warmth, store capacity | housed people stay warmer; stores halve spoilage; `store-food` works |
| 13.9 | Editor Build tool | Place finished, turn, set interior, owner, blueprint | saved in the level (`buildings`, version 6) |
| 13.10 | Prefab tab | Make a prefab on the grid, set size, uses, cost, build time, Save | appears in the Build tool and, if buildable and known, in the game's menu |
| 13.11 | Screenshots | Ghost, blueprint, roof fade, fire, interior | saved (closes M8d and M8e owner rows) |

### 4.14 Level Editor (M2d, M8 to M9)

| # | Feature | How to check | Expect |
|---|---|---|---|
| 14.1 | Ground tools | Brush, Rect, Fill, Erase; solid grounds stone, water, brick, lava | one drag is one Undo step; solids block walking |
| 14.2 | Place | Characters, animals (6 pages), Arms, Plants (153, paged), Fx, Lights | all place, select, drag, **R** turn, Delete |
| 14.3 | Undo and Redo | Ctrl+Z, Ctrl+Y over 100 steps; across map, graph and NPC edits | one stack, no corruption |
| 14.4 | Save and backups | Ctrl+S three times | `.bak1` to `.bak3` exist; old versions (1 and 2) still open |
| 14.5 | Level settings | Name, Width, Height (8 to 256), default ground, New, Open, Close | shrinking cuts and Undo restores; unsaved-changes prompt: Save, Discard, Cancel |
| 14.6 | Sky preview | Sky button and slider | preview only, not saved, not an Undo step |
| 14.7 | START marker | Drag it, F1 | hero starts there |
| 14.8 | Plant overrides | Select plant, Own: `gather.delay=60` | that bush regrows in 60 s; a bad value is named and changes nothing |
| 14.9 | Compatibility | Open every shipped level (`valley`, `level-1` to `level-5`, `camp`, `range`) | all load; save and reload gives the same text |

### 4.15 Graph editor (M9: US-170 to US-175)

In the Editor press **Talk** (dialogue files) and **Rules** (interaction files). Esc returns to the map.

| # | Feature | How to check | Expect |
|---|---|---|---|
| 15.1 | Canvas | Right-drag pan, wheel zoom, left-drag cards, box select, frame all | zoom clamps 25 to 400 percent around the pointer; stays smooth with 200 cards |
| 15.2 | Dialogue cards | Open `elder-fire.dlg`: Node, Line, Choice, If, Do, Note, Goto | cards map to the file; ports wire as in the guide table |
| 15.3 | Round trip | Open, Save with no change, diff the `.dlg` | no change except canonical form; `#` notes kept; `.dlg.bak` written |
| 15.4 | Layout sidecar | Move cards, Save, reopen; delete the layout file | positions return; without it, a neat left-to-right layout |
| 15.5 | Interaction graph | Open `gather.json` in Rules | Actor, Verb, Target, Needs, Effects, NPC rule, Chronicle cards; Save runs the real loader first |
| 15.6 | Undo | 10 graph edits, then Ctrl+Z ten times | back to start; redo returns |
| 15.7 | Save refusals | Make a graph the loader refuses | first mistake named, nothing written |
| 15.8 | Changed on disk | Edit the `.dlg` in a text editor while the graph is open, Save | asks before overwriting |
| 15.9 | Check list | Create unreachable node, dead end, unknown node, unknown item, unknown need | listed live; error versus warning as in the table; click jumps to the card |
| 15.10 | New and Tidy | Type a name, New, Save; Tidy | new file written only on Save; taken names refused; Tidy is one Undo step |
| 15.11 | Attach | Select a placed NPC: Talks with, Script, Pick, Graph | picks save in the level; old levels load unchanged |
| 15.12 | Test-play | Test button: `opinion=25 item.berries=2 time=night season=winter`, Play, From here, Leave, Stop | log lists lines, choices and effects; **nothing is saved** (compare level and save files before and after) |
| 15.13 | Bad test word | Type an unknown word | the word is named and nothing starts |
| 15.14 | Authoring trial | Author a new 6-node conversation and a new interaction from scratch, attach to an NPC, F1 | works in play without hand-editing a file; **note every friction point** (this is the owner-value question of M9) |

## 5. Pass C: stress, soak, robustness

| # | Check | How | Expect |
|---|---|---|---|
| C1 | 10-minute performance | 500 people, 1080p, High, Release, F3 overlay | 59 to 60 FPS, no hitch growth |
| C2 | Soak, M9c row 6 | `ctest --preset windows-x64-release -L soak` overnight | passes; same seed gives the same save hash twice |
| C3 | Long play | 3 in-game hours, then 10 in-game years headless | no memory growth, no crash |
| C4 | Debug ASan | Play 30 minutes in Debug, use Editor, graph editor, trade, build | no ASan report |
| C5 | Save abuse | Kill the process during an autosave | next start loads the newest intact save; three backups kept |
| C6 | Bad data | Break one JSON in `assets/data`, `assets/data/npcs`, `dialogue`, `buildings`; start and F5 | each error names file and field; game still runs |
| C7 | Odd hardware | Run on a second PC or laptop if available; also on integrated GPU | starts, fallback renderer works, record FPS |
| C8 | First frame | Cold start 5 times in Release | under 3 s every time |
| C9 | Resize and focus | Alt-Tab in Exclusive, minimise and restore, change monitor | no black screen, no input stuck |

## 6. Known gaps (do not log as bugs)

- Forcing random rolls in test-play is not offered (the dialogue language has no roll).
- The hero cannot hit rival buildings (they are lists, not cells).
- The Editor keeps its 1x layout inside the 960x540 view (M8b note).
- Quests, data editors, world editing, politics and technology are M10 to M14.
- A graph card that nothing leads to is not saved to the text; Save says so.
- Anything under `docs/gates/test-debt.md` that is marked open.

## 7. Bug sheet and exit

Use one row per finding:

`ID | section row | severity | steps | expected | seen | log file | screenshot | build (Debug/Release, commit)`

| Severity | Meaning |
|---|---|
| S1 | crash, data loss, cannot continue, corrupted save or level |
| S2 | feature does not work as designed, wrong results |
| S3 | looks or feels wrong, confusing, slow but usable |
| S4 | idea, polish, wording |

**Exit.** Beta passes when: pass A is clean; no S1; every S2 has a story or decision request; section 4.15 row 14 (authoring trial) and 4.6 row 7 (is it a story?) have your written verdict; the owner rows listed in section 1 are closed with evidence in `docs/evidence/beta-1/`. Then Dominus sorts the findings into fix stories, the Codex gets an amendment from Anima if any prompt needs a new version, and M9 fixes run before K-M10.

**Playtesters (M6 row 10).** Once you are satisfied, give 3 to 8 people the zip from 4.1 row 11, the tutorial only and 30 minutes, then ask the questions of `docs/plans/M6-playtest-plan.md`.
