# Playtest plan: kill gate 2 (X-M6)

Owner decision D-48 (2026-10-01): plan now, recruit from the start of M13, play after X-M14. Success criteria come from requirements section 12.4 and stay fixed, so the result cannot be rationalised afterwards.

## 1. What the gate measures
| Criterion | Pass | How measured |
|---|---|---|
| Voluntary play time | 5 of 8 play 30+ minutes without being asked to continue | Observer's clock and the local session statistics file (US-092) |
| Emergent story | 3 of 8 retell a story that came from the simulation, not from a script | The interview (section 5), judged by the owner against the session's chronicle |
| Stability | No crash in any session | Crash folder empty (US-091) and the observer's notes |

Gate decision (owner): **Go** if all three pass; **Pivot** if stability passes but one of the others fails (fix the loop, then retest with new players); **Stop and rethink** if both play time and story fail.

## 2. Who
- Eight people who have not seen the game or the project, aged 16 or older, who play PC games at least monthly. At most two may know the owner well; nobody who has worked on the game.
- A mix: about half who like simulation or management games (RimWorld, Dwarf Fortress, The Sims, Crusader Kings), half who mostly play other genres.
- Each plays on a Windows 10/11 PC at or above the target spec (D-06: 6-core CPU, 16 GB RAM, RX 6600 / RTX 3060 class GPU), their own or the owner's.

## 3. Recruiting (from K-M13)
| When | Step |
|---|---|
| K-M13 | Owner writes the invitation (two sentences, no spoilers) and asks friends of friends, a local gaming club, or a small Discord or subreddit for the genre |
| During M13-M14 | Collect ten names (two spares), agree dates and whether in person or remote (screen share with camera off is fine) |
| X-M14 | Confirm all eight and send the package link one day before |

## 4. Session script (about 75 minutes)
1. **Welcome (3 min):** "We are testing the game, not you. Play as long as you like and stop whenever you want; there is no right way. Please think aloud if you can." Ask the statistics question honestly (US-092): the file stays on their PC and they choose whether to send it.
2. **Play (up to 60 min):** start a new game with default settings and the tutorial on. The observer says nothing, does not help, and notes the time of every stop, confusion, laugh and question. At 30 minutes, the observer does **not** ask whether they want to continue; the clock simply keeps running until they stop or 60 minutes pass.
3. **Interview (10 min):** section 5.
4. **Close (2 min):** thank them; ask whether the statistics file and any crash folder may be sent.

## 5. Interview questions (same order every time)
1. "Tell me what happened in your game, as if to a friend who did not see it."
2. "Who was the most interesting person in your clan, and why?"
3. "Was there a moment that surprised you?"
4. "When did you feel like stopping, and why?" (or "Why did you keep going?")
5. "What did you want to do that the game did not let you do?"
6. "Would you play again tomorrow? What would you do first?"

The answer to question 1 (and 2, 3) is scored for "emergent story": it counts when the player retells a chain of at least two simulated events with a cause (a feud, a hunt gone wrong, a courtship, a theft, a hard winter), not the tutorial or a scripted crossroads event alone.

## 6. Evidence (filled in at X-M6)
`docs/gates/M6.md`, one row per player: ID (P1..P8, no names), date, PC, minutes played, why they stopped, story retold (yes or no, with the quote), crashes, statistics file received; then the three criteria counted and the owner's gate decision (D-GATE-M6).

## 7. Privacy
No names in the repository; players are P1..P8. The statistics file and crash logs are only sent if the player agrees; nothing is collected automatically. Interview notes keep the quotes, not personal details.
