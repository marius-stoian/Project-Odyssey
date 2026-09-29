# Technical Writer playbook

## Mindset
- **Write for a reader with a task.** Every doc answers: who is reading, what are they trying to do, what do they already know?
- **Docs are product.** Wrong docs are worse than no docs. A doc nobody finds doesn't exist.
- **Show, then tell.** A working example beats three paragraphs of explanation.
- **Cut ruthlessly.** Short sentences, active voice, one idea per paragraph, the most important thing first.
- **Consistency is kindness.** Same term for the same thing everywhere; same structure for the same kind of page.

## Diátaxis — pick the right document type
| Type | Serves | Shape |
|---|---|---|
| **Tutorial** | Learning, newcomers | Guided lesson, guaranteed success, minimal choice |
| **How-to guide** | A goal, competent user | Numbered steps to a specific result |
| **Reference** | Lookup | Complete, accurate, consistently structured (APIs, config, CLI) |
| **Explanation** | Understanding | Why things are the way they are, trade-offs, architecture |
Never mix types on one page; link between them.

## Templates

### README
```
# Project name
One-sentence what-and-why. (Screenshot/GIF if visual.)

## Quick start       ← copy-paste to a working result in < 5 minutes
## Requirements
## Installation
## Usage             ← the 3 most common tasks with examples
## Configuration     ← table: option | default | description
## Project structure (for contributors)
## Development       ← build, test, lint commands
## Contributing
## License
```

### How-to guide
```
# How to [achieve goal]
Before you begin: prerequisites (versions, permissions, prior steps).
1. [Imperative step]. Expected result: [what they should see].
2. ...
Verify: how to confirm it worked.
Troubleshooting: symptom → cause → fix.
Next steps: links.
```

### API reference entry
```
## METHOD /path  (or functionName(params))
Summary sentence.
Parameters: name | type | required | default | description
Request example / Response example (real, runnable)
Errors: code | meaning | how to fix
Notes: rate limits, auth, idempotency, versioning
```

### Architecture Decision Record (ADR)
```
# ADR-NNN: [decision]
Status: proposed / accepted / superseded by ADR-XXX
Context: forces at play
Decision: what we chose
Consequences: good, bad, and what it rules out
Alternatives considered: and why not
```

### Release notes / changelog
Keep a Changelog format: Added / Changed / Deprecated / Removed / Fixed / Security. Lead with user benefit; link issues; flag breaking changes and migration steps at the top.

### In-product text (microcopy)
Buttons: verb + object ("Save house", not "OK"). Errors: what happened + why + what to do next, no blame. Empty states: what goes here + how to start. Keep under the UI's space budget; plan for 30–40% text expansion in localization.

## Style rules
- Second person ("you"), present tense, active voice, imperative steps.
- Define acronyms once; maintain a glossary for the project.
- Code blocks: complete, copy-pasteable, language-tagged; show expected output.
- Headings are task-oriented ("Configure save slots", not "Save slot configuration").
- Accessibility: descriptive link text, alt text for images, don't rely on color.

## Review checklist
Accurate against the current code (actually run the steps)? · Prerequisites complete? · Every step has an observable result? · Terms consistent with UI and code? · Scannable (headings, lists, tables)? · Dated/versioned where it matters?

## Red flags
"Simply", "just", "obviously" · screenshots of code · docs describing features that were cut · walls of text before the first example · duplicated content that will drift.
