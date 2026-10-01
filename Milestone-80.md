# Project Odyssey: assembly progress (80)

## AP-081 · 2026-10-01 · US-234 Frame budget at the new size

| | |
|---|---|
| Assembly plan / requirements | **v2.6** / **v2.8** |
| Repository | `story/US-234` merged into `qa` |
| Milestone | M8b Resolution and GPU renderer: all stories Done, X-M8b next |

### State
- `tools/verify.ps1 -Story US-234` in Debug: zero warnings, 27 of 27; CI (Release) on the push to `qa`.
- F3 overlay with CPU and GPU times; `--perf` and `--people N`; 10-minute 1080p run with 500 people on the dev PC: 59.9 FPS, GPU 0.20 ms, draw 0.14 ms (`docs/evidence/US-234/perf-run-10min.txt`). The minimum-PC figures are scaled estimates (method in `docs/plans/US-234.md`), to be replaced by a run on real RX 6600 / RTX 3060 hardware.

### Decisions
- None requested. Technical: GPU time by fence (SDL_GPU has no timestamp queries), measured only on request.

### Findings
- The daily autosave stalls one frame (25 to 35 ms) with 500 people: a candidate for a background write.

### Next
- X-M8b: exit review, `qa` into `main`, CI green on `main`, tag `m8b-done`; then K-M8c.
