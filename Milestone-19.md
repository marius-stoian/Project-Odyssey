# Project Odyssey: assembly progress (19)

## AP-020 · 2026-09-30 · after US-013

| | |
|---|---|
| Codex / requirements | v1.5 / v1.5 |
| Repository | `qa` (this snapshot's commit); `main` tagged `m0-done`, `m1-done`, `m1b-done` |
| Milestone | **M2 Console clan simulator: 4 of 7** |
| Next | S-US-014 Write a readable chronicle |

### Story just finished: US-013 Remember events and spread gossip
- People remember: a gift or a theft someone saw becomes a memory of who did what, when and how it felt, and it changes what they think of each other (opinions -100..100).
- Gossip: when two people talk, the speaker may pass on the strongest story the listener lacks, at half strength, so reputations travel. Kind people give gifts; Greedy people sometimes steal from the store.
- Minor memories fade after 60 days; a theft is remembered for life.
- One year of seed 42: 85 gift memories, 26 witnessed thefts, 14 stories heard second-hand. All numbers are in `assets/data/sim/social.json`.
- CI on `qa`: green ([run 36653288393](https://github.com/marius-stoian/Project-Odyssey/actions/runs/36653288393)).

### Milestones
| ID | Milestone | Stories done | State |
|---|---|---|---|
| M0 | Tooling ready | 4 / 4 | Done, `m0-done` |
| M1 | Luna engine: walking skeleton | 5 / 5 | Done, `m1-done` |
| M1b | Luna Physics | 5 / 5 | Done, `m1b-done` |
| M2 | Console clan simulator (KILL GATE 1) | 4 / 7 | In progress |
| M3-M6 | | 0 / 28 | |
| | **MVP total** | **18 / 49** | |

### Decisions and codex issues
- No new decisions. Awaiting your review (delegated to Dominus): D-02, D-16, D-17.
- No new codex issues.
