# Project Manager playbook

## Mindset
- **The plan is a communication tool, not a prophecy.** Its value is in shared understanding and early warning, not in being right.
- **Scope, time, cost, quality — pick your constraint.** When one moves, something else must. Make the trade explicit.
- **Risks are managed before they are issues.** An issue is a risk you ignored.
- **Bad news early is good news.** Create a culture where red status is safe to report.
- **Decisions need owners and dates.** Unowned decisions become schedule slips.
- **Protect the team's focus.** Context switching is the silent schedule killer.

## Choose the method
- **Scrum**: product discovery, evolving requirements, stable team, 1–2 week sprints.
- **Kanban**: continuous flow, support/live ops, variable request sizes — limit WIP.
- **Waterfall/stage-gate**: fixed scope with regulatory or contractual constraints.
- **Hybrid**: milestone plan for stakeholders + agile execution inside milestones (most common in games: concept → pre-production → vertical slice → production → alpha → beta → gold → live).
- **Solo/indie**: a simple milestone list, a weekly goal, and a ruthless cut list is enough.

## Deliverables

### Project charter (one page)
```
Project: [name]   Sponsor:   PM:   Date:
Why: problem / opportunity and business case
Objectives & success metrics:
Scope: in / out
Key deliverables & milestones: [milestone — date — exit criteria]
Budget / resources:
Stakeholders & communication plan:
Top risks:
Assumptions & constraints:
```

### Work breakdown & estimate
- Break work down until items are ≤ 2–3 days or clearly estimable.
- Estimate with ranges or three-point (optimistic / likely / pessimistic → PERT = (O + 4L + P) / 6). Add explicit contingency (15–30%) visible to stakeholders, not hidden in tasks.
- Identify the critical path and dependencies; front-load the riskiest work.

### Milestone plan
| Milestone | Target date | Exit criteria (demonstrable) | Owner | Status |
Exit criteria must be observable ("player can complete one full in-game day on desktop and Android"), not activity-based ("art 50% done").

### Risk register
| ID | Risk (cause → event → effect) | Probability 1–5 | Impact 1–5 | Score | Response (avoid/mitigate/transfer/accept) | Owner | Trigger | Status |

### RACI (only when multiple parties)
| Deliverable | Responsible | Accountable (one) | Consulted | Informed |

### Status report
```
Overall: 🟢/🟡/🔴 — one-sentence why
Since last: done
Next: planned
Risks/issues: top 3 with action and owner
Decisions needed: [decision] by [date] from [who]
Metrics: burn-up / milestone progress / budget
```

### Retrospective
What went well · what didn't · what we'll try (max 3 actions, each with owner) · check last retro's actions.

## Scope control
Change request: what, why, impact on time/cost/quality/risk, options (do / defer / swap / reject), decision owner. Maintain a visible **cut list** — pre-agreed features to drop if time runs short.

## Solo & small-team heuristics
- Timebox everything; a "done" vertical slice beats five half-built systems.
- Keep a single source of truth (one board, one doc).
- Estimate, then multiply by your personal historical factor (track it!).
- Weekly review: shipped? blocked? what to cut?

## Red flags
100% utilization planned · no contingency · milestones without exit criteria · status always green until it's red · "we'll catch up in the next phase" · decisions made in chat and never recorded.
