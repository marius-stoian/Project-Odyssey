# Story plans: M9

Per-story plans for milestone M9, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-170](#us-170)
- [US-171](#us-171)
- [US-172](#us-172)
- [US-173](#us-173)
- [US-174](#us-174)
- [US-175](#us-175)

---

<a id="us-170"></a>

## US-170 Node-graph widget: plan and manual checks

Design: docs/plans/M9-graph-editor-design.md section 2. Owner answers: D-56 Q5 (colour-coded cards), Q7 (right-drag pan, wheel zoom 25-400%), Q8 (layered layout), Q19 (one History).

### Automated (Debug, ran alone)
`luna_tests -tc="US-170*"` (9 cases, 71 assertions) and `odysseus_game_tests -tc="US-170*"` (1 case, the real History).

### Manual checks for the owner (later, with the Editor tabs of US-171)
The widget has no screen of its own yet; US-171 puts it in the Editor. Then check: right-drag pans; the wheel zooms around the cursor from 25 to 400%; a drag from a yellow output port to a white input port draws a wire; Delete removes the selected card and its wires; Ctrl+Z walks back.
Screenshots (`odysseus.exe --screenshot`) are taken from US-171 on, in docs/evidence/US-171/.

---

<a id="us-171"></a>

## US-171 Dialogue graph editor: plan and manual checks

Design: docs/plans/M9-graph-editor-design.md section 3. Owner answers: D-56 Q5-Q11, Q17-Q19.

### Built
`src/game/dialogue_graph.{h,cpp}` (script to cards and wires and back, rows layout, layout sidecar), `src/game/graph_editor.{h,cpp}` (the full-screen editor: file list, add-card bar, side panel, Save), the Editor's **Talk** button and key handling (`src/game/editor.*`), Save reloads the data like F5 (`odyssey_game.cpp`). Guide: docs/guides/dialogue-format.md, section "The graph editor".

### Automated (Debug, ran alone)
`odysseus_game_tests -tc="US-171*"`: 14 cases. The shipped `.dlg` files go to the graph and back as the same text; Open, Save, Hand edits and overwrite-asking scenarios; Undo through the one History; a mistake is reported and the file untouched.

### Not in this story
The Sub-conversation call card (D-56 Q10): it needs a new `.dlg` element and runtime support, which no Codex story covers (CI-014).

### Manual checks for the owner (later; picture in docs/evidence/US-171/dialogue-graph.png)
Press F2 for the Editor, click Talk, open `elder-fire`, drag a card, press Ctrl+S: the file keeps its `#` notes and `elder-fire.dlg.layout.json` appears beside it. Edit a line in Notepad, save, and in the game press F5 to see the new words.

---

<a id="us-172"></a>

## US-172 Interaction graph editor: plan and manual checks

Design: docs/plans/M9-graph-editor-design.md section 4. Owner answers: D-56 Q12, Q17-Q19.

### Built
`src/game/interaction_graph.{h,cpp}` (interaction to cards and wires and back, checked by the real loader; row layout; leading `//` comments kept), `GraphEditor` now serves two kinds (conversations and interactions; `Kind`), the Editor's **Rules** button. Guide: docs/guides/interaction-data.md, section "The graph editor".

### Automated (Debug, ran alone)
`odysseus_game_tests -tc="US-172*"`: 11 cases. Every shipped interaction goes to the graph and back as the same data; edits reach the text; mistakes are named and nothing is written; the id must match the file; Undo through the one History.

### Not in this story
Creating a new interaction file from the graph (the verb id is the file name), and comments inside the braces of a file (not kept by the canonical text; use the `note` field).

### Manual checks for the owner (later; picture in docs/evidence/US-172/interaction-graph.png)
F2, Rules, open `gather`, change the range on the verb card, Ctrl+S: `gather.json` keeps its top comment and shows the new range; press F5 and gather in the game uses it.

---

<a id="us-173"></a>

## US-173 Attach to placed things: plan and manual checks

Design: docs/plans/M9-graph-editor-design.md section 5. Owner answers: D-56 Q13 (inspector field with a pick list and an Open-in-graph button), Q14 (the level carries the change; the shared files stay untouched).

### What already stood (M9a, not rewritten)
A placed NPC already has a per-partner dialogue (`dialogues`, `setSelectedDialogue`, the *Script* field), saved as a difference only, and the game's `npcDialogueFor`. US-173 adds the pick list and Graph button to that row and leaves the rest as is.

### Built
- Pick and Graph buttons under the *Script* field of the NPC panel; `GraphEditor::dialogueNames`.
- Own values for a placed plant: `ThingOverride` and `PlacedPlant::overrides` (level.h), reading and writing in level.cpp (a plant with none writes none), `WorldPlant::overrides`, `sim::applyPatch` (interaction.h/.cpp), `ActionRunner::setAdjuster` (asked when an action starts and when it ends), `OdysseyGame` sets the adjuster, `Editor::setSelectedOverrides` and the plant panel.
- Guide: docs/guides/editor.md, "A conversation for a character, own values for a plant".

### Automated (Debug, ran alone)
`odysseus_game_tests -tc="US-173*"`: 6 cases (only the plant with its own regrow time waits 60 s; a duration of its own; saved and loaded with an unchanged file for a level without overrides; a mistake names the field; the Editor sets and undoes; a picked conversation is the one that starts and survives a save).

### Not in this story
Own values for things other than plants, and interaction fields other than `delay` and `duration` (the range is checked when the menu is built, which this story does not touch).

### Manual checks for the owner (later)
Place two bushes, select one with Select, type `gather.delay=60` in *Own*, save, press F1 and gather both: the first is ripe again after a minute, the second after 15 seconds.

---

<a id="us-174"></a>

## US-174 Test-play: plan and manual checks

Design: docs/plans/M9-graph-editor-design.md section 6. Owner answers: D-56 Q15 (settable values), Q16 (never touches the saved world).

### Built
`src/sim/test_play.{h,cpp}` (headless: `TestState`, `applyTestState` words, `TestWorld` that answers conditions and carries out effects on a state of its own, `TestPlay` over `Conversation`), `Conversation::jumpTo` (Play from here), and in `GraphEditor` the Test card (state field, Play, From here, Leave, Stop, Close, numbered choices, effect log). Guide: docs/guides/dialogue-format.md, "Test-play". The graph played is the one on screen, saved or not; it goes through `graphToDialogue`, so a mistake is named instead of played.

### Automated (Debug, ran alone)
`odysseus_sim_tests -tc="US-174*"` (7 cases) and `odysseus_game_tests -tc="US-174*"` (5 cases: Branch with opinion 25; Start anywhere with an unsaved edit; No side effects: file, undo steps, sidecars and save callback unchanged; a mistake in the words; the card drawn and a click on a choice). Picture: docs/evidence/US-174/test-play.png.

### Not in this story
- Forced rolls (D-56 Q15 last item): the dialogue and interaction languages have no random roll today; the haggle is a trade screen, not a conversation. The test world therefore has none, and CI-014 asks Anima whether the owner wants this once a roll exists.
- Test-play of an interaction (the Codex also names the action runner) is not built: the interaction graph shows and checks the rule, and the in-game menu plays it; say so if you want it.

### Manual checks for the owner (later)
Talk, open `elder-fire`, press Test, type `opinion=25 item.berries=1`, Play: choose "Offer berries" and watch the log. Select a card in the node `thanks`, press From here: the talk starts there. Close the editor: the level, the saves and the file are as they were.

---

<a id="us-175"></a>

## US-175 Graph validation: plan and manual checks

Design: docs/plans/M9-graph-editor-design.md section 7. Owner answers: D-56 Q17 (live, on save, in verify), Q18 (save with errors allowed).

### Built
`src/sim/graph_check.{h,cpp}` (headless: `checkDialogue`, `checkInteraction`, `GraphCatalog`), the finding list under the canvas in `GraphEditor` (live recheck, click selects and shows the card, errors and warnings counted in the status line, Save says how many errors remain), `OdysseyGame::syncGraphCatalog` (the game's items, built-in actions, conversations, interactions and tags). Guide: docs/guides/dialogue-format.md, "The check".

### Automated (Debug, ran alone)
`odysseus_sim_tests -tc="US-175*"` (7 cases, including every shipped file with the real item list) and `odysseus_game_tests -tc="US-175*"` (4 cases: the list, the click, saving with errors, the interaction graph).

### Manual checks for the owner (later)
Talk, open `elder-fire`, change `give`/`take` to an item name that does not exist: the list shows the item and the node at once; click the line and the card is selected; fix it and the line goes.
