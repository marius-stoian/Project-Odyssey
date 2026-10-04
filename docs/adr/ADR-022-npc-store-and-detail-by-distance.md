# ADR-022: A compact NPC store, a spatial grid and detail by distance

Status: Accepted (owner decisions D-52 Q-06 and S-02, 2026-10-04; technical choices by Dominus in US-262 and US-263; requirements v2.10, NFR-08).

## Context
The owner wants up to 100,000 region-loaded NPCs in the end game (D-52). The clan simulation (`sim::World`) cannot do that: every `Person` there is a large object with its own vectors (memories, grudges) and an opinion table the size of the whole population, and every person is simulated every day with the clan's store, courtship and feuds. 100,000 of those would need gigabytes and O(n^2) work.

## Decisions
1. **A second population, not the clan.** The placed people of a level and the crowds of a region live in `sim::NpcPopulation` (`src/sim/npc_population.*`). The clan keeps `World` unchanged (its determinism hash and saves are untouched). A person of the population has needs, age, a family id, a position and the last six memories; opinions between persons come in US-264 as a sparse table of pairs that have met.
2. **Struct of arrays.** One `std::vector` per field (id, kind, age, family, x, y, needs, hours applied, note count and head, a ring of six notes); the index of a person is the same in every array. No object and no heap block per person; kinds and memory texts are interned (a number refers to a shared text). About 80 bytes a person plus the grid and the id lookup: 100,000 persons are about 15 MB.
3. **A spatial grid.** Cells of 256 pixels (8 tiles) hold the indices of the persons in them; `near(x, y, radius)` visits only the cells the circle touches and returns indices in ascending order, so the answer never depends on the grid's insertion order. Nothing walks the whole store to find who is near (the O(n^2) trap: 100,000 persons looking at each other is 10 billion pairs a day).
4. **Detail by distance.** The focus is the hero. Every game hour (100 ticks) the persons within the near radius (800 pixels, 25 tiles, more than the half diagonal of the 960 x 540 screen) are simulated for that hour: their needs fall by the hour's share of the daily rate. Everyone else is left alone until the day ends, when **every** person is brought to the end of the day, ages one day, is restored by what the land, the fire and the neighbours give if a need ended low, and remembers the day. The fall through hour h of a day is `rate * h / 24` (whole numbers), so any way of splitting the day gives the same total: a person who walks in or out of the radius, or is far all day, is the same at the end of a day. Mid-day, a person who becomes near catches up the hours they missed at once.
5. **Persistence.** `npcs.json`, versioned JSON (version 2), one short array per person (about 47 bytes a person), written with the autosave by `writeSaveText` (temporary file, rename, three backups). The grid is rebuilt when loading.
6. **Determinism.** Whole numbers only, no random numbers, no unordered iteration that matters (the id lookup and the grid are only looked up; `near` sorts). The population has its own hash over every field; the same ticks and the same focus give the same hash.

## Numbers (measured 2026-10-04, Release, developer PC)
| What | Measured | Budget (D-06 PC, Release) |
|---|---|---|
| 100,000 persons, one in-game day (2,400 ticks): all ticks together | 1.5 ms | 50 ms |
| the worst single tick (the day-end tick, which touches everybody) | 1.0 ms | 8 ms |
| save of 100,000 persons | 16 ms, 4.7 MB | 100 ms (the whole autosave must stay under 200 ms, US-080) |
| a minute of play with 100,000 far persons | see `docs/evidence/US-263/` after X-M9a | worst update under 16 ms |

The budgets leave a margin of three or more over the measurement for the D-06 PC, which is slower than the developer PC.

## Consequences
- Far persons cost one pass over the arrays a day; near persons cost an hourly grid query. The frame never loops over the population.
- A person that moves must call `move` so the grid follows (schedules, M9c).
- Persons do not die in M9a (no food model); ageing past old age and births belong to later stories.
- Autosave size grows with the crowd (47 bytes a person); a binary format or a background write is the answer if a real region ever needs more than the 100 ms budget.
