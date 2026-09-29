# Programmer playbook

## Mindset
- **Code is read far more than written.** Optimize for the next person (often you, in six months).
- **Make it work, make it right, make it fast — in that order,** and only make it fast where a profiler says so.
- **Simple > clever. Explicit > implicit. Delete > add.** The best code is the code you didn't need to write.
- **Understand before changing.** Read the surrounding code, run it, reproduce the bug. Fixes to misunderstood code create new bugs.
- **Leave it better than you found it** — within the scope of the task, not as a surprise rewrite.

## Craft principles
- Small functions with one purpose; names that reveal intent (`daysUntilHarvest`, not `d`).
- Pure functions for logic; side effects pushed to the edges.
- Make illegal states unrepresentable (enums/sum types over boolean flags; value objects over primitives).
- Fail fast and loudly at boundaries; validate inputs; never swallow exceptions silently.
- DRY for knowledge, not for coincidentally similar code. Three strikes before abstracting.
- Composition over inheritance. Dependency injection over hidden globals/singletons where testability matters.
- SOLID as a guide, not a religion. YAGNI always.
- Comments explain *why*, not *what*. Match the codebase's comment density.

## Working method in Claude Code
1. **Locate**: find the relevant files, entry points, and existing patterns (Grep/Glob) before writing.
2. **Reproduce** (bugs): get a failing test or exact repro steps first.
3. **Plan the smallest change** that solves the real problem. For bigger work, outline steps and confirm direction.
4. **Implement** in the codebase's own style, language level, and idioms. No new dependencies without justification.
5. **Verify**: build, run tests, run the app/feature if possible. Add a test for new behavior and every bug fix.
6. **Report honestly**: what changed (file:line), how it was verified, what's left or uncertain.

## Debugging protocol
Reproduce → read the actual error and stack trace fully → form one hypothesis → test it with the smallest experiment (log, breakpoint, bisect with `git bisect`) → fix the root cause, not the symptom → add a regression test → check for the same bug pattern elsewhere.

## Refactoring
Only with tests as a safety net (write characterization tests first if none exist). Small behavior-preserving steps: rename, extract function, inline, move, replace conditional with polymorphism/table. Separate refactor commits from behavior changes.

## Performance
Measure first (profiler, benchmarks, frame timings). Fix algorithmic complexity before micro-optimizing. Common wins: avoid N+1, cache expensive pure results, batch I/O, avoid allocations in hot loops (critical in game loops — pool objects), spatial partitioning (grids/quadtrees) for many entities, lazy loading.

## Language/paradigm notes
- **Java / JVM** (incl. libGDX-style games): prefer records/immutable value types, `final` by default, avoid GC churn in the render loop, use interfaces at module seams, Gradle multi-module for core/desktop/android/etc.
- **TypeScript**: `strict` on, discriminated unions, no `any` without comment, zod/valibot at I/O boundaries.
- **Python**: type hints, dataclasses, `pathlib`, virtual envs, `pytest`.
- **C# / Unity**: avoid `Update()` bloat, ScriptableObjects for data, cache component lookups.
- Otherwise follow the ecosystem's standard formatter and linter.

## Code review checklist
Correct for edge cases (null/empty/max/concurrent)? · Errors handled and surfaced? · Tests meaningful and deterministic? · Names clear? · No dead code, debug prints, or commented-out blocks? · Security (input handling, secrets)? · Consistent with codebase conventions?

## Red flags
Copy-pasted blocks with small tweaks · boolean parameters controlling behavior · god classes/managers · catch-all exception handlers · magic numbers · TODOs without owners · "it works on my machine."
