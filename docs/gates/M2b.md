# Exit review M2b: Story engine (KILL GATE 1 retry)

Codex v1.6, prompt X-M2b, 2026-09-30. Exit criteria: the chronicle tells the clan's story in fewer, bigger episodes with reasons; every death and feud says why; the 100-year soak still runs without crashing and the determinism test passes; the owner reads the printed story and judges whether it is a story.

**Result: all four criteria met. The owner answered Option A, Go (2026-09-30): "it is a story".**

## 1. Fewer, bigger episodes with reasons: **Met**
`odysseus_headless --seed S --years 100 --story` prints at most 40 named episodes (each: beginning = the cause, turn, end, and the people), then the births, deaths, pairings and feuds with their reasons; `--chronicle` still prints every event. Tests: `US-115 Episode`, `US-115 Fewer, bigger`, `US-115 The story, then the lines with reasons`, and the command-line check `US-115 Both views` (evidence: [US-115](../evidence/US-115/headless-seed7-100years-story.txt)).

## 2. Every death and feud says why: **Met**
Deaths name their cause and the event behind it (hunger after the failed harvest or a theft, a fight, wounds, sickness, childbirth, a mammoth hunt); feuds name the strongest grudge ("over stolen meat", "after a bitter quarrel", "over their love for X"); blame, revenge, exile, rejection, jealousy and parting all cite their causes, and `--why <id>` prints the chain (`US-110 Traceable`).

## 3. The 100-year soak runs without crashing; determinism holds: **Met**
Your limit for this review was 10 seeds. Every run finished with exit code 0 and no extinction:

| Seed | Alive after 100 years | Episodes told | World hash |
|---|---|---|---|
| 1 | 48 | 40 | 4430621202385867800 |
| 2 | 44 | 40 | 9643953432220383216 |
| 3 | 50 | 40 | 533555592276737973 |
| 4 | 51 | 40 | 12870503649383978911 |
| 5 | 31 | 40 | 7146766855319735141 |
| 6 | 29 | 40 | 13684250523337908414 |
| 7 | 45 | 40 | 15810530208701249003 |
| 8 | 39 | 40 | 8468189839815333578 |
| 9 | 33 | 40 | 14782575668967733311 |
| 10 | 29 | 40 | 15470103868849704145 |

`ctest` passes 21 of 21 in Debug and in Release with zero warnings, including the determinism tests, `US-015 Run` (seed 7, 100 years) and the save round trips. CI on `qa` is green for US-113 and US-114; US-115's run was queued when this review was written.

## 4. The owner reads the printed story and judges: **Waiting for you (human gate)**
Agents cannot measure this. Read [M2b-reader-packet.md](M2b-reader-packet.md) and answer in [D-GATE-M2b](../decision-requests/D-GATE-M2b.md).

## Team notes
- What we see as weak: many episodes are hunts and hard winters; partings and rejections are rare; feud episodes can span years.
- Balance checks used 10 seeds, not 100, at your request; the population is healthier than before M2b (29 to 51 alive; the low end was 9).
- Also merged to `qa` at your request: the hero's sword, the enemy with HP and the red hit flash (a demo outside the Codex).
