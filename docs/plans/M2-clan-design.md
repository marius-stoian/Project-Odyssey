# Clan simulator design (M2: console clan simulator, Kill Gate 1)

Architect's and Game Designer's design for M2, shared by US-010..US-016. Codex v1.3; requirements v1.4 (ADR-004, ADR-006, ADR-010, ADR-011, ARC-07, ARC-08, MVP-12). Owner decisions D-01 (pausable real time with speed control), D-03 (needs), D-13 (libraries); delegated D-02 ([side characters](../decision-requests/D-02.md)).

## Principle
Everything in `src/sim/` (`odysseus::sim`), run headless by `odysseus_headless.exe` and the tests. No SDL3, no Luna (Charter rules 3 and 9; the build rejects it). Deterministic (rule 6): integers only for world state, all randomness from seeded PCG32 streams (one per system), no wall-clock time, no unordered-container iteration. Content numbers are data in `assets/data/sim/*.json`, validated at load with errors naming file and field (rule 7, ARC-08).

## Time (US-010)
- 20 ticks per second of game time (ADR-006). **1 day = 2400 ticks** (2 game minutes at 1x); **7 days per season, 4 seasons, 28 days per year** (`calendar.json`). A 100-year soak test is 6.72 million ticks.
- Speed control (D-01): paused, 1x, 2x, 4x: `GameClock` turns real time into ticks (4x = 80 ticks per real second). The headless runner simply runs as fast as it can.
- People decide what to do every 100 ticks (5 game seconds), not every tick; needs change once per game hour-equivalent (every 100 ticks) using daily rates / 24. All integer maths, remainders carried.

## People (US-011..US-013)
`struct Person` (plain data, a "component" in spirit; EnTT arrives in M3 per ADR-005): id, name, sex, age in days, alive, needs (4 x int 0-100), traits (bitset of 6), skills (gathering, hunting), current action, kinship ids, memories (vector), days at zero hunger/warmth. Opinions: a dense `vector<int>` of population x population (2D grid in 1D, like the tile map).

## Choosing actions (US-012, utility AI)
Each candidate action gets an integer score from the person's needs, traits, skills, age and the season; the highest wins, ties broken by a seeded random stream. Every score is kept for the last decision, so the headless runner can print it ("Inspectable"). Actions: Eat (from the clan store), Gather, Hunt (adults; small game or, rarely, a mammoth), Sleep, Warm by fire, Talk (Social, gossip), Give gift (Kind), Steal (Greedy, from the store), Rest, Wander (the fallback when nothing helps).

## Clan economy
One shared food store (integer units). Gathering yields less in autumn and almost nothing in winter; hunting is riskier but richer. Lean winters are where stories come from.

## Memories, gossip, relationships (US-013)
As decided in D-02: memories with who/what/when/feeling/importance; talking passes one missing memory at half strength; minor memories forgotten after 60 days; opinions move with memories; pairing and births; feuds.

## Chronicle (US-014)
Events have an importance (0-100). The chronicle keeps sentences like "Spring, year 3: Ura was born to Tok and Maa."; printing a year shows only events above the threshold (default 50): births, deaths with cause, pairings, feuds, first mammoth hunt, the clan's first starvation winter.

## World hash and saves (US-010, US-016)
- World hash: 64-bit FNV-1a over a canonical byte stream of the whole state (tick, RNG states, food, every person in id order, opinions, chronicle). Same seed + same inputs = same hash (the determinism test).
- Saves (ADR-010): versioned JSON (nlohmann/json, D-13), written to `<file>.tmp` then renamed; the previous file rotated to `.bak1..bak3`. Loading re-creates the exact state; the hash proves it. Older save versions are upgraded step by step, or refused with a clear message.

## Headless runner (US-015)
`odysseus_headless --seed 7 --years 100 [--chronicle] [--inspect <name>] [--save file] [--load file]` prints population, deaths by cause, average needs, tick time and, on request, the chronicle.
