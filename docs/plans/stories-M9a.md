# Story plans: M9a

Per-story plans for milestone M9a, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-260](#us-260)
- [US-261](#us-261)
- [US-262](#us-262)
- [US-263](#us-263)
- [US-264](#us-264)
- [US-265](#us-265)
- [US-266](#us-266)
- [US-267](#us-267)
- [US-268](#us-268)
- [US-269](#us-269)
- [US-270](#us-270)

---

<a id="us-260"></a>

## US-260 manual checks (NPC Classes)

1. **Create.** F1 into the Editor, click **Class**, **New**, Id `healer`, Label `Healer`, Colour `#3a9a4c`, click **Icon** until `cross`, Tags `healer`, **Save**. `assets/data/npc-classes/healer.json` exists with the fields in the guide order; the list shows `healer`.
2. **Edit.** Pick `healer` in the list, change Label, **Save**: the file changes.
3. **Delete.** Delete `healer`: the file is gone. Try to delete `trader` after placing an NPC with that class by hand in a level: refused, with the NPC names in the status line.
4. **Mistake.** Change an icon to `banana` in a class file and restart: the log says `npc-classes/<id>.json:<line>: unknown icon`; the other classes load. Fix it and press **F5**: it loads.
5. **Screenshot.** `odysseus.exe --screenshot x.bmp --quit-after 1` in the Editor with the Class panel open, saved in `docs/evidence/US-260/`.

---

<a id="us-261"></a>

## US-261 manual checks (kind defaults and placed-NPC overrides)

1. **Kind files.** `assets/data/npcs/goblin.json` says `monster` and `hostile`; in the game goblins still fight as before.
2. **Override.** In a copy of a level add `"attitude": "friendly"` to one placed goblin, start the game: the goblin still fights (attitudes act from US-264); the log shows no mistake. Put `"attitude": "grumpy"`: the game stops naming the level file and `characters[n].attitude`.
3. **F5.** Change `wanderer.json` attitude to `wary`, press F5 in the game: no message (resolution is checked by the tests; the panels of US-268 show it).
4. **Old level.** Open `assets/levels/camp.json` in the Editor, save: `git diff` shows no change from the NPC fields.

---

<a id="us-262"></a>

## US-262 manual checks (placed NPCs are persons)

1. **Person.** Open a level with a placed wanderer and a goblin, run the game for a few minutes (a day is two minutes). The log shows nothing new; the tests show the wanderer's age and needs changing. The goblin fights as before.
2. **Creatures.** A placed deer and a goblin of class `monster` stand and behave as before (no change in the game).
3. **Save.** Play a level with a clan across one day: `npcs.json` appears in the save folder next to `clan.json`; load the save (the load key): the people are as saved.
4. **Meeting.** Walk up to a placed wanderer and wait a second: it remembers (test `US-262 Meeting`).

---

<a id="us-263"></a>

## US-263 manual checks (the NPC store and detail by distance)

1. **Load.** `odysseus_sim_tests -tc="US-263 Load*"` in Release prints the day, save and load times of 100,000 persons; keep them in `docs/evidence/US-263/npc-load.txt` and compare with ADR-022.
2. **Frame.** `odysseus_game_tests -tc="US-263 Frame*"` in Release: worst update under 16 ms with 100,000 far persons loaded. For the eyes: start any level; frame times show no change from the placed people.
3. **Near and far.** Walk toward and away from a placed wanderer for a few game days: no jump (the test `US-263 Near and far` checks the numbers).

---

<a id="us-264"></a>

## US-264 manual checks (attitudes and opinions)

1. **Title.** Place a goblin and a wanderer, right-click the goblin: the menu title reads `Name (hostile)`.
2. **Fight switch.** In a copy of a level give a placed goblin `"attitude": "friendly"`: it no longer fights. Give a placed wanderer `"attitude": "hostile"`: it fights.
3. **Family.** Give two placed wanderers `"family": 4` and load: the log shows no mistake (the same-family opinion is checked by the tests).
4. **Data.** Change `gift` in `opinions.json` and restart: the amounts follow; a wrong value stops the game naming file and field.

---

<a id="us-265"></a>

## US-265 manual checks (talk with placed NPCs)

1. **Talk.** In a level with a placed wanderer of a class whose `dialogues.player` names a script, walk to within 2 m, right-click it: the title is `Name (attitude)`, Talk is offered; choose it: the panel opens and the world stops.
2. **No script.** A placed wanderer without a player dialogue: the right-click menu has no Talk.
3. **Choices.** In the conversation pick a choice that changes opinion (`opinion npc hero 5`): the next time the line that reads the opinion changes.
4. **Clan.** In a run, talk to a clan member: the same conversation as before.

---

<a id="us-266"></a>

## US-266 manual checks (confront)

1. **Menu.** Right-click a placed NPC: the menu has Talk (when it has a dialogue) and "Confront..." and none of the five. Choose "Confront...": Taunt, Insult, Ask for peace, Antagonise, De-escalate.
2. **Key.** Stand within 6 m of an NPC and press **C**: the same five. Press C with nobody near: the status line says there is no one to confront.
3. **Insult.** With two friends (same `"family"`) near an NPC, insult it; then talk to a friend: its title attitude has worsened (the numbers are checked by the tests).
4. **Fight.** Hit a goblin once (it winds up), press C, De-escalate: it stops (70 in 100); Antagonise a peaceful wanderer: it strikes back.
5. **Data.** Change an amount in `insult.json`, press F5, insult again: the new amount.

---

<a id="us-267"></a>

## US-267 manual checks (actions and the Actions pop-up)

1. **Hidden.** Add an interaction for the `trader` tag whose `requires` needs `opinion(npc, hero) >= 10` with the `else` "needs: friendly or better" (see `docs/guides/npc-data.md`). Place a trader with attitude `wary`: right-click it: the action is not in the menu.
2. **Pop-up.** Press **X** next to the trader: the pop-up lists the action greyed out with "needs: friendly or better". Give the trader a gift (or set its attitude to friendly): the action can be chosen.
3. **Lists.** Put the action id in the trader's `actions.deny`: it is gone from both. Put it in the `actions.allow` of a plain wanderer: it is offered.
4. **Too far.** Walk away 12 m: an action with range 8 stays in the menu greyed out with "Too far away".

---

<a id="us-268"></a>

## US-268 manual checks (Editor NPC panel)

1. **Panel.** F2, Select tool, click a placed wanderer: the NPC panel opens under the properties. Click a goblin: it opens too (it has a kind file); click the hero's start: it does not.
2. **Edit.** Tick trader and elder, untick talker, click Attitude until friendly, type a script name, Ctrl+S. Open the level file: only `classes`, `attitude` and `dialogues` were added to that character. F1: the right-click title shows `(friendly)`.
3. **Actions.** Click `talk` in the actions list: it shows `[ ]`. F1: the NPC's menu has no Talk.
4. **Undo.** After five changes press Ctrl+Z five times: the NPC is as it was.
5. **Screenshot.** `docs/evidence/US-268/`: the panel open on a wanderer.

---

<a id="us-269"></a>

## US-269 manual checks (Editor kinds tab and map markers)

1. **Tab.** F2, click **Class**: the panel has two tabs, **Classes** and **Kinds**. Click **Kinds**: a list of every character and animal kind (the hero is not there; a kind with no file says `(no file)`).
2. **Kind.** Click `goblin`: its classes (`monster` ticked), attitude `hostile`. Click **Attitude** until `neutral`, click **Save**: the status line says `saved kind goblin`, and `assets/data/npcs/goblin.json` holds `"attitude": "neutral"`. Place two goblins, give one the attitude `scared` in the NPC panel, press F1: right-click the first goblin: its menu title says `(neutral)`, the second says `(scared)`.
3. **Fields.** Tick `trader` too, type `market` in Tags, `player=ossa.dlg` in Talk, `barter` in Deny, Save: the file has them in the guide's order (kind, classes, attitude, tags, dialogues, actions).
4. **Mistake.** Type `player` (no `=`) in Talk: the status line says why and the draft keeps its old value.
5. **Markers.** Place a wanderer with the class `trader`, one with `guard` and a goblin: under each there is a ring (gold, grey, red) with a coin, a shield and a skull. Give one NPC two classes (trader, guard): the ring is half gold, half grey, the icon is the coin. Press F1: no ring anywhere. F2: they are back.
6. **Screenshots.** `odysseus.exe --screenshot` of the Editor with the Kinds tab open and of the three markers, into `docs/evidence/US-269/` (owner, with the GPU).

---

<a id="us-270"></a>

## US-270 manual checks (the NPC test level)

1. **Start.** `odysseus.exe --level assets/levels/npc-test.json`: seven characters in a row along a path, the log shows `0 error(s)` for NPC classes and interactions and no dialogue error.
2. **Follow the walk-through** in `docs/guides/npc-data.md` (the table "The test level and its walk-through"), NPC by NPC, west to east. Every NPC offers exactly the actions its classes and overrides allow; the Actions pop-up (**X**) explains the greyed ones; confronting Gur lowers opinions; the goblin attacks.
3. **Editor.** F2: a ring and icon under each of the seven; **Class**, **Kinds**, `goblin`: attitude neutral, **Save**, F1: the goblin no longer attacks. Set it back to hostile and Save.
4. **Screenshots.** `odysseus.exe --level assets/levels/npc-test.json --screenshot x.bmp --quit-after 3` into `docs/evidence/US-270/`.
