# Story plans: M8d

Per-story plans for milestone M8d, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-250](#us-250)
- [US-251](#us-251)
- [US-252](#us-252)
- [US-256](#us-256)

---

<a id="us-250"></a>

## US-250 manual checks (building pieces and kinds as data)

1. Start the game: no message about buildings. Open `assets/data/buildings/kinds.json`, change `"footprint": [3, 3]` of the hut to `[0, 3]` and start again: the message names `buildings/kinds.json:` and the line; the other kinds still load. Put it back.
2. F2, then the **Build** tool: the palette lists Hut, Windbreak, Storage pit, Drying rack, Palisade. Place a hut: the placeholder hut stands in the level. Save, close, start again: it is still there (the level file has its own `buildings` list).
3. In `pieces.json` give `wall-wood` the material `stone`: the message says the material is not in the table.

---

<a id="us-251"></a>

## US-251 manual checks (building from blueprints)

1. Press **B**: the Build menu opens on the left with *Hut* and *Windbreak* (D-55). Click *Hut*: a ghost follows the pointer; green where it can stand, red over water or rock or a tree. **R** turns it.
2. Click on the ground: a see-through blueprint with a bar and a line "Hut: needs ...". Right-click it: *Bring materials*, *Build* (greyed: "Bring the materials first"), *Cancel*.
3. With a run: gather wood, herbs and fur (the hut needs 14 wood, 10 herbs, 1 fur), choose *Bring materials*, then *Build* again and again (four seconds each): the bar fills; the finished hut appears, its walls block you, its door lets you in.
4. *Cancel* a blueprint you brought materials to: the materials lie on the ground; walk over the spot to take them back.
5. Without a run (a plain level) the materials are free.

---

<a id="us-252"></a>

## US-252 manual checks (building piece by piece)

1. **B**, tab *Pieces*: pick *Wooden wall* and place a ring of walls round an empty 3 x 3 square with the middle left free; place a *Wooden door* in one cell of the ring and a *Thatch roof* over the middle. Build each (right-click, bring materials, build).
2. When the last piece stands, the roof over the middle fades when you step inside: it is a room (warm and sheltered). Walk round it: you can only get in through the door.
3. Leave out the door or the roof and the space is no room (no fade).

---

<a id="us-256"></a>

## US-256 manual checks (prefab editor and placing buildings)

1. F2, **Prefab**: pick *Wooden wall* and click the grid to draw a ring, a *Wooden door*, a roof; set the Id (`my-lodge`) and Save. The status line says `saved prefab my-lodge`; the file is in `assets/data/buildings/prefabs/`.
2. The **Build** tool lists `* my-lodge`. Click the map: the finished lodge stands there. Select it: turn it, change *Interior* (the kind's own, fade, map), set an owner, delete it; each is a step of Undo (Ctrl+Z).
3. F1: the lodge stands in the game, finished. With *Buildable* and *Known at start* ticked it is in the Build menu (B) too.
