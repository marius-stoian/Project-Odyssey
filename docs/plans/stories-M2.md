# Story plans: M2

Per-story plans for milestone M2, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-010](#us-010)
- [US-011](#us-011)
- [US-012](#us-012)
- [US-013](#us-013)
- [US-014](#us-014)
- [US-015](#us-015)
- [US-016](#us-016)

---

<a id="us-010"></a>

## Plan US-010: Advance a seeded world clock

Codex v1.3, prompt S-US-010. Design: [M2 clan design](M2-clan-design.md). Traces to ADR-006, ADR-011, ARC-07, MVP-12. Owner decision D-01 (pausable real time, speed control).

### Files
| File | Layer | What |
|---|---|---|
| `src/core/random.{h,cpp}` | Core | `Pcg32`: seeded, streamed, unbiased `below()`, `chance()`, state save/restore |
| `src/core/hash.h` | Core | `Hasher`: 64-bit FNV-1a for world hashes |
| `src/sim/data.h`, `json_data.{h,cpp}` | Simulation | `DataError` naming file and field; JSON reading helpers (nlohmann/json, PRIVATE to Simulation) |
| `src/sim/calendar.{h,cpp}` | Simulation | Seasons, `CalendarConfig` from `assets/data/sim/calendar.json`, `Calendar::dateAt(tick)` |
| `src/sim/game_clock.{h,cpp}` | Simulation | `GameClock`: pause / 1x / 2x / 4x, whole-nanosecond accumulator, 20 ticks per game second |
| `src/sim/world.{h,cpp}` | Simulation | `World`: ticks, calendar, daily weather from its own random stream, `hash()` |
| `apps/headless/main.cpp` | App | `--seed`, `--days`, `--data`; prints date, temperature, hash |

Calendar (data): 2400 ticks per day (2 game minutes at 1x), 7 days per season, 28 days per year.

### Tests (`odysseus_sim_tests`, Simulation identity)
| Scenario | Test |
|---|---|
| Calendar: seed 42, one in-game year: four seasons, day counter = year length | `US-010 Calendar` |
| Determinism: two runs, seed 42, 10,000 ticks: same world hash | `US-010 Determinism` (also: a different seed gives a different hash) |
| Speed control: at 4x, one real second = 4 game seconds | `US-010 Speed control` (also 1x, pause, and 144 Hz frames without drift) |
| Data validation (ARC-08) | `Content errors name the file and the field` |

### Verification results (2026-09-30)
Builds 0 warning lines; ctest 13/13 in Debug and Release ([windows-debug.txt](../evidence/US-010/windows-debug.txt), [windows-release.txt](../evidence/US-010/windows-release.txt)); sim tests 4 cases / 19 assertions; headless: seed 42, 28 days -> Spring, year 2, day 1 ([doctest-sim.txt](../evidence/US-010/doctest-sim.txt)). The determinism test now runs in every CI build (Charter Definition of Done from US-010 on).

Acceptor verdict (2026-09-30): **ACCEPT**.

---

<a id="us-011"></a>

## Plan US-011: Give every person needs that change over time

Codex v1.5, prompt S-US-011. Design: [M2 clan design](M2-clan-design.md). Traces to SDC-01 (candidate), Overview v0.1. Owner decision D-03 (Hunger, Energy, Warmth, Social); delegated D-02 ([side characters](../decision-requests/D-02.md): death after 3 days at zero Hunger, or at zero Warmth in winter).

Paused on 2026-09-30 for M1b (needs data, `Person`, `Chronicle` written); resumed after X-M1b on the same branch, merged with the latest `qa`.

### Files
| File | Layer | What |
|---|---|---|
| `assets/data/sim/needs.json` | Data | Maximum 100; daily decay Hunger 30, Energy 35, Warmth 20 (winter 45), Social 15; meal 40, sleep 60, fire 50, talk 25; 3 days at zero before death |
| `assets/data/sim/clan.json`, `names.json` | Data | 20 founders aged 2-45, 300 meals in store; 24 female and 24 male names |
| `src/sim/needs.{h,cpp}` | Simulation | `Needs` (4 whole numbers), `NeedsConfig` + loader; `hourlyDrop` (24 hourly drops that add up exactly to the daily rate), `dailyRate` (winter Warmth), `decayForHour`, `satisfy` (capped) |
| `src/sim/person.{h,cpp}` | Simulation | `Person` (plain data: id, name, sex, age, alive, cause of death, needs, days at zero), `causeName` |
| `src/sim/clan.{h,cpp}` | Simulation | `ClanConfig`, `NameList` + loaders, `pickName` (avoids names of the living), `makeStartingClan` (people random stream) |
| `src/sim/chronicle.{h,cpp}` | Simulation | `ChronicleEntry` (date, importance, sentence), `Chronicle::add` |
| `src/sim/calendar.{h,cpp}` | Simulation | `kHoursPerDay`, `ticksPerHour()`; `ticksPerDay` must be a multiple of 24 |
| `src/sim/world.{h,cpp}` | Simulation | The clan, food store and chronicle live in `World`. Needs decay every game hour (100 ticks); each morning people age, and a need at zero for more than the configured days kills (Starvation; Cold only in winter), recorded in the chronicle. The world hash covers people, food and chronicle |

Nobody eats, sleeps or talks yet (US-012 adds action choice), so until then everyone starves on the morning of day 8, which the Consequence test uses.

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Decay: full needs, one day without eating, sleeping, warmth or company -> each need dropped by its daily rate | `US-011 Decay`: in the real world after 2400 ticks: Hunger 70, Energy 65, Warmth 80, Social 85 for all 20 people; the 24 hourly drops add up to the daily rate for every rate 0..100; winter Warmth drops by 45 |
| Satisfaction: a hungry person eats a meal -> Hunger rises by the meal's value, capped | `US-011 Satisfaction`: 30 -> 70; 95 -> 100 |
| Consequence: Hunger at zero for the configured days -> the next morning they die, the chronicle records the cause | `US-011 Consequence`: all alive on the last evening (3 days at zero), all dead the next morning with cause Starvation; "Summer, year 1: Garu died of starvation." |
| Supporting | `US-011 The starting clan comes from data` (ages within the configured range, food store, same seed = same clan) |

Manual checks: none (headless).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-011` |
| ctest | 17/17 Debug and Release | [windows-debug.txt](../evidence/US-011/windows-debug.txt), [windows-release.txt](../evidence/US-011/windows-release.txt) |
| Sim tests | 8 cases, 394 assertions; the US-010 determinism test still passes with people in the hash | ctest logs |

Acceptor verdict (2026-09-30): **ACCEPT**. Decay, Satisfaction and Consequence pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-012"></a>

## Plan US-012: Let people choose what to do (utility AI)

Codex v1.5, prompt S-US-012. Design: [M2 clan design](M2-clan-design.md) (Choosing actions, Clan economy). Traces to SDC-01 (candidate utility AI). Delegated D-02 (the six traits).

### Files
| File | Layer | What |
|---|---|---|
| `assets/data/sim/actions.json` | Data | Work and hunting ages, night hours, meal hour, reserve days, spoilage, gathering yield per season, hunting odds and yields (small game, rare mammoths and their danger), per-hour effects of sleep, fire, talk and rest, score weights |
| `assets/data/sim/needs.json` | Data | Per-hour effects moved to `actions.json`; needs keep decay, meal value and death rules |
| `src/sim/actions.{h,cpp}` | Simulation | `Action` (Gather, Hunt, Sleep, WarmByFire, Talk, Rest, Wander), `ActionConfig` + loader, `isNight` |
| `src/sim/ai.{h,cpp}` | Simulation | `Situation` (hour, season, age, store pressure, someone awake to talk to, fire), `availableActions`, `scoreActions`, `decide` (highest score, ties by the seeded Decisions stream), `describeDecision`, `urgency` (grows with the square of what is missing) |
| `src/sim/person.{h,cpp}` | Simulation | Traits (D-02), gathering and hunting skills that grow with practice, the current action, the last `Decision` with every score |
| `src/sim/clan.{h,cpp}` | Simulation | Founders get 1-2 traits (never Brave and Timid) and skill from their years |
| `src/sim/world.{h,cpp}` | Simulation | Every game hour: needs decay, each person's chosen action pays off (food into the store, sleep, warmth, company, practice; hunting can meet a mammoth, which can kill), at hour 19 the clan eats together (hungriest first, one meal each while the store lasts), then everyone decides the next hour. Each morning 2% of the store spoils. `setDailyLife(false)` switches all of that off for the US-011 needs tests |
| `apps/headless/main.cpp` | App | `--inspect <name or id>` prints the last decision; population and food are printed |

Scoring (whole numbers): food work only when a belly is really empty or the store is below its reserve (10 days of meals, 20 in autumn); then skill and traits (Diligent, Brave, Timid) decide who gathers and who hunts. Sleep wins at night; WarmByFire, Talk and Rest follow the matching need; Wander is the last resort.

Balance (Dominus): a first version let well-fed people forage all day and met mammoths too often (24,514 meals in store after 10 years). Now only real hunger or a low store sends people out, mammoths are 2 per 1,000 hunting hours, and 2% of the store spoils daily. The store swings between about 200 meals in summer and 400 at the end of autumn, eaten down over winter.

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Pick best action: very hungry, slightly tired -> a food action (gather or hunt) scores highest and is chosen | `US-012 Pick best action`: Hunger 10, Energy 70 at noon -> Gather 164, Hunt 144, Sleep 27, Rest 14; at night the same person sleeps |
| No option: no reachable action satisfies a need -> wander or rest, no error | `US-012 No option`: a cold, hungry, lonely 3-year-old with no fire and nobody awake rests or wanders; with nothing at all possible the answer is Wander |
| Inspectable: print the last decision in the headless runner -> every action's score and the chosen one | `US-012 Inspectable` (`describeDecision`), and `odysseus_headless --inspect 0` ([headless-inspect.txt](../evidence/US-012/headless-inspect.txt)) |
| Supporting | `US-012 The clan feeds itself through its first year` (no starvation, 20 alive) |

Manual checks: none (headless).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-012` |
| ctest | 17/17 Debug and Release | [windows-debug.txt](../evidence/US-012/windows-debug.txt), [windows-release.txt](../evidence/US-012/windows-release.txt) |
| Sim tests | 12 cases pass, including the US-011 needs tests (daily life off) and the US-010 determinism test | [doctest-sim-Release.txt](../evidence/US-012/doctest-sim-Release.txt) |
| Economy | seeds 42 and 7, 20 years: 19-20 alive, store 198-410 meals | headless runs |

Acceptor verdict (2026-09-30): **ACCEPT**. Pick best action, No option and Inspectable pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-013"></a>

## Plan US-013: Remember events and spread gossip

Codex v1.5, prompt S-US-013. Design: [M2 clan design](M2-clan-design.md) (Memories, gossip, relationships). Traces to GD-03 (candidate), SDC-01. Delegated D-02 (memories with feelings, gossip at half strength, minor memories forgotten after 60 days, opinions -100..100).

### Files
| File | Layer | What |
|---|---|---|
| `assets/data/sim/social.json` | Data | Gift and theft feelings and opinion changes, meals a thief takes, witness chance, theft cooldown (5 days), talk warmth, gossip chances (50%, Talkative 80%), minor memory lifetime (60 days), memory limit (40) |
| `src/sim/memory.{h,cpp}` | Simulation | `Memory` (who, to whom, what, when, feeling, major, second-hand), `SocialConfig` + loader, `forgetOldMemories` (erase-remove), `remember` (a full memory drops its oldest minor memory) |
| `src/sim/person.h` | Simulation | Memories (oldest first), an opinion of every person, last gift and theft days |
| `src/sim/actions.{h,cpp}`, `ai.{h,cpp}` | Simulation | New actions GiveGift (Kind people, daylight, one a day) and Steal (Greedy people, at most every 5 days); Talk now has a partner |
| `src/sim/world.{h,cpp}` | Simulation | `giveGift` (the receiver remembers it, +15 opinion), `recordTheft` (the witness remembers it for life, -25 opinion), `talk` (both warm up +2; the speaker may pass on the strongest memory the listener lacks and is not about the listener, as a half-strength second-hand copy that moves the listener's opinion of its subject); partners and receivers are the awake person one likes best (ties by the seeded Social stream); every morning minor memories older than 60 days are forgotten; memories and opinions are in the world hash |

Scope note (Dominus): pairing, births, old age and feuds are recorded by the chronicle (US-014), so they are built there.

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Memory: a person given a gift stores who, what, when and a feeling | `US-013 Memory`: "Voll remembers: Garu gave a gift on day 0, feeling 40"; opinion of the giver rises by 15 |
| Gossip: the listener may gain a slightly weaker copy of a memory it lacks | `US-013 Gossip` (gossip chance forced to 100% for an exact test): a theft story passes on at half feeling, second-hand, and lowers the listener's opinion of the thief; the same event is never copied twice; with gossip at 0% nothing passes |
| Forgetting: a minor memory older than its lifetime is forgotten at day end; important memories are kept | `US-013 Forgetting`: at exactly 60 days kept, at 61 the gift is gone and the theft stays, both as a function and in the running world |
| Supporting | `US-013 A full memory makes room by forgetting the oldest minor memory`; `US-013 Gifts, thefts and gossip happen in a living clan` (one year, seed 42: 85 gift memories, 26 theft memories, 14 heard second-hand) |

Balance: the first version had no thefts at all (greedy people were never hungry), then 139 witnessed thefts a year; greed now tempts even the fed, with a 5-day cooldown.

Manual checks: none (headless).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-013` |
| ctest | 17/17 Debug and Release | [windows-debug.txt](../evidence/US-013/windows-debug.txt), [windows-release.txt](../evidence/US-013/windows-release.txt) |
| US-013 cases | 5 cases, 34 assertions | [doctest-sim-Release.txt](../evidence/US-013/doctest-sim-Release.txt) |

Acceptor verdict (2026-09-30): **ACCEPT**. Memory, Gossip and Forgetting pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-014"></a>

## Plan US-014: Write a readable chronicle

Codex v1.5, prompt S-US-014. Design: [M2 clan design](M2-clan-design.md) (Chronicle; Memories, gossip, relationships). Traces to NA-02 (candidate tapestry). Delegated D-02 (pairing, births, feuds).

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/chronicle.{h,cpp}` | Simulation | Importance levels (gift 10, theft 35, empty store 55, mammoth 60, peace 60, pairing 70, feud 75, first mammoth 80, birth 85, death 90; threshold 50), `Chronicle::select(year, threshold)`, `formatEntry` ("Spring, year 3: Ura was born to Tok and Maa.") |
| `assets/data/sim/life.json`, `src/sim/life.{h,cpp}` | Data, Simulation | Adult age, pairing (mutual opinion 50, 10% a morning, courting bonus 30), fertility (16-40, 2.5% a morning, 21-day pregnancy, 1 year between births, none in famine), childbirth risk 3%, old age from 45 (0.1% a day per year past 45), feuds at mutual -50, parent-child love, grief |
| `src/sim/world.{h,cpp}` | Simulation | Each morning: old age, pregnancies and births (child named, traits inherited or new, parents and child fond of each other; newborns join after the walk over the people vector), conception, pairing, feuds and peace; deaths have their own sentences and close kin grieve (a Death memory); gifts, witnessed thefts, mammoths and the first evening of an empty store go into the chronicle; `pair`, `bringDownMammoth`, `feuds()` |
| `src/sim/clan.cpp` | Simulation | Names: unused names first, then "Mira the Second" so no two people share a name in the chronicle |
| `src/sim/memory.{h,cpp}`, `person.{h,cpp}` | Simulation | `MemoryKind::Death`; mother, father, partner, pregnancy, last birth day |
| `assets/data/sim/actions.json`, `world.cpp` | Data, Simulation | Carrying capacity: the land offers 25/40/30/0 meals a day by season and 3 small game; a gatherer nibbles only what was found; hunger damage builds up over half-rationed days |
| `apps/headless/main.cpp` | App | `--chronicle [year]`, `--threshold` |

### Balance (Dominus, recorded for the owner)
The chronicle is only as good as the lives behind it, so the M2 life model was tuned across 12 seeds x 100 years:
1. Without a limit on foraging, clans grew to 150+ and nobody ever went hungry: a daily forage and game budget now gives the valley a carrying capacity.
2. Nibbling nothing still fed gatherers, and the hungriest-first meal kept everyone just alive on half rations: snacks now need found food, and hunger damage is paid off one day at a time.
3. Couples rarely formed (people always talked to their best friends): unpaired adults now court (pairing at mutual +50 instead of D-02's +60).
Result (seeds 1-12, 100 years): 46-76 alive, 85-152 births, lean winters in most runs, famine deaths in a few. D-02's numbers live in `life.json`; the change of the pairing threshold is noted in [D-02](../decision-requests/D-02.md).

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Record: a birth, death, first mammoth hunt or feud adds one sentence with the in-game date, e.g. "Spring, year 3: Ura was born to Tok and Maa." | `US-014 Record`: "Spring, year 1: Oren was born to Voll and Ilka." (father first, as in the example; the format is checked with a pattern), "... brought down the clan's first mammoth." (then an ordinary mammoth is less important), "A feud broke out between Ena and Ama." after mutual witnessed thefts, "... died of starvation." |
| Filter: hundreds of minor events in a year -> printing that year shows only events above the threshold | `US-014 Filter`: 300 gifts and 3 major events: printing year 2 shows exactly the 3; the living clan's first year: 83 events, 3 worth telling; `odysseus_headless --chronicle 1` ([filtered](../evidence/US-014/chronicle-year1-filtered.txt)) vs `--threshold 0` ([all 89 lines](../evidence/US-014/chronicle-year1-all-events.txt)) |
| Supporting | `US-014 A clan lives for generations` (30 years: births, old age, feuds) |

Evidence of the story: [a century of seed 42](../evidence/US-014/chronicle-seed42-100-years.txt).

Manual checks: none (headless). Whether a reader finds a story is Kill Gate 1 (X-M2), a human gate.

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-014` |
| ctest | 17/17 Debug and Release | [windows-debug.txt](../evidence/US-014/windows-debug.txt), [windows-release.txt](../evidence/US-014/windows-release.txt) |
| Sim tests | 20 cases pass (all M2 stories so far, determinism included) | ctest logs |
| 100 years, seed 42 | 0.3 s in Release; 39 alive at the end, 400 chronicle lines above the threshold | [chronicle](../evidence/US-014/chronicle-seed42-100-years.txt) |

Acceptor verdict (2026-09-30): **ACCEPT**. Record and Filter pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-015"></a>

## Plan US-015: Soak-test the simulation from the command line

Codex v1.5, prompt S-US-015. Design: [M2 clan design](M2-clan-design.md) (Headless runner). Traces to AQ-01, NFR-03, NFR-04. Depends on US-010..US-014 (Done or merged).

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/report.{h,cpp}` | Simulation | `SimReport` (alive, founders, born, died, deaths by cause, average needs of the living, food, couples, feuds, mammoths, chronicle size), `makeReport`, `formatReport` |
| `apps/headless/main.cpp` | App | One-pass command-line parser: `--seed`, `--years` (new), `--days`, `--data`, `--inspect`, `--chronicle [year]`, `--threshold`, `--help`; numbers are read with `std::from_chars` and range-checked, so "-5", "abc", "0", "12x", unknown options and missing values print the usage message and exit with code 2; the report and the tick time (measured with `steady_clock` in the app only; the simulation never sees a clock) |
| `src/sim/world.{h,cpp}` | Simulation | Balance found by the soak runs: the mammoth herd passes once a year (at most one mammoth a year; before: 214 mammoths and 29 hunters killed in a century) |
| `tests/sim/run_headless_soak.cmake`, `tests/sim/report_test.cpp` | Tests | End-to-end runs of the real program; the report's numbers add up |

### Tests
| Scenario | Test |
|---|---|
| Run: `odysseus_headless --seed 7 --years 100` finishes without crashing and prints population, deaths by cause, average needs and tick time | `US-015 Run` (ctest, the real program; checks every report line); `US-015 The report adds up` (unit: founders + born = everyone, alive + died = everyone, causes sum to the dead) |
| Bad input: `--years -5` prints a usage message and exits with an error code | `US-015 Bad input`: `--years -5`, `--years abc`, `--years 0`, `--years 12x`, `--seed -1`, `--frobnicate`, `--years` (no value): each exits with code 2 and prints "Usage: odysseus_headless ..." |

Manual checks: none (headless).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-015` |
| ctest | 19/19 Debug and Release (new: `US-015 Run`, `US-015 Bad input`) | [windows-debug.txt](../evidence/US-015/windows-debug.txt), [windows-release.txt](../evidence/US-015/windows-release.txt) |
| 100 years, seed 7 | 47 alive (20 founders, 93 born, 66 died: old age 53, mammoth 8, childbirth 5); Release 0.08 microseconds per tick (0.5 s), Debug with AddressSanitizer 3.2 (21.8 s); **the same world hash, 10149270126131427195, in Debug and Release** | [Release](../evidence/US-015/soak-seed7-100-years-release.txt), [Debug](../evidence/US-015/soak-seed7-100-years-debug.txt) |
| Bad input | exit code 2 with the usage message | [bad-input.txt](../evidence/US-015/bad-input.txt) |
| Nine seeds x 100 years | 28-50 alive, 82-100 born, 51-82 mammoths, 5-9 hunting deaths, 0-4 starvation deaths | headless runs |

Acceptor verdict (2026-09-30): **ACCEPT**. Run and Bad input pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).

---

<a id="us-016"></a>

## Plan US-016: Save and load the simulation

Codex v1.5, prompt S-US-016. Design: [M2 clan design](M2-clan-design.md) (World hash and saves). Traces to ADR-010, NFR-05. Owner decision D-13 (nlohmann/json).

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/save.{h,cpp}` | Simulation | `saveWorld` (JSON with every piece of state: all six random streams, people with needs, traits, skills, memories, opinions and kinship, feuds, the chronicle; written to `<file>.tmp`, backups rotated to `.bak1..bak3`, then renamed in one step), `loadWorld` (the save, else the newest intact backup; version 1 upgraded to 2; a newer version or an inconsistent save refused with a clear message), `backupPath`; `WorldArchive`, the only code allowed to see the World's private state (a `friend`) |
| `src/sim/world.h` | Simulation | `friend struct WorldArchive` |
| `apps/headless/main.cpp` | App | `--save <file>`, `--load <file>` |
| `tests/sim/save_test.cpp` | Tests | See below (uses nlohmann/json to build a real version-1 file) |

Save versions: version 1 is the needs-only world of US-011 (no traits, skills, memories, opinions, kinship or life events, and only the weather and people random streams); version 2 is today's. Upgrading fills what version 1 did not have exactly as a new world would.

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Round trip: after 50 years, save and load -> the world hash after loading equals the hash before saving | `US-016 Round trip` (seed 42, 50 years, 1.1 MB save): equal hashes, and one more year on both gives equal hashes again. From the command line: 50 years, `--save`, `--load`, 50 more years gives 10149270126131427195, the same as 100 years straight ([cli-save-load-50-plus-50.txt](../evidence/US-016/cli-save-load-50-plus-50.txt)) |
| Crash-safe: a save interrupted halfway -> the previous complete save loads and nothing crashes | `US-016 Crash-safe`: a half-written `.tmp` is ignored; a save cut off halfway is skipped for `.bak1` (the previous save, its exact hash), with a note; with nothing usable, a clear error, no crash. `US-016 Three backups are kept` |
| Old version: a save from an earlier version is upgraded and loads, or a clear message explains why not | `US-016 Old version`: a real version-1 file (version-2 fields removed) is upgraded, keeps names and needs, and lives on for a year; a version-99 save is refused: "the save was made by a newer version of the game (this one reads up to 2)". `US-016 A save that does not add up is refused` |

Manual checks: none (headless).

### Verification results (2026-09-30)
| Check | Result | Evidence |
|---|---|---|
| Builds | Debug and Release, 0 warning lines | `tools/verify.ps1 -Story US-016` |
| ctest | 19/19 Debug and Release | [windows-debug.txt](../evidence/US-016/windows-debug.txt), [windows-release.txt](../evidence/US-016/windows-release.txt) |
| US-016 cases | 5 cases, 30 assertions | [doctest-sim-Release.txt](../evidence/US-016/doctest-sim-Release.txt) |

Acceptor verdict (2026-09-30): **ACCEPT**. Round trip, Crash-safe and Old version pass with evidence; Definition of Done holds (CI on `qa` confirms after the merge).
