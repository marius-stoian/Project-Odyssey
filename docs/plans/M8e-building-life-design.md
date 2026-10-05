# M8e design: building life (written at K-M8e, 2026-10-05)

Owner decisions: D-42, D-43, D-55 (docs/decision-requests/D-55.md). Design of the data and the store: docs/plans/M8d-buildings-design.md. A format change is a design question for the owner.

## K-M8e owner answers (2026-10-05)
| Question | Answer |
|---|---|
| Wear | Very slow: wear is almost off; buildings are damaged by events (fire, raids, storms) |
| Rivals | May damage or burn the hero's buildings only at war or feud |
| Interiors by default | Every roofed kind fades its roof; none opens an interior map unless the Editor says so |
| Fire | Spreads piece to piece by material (wood and straw burn, mud resists, rain slows and puts out); the clan fetches water and the hero can douse it |
| Tests | Written per story; the one full verify (Debug and Release) at X-M8e |

## 1. What already stands from M8d
The store has the fire, wear, damage and repair rules (`BuildingStore::advance`, `seasonEnded`, `damageAt`, `igniteAt`, `douse`, `repair`), rooms and `sheltered`, owners and contents, and a hash; `BuildingLayer::tick` ends seasons and days from the clan's clock and runs the fire with the weather's rain. The roof of the room the hero is in fades (`drawRoofs`). M8e adds who builds, who fights fire, interior maps, what hits buildings, and what buildings do for the clan.

## 2. US-253 Clans build together
- **Jobs.** A blueprint (state `waiting` or `ready`) is a construction job. Clan members choose among the interactions `deliver-materials` and `build-work` with the `npc` rule of their files (a score from the hunger-free, idle state: `trait(diligent)`, a base score above the minimum so an idle person helps); the same action runner and the same built-in actions as the hero. Technical choice (D-35 does not cover it): the clan's materials come from its *surroundings*: a clan member delivers up to 2 of each missing item per delivery, then rests (cooldown 60 s); the hero's bag is the hero's own.
- **Rivals.** `sim::RivalBuilders` (`src/sim/building_life.h`): once at the start of every season each rival clan with at least 6 people picks the first kind (in file order) whose use it lacks (sleep before store before work) and places it on the nearest free cell within 8 cells of its camp, then adds work every day until it is finished; seeded (the stream `buildings`), in the store's hash. A rival's buildings are faction n + 1.
- **Deterministic.** Everything is whole numbers and the store's own stream; `BuildingStore::hash()` joins the game's determinism hash.

## 3. US-254 Interiors
- **Fade.** Done in M8d: the roof pieces of the buildings that close the hero's room fade (interior mode `fade`). Also a building the hero stands in the footprint of fades its roof when it is not a room.
- **Map.** A finished building whose interior mode is `map` has the tag `enterable`; the interaction `enter-building` (door cell) stores the way back (the level file, the building id, the hero's cell outside) and loads the level named by `interiorLevel` (assets/levels/<name>.json, a level made in the Editor) as the level being played; the interior has a place named `exit` tagged `exit`; the interaction `leave-building` (on the place) brings the outside level back with the hero at the door. While inside, the outside world is kept (stashed) and the clan's clock goes on.
- **Setting.** The kind says, the placed building overrides (Editor); the game follows (`BuildingStore::interiorMode`).

## 4. US-255 Wear, repair, damage, fire
- **Wear** at each season end by the kind's `wear` table (D-55 Q5: only the hut and the windbreak wear, by 1 hp in winter). `repair` interaction (`do repair 10`, target tag `repairable`).
- **Damage.** A weapon that stops against a standing piece (a shot, an arc, a sword swing with nothing to hit) hits it (`damageAt`) when the building belongs to a rival; rivals at war or feud (the hero's relation to them at or below -50) raid the hero's buildings at the start of a season: one hit of 40 hp and, 1 time in 3, a fire. A fire weapon sets the piece alight.
- **Fire.** `BuildingStore::advance`: a burning piece loses hp over its material's burn time, spreads to neighbours once a second by the flammability of their material, goes out by itself in rain. The clan's persons use `douse-fire` (their `npc` score is high for a burning building within 12 m), the hero uses it from the menu. Presentation: a flame effect on a burning piece and a fire light.

## 5. US-257 Buildings in the clan's life
- **Owners.** At each dawn a finished building whose kind says `owner: person` and that has no owner is given to the lowest-id living adult without one (saved, shown in the Editor).
- **Warmth.** A housed person (an owner of a finished room, or a bed in a finished room with `sleep` in its uses: 4 beds a hut) loses Warmth at the `shelterWarmthPercent` (40) of the night rate. `World::setHoused` is set by the game each hour; it is not saved.
- **Storage.** A storage pit holds up to its `capacity` meals of the clan's store at half the spoilage (`World::setSpoilFactor`); the interaction `store-food` gives the hero's berries to the clan store. The pit's `contents` are the building's own list (for other goods and for saves).
- **Saved.** The store's JSON already holds owners, condition, contents and fire.

## 6. Order and tests
US-253, US-254, US-255, US-257, X-M8e. Tests are written per story; the one full verify runs at X-M8e.
