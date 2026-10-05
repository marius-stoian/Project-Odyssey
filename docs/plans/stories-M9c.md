# Story plans: M9c

Per-story plans for milestone M9c, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-290](#us-290)
- [US-291](#us-291)
- [US-292](#us-292)
- [US-293](#us-293)
- [US-294](#us-294)

---

<a id="us-290"></a>

## US-290 manual checks (day and night schedules)

1. **Places.** F2, **Level**, **Economy...**: type `market=20,10` in **Places** and press Enter: the status line says `places: market=20,10`; Ctrl+Z removes it. A place that does not fit (`home=1,1`, `x=999,1`) is refused with the reason.
2. **Schedule.** Select an NPC; in the Trade section type in **Day**: `06:00 work market; 21:00 sleep home`, press Enter (status: `schedule day`). Save, F1.
3. **Follow.** Speed the game up (the speed keys). At 06:00 the NPC walks to the market; at 21:00 it walks home and sleeps. Right-click it at the market: its menu is there.
4. **Interrupt.** Let an NPC get very hungry (or set `eat.below` high in `schedule.json`, F5): it walks home, eats and returns to its place.
5. **Danger.** Put a goblin within 6 m of a scheduled NPC: at the next hour it goes home; remove the goblin (the Editor) and it resumes.
6. **Night.** Give an NPC a **Night** line; after 21:00 it follows the night blocks.
7. **Mistakes.** `6am work`, `06:00` alone, two blocks at one time, an unknown place: each is explained (the unknown place in the log when the level loads).
8. **Far.** Walk far away from the NPC and wait a day: it is where its schedule puts it when you come back, with no jump of needs (ADR-022).

---

<a id="us-291"></a>

## US-291 manual checks (action sources)

1. **Class.** F2, **Class**, `guard`: the **Does** line says `patrol`. Place a guard and two places tagged `post` (**Level**, **Economy...**, **Places**: `gate=20,10/post tower=8,20/post`). F1: at the next hour the guard walks to a post; the hour after, to the other.
2. **Custom.** Select a plain wanderer, type `patrol` in **Does** (NPC panel, Trade section): it patrols too. A typo (`Patrol`) is refused with the reason.
3. **Event.** Place a talker 5 tiles from a fire pit. F1, light the fire (a fire drill from your bag): at once the talker walks to the fire. A hunter 20 tiles away stays where she is.
4. **Not on duty.** Give the guard a schedule whose block is `sleep`: it does not patrol while it sleeps.
5. **Deny.** Put `patrol` in the guard's **Deny** (Actions panel of the NPC): it stops patrolling.
6. **Log.** Type a `does` that is no interaction in a level file: the log names the NPC and the id.
7. **No quests.** The Does line has class, custom and event sources only (D-54 Q11).

---

<a id="us-292"></a>

## US-292 manual checks (NPCs act on each other)

1. **Talk.** Put two wanderers side by side (3 tiles apart, away from the hero's camp) and F1. Within an in-game hour a few words show in a bubble over one of them; their opinions of each other have changed (select one in the Editor: the Actions pop-up shows the attitude word).
2. **Trade.** Give one `"trade": { "stock": { "flint": 3 }, "wants": ["fur"] }` and the other `"stock": { "fur": 2 }, "wants": ["flint"]`: after an hour the stocks changed (the Trade screen of each shows it; the log says `NPCs: ... traded`).
3. **Gift.** Give one goods and an opinion of 60 of the other (a family id they share starts at +20; help by Editor attitudes): it gives a gift.
4. **Fight.** A feud grows by itself: two persons who dislike each other confront each other hour after hour (`npc-confront`) until one is hostile, and then they fight. The opinion between two persons has no Editor control yet; `tests/game/npc_dealings_game_test.cpp` sets it directly. When the fight runs the weaker is gone from the world: its figure leaves and nobody can talk to the dead.
5. **Witnesses.** Put a third person who knows the victim within 12 tiles: after the fight it thinks less of the killer.
6. **Far.** Walk far away from two hostile persons and wait a day: when you return one of them may be dead, with no animation (the log names it).
7. **Budget.** In `schedule.json` set `maxPerHour` to 1 with many persons: one dealing an hour; the turn goes round.

---

<a id="us-293"></a>

## US-293 manual checks (default interactions by partner type)

1. **Types.** Add `"buildings"` to `types` in `assets/data/sim/partner-types.json`, restart: in the NPC panel (Trade section beside it), **Defaults with:** cycles through player, animal, environment, buildings, one `class:<id>` for each class and `class`.
2. **Environment.** In the Editor (Level, Economy..., Places) add `grove=15,10/forage hut=5,18/shelter pond=25,18/water shrine=12,20/shrine`. F1: a hungry NPC goes to the grove, a tired one to the hut, a lonely one to the shrine (watch with the speed keys); the log shows no problem.
3. **Animals.** Select a wanderer, give it the class **hunter**, place a deer 5 tiles away: at the next hour the hunter walks to the deer; half the hunts kill it (the deer disappears).
4. **Override.** Type nothing in **Does** under **Defaults with: environment** of an NPC and press Enter, or type `pray` only: it does only that for the environment; clear the line and the defaults come back.
5. **Class default.** Under **Defaults with: class** give a talker `npc-chat`: it chats more eagerly with other classes.
6. **Mistakes.** `Repair` (capitals) in a Does line is refused; a `partnerActions` key that is no type in a level file is refused with the file and the field.
7. **Defaults files.** Break `defaults-animal.json` (wrong `partnerType`): the log names the file and line; F5 after fixing it.

---

<a id="us-294"></a>

## US-294 manual checks (living test level)

1. Open `assets/levels/npc-test.json`, press F1 and speed 4; follow the table in the guide ("The living test level and the soak") hour by hour: the market at 06:00, the swap of Tala and Harn at the market, everyone home at 12:00, Vell at the shrine at 10:00, everyone asleep at 21:00.
2. Right-click Tala at 09:00: the trade screen shows more berries and less flint than at the start.
3. Watch for the speech bubbles of Vell, Ossa and Tala.
4. Take screenshots with `odysseus.exe --screenshot` at 07:00, 11:00, 15:00 and 22:00 into `docs/evidence/US-294/` (GPU, owner only).
5. Soak: `ctest --preset windows-x64-release -R odysseus_sim_soak`; the message line shows the worst day, the worst tick and the save hash; run it twice and compare the hash.
