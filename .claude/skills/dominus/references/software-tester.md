# Software Tester playbook

## Mindset
- **Testing is risk reduction, not bug counting.** Test most where failure is likeliest and costliest.
- **Quality is built in, not inspected in.** Shift left: review requirements and AC for testability before code exists.
- **Think like a user, then like an adversary.** Happy path first, then misuse, edge values, interruptions, concurrency, bad data, hostile input.
- **Automate the boring, explore the interesting.** Regression belongs to machines; discovery belongs to curious humans.
- **A bug report is a sales pitch for a fix.** Make it reproducible, specific, and show impact.

## Test strategy (one page)
```
Scope: in / out
Risks ranked: [risk] — likelihood — impact — how we test it
Levels: unit / integration / contract / E2E / manual exploratory / UAT
Types: functional, regression, performance, security, accessibility, compatibility (devices/OS/browsers/screen sizes), localization, save/load & migration, install/upgrade
Environments & data: how test data is created and reset
Entry / exit criteria: what "ready to test" and "ready to ship" mean
Automation: what's automated, where it runs (CI), flake policy
Tools:
```
Test pyramid: many fast unit tests, fewer integration, few E2E. For games add: playtests, soak tests (long sessions), save-file compatibility, frame-time budgets, input devices.

## Test design techniques
- **Equivalence partitioning** and **boundary value analysis** (min-1, min, min+1, max-1, max, max+1, empty, null, huge).
- **Decision tables** for business rules with multiple conditions.
- **State transition** testing for anything with states (orders, quests, NPC moods, day/night).
- **Pairwise** for combinations of settings/devices.
- **Error guessing / heuristics**: SFDIPOT (Structure, Function, Data, Interfaces, Platform, Operations, Time); interrupt it, repeat it, reverse it, do it twice fast, do it offline, do it at midnight / DST / leap year, use unicode and very long strings.

## Test case format
```
ID: TC-042   Traces to: US-17 / FR-07
Title: [behavior under condition]
Preconditions:
Steps: 1. ... 2. ...
Test data:
Expected result: (observable, specific)
Priority: P1–P3   Type: functional / negative / boundary / ...
```
Gherkin AC from the PO can double as test cases — reuse, don't rewrite.

## Bug report
```
Title: [Where] [what goes wrong] [when/condition]  e.g. "Inventory: stacked items duplicate when dragged during autosave"
Environment: build/commit, OS/device, config
Steps to reproduce: numbered, minimal
Expected: / Actual:
Frequency: always / 3 of 10 / once
Severity (impact): blocker / critical / major / minor / trivial
Priority (urgency): set with PO
Evidence: logs, screenshot/video, save file, stack trace
Notes: workaround, first bad build, suspected area
```

## Exploratory testing charter
`Explore [area] with [resources/technique] to discover [information/risk].` Timebox 60–90 min; record notes, bugs, questions, and coverage.

## Release readiness (go/no-go)
Open blockers/criticals = 0 · regression suite green · performance within budget · no known data-loss or security issues · known issues documented in release notes · rollback plan exists · monitoring/alerts in place.

## Writing automated tests (with the Programmer hat)
- Arrange / Act / Assert; one behavior per test; name tests as sentences (`deducts_stamina_when_chopping_tree`).
- Test behavior, not implementation; avoid mocking what you own when a real object is cheap.
- Deterministic: seed RNGs, freeze time, isolate filesystem/network.
- Every bug fix ships with a test that failed before the fix.

## Red flags
AC that say "works correctly" · 100% coverage used as a quality claim · flaky tests ignored · testing only on the dev's machine · QA as a phase at the end.
