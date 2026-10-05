# M7 World interactions: design (K-M7)

Architect: Mraw (Solution Architect hat). Date: 2026-10-01. Codex v2.0. Source of truth: requirements v2.0 (INT-01..INT-06, ADR-019), brief `docs/plans/M7-M9-interactions-brief.md`, decisions D-34, D-35, D-36.
Goal of M7: every action in today's game comes from data; the player, clan members and animals act on things with timed, visible, saved results; files edited offline reload with F5.

## 1. Owner decisions that shape this design (D-36, 2026-10-01)
| Question | Answer | Consequence |
|---|---|---|
| Progress look | Ring over the target | The renderer draws a ring above the thing being worked on, for any actor (hero, NPC, animal). One art routine, no per-actor art. |
| What interrupts NPCs and animals | Danger and need spikes | An actor drops its action when hit, when a hostile or predator is within 6 m, or when a need crosses the critical line (starving, freezing). Otherwise it finishes. The player's actions stop on move or attack (US-153). |
| Do animals flee from the hero | Prey flee if you run or are armed | Prey flee from predators always, and from the hero within 5 m when the hero runs or holds a weapon. Data: `"flee"` block on the animal catalog entry. |
| Regrowth speed | Short real-time seconds | Written as `after 15s set target.state ripe` (today's 15 s, `kRegrowTicks = 300`). Seasons still switch growth off through the `requires` of Gather (`season != winter`), not through the timer. |

## 2. Where things live (layers)
| Part | Layer | Why |
|---|---|---|
| Lexer, parser, expression trees, evaluator | `src/sim/rule_*.{h,cpp}` (flat: the layer check wants every header to include its own `boundary.h`; US-150) | Pure logic, headless, shared by interactions and dialogue (ADR-019). No Engine, Platform or SDL. |
| Interaction registry, JSON loader, validator | `src/sim/interaction.{h,cpp}` | Same. Reads files with nlohmann/json (comments allowed). |
| Thing states, running actions, flags (the saved state) | `src/sim/` (`ThingStore`, `ActionRunner`, new files in US-153) | Saved with the world; the determinism hash covers it. |
| NPC and animal scoring loop | `src/sim/` (`Chooser`, US-154) | Uses the simulation's seeded streams. |
| What things exist, where they stand, what they look like | `src/game/` (plants, animals, objects, clan figures) | They are Game objects today (`WorldPlant`, animals, `Figure`), shown to the rules through a small interface, `WorldView`. |
| Context menu, progress ring, validation panel, F5 | `src/game/` | Presentation. |

`WorldView` (defined in sim, implemented in Game) is the only door between the two: `thingsNear(x, y, radius)`, `thing(id)` returning `{id, kind, tags, x, y}`, `actorInfo(id)`, `season()`, `timeOfDay()`, `inventory(actor)`. Rules never hold a pointer to a thing, only its id (Charter rule 6, brief section 6). The hero, every clan member, plant, animal and placed object gets a stable `ThingId` (an int never reused in a run); people keep their `PersonId` as their id in the `person:` range.

## 3. The language (one grammar for conditions and effects)
### 3.1 Tokens
Whitespace separates tokens. Numbers: whole (`2`, `-20`) or with a time suffix (`15s`, `2m`, `1d`; seconds, minutes, in-game days). Strings: `"quoted"` or a bare word of letters, digits, `_`, `-`. Names: dotted paths (`target.state`, `actor.name`). Operators: `== != < <= > >=`, `and or not`, `+ - * /`, parentheses, commas. Everything is whole numbers or text; there are no floats (Charter rule 6), and division rounds toward zero.

### 3.2 Grammar (condition and score expressions)
```
expr      := or
or        := and ( "or" and )*
and       := not ( "and" not )*
not       := "not" not | compare
compare   := sum ( ("==" | "!=" | "<" | "<=" | ">" | ">=") sum )?
sum       := product ( ("+" | "-") product )*
product   := unary ( ("*" | "/") unary )*
unary     := "-" unary | primary
primary   := number | string | path | call | "(" expr ")"
call      := name "(" ( expr ( "," expr )* )? ")"
```
Precedence, loosest to tightest: `or`, `and`, `not`, comparison, `+ -`, `* /`, unary minus. A bare word that is not a path or call is a string (`season != winter`). Comparing different kinds (a number with text) is a load error where it can be seen statically, otherwise false.

### 3.3 Functions (read-only)
`has(item, n)`, `need(name)`, `skill(profession)`, `trait(name)`, `opinion(a, b)`, `kin(a, b)`, `flag(name)`, `tag(thing, name)`, `time`, `season`, `distance`, as in the brief. More are added when a story needs them and documented in the guide in the same commit: `count(item)` (how many of an item the actor holds) is expected in US-152 for barter/craft conditions. Paths resolve against `actor`, `target`, `npc`, `hero`: `target.state`, `target.kind`, `actor.name`.

### 3.4 Effects
One verb per string: `give actor berries 2`, `take`, `set target.state picked`, `flag name [value]`, `opinion a b +5`, `remember`, `start`, `talk`, `say`, `fx`, `sound`, `after <t> <effect>`, `chronicle`. Parsed into `Effect` nodes: `std::variant<Give, Take, Set, Flag, Opinion, Remember, Start, Talk, Say, Fx, Sound, After, Chronicle>`. `after` wraps another effect and becomes a timer (section 5). Arguments are separated by spaces; each is one word, number, "quoted text", call or (bracketed expression), so a sum needs brackets: `give actor berries (1 + skill(gather))` (settled in US-150).

### 3.5 Errors
Every error is `file:line: message` plus the field (`requires[1].if`). The lexer tracks line and column inside a string by the line of the JSON field (nlohmann reports byte offsets; the loader converts them to lines). The loader collects every error in a file, not only the first, so F5 can list them all (US-156). A file with errors is rejected as a whole; the last good version of that file stays.

## 4. Registry and matching
- `InteractionRegistry` owns `std::vector<Interaction>` sorted by id, plus an index `tag -> interactions`. One file per interaction in `assets/data/interactions/`.
- An interaction matches an (actor, target) pair when: the actor kind is in `actors`; the target has every tag in `target.tags` (and any in `target.kinds` if given); the distance is within `range` (this is **availability**, shown as a disabled item with the reason "Too far away"); and each `requires` entry, evaluated in order, passes (the first failing entry's `else` text becomes the reason).
- `offered(actor, target)` returns every interaction whose actor and target match, each with `enabled` and `reason`. The context menu (US-152) shows all of them, disabled ones greyed with the reason, which reproduces today's behaviour ("Too far away", "You have no berries").
- Tags and states (US-151): catalogs gain `"tags": [...]` and, where something has state, `"states": [...]` with the first as default. `plants.json`, `animals.json`, `characters.json` and the new `objects.json` get tags; an interaction naming a tag no catalog uses is a load warning naming file and tag.
- Menu order: the interaction `"order"` field (default 100), then label. Inspect is always last.

## 5. Action runner and timers
- `ActionRunner` holds at most one `RunningAction` per actor: `{actor, interaction, target, startTick, endTick, rangeSnapshot}`. `start(actor, interaction, target)` re-checks availability and `requires`, then either runs the effects at once (duration 0) or registers the running action.
- Time is ticks (20 per second, ADR-006). `duration` in seconds becomes `endTick = now + round(duration * 20)`. A progress value 0..100 for the ring is derived per tick: `100 * (now - start) / (end - start)`.
- Per tick, for each running action (iterated in ascending actor id, never an unordered container): check `interrupt` rules (5.1), then if `now >= endTick` apply the effects and clear the action.
- Delayed effects (`after 15s ...`) go into a `TimerQueue`: a vector of `{dueTick, seq, effect, actor, target}` sorted by `(dueTick, seq)`, where `seq` is a counter so ties resolve in creation order (deterministic). A timer on a target persists after the actor leaves.
- 5.1 Interrupts: the hero's action stops when a Move or Attack intent is pressed (US-153: gives nothing, no effects run, nothing is spent). For NPCs and animals (D-36): hit, hostile within 6 m, or a need above its critical line; partial results are never given.
- Effects run in the order written; the first failing effect (for example `take` with too little) cancels the effects after it and reports the reason; effects that already ran are kept (`give` and `take` are applied as a pair at the end, the only order the story needs).
- The ring and the working pose are presentation: the Game layer reads `ActionRunner::progressOf(actor)` and draws.

## 6. State storage and saves
- `ThingStore`: `std::map<ThingId, ThingState>` where `ThingState = {std::string state; std::map<std::string, int> counters;}`, only for things whose state differs from the default (a fresh world saves almost nothing). Flags: `std::map<std::string, int> flags_` on the same object.
- Saved inside the existing sim save as new top-level sections `things`, `timers`, `running`, `flags`. `kSaveVersion` goes 3 -> 4 with a migration from 3 that adds the four sections empty; the loader refuses a newer version as today (ADR-010). The hero save (`HeroLife::save`, version 1) is unchanged in M7; the hero's running action is stored in `running` like everyone else's.
- Plants today regrow through `WorldPlant::regrowTicks` in Game. US-153 replaces that with a state: Gather sets `target.state picked` and a timer `after 15s set target.state ripe`; the picture changes by state (picked = no berries). Destroying plants for wood (Chop) stays as is until US-152 moves it to data as `set target.state felled` with an `after` regrow.
- The determinism hash (`World::hash`) includes `things`, `timers`, `running`, `flags` in map order (ADR-011).
- Region delta (US-043) keeps tile changes; states of region plants are saved through `ThingStore`, keyed by stable ids (a plant's id derives from its region cell and kind, so it survives regeneration from the same seed).

## 7. NPC and animal choice (US-154)
- `Chooser::choose(actor)` runs for an actor with no running action, at most once per `thinkTicks` (default 20, data), staggered by actor id so not everyone thinks in the same tick.
- Candidates: `thingsNear(actor, 12 m)`, offered interactions that are enabled and have an `npc.score`. Score = the file's expression (`need(hunger) * 2 + trait(diligent) * 10`), floored at 0; interactions on `cooldown` for this actor are skipped. Highest score wins; ties go to the seeded stream of the actor's system (PCG32, one stream per system, Charter rule 6).
- If the best score is below `minScore`, the actor does the existing M2 behaviour (wander, rest). The clan scoring in `src/sim/ai.cpp` stays authoritative for the clan's hourly plan; the Chooser only decides what to do with the hour's attention at scene level, and `actions.json` keeps only tuning numbers (as the brief promises) once Gather and Talk read their scores from interaction files.
- Animals (D-36): the same loop with their interactions (`graze`, `flee`, `hunt`). `flee` has an actor condition `near(predator, 6)` or `hero near 5 and (hero running or hero armed)`; this needs two read-only `WorldView` helpers, `heroRunning()` and `heroArmed()`, documented in the guide.
- Walking to the target: the Chooser emits a `MoveTo` order that the Game layer executes with the existing movement and collision; arrival within `range` starts the action. The order is dropped when the target is gone or the actor is interrupted.

## 8. Hot reload and the validation panel (US-156)
- F5 (a new `Reload` input intent, ARC-03) calls `InteractionRegistry::reload(folder)`: parse all files into a **new** registry on the side. With zero errors, swap it in at the start of the next tick (atomic: a `std::shared_ptr<const Registry>` is replaced; running actions keep the interaction id and look it up again each tick). With errors, keep the old registry and hand the error list to the panel.
- Running actions of a removed or changed interaction are cancelled with no effects (the ring disappears); timers on removed effects are dropped.
- The panel lists `file:line: message`, scrolls, and closes by itself on the next successful reload. Reload takes under 1 second for the shipped data (a test measures it).
- No file watching in M7 (F5 only), as the story says.

## 9. Moving today's hard-coded actions into data (US-152)
The 16 actions of `RunFlow::openContext` become files, one per interaction, with their exact conditions and reasons: `talk` (until M8 it keeps today's one-line talk through a `talk` effect that opens the simple dialogue), `give-berries`, `ask-to-teach` (per profession: one file, profession in the actor/target data), `craft-knapping-stone`, `craft-fire`, `eat-berries`, `tend-fire`, `tend-sacred-fire`, `hold-ritual`, `barter`, `gather`, `knap`, `pick-up-by-hand`, `chop-wood`, `inspect`.

Effects that call into `HeroLife` (`gatherBerries`, `knapFlint`, `chopWood`, `talkTo`, ...) are exposed as named **built-in effects** (`do hero.gather`, `do hero.chop`, ...) so behaviour stays byte-identical in the first cut. The economy rules then move to plain `give`/`take`/`set` effects one by one, only where a test proves the same result. The regression baseline is every M5 and M6 test, unchanged.

## 10. Data formats (summary; brief section 4 is the contract)
- Interaction JSON fields: `id, label, actors, target{tags,kinds}, range, duration, requires[{if,else}], effects[], npc{score,cooldown}, chronicle, order, note`. Unknown fields are an error naming file and field (catches typos). `note` is kept by Editor saves.
- Catalog additions: `tags`, `states`, `defaultState`, and for animals `flee{radius,fromHeroWhen}`.
- Object catalog `assets/data/objects.json` (US-155): fire pit, knapping stone, food store, shelter, flint nodule, water source, sleeping furs, each with tags, states and art frames.
- Guide: `docs/guides/interaction-data.md` (US-150), one example for each function and verb.

## 11. Test plan
| Story | Tests (all headless unless noted) |
|---|---|
| US-150 | lexer and parser table tests for every token and precedence rule; evaluator tests per function; every shipped file loads; error tests per class of mistake with the exact `file:line: message`; a fuzz-style test feeding 200 random mutations of gather.json that must never crash and always report an error or load. |
| US-151 | tag matching; unknown-tag warning; state default; right-click offers Gather and Inspect for a new plant kind (Game test, scripted pointer). |
| US-156 | reload swaps data; a broken file keeps the old data; fixing it closes the panel; timing under 1 s; running actions of a removed interaction cancel cleanly. |
| US-152 | the whole M5 and M6 test set unchanged; a table test: for each of the 16 old actions, the same actor and target give the same label, enabled flag, reason text and effect as before. |
| US-153 | duration and progress by tick; interrupt by move and by attack gives nothing; save, load and continue gives the same world hash; picked bush and lit fire survive a save. |
| US-155 | each object placeable in the Editor, appears in Game, offers its interactions, round-trips through the level file. |
| US-154 | hungry person walks to a ripe bush and gathers; deer grazes and flees from a wolf; prey flee from a running or armed hero but not from a walking unarmed one; interrupt rules; 10,000 ticks twice with the same seed give the same hash; 100 seeds never leave an actor stuck. |
| All | the CI validator test (every shipped JSON passes), round-trip load-save-load, `tools/verify.ps1` at every story, green CI on `qa`. |

## 12. Risks and mitigations
| Risk | Mitigation |
|---|---|
| US-152 changes behaviour by accident | Built-in effects first (section 9); the old-versus-new table test; M5 and M6 tests are the gate. |
| Two sources of NPC actions drift | The Chooser reads scores from interaction files only for scene-level actions; the clan's hourly plan keeps its numbers in `actions.json` (documented in the guide). |
| Hot reload swaps data under a running action | Id lookup each tick; cancel on removal; swap only at tick boundaries. |
| Save format drift | One version bump (3 -> 4) with a migration test that loads a version 3 file from `tests/data/`. |
| Tests slow in Debug | New heavy loops get a thinner Debug size (CI rule of D-35's CI speed-up); sharded runs stay balanced. |

## 13. Story order (Codex)
US-150 -> US-151 -> US-156 -> US-152 -> US-153 -> US-155 -> US-154 -> X-M7. Each story: branch from `qa`, `tools/verify.ps1`, CI green on `qa`, a Milestone-<n>.md and a teach-back entry.
