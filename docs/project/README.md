# Project documents

Everything written about Project Odyssey outside the code: the requirements, Anima's Codex material, diagrams and the superseded discovery files.

**Masters live on Google Drive** (`My Drive/~gamerrr/Project Odysseus/`), where Dominus and Anima write them. The files here are mirrored copies: when Claude Code starts in this repo, [tools/sync-workspace.ps1](../../tools/sync-workspace.ps1) copies any Drive file that changed into this folder, and Claude commits it. Edit the Drive file, not the copy here, or the next sync will overwrite your change.

## requirements/

| File | What it is | Status |
|---|---|---|
| [Project Odyssey.docx](requirements/Project%20Odyssey.docx) | The requirements, architecture, MVP plan and all 44 user stories | **Source of truth for WHAT**, v1.5 (Luna first, Luna Physics) |
| [Project Odyssey - MVP Backlog.xlsx](requirements/Project%20Odyssey%20-%20MVP%20Backlog.xlsx) | Working tracker: timeline with Gantt chart, dependencies, epics, stories with status | Generated from chapter 12 of the docx; the docx wins if they differ |

## codex/

The Codex itself is [docs/Codex.md](../Codex.md) (source of truth for HOW and ORDER, v1.4). Supporting material:

| File | What it is | Status |
|---|---|---|
| [Project Odyssey - Mraw Build Codex.docx](codex/Project%20Odyssey%20-%20Mraw%20Build%20Codex.docx) | Anima's analysis behind the Codex: brief, delivery format, prompt design | Written for Codex v1.1; does not cover Luna (v1.2 changes are in the Codex amendment log) |
| [Anima Prompt Catalog (pending upload).html](codex/Anima%20Prompt%20Catalog%20(pending%20upload).html) | Anima's prompt catalog, ready to become a Google Doc | Pending: the Drive connector could not create the Doc on 2026-09-29 |
| [anima-SKILL (upload to claude.ai).md](codex/anima-SKILL%20(upload%20to%20claude.ai).md) | The Anima skill, prepared for upload to claude.ai | Same text as [.claude/skills/anima/SKILL.md](../../.claude/skills/anima/SKILL.md) |

## diagrams/

| File | What it shows | Status |
|---|---|---|
| [Amek Workflow.png](diagrams/Amek%20Workflow.png) | Brief (Mraw) -> Codex (Anima) -> Assemble (Mraw) | Current |
| [Odysseus - Architecture Diagram.png](diagrams/Odysseus%20-%20Architecture%20Diagram.png) | The five layers (Figure 1 in the docx) | Drawn before the name Luna and before Luna Physics (ARC-09, ARC-10): the Physics layer is not in the picture |
| [Odysseus - MVP Timeline.png](diagrams/Odysseus%20-%20MVP%20Timeline.png) | MVP timeline (Figure 2 in the docx) | Drawn for v1.3: shows neither the engine-first order nor M1b. The table in docx section 12.5 is correct |

## archive/

Discovery files that were merged into the requirements document (v1.0, section 1.2) and are superseded by it. Kept for history; do not work from them.

| File | What it was |
|---|---|
| [Odysseus - Overview.md](archive/Odysseus%20-%20Overview.md) | The first overview (v0.1), from the Java/libGDX idea |
| [Odysseus - Council Review.xlsx](archive/Odysseus%20-%20Council%20Review.xlsx) | 22 council proposals; merged in Appendix A |
| [Project Odysseus - Requirements.xlsx](archive/Project%20Odysseus%20-%20Requirements.xlsx) | The 12-sheet requirements workbook; merged in full |
| [Top_50_Professions.xlsx](archive/Top_50_Professions.xlsx) | Fifty professions with first-documented dates; merged in section 5.5 |

Not mirrored: *Project Odysseus Design* is a Google Sheet (a Drive shortcut, not a file). Open it in Drive; its five design pillars now structure chapter 5 of the requirements.
