# Project Odyssey: assembly progress (147)

## AP-148 · 2026-10-06 · M11 Data editors (S-US-194 Done)

Codex v2.13, requirements v2.12.

### State
- S-US-194 Done on `qa` (branch `story/US-194`): every mechanics and story file (`sim/`, `hero/`) opens as forms; the Data tab's **Quick check** runs the clan for 20 years on the saved data and the seed of the level in play (30 days per update, no thread) and shows population, births, deaths by cause, feuds, episodes and the world hash beside the run before; the same data and seed give the same summary.
- **Mechanics are live** (CI-007 closed for them): `sim/`, `hero/` and `story/` are the data set `mechanics`; a save changes the running clan and hero between two ticks. A change the clan cannot take (ticks per day, days per season, the top of the needs scale) or a hero file that drops the run's preset or comfort level is refused with the reason and the old rules stay.
- Debug build of every program and test executable: zero warnings. Own cases: sim 4 and game 5, and the US-191 and US-303 reload cases still pass; full verification and CI at X-M11 (D-41).
- Guides `docs/guides/data-editor.md` and `docs/guides/editor.md`, plan `docs/plans/US-194.md`, teach-back in `docs/learning-journal.md`, evidence `docs/evidence/US-194/`.

### Decisions and Codex issues
- No new owner-facing question. D-60 Q9 followed (in process, time-sliced). The headless program keeps its own loop (it prints more); both use the same `World` and report.
- Codex issues: none new.

### Next
- S-US-195 Game Rules (`sim/play_rules`, `rules/standard.json`, the New Game picker, a level's `rules`).
