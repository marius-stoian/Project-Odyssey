# Project Odyssey: assembly progress (23)

## AP-024 · 2026-09-30 · M2 built; Kill Gate 1 waits for you

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit, CI green); `main` tagged `m0-done`, `m1-done`, `m1b-done` |
| Milestone | **M2 Console clan simulator: 7 of 7 stories Done; exit review X-M2 waiting for the reader test** |
| Next | **You**: the reader test, then answer [D-GATE-M2](../../decision-requests/D-GATE-M2.md). After your answer: X-M2 finishes (merge to `main`, tag `m2-done`), then K-M3 |

### Your part: the reader test (about an hour in total)
1. Open [the reader packet](../../gates/M2-reader-packet.md). It has the instructions, three questions and a clan's 100-year chronicle.
2. Give it to 3 people who have not seen the project; ask the three questions.
3. Write in [D-GATE-M2](../../decision-requests/D-GATE-M2.md) how many of the 3 found a story, and pick A-D. The team recommends **Go** if 2 or 3 found one.

### What M2 built (this session)
- A clan that lives by itself in the console: needs (US-011), a utility AI choosing what to do every hour (US-012), memories and gossip (US-013), a chronicle of births, deaths, couples, feuds, mammoths and famines (US-014), a 100-year soak test with a report (US-015), and safe versioned saves with backups (US-016).
- **Stable**: 13 different centuries, all finished; clans of 20-49 people after 100 years; a century takes about 0.4 s.
- **Deterministic**: the same 100 years give the same world hash in Debug and Release, and saving at year 50, loading and running on gives the same world as one straight run.
- Try it: `odysseus_headless --seed 7 --years 100 --chronicle` and `--inspect <name>`.

### Gate evidence
[docs/gates/M2.md](../../gates/M2.md): criterion 1 (100 years, no crash) met, criterion 3 (determinism) met, criterion 2 (readers) waiting for you.

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 7 / 7 | **Kill Gate 1: waiting for the reader test** |
| M3-M6 | | 0 / 28 | Not started (the gate comes first) |
| | **MVP total** | **21 / 49** | |

### Decisions and codex issues
- Waiting for you: **D-GATE-M2** (the gate result).
- Please review when convenient (delegated to Dominus): D-02 (with a tuning note: couples pair at +50, not +60), D-16, D-17.
- Open for Anima (A-002): **CI-006**, US-020's 3-second first-frame check fails now and then on GitHub's Debug runner (3.2 s and 8.1 s seen; re-runs passed).
- Also for later milestones: M3 needs D-05 (art source); M4 needs D-06, D-07, D-08, D-09; M5 needs D-10, D-11; M6 needs D-14 (eight playtesters, a human gate).
