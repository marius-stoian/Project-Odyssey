# M10b walkthrough: Editor help and live data (about 10 minutes)

For the owner (D-58 Q11). You need the game built from `qa` (or the `m10b-done` candidate), a text editor such as Notepad, and a fresh copy of your level if you want to keep `valley.json` as it is (this script saves the level once; the last three saves are kept as `.bak1` to `.bak3`). Say **Pass** or **Fail** with notes at the end; that is the only stop of M10b.

Keys you use: **F2** opens the Editor, **F1** plays, **F5** reloads every data file, **Ctrl+S** saves the level.

## 1. Tooltips (2 minutes)
1. Start `odysseus.exe`, press **F2**.
2. Click **Level**. Rest the pointer on **Name**, **Width**, **Height** and two more fields, about half a second each. A small box appears with what the field is for, its range and an example. Move the pointer away: it goes.
3. Click **Select**, click a placed person. Rest the pointer on **HP**, **Sword**, **Script** and **Does** of its panel (the Trade, Day, Night and Does rows are further down).
4. Open the **Build** panel (lesson 11 of `docs/guides/editor-tutorial.md`), pick a building and rest the pointer on five of its fields; then the **Prefab** tab.
5. Click the **Rules** tab and the **Dialogue** tab (lessons 16 and 17). Rest the pointer on five fields of the cards: **says:**, **range**, **tags**...
6. Open the **Events** list and rest the pointer on **who** and **says**.

**Pass if** every field you tried explains itself with a purpose, a range and an example, and a button still shows its own hint.

## 2. Suggestion lists (2 minutes)
1. Select a placed person. Click **HP**: a list opens with the default of its kind, the smallest and largest value, and the numbers you typed here before. Type `9`: the list narrows. Press **Down** twice, then **Tab**: the number is taken.
2. Click **Script**: a list of the `.dlg` files of the dialogue folder opens. Press **Esc**: the list closes and your text stays. Open it again and click a row.
3. Click **Class** and then a field of words such as **Does** or **Tags**: type `ga`, press **Down**, **Enter**: the word after the last comma is completed and the ones before it stay.
4. In the NPC panel the **Classes** of a person are ticked in a list (they are not typed): tick one and see it listed. (The Codex text calls this a text field; see the note under Exit result.)

**Pass if** every list opens on focus, narrows as you type, answers Up, Down, Tab, Enter and Esc, and a click takes a row.

## 3. Live data from a text editor (3 minutes)
1. Press **F1** to play. Find a campfire light (or place one with **Light** first).
2. Leave the game open. In Notepad open `assets/data/light/lights.json`, change the `"color"` of the `campfire` line (for example `[255, 60, 40]`) and save. **Within about a second** the fire changes colour and a toast says `Reloaded lights`.
3. Open `assets/data/objects.json`, copy one object line, give the copy the `"name"` `tut-stone`, save. Press **F2**: the object page of the palette lists `tut-stone` (no restart). Remove it again and save: a placed `tut-stone` would show a red **?**.
4. Open an interaction file, for example `assets/data/interactions/gather.json`, change a number and save: the toast says `Reloaded interactions`. Now break the file (delete a quote) and save: a red panel at the top names the file and the line, and the game keeps playing with the last good data. Fix the file: the panel goes.
5. In the Editor open the **Rules** tab, change an interaction and press **Save**: the change is live at once and you see no second reload from the watcher.

**Pass if** each change shows in the running game within about a second, a mistake never half-applies, and the red panel names the line.

## 4. A moved bush wins over the run save (3 minutes)
Your level needs the clan switched on (the **Level** panel) so that a run exists and autosaves.
1. Start `odysseus.exe --new-game`, begin a run and play until a new game day starts (about two minutes; the game autosaves when a day begins, and the log line says `Autosaved`). Pick berries from one bush. Close the game.
2. Start `odysseus.exe --editor`, **Select** another berry bush, drag it somewhere else and press **Ctrl+S**. Close the game.
3. Start `odysseus.exe --load`. The status line says `Loaded clan.json: day 1. The level updated 1 thing`. The moved bush stands in its new place with berries on it, the bush you picked from is still picked, and the clan and the hero are as you left them.
4. Press **F2**, delete a placed person, save, press **F1**: the run is loaded again from its save, the person is gone and the status line counts the update.

**Pass if** the moved bush is fresh in its new place, the others keep their state, a deleted person is gone and nothing else about the run changed.
## Your answer
Pass or Fail, and what you noticed (anything that felt slow, confusing or wrong):
