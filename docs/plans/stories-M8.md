# Story plans: M8

Per-story plans for milestone M8, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-160](#us-160)
- [US-161](#us-161)
- [US-162](#us-162)
- [US-163](#us-163)
- [US-164](#us-164)
- [US-165](#us-165)

---

<a id="us-160"></a>

## Plan US-160: The .dlg format

Assembly plan v2.0, prompt S-US-160. Design: [M8 dialogue design](M8-dialogue-design.md), sections 2 and 3. Traces to SDC-04, ADR-019, D-34, D-38.

### What was built
| File | What |
|---|---|
| `src/sim/dialogue_script.{h,cpp}` | the script model (`DlgScript`, nodes, lines, choices, headers, notes), `parseDialogue` (line based, every mistake as `file:line: message`), `writeDialogue` (the canonical text), `DialogueLibrary` (loads a folder script by script) |
| `assets/data/dialogue/elder-fire.dlg` | the elder at the clan fire: three nodes, a conditional line, three choices, notes; the brief's example |
| `docs/guides/dialogue-format.md` | the format guide, with the shipped file whole as its example |
| `src/game/odyssey_game.{h,cpp}` | loads `assets/data/dialogue/` at start; F5 reloads it with the interactions (all or nothing); mistakes show in the same red panel |
| `tests/sim/dialogue_test.cpp`, `tests/game/tags_test.cpp` | see below |

Technical choices (Dominus): a node lists what the character says, then the choices (a line after a choice is an error), so the canonical writer reproduces a file exactly; the Editor (M9) writes the same form. `[if]`, `[else]` and `{effects}` on a choice may be written in any order and are written back as `[if] [else] {effects}`. A node that is not reached loads with a warning. More than five choices in a node is an error (D-38). Free text after `{...}` effects uses the whole interaction language.

### Tests
| Scenario | Test |
|---|---|
| Load | `US-160 elder-fire.dlg has three nodes, a conditional line and three choices that link to them` (headers, notes, conditions, the else text, the three effects, the links) |
| Error | `US-160 A choice to a missing node on line 9 is named exactly` (`dialogue/elder-fire.dlg:9: unknown node "hunts"`), `US-160 Every kind of mistake is named with its line` (25 kinds), `US-160 At most five choices fit the panel...`, `US-160 A folder loads script by script...`, `US-160 The game loads the conversations at start, and F5 reloads them with their mistakes named` |
| Round trip | `US-160 Every shipped .dlg is written back as the same text, notes included`; `US-160 The reader is relaxed about spacing and line ends, the writer is exact` |
| Damage | `US-160 Damaged scripts never crash the parser` (300 mutations; whatever loads writes and reads back) |
| Guide | `US-160 Guide: every header, line type and the brief's example are explained` |

### Manual checks (owner)
1. In `assets/data/dialogue/elder-fire.dlg`, change `=> hunt` on line 9 to `=> hunts`, start the game or press F5: the red panel says `dialogue/elder-fire.dlg:9: unknown node "hunts"`.
2. Put it back and press F5: the panel closes.
3. Add a note with `# ...` above a line and a new node of your own; the Editor of M9 will keep the note.

---

<a id="us-161"></a>

## Plan US-161: Conversations and the dialogue panel

Assembly plan v2.0 (Codex v2.1 in force), prompt S-US-161. Design: [M8 dialogue design](M8-dialogue-design.md), sections 4, 5 and 1. Traces to SDC-03, D-34, D-35, D-38.

### What was built
| File | What |
|---|---|
| `src/sim/conversation.{h,cpp}` | `Conversation`: a copy of the script and the node the talk is at; `view()` (lines whose `[if]` holds, choices shown, hidden or greyed, tokens filled); `choose()` (effects through the runner, then the next node or END); `leave()`; `moodWord(opinion, needs)`; `fillDialogueTokens` (`{hero}`, `{npc}`, then the rule language's tokens) |
| `src/sim/dialogue_select.{h,cpp}` | `rolesOf` (`elder` = the oldest living member, `child` = under 12) and `selectScript` (the script that speaks for someone: name over role over kind, then priority, then file name; barks and pair scripts never answer Talk) |
| `src/sim/action_runner.{h,cpp}` | `runEffects`: a choice's effects take the same path as an interaction's (`after` waits in the queue) |
| `src/game/run_flow.{h,cpp}` | `Screen::Talk`: `openTalk`, `buildTalk` (name and mood, lines wrapped at 56 characters, up to five numbered buttons, a greyed one shows its reason), keys 1 to 5 and Esc, `shownText()` for tests |
| `src/game/builtin_actions.cpp`, `game_rules.{h,cpp}`, `odyssey_game.{h,cpp}` | `do talk` opens the panel when a script fits (else the plain talk of before); `openConversation`, `chooseConversationOption`; the effect `opinion npc hero n` (`changeOpinion`); `opinion(a, b)` in conditions; `personNamed` |
| `tests/sim/conversation_test.cpp`, `tests/game/conversation_test.cpp` | see below |
| `docs/guides/dialogue-format.md` | new section "Talking: who speaks, and the panel" |

Technical choices (Dominus): the panel is a screen of `RunFlow`, the same machine as Barter and the context menu, so pausing, the mouse, hit-testing and drawing already exist (the design's `dialogue_panel.*` became `Screen::Talk` instead of a new class). The conversation keeps a **copy** of the script, so F5 while a panel is open cannot change the words under the player. Opening a conversation does not call the old `talkTo` (no free opinion gain from opening and closing the panel; opinion changes come from the script's own effects). `mood()` as a script function is not added here (no scenario needs it) and stays for US-162/163.

### Tests
| Scenario | Test |
|---|---|
| Talk | `US-161 Talk: the game pauses and the panel shows the elder's name, mood, first line and numbered choices` (the world's tick count stands still for 40 updates) |
| Choose | `US-161 Choose: the key 2 takes a berry, raises the elder's opinion by 5 and shows the next node`; the same with a mouse click, then Back and Leave end the talk and the world runs again; Esc leaves with no effect |
| Hidden | `US-161 Hidden: a greyed choice cannot be picked, and without an [else] a choice is not shown at all`; sim: `US-161 A node shows the lines whose condition holds and the choices it may`, `A greyed-out or missing choice does nothing` |
| Runtime | effects in order and END (`US-161 A choice runs its effects in order...`), a delayed effect waits in the runner, an empty script is over at once, the shipped elder script end to end |
| Mood and choice of script | `US-161 The mood word is one word...` (every band, a pressing need, a tie), `US-161 The script that speaks for someone...` (name, role, kind, priority, `@when`, bark ignored) |
| Fallback | `US-161 Without a script for them, Talk is the plain talk of before` |

### Manual checks (owner)
1. Start a game, walk up to the oldest clan member (the elder), right-click them and choose Talk. The game stops; a panel shows their name and a mood word in brackets, their words and three numbered choices.
2. With no berries, choice 2 "Offer berries" is greyed out and says why; pressing 2 does nothing. Pick a berry bush first (Gather), talk again and press 2: you lose a berry, the elder's opinion rises by 5 and they say "The clan remembers kindness."
3. Press Esc: the panel closes and the world runs again.
4. In `assets/data/dialogue/elder-fire.dlg`, add a line, press F5, talk again: your line is there.

Evidence: `docs/evidence/US-161/talk-panel.png` is the panel for a clan member standing next to the hero (seed 7). The evidence copy of the game carried one extra script file, `@who person`, a copy of the elder's, so that someone near the hero had a script; the shipped files are unchanged.

### Notes for the next stories
- US-162: seeded tie-break between equal scripts, barks (`@bark`), roles for hunter and gatherer, the `mood(npc)` function.
- US-163: `{smalltalk.hunt}` still shows as written; the generated small talk fills it. People without a script still get the plain talk until then.
- US-164: `remember` and `flag` effects are read and checked but not carried out yet (the game logs "is not carried out yet").

---

<a id="us-162"></a>

## Plan US-162: Who says what

Assembly plan v2.0 (Codex v2.1 in force), prompt S-US-162. Design: [M8 dialogue design](M8-dialogue-design.md), sections 1 and 5. Traces to SDC-03, D-34, D-38.

### What was built
| File | What |
|---|---|
| `src/sim/dialogue_select.{h,cpp}` | `selectScript` now breaks exact ties with the seeded stream (one number drawn every call, tie or not, as the NPC chooser does); `selectBark` for greetings; `barkText`; `rolesOf` adds `hunter` and `gatherer` (the grown members with the best skill, ties to the lowest id) next to `elder` and `child`; ranking uses `std::ranges::max_element` |
| `src/sim/rule_expr.cpp`, `docs/guides/interaction-data.md` | the condition function `mood(who)` (the word of the conversation panel), so a script can say `@when mood(npc) == wary` |
| `src/game/bubbles.{h,cpp}` | `Bubbles` (words over heads, one per person, counted down while the world runs) and `updateGreetings` (once per play tick: friendly people within 3 m whose minute is over greet; nearest first; at most two bubbles at once) |
| `src/game/odyssey_game.{h,cpp}`, `game_rules.cpp`, `builtin_actions.cpp` | the bubbles, the greeting cooldowns (the M7 `CooldownTable`, key `greeting`) and the stream "dialogue" (`Pcg32(1, 8)`) belong to the game and reset with a run; the bubble is drawn over the head, above a need's face; the `mood(...)` call |
| `assets/data/dialogue/greet-elder.dlg`, `greet-friend.dlg`, `greet-friend-warm.dlg` | the shipped greetings: the elder's own, and two equally fitting plain ones (the seeded stream chooses) |
| `docs/guides/dialogue-format.md` | roles, ties, mood, and a new section "Greetings" with an example |

Technical choices (Dominus): a greeting is an ordinary `.dlg` script marked `@bark`, chosen by the same rules as a conversation, its words the first line of its first node whose `[if]` holds, so a bark needs no new format. **At most two greeting bubbles are on screen at once**, nearest first (a screenshot of the first version showed the whole camp greeting at the same instant, an unreadable pile; those who wait keep their minute unspent and speak when a bubble has gone). The acceptance scenario ("a short greeting bubble appears, at most once a minute per NPC") holds. Whether two at once is the right number is a taste question for the owner at X-M8.

### Tests
| Scenario | Test |
|---|---|
| Specific first | `US-162 Specific first: a script for Ama beats the one for all elders` (even with a priority of 50 on the elders' script) |
| Opinion | `US-162 Opinion: a script for those who dislike the hero is used when someone does` (`opinion(npc, hero) < -30`: -50 yes, -30 no); game: `the mood function and the opinion condition read the real world` |
| Greeting | game: `US-162 Greeting: friendly people within 3 m say a short greeting in a bubble, two at a time, the nearest first` (the bubbles are exactly the nearest two who qualify, the next two follow when they go); `at most once a minute per person` (55 s at the fire: nobody twice; after the minute someone greets again); `those who dislike the hero keep quiet` |
| Ties and stream | `US-162 Scripts that fit equally well are chosen between by the seeded stream` (same seed same choices, all three get a turn, the stream state is the same with none, one or three candidates) |
| Barks and roles | `A greeting is a script marked @bark; a talk never uses one and a greeting never uses a talk`, `A greeting says the first line whose condition holds`, `Roles come from the world`, `The shipped greetings are loaded...` |

### Manual checks (owner)
1. Start a game (the camp is full of friendly people) and watch the first seconds: two people in turn say "Good day, <you>." or "Well met, <you>." in a bubble over their heads for three seconds; the elder says "The fire keeps you well, <you>.".
2. Stand still beside the fire: nobody greets you twice within a minute.
3. In `assets/data/dialogue/greet-friend.dlg`, change the words, press F5, walk away and back after a minute: the new words.
4. Write a script for one person by name, for example `@who Ama` with a different priority, and talk to Ama: her own script is used, not the elder's.

Evidence: `docs/evidence/US-162/greeting-bubbles.png` (seed 7, the first seconds of a run; two bubbles over the heads near the hero).

### Notes for the next stories
- US-163 fills `{smalltalk.topic}` and gives people without a script small talk; US-164 makes `remember` and `flag` real; US-165 reuses `Bubbles` for NPCs talking to each other.
- The elder, hunter and gatherer roles are computed from the world every time (cheap for a clan of twenty).

---

<a id="us-163"></a>

## Plan US-163: Generated small talk

Assembly plan v2.0 (Codex v2.2 in force), prompt S-US-163. Design: [M8 dialogue design](M8-dialogue-design.md), sections 6, 7 and 1. Traces to SDC-03, STO-02, D-34, D-38.

### What was built
| File | What |
|---|---|
| `assets/data/dialogue/smalltalk.json` | the templates: topics `memory`, `people`, `needs`, `season`, `hero` (each 8 or more any-mood sentences, plus some per mood) and `hunt` (asked for by the elder's script) |
| `src/sim/smalltalk.{h,cpp}` | `SmalltalkData` (reads and checks the file: every mistake as `dialogue/smalltalk.json:<line>: message`), `SmallTalk::say` (the generator) |
| `src/sim/memory.h`, `person.h`, `save.cpp`, `world.cpp` | `MemoryNote` and `Person::notes`: what a person remembers that is not about two people ("a wolf at the fire"); saved (old saves load), part of the world hash, never read by the simulation |
| `src/sim/conversation.{h,cpp}` | `{smalltalk.topic}` in a script is asked of a source once per node entered and kept while on screen; `smalltalkScript` (the line, **Thank you**, **Be quiet** = opinion -10) |
| `src/game/odyssey_game.*`, `builtin_actions.cpp` | loads `smalltalk.json` with the dialogue files (F5 reloads all or nothing, mistakes in the same red panel; a game without the file plays without small talk); Talk with no script opens the small-talk conversation (and still warms the two as the plain talk did); a script's `{smalltalk.topic}` calls the same generator |
| `docs/guides/dialogue-format.md` | new section "Small talk" with an example, the topics, moods and every token |

How a line is made (all whole numbers, the game's seeded stream "dialogue"; three numbers drawn every call so the stream never depends on what a person remembers):
1. Topic: asked for, or chosen by weights from the person: a fresh memory (6, else 2), gossip they heard (4 or 3), a need under 60 (8 under 25, 4 under 45, else 2), the season (1), the hero (2). A requested topic whose facts are missing falls back to the season.
2. Fact: one of the three newest memories (their own, or heard).
3. Template: those for the person's mood word (or any mood), not one they said in their last three, not one anybody said twice in the last fifty; one of the rest by the stream.

Technical choices (Dominus): the free-text memory (`MemoryNote`) from design section 7 came forward from US-164 because the Memory scenario needs "a wolf at the fire", which is no memory of two people; US-164 adds the `remember` effect and passing notes on by gossip. `{memory.what}` is a plain phrase that follows "about" (Bo blaming Ama, you giving me a gift, a wolf at the fire). Small talk is chosen when no script fits, so a written script always wins.

### Tests
| Scenario | Test |
|---|---|
| Memory | `US-163 Memory: someone who saw a wolf at the fire yesterday mentions the wolf` (sim: always, with "yesterday"; a fresh memory is the usual topic); game: `Memory: Talk with no script, and they mention the wolf they saw at the fire yesterday` |
| Gossip | `US-163 Gossip: someone who heard that Bo blamed Ama repeats it and says how they feel` ("Bo blaming Ama" and "uneasy" in every line; gladly heard things are said gladly; the hero is "you") |
| Variety | `US-163 Variety: 50 lines from one seed, no line more than twice` (four seeds, a clan with 40 days of past; 39 to 42 different lines in 50) |
| The file | `The shipped small talk loads...`, `Mistakes in the file are named with their line` (unknown token on its line, a token in the wrong topic, too few templates, a missing topic, an unknown mood, too long, damaged JSON), game: a mistake shows in the red panel and F5 with a good file takes it; `Guide: every token, topic and mood ... is explained` |
| Around it | the fallback and the stream (`falls back to the season, and the stream moves the same either way`), mood changes the words, `{smalltalk.topic}` is filled once per node, the generated talk has Thank you and Be quiet (opinion -10), notes are saved, loaded and hashed |

### Manual checks (owner)
1. Walk up to a clan member who is not the elder, right-click and choose Talk: a panel with their name, a mood word, one short sentence and two answers. Pick **2. Be quiet**: their opinion of you falls (the mood word may change).
2. Talk to the elder, choose **1. Ask about the hunt**: a line about hunting (it stays the same while the panel is open; ask again for another).
3. Read `docs/evidence/US-163/samples-seed-7.txt`: 50 lines, nobody repeated more than twice. Say which sound wrong; the templates are in `assets/data/dialogue/smalltalk.json`, edit and press F5.

Evidence: `docs/evidence/US-163/samples-seed-7.txt` (the 50 lines the owner reads at X-M8) and `small-talk-panel.png` (seed 7, a clan member beside the hero).

### Notes for the next stories
- US-164: the `remember` and `flag` effects (a rude answer should also leave a bad memory), free-text notes passed on by gossip, the chronicle line. Until then the game only logs that `remember` "is not carried out yet".
- Nothing in the game writes a `MemoryNote` yet except tests; US-164 and later stories (wolf sightings by NPC life) do.

---

<a id="us-164"></a>

## Plan US-164: Conversations are remembered

Assembly plan v2.0 (Codex v2.2 in force), prompt S-US-164. Design: [M8 dialogue design](M8-dialogue-design.md), section 7. Traces to SDC-03, SDC-02, STO-03.

### What was built
| File | What |
|---|---|
| `src/sim/world.{h,cpp}` | `World::rememberConversation(holder, other, text, feeling)`: an ordinary memory (Gift for a good feeling, Quarrel for a bad one, major from 60) made through the simulation's own `remember()`, so forgetting, gossip at half strength and gossip's opinion changes apply; plus a free-text note (`MemoryNote`, at most 20 per person) |
| `src/sim/memory.h`, `save.cpp` | `MemoryNote::clause` (the text says what happened; small talk tells it as "the day ..."), saved and hashed |
| `src/sim/flag_store.{h,cpp}` | `FlagStore`: story notes, an ordered map name to whole number, saved as JSON, hashed (0 = never set) |
| `src/game/builtin_actions.cpp`, `game_rules.cpp` | the effects `remember`, `flag` and `chronicle` are carried out; `flag(name)` reads the store (`sacred-fire` as before) |
| `src/game/odyssey_game.{h,cpp}` | `flags()`, `rememberConversation`, `chronicleLine`; flags are saved in `things.json` (old saves load) and start empty in a new run; bubbles, greeting cooldowns and small talk history also reset with a new run |
| `src/sim/conversation.cpp` | the rude answer of generated small talk also leaves a memory (-40) |
| `assets/data/dialogue/elder-fire.dlg`, guide | the elder's example remembers the berries with a feeling of 20; the guide has a new section "Being remembered" |

Technical choices (Dominus): the memory is an ordinary `Memory` plus a note, not a new store, so gossip needed no new code (the world's own `talk` passes it on at half strength with `secondHand` and moves the listener's opinion by a quarter of the heard feeling). The default feeling of `remember` is 10. `chronicle "line"` is the existing verb, now carried out, and "marked `chronicle` in the script" is read as a choice that writes it (no new header, so no format change); a chronicle line has importance 30, like a quarrel.

### Tests
| Scenario | Test |
|---|---|
| Memory | sim: `US-164 Memory: an insult leaves a bad memory...` (kind, feeling, object, not second-hand, minor under 60, major from 60, clamping, invalid people refused); game: `Memory: being rude to someone leaves a bad memory and lowers their opinion` (Be quiet: opinion -10, a Quarrel of -40, the note), `the elder remembers the berries they were given` |
| Gossip | sim: `Gossip: a friend of the insulted person hears it at half strength` (-40 becomes -20, second-hand), and `Gossip: two days pass and the friends of the insulted person have heard it at half strength` (seed 42, a grown clan member: someone heard it within two days, at -20) |
| Saved | sim: `Saved: conversation memories survive a save and a load` (memory, note and the world hash); `Flags: a name with a whole number, saved and read back` (damaged input never crashes); game: `Saved: flags set in a conversation, the memories it left and its chronicle line are there after a save and a load`, `A new run starts with no flags` |
| Around it | `What was said comes up in small talk as the day it happened`; the choice sets a flag and the next node's `[if flag(...)]` reads it |

### Manual checks (owner)
1. Talk to a clan member who is not the elder, press **2. Be quiet**. Talk again: their mood word is worse.
2. Let a few days pass (or speed the clan up). Talk to their friends: some may mention it ("the day Voll told Maa to be quiet") or think less of you.
3. Save, close, start again and load: talk to the elder; the berries you gave are still remembered.
4. Write a choice with `flag promised-hunt; chronicle "{hero} promised {npc} a hunt"` in a `.dlg` file of your own (the guide has the example), press F5 and open the clan's chronicle: your line is there.

No screenshot: the change is in what the simulation remembers, shown by the tests above.

### Notes for the next stories
- US-165: NPCs talking to each other in bubbles reuse `Bubbles` and the `@pair` scripts.
- A rival camp and the Editor (M9) will want to read and write flags and notes; the stores are plain and ordered for that.

---

<a id="us-165"></a>

## Plan US-165: NPCs talk to each other

Assembly plan v2.0 (Codex v2.2 in force), prompt S-US-165. Design: [M8 dialogue design](M8-dialogue-design.md), section 8. Traces to SDC-05, SDC-02.

### What was built
| File | What |
|---|---|
| `src/sim/world.{h,cpp}` | `World::takeTalks()`: a queue of who has just talked with whom, filled by `talk()`; for the screen only (not saved, not hashed, nothing in the simulation reads it; the newest 32 are kept if nobody takes them) |
| `src/sim/dialogue_select.{h,cpp}` | `selectPair`: the `@pair first second` script that fits two people, for a kind of event (`@bark <kind>`, `talk` by default): both matches added (name 3, role 2, kind 1), then priority, then the seeded stream (one draw a call) |
| `src/game/bubbles.{h,cpp}` | `Exchanges`: each tick it queues the new events (`takeTalks` and the new chronicle entries of kind Quarrel, Courtship, Pairing, Sharing, Gift) of two clan members within 12 m of the hero (not the hero's own), builds the lines (a `@pair` script, else two lines of `social.<kind>`), and plays them one at a time: each line 3 s over the speaker, the last one removed as the next starts; one exchange at a time, three may wait, the same two are not queued twice; `Bubbles::remove` |
| `src/game/odyssey_game.*` | owns the exchanges, ticks them with the world, resets them for a new clan or a load (what happened before is not shown again) |
| `assets/data/dialogue/smalltalk.json`, `pair-elder-child.dlg` | `social.talk`, `social.quarrel`, `social.courtship`, `social.sharing`, `social.gift` (8 lines each); the elder calls a child over |
| `docs/guides/dialogue-format.md` | new section "Clan members talking to each other" |

Technical choices (Dominus): the bubbles show the simulation, they never decide it, so the game polls what the simulation already writes (the chronicle) and adds one tiny queue for `talk`, which leaves no chronicle entry. "Near the camera" is within 12 m of the hero (the camera follows the hero). The generated lines are one per speaker from the same `social.<kind>` topic, so no new generator is needed.

### Tests
| Scenario | Test |
|---|---|
| Bubbles | game: `US-165 Bubbles: two friends who talk say a short line each, in turn, and each goes after 3 seconds` (the first speaks, the second begins after 60 ticks with the first gone, both gone after 3 s more) |
| Quarrel | `US-165 Quarrel: a quarrel near the hero is shown as angry lines and both opinions fall` (the simulation's `quarrel` lowers both opinions; the lines are from `social.quarrel`) |
| Readable | the same Bubbles test (3 s, or the next line starts), and the queue test: others wait their turn |
| Around it | `A script for the two of them is used: the elder calls a child`; `The hero's own talk and people far from the hero make no bubbles; the same two are not queued twice; others wait their turn`; `What happened before a save is not shown again`; sim: `Two people talking: the script that fits both...`, `Pair scripts that fit equally well...`, `The world says who has just talked...` |

### Manual checks (owner)
1. Start a game and speed the clan up (`--clan-speed 6`) or wait: near the fire, now and then two people say a short line each in turn over their heads ("A good spring so far."), 3 seconds each.
2. Watch for a quarrel or a courtship near you: angry or tender lines (and the opinions change as in the clan's chronicle).
3. Add a `.dlg` file of your own with `@pair Ama Tok` and `@bark quarrel` (the guide has the example), press F5, and wait for those two to quarrel (or use a clan where they do): your lines.

Evidence: `docs/evidence/US-165/bubbles-between-people.png` (seed 7, clan sped up: "A good spring so far." over one person while another greets the hero).

### Notes for X-M8
- Two taste questions for the owner: the greeting rule (at most two bubbles at once) and how often people talk in bubbles (it follows how often the simulation makes them talk).
- All five dialogue stories of M8 are Done once this one is; the exit review follows.
