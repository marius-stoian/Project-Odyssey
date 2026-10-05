# Story plans: M8e

Per-story plans for milestone M8e, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-253](#us-253)
- [US-254](#us-254)
- [US-255](#us-255)
- [US-257](#us-257)

---

<a id="us-253"></a>

## US-253 manual checks (the clan builds)

1. Place a hut or windbreak blueprint (**B**) near the camp, not far from the clan's fire.
2. Wait. Clan members who are idle walk to the blueprint, bring up to 2 of each missing material from their surroundings (a clan member rests 60 s between deliveries) and work on it in four-second steps. The chronicle names the finished building.
3. Rival clans keep a list of buildings (`sim::buildings::RivalBuilders`): at each season start a rival with at least six people starts the first kind whose use it lacks and works on it every day.

**Technical choice (recorded for the owner).** Rival camps lie far from the level the hero plays, so a rival's buildings are a list of kinds and progress, not cells of the level. The hero meets their effect, not their walls. Raids on the hero's buildings (US-255) are real. If the owner wants rival buildings drawn on the map, that is a design question for a later milestone.

**Where to look.** `src/sim/building_life.h/.cpp`, `BuildingLayer::clanDeliver/clanWork`, `NpcLife::think`, `assets/data/interactions/deliver-materials.json` and `build-work.json` (their `npc` rules).

---

<a id="us-254"></a>

## US-254 manual checks (inside buildings)

1. A roofed kind fades its roof when the hero stands in its room (done in M8d).
2. In the Editor select a finished building, set *Interior* to `map` and the level name `interior-hut` (a sample level is `assets/levels/interior-hut.json`).
3. In the game right-click the building: *Go into Hut*. The inside level loads; the clan and the outside wait. Right-click the place named `exit` (tag `exit`): *Go outside*. The hero is back at the door.

While inside, the clan's clock goes on, but the outside buildings, fires and clan walkers wait (their world is kept and comes back unchanged). Autosave is skipped inside.

**Where to look.** `OdysseyGame::enterBuilding/leaveBuilding`, `Subject::Kind::Place` (a named place with tags can be right-clicked), `assets/data/interactions/enter-building.json` and `leave-building.json`.

---

<a id="us-255"></a>

## US-255 manual checks (damage, repair, fire)

1. A rival the hero is at war or at feud with (relation -50 or lower) strikes one finished building at the start of a season: 40 hit points and, one time in three, a fire. The chronicle and a message say so.
2. Right-click a damaged building: *Repair* (ten seconds of work each time). Right-click a burning one: *Put out the fire*; idle clan members do the same.
3. A fire weapon (fire element) that stops against a standing piece sets it alight. Burning pieces show sparks and shine like a campfire; finished buildings with a `light` shine at night.
4. Rain slows and puts out fires; wood and straw spread them, mud resists (D-55).

Weapons never hurt the hero's own buildings (only fire can). Rival buildings are not on the level (see US-253), so there is nothing for the hero to hit.

**Where to look.** `BuildingLayer::raidsAtSeasonStart/raidFrom/fireHit/clanDouse`, `OdysseyGame::lightSources`, `assets/data/interactions/repair.json` and `douse-fire.json`.

---

<a id="us-257"></a>

## US-257 manual checks (buildings in the clan's life)

1. Finish a hut. At the next dawn it is given to the lowest-id living adult without a home; the Editor panel shows the owner.
2. A housed person (an owner or one of four beds of a sleeping place) loses warmth at 40% of the normal rate (`shelterWarmthPercent` in `needs.json`).
3. Build a storage pit (not known at the start: `do learn-blueprint storage-pit` or the Editor makes it known). Right-click it with 2 berries: *Put berries in*. Meals in the pit (up to its `capacity`, 40) spoil at half the daily rate.

The tag `storage` is the building's store use (the old `store` tag stays the clan's food store place, so the old put-in-store interactions do not match buildings).

**Where to look.** `BuildingLayer::applyClanLife`, `World::setHoused/setStorageMeals`, `assets/data/interactions/store-food.json`.
