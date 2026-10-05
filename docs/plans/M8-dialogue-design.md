# M8 Speak to NPCs: design (K-M8)

Architect: Mraw (Solution Architect hat). Date: 2026-10-01. Codex v2.0. Source of truth: requirements v2.0 (SDC-03..SDC-05, ADR-019), brief `docs/plans/M7-M9-interactions-brief.md`, decisions D-34, D-35, D-38. Builds on M7 (the rule language, the action runner, the Game's rule context).
Goal of M8: the player talks to any clan member; written conversations branch on the simulation; without a script NPCs make small talk from their memories; talk changes opinions, items and memories; NPCs talk to each other in bubbles.

## 1. Owner decisions that shape this design (D-38, 2026-10-01)
| Question | Answer | Consequence |
|---|---|---|
| Mood in the panel | One mood word | `moodOf(npc)` returns one word from the opinion of the hero and the most urgent need (warm, friendly, neutral, wary, hostile; hungry, cold, tired, lonely). Scripts can test it: `mood(npc) == wary`. |
| Choices at once | Up to 5 | The panel shows at most 5 numbered choices (keys 1 to 5). A node with more is a load error in the script checker ("a node has 7 choices; the panel shows 5"). |
| Voice of small talk | Plain and short | Templates are one or two short sentences of simple speech. A "no line repeats more than twice in 50" test keeps them varied. |
| Insults | Yes, also in small talk | Generated talk offers a friendly choice and a rude one (a tease that lowers opinion by 10 and leaves a bad memory). Script writers may add as many rude choices as they like. |

## 2. Where things live (layers)
Flat files again (the layer check wants each header to include its layer's own `boundary.h`):
| Part | Files | Layer |
|---|---|---|
| The `.dlg` format: lexer-less line parser, script model, canonical writer, validator | `src/sim/dialogue_script.{h,cpp}` | Simulation |
| Selection (who says what), mood | `src/sim/dialogue_select.{h,cpp}` | Simulation |
| The conversation runtime | `src/sim/conversation.{h,cpp}` | Simulation |
| Small talk generator, `smalltalk.json` | `src/sim/smalltalk.{h,cpp}` | Simulation |
| Flags and conversation memories (the saved part) | in `things.json` (US-153) via `FlagStore` in `flag_store.{h,cpp}`, next to `src/sim/action_runner.*` | Simulation |
| The dialogue panel, bubbles | `src/game/dialogue_panel.*`, `src/game/bubbles.*` | Game |

The runtime reads the world through the same `RuleContext` the interactions use (extended with `mood(...)`, a real `flag(...)`, `opinion(...)`, `kin(...)`, `skill(...)`), and changes it through the action runner's `EffectHost`, so a dialogue effect and an interaction effect are the same thing.

## 3. The `.dlg` grammar (US-160)
Line based; a line is decided by its first characters, so the parser needs no lookahead beyond one line and every error has a line number.
```
file      := (note | header | blank)* node+
note      := "#" text                       # kept, attached to the next element
header    := "@who" word+ | "@when" expr | "@priority" int | "@bark" word | "@pair" word word
node      := "===" id  (line | choice | note)*
line      := Speaker ":" text [ "[if" expr "]" ]
choice    := "->" text [ "[if" expr "]" ] [ "[else" text "]" ] [ "{" effects "}" ] "=>" (id | "END")
text      := words with {tokens}: {hero} {npc} {npc.name} {target.name} {smalltalk.topic}
effects   := effect (";" effect)*
```
- `@who`: a placed character's id or name, a role (`elder`, `hunter`, `child`), or a kind. `@when`: a condition of the shared language. `@priority`: default 0. `@bark`: marks a short greeting script (US-162). `@pair a b`: for two NPCs talking (US-165).
- A choice with `[else "..."]` is shown greyed out with that reason when its `if` is false; without it, it is hidden (US-161 scenario).
- Errors: `file:line: message`, same names as M7. Checks: unknown node, duplicate node, no `start` node, unreachable node (warning), node with no way out, more than 5 visible choices, bad expression or effect (the M7 messages), unknown speaker word.
- The **canonical writer** prints a script back with `#` notes in place, one blank line between nodes, effects normalised (`;` and one space). Round trip: parse, write, parse, write is the identity (tested on every shipped file).

## 4. The runtime (US-161)
`Conversation` is a small state machine: `{script, node, history}`.
- `visible(ctx)` returns the lines whose `if` holds (all of them, in order, as the NPC's speech) and the choices (enabled, hidden or greyed).
- `choose(i, host)` runs the choice's effects through the host (`give`, `take`, `opinion`, `remember`, `flag`, `start`...), then moves to the target node, or ends.
- `end(host)` runs the node's `on end` effects once.
- The game pauses while a conversation is open (D-35): the panel is a modal screen of `RunFlow`.

Panel (Game): the NPC's name, the mood word, the visible lines (wrapped at 56 characters), up to 5 numbered choices (mouse or keys 1 to 5), Esc leaves. Greyed choices show their reason.

## 5. Who says what (US-162)
`select(npc, hero, scripts, ctx, random)`: candidates are scripts whose `@who` matches the NPC (most specific match wins: placed id or name over role over kind) and whose `@when` holds; the highest `@priority` wins; ties go to the seeded stream. Roles come from the sim: elder (oldest), hunter/gatherer (best skill), child (under 12). If no script fits, the small-talk generator is used (section 6).

Barks are `@bark` scripts of one node and one line. The hero passing within 3 m of a friendly NPC (opinion >= 0) triggers one as a bubble, at most once a minute per NPC (a `CooldownTable` from M7).

## 6. Generated small talk (US-163, D-38)
`assets/data/dialogue/smalltalk.json`: `{ "topics": { "memory": [templates], "people": [...], "needs": [...], "season": [...], "hero": [...] } }`. Each topic has at least 3 templates and several variants per mood, each a plain sentence with tokens: `{memory.what}`, `{memory.who}`, `{memory.when}`, `{gossip.who}`, `{need.name}`, `{season}`, `{hero}`.

The generator picks the topic by weights from the NPC's state (an urgent need, a fresh memory, an unheard gossip, the season), then a template with the seeded stream, avoiding the last 3 lines this NPC said. The conversation it makes is the line, then a friendly choice ("Thank you") and a rude one ("Be quiet", opinion -10, a bad memory), then END. `{smalltalk.topic}` tokens in a script call the same generator.
Test: 50 lines from one seed, no line more than twice.

## 7. Memory, opinion, flags and the chronicle (US-164)
- `remember npc "text" [feeling]` adds a `Memory` to the NPC (`subject` = the hero, `object` = the NPC, kind `Gift`/`Quarrel`/`Rejection` by the sign of the feeling, `major` when |feeling| >= 60) through the simulation's own `remember()`, so gossip, forgetting and chronicle links already apply. Free text is kept in a saved `conversationNote` string table on the person and shown in small talk as "they said...".
- `opinion npc hero +5` is `World::adjustOpinion`. `flag name [value]` goes to a `FlagStore` (ordered map, saved in `things.json`, hashed).
- `chronicle "line"`, or a script marked `@chronicle`, writes a chronicle entry with its reason (`World::note`).
- Gossip needs nothing new: the existing two-day gossip spreads memories at half strength, which US-164's test checks.

## 8. NPCs talking to each other (US-165)
When the social simulation fires an event between two people near the camera (a `talk`, a quarrel, a courtship step, a share), the Game looks for a `@pair` script (event kind as `@bark` topic) or a bark, and plays its lines as bubbles over the two heads, 3 s each, one at a time, alternating. The outcome is already in the simulation; the bubbles only show it. Without a script, a short generic line from `smalltalk.json` topic `social` is used.

## 9. Test plan
| Story | Tests |
|---|---|
| US-160 | grammar table tests per line type; every error class with its `file:line`; the missing-node error of the acceptance criteria exactly; round trip of every shipped `.dlg` including notes; a 300-mutation fuzz of `elder-fire.dlg` (never crashes, always loads or errors). |
| US-161 | runtime: lines and choices by condition; hidden and greyed choices; effects run in order; `END` closes; the Game test opens the panel from Talk, the game pauses, choice by mouse and key 2 takes a berry and raises opinion by 5. |
| US-162 | selection by id over role over kind; `@when` on opinion; priority; ties by seeded stream; bark cooldown 60 s; greeting bubble at 3 m. |
| US-163 | 50 lines from one seed, no line repeated more than twice; a remembered wolf is mentioned; gossip is repeated with the feeling; every template has valid tokens (CI validator). |
| US-164 | an insult leaves a bad memory and lowers opinion; two days later friends have it at half strength; flags and memories survive save and load. |
| US-165 | two friends at the fire exchange bubbles in turn; a quarrel shows angry lines and both opinions fall; a bubble ends after 3 s or when the next starts. |
| All | `verify.ps1`, CI green on `qa`, determinism hash (conversations use only the seeded streams). |

## 10. Risks
| Risk | Mitigation |
|---|---|
| Free-text memories break the simulation's neat enum memories | The memory keeps its kind and feeling for the sim; the text is a side table used only for talk. |
| Small talk reads robotic | Plain style by decision; several templates per topic and mood; the owner reads 50 samples at X-M8. |
| Dialogue pauses the game but NPC bubbles run in the world | Bubbles are drawn by the Game only when the game is not paused; a conversation freezes them. |

## 11. Story order (Codex)
US-160 -> US-161 -> US-162 -> US-163 -> US-164 -> US-165 -> X-M8.
