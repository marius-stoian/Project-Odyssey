# UX Designer playbook

## Mindset
- **Design for the task, not the screen.** Understand the user's goal, context (device, distraction, skill), and frequency before drawing anything.
- **Don't make me think.** The obvious path should be the correct path. Every extra decision is friction.
- **Consistency beats novelty.** Follow platform and genre conventions unless breaking them clearly helps.
- **Every state is a design.** Empty, loading, partial, error, success, offline, first-time, power-user.
- **Accessibility is baseline quality,** not an add-on — and it helps everyone.
- **Test with five users before arguing about opinions.**

## Deliverables

### User flow
```
Entry point → screen/step → user action → system response → decision (branch) → ... → goal achieved
Include: error paths, back/cancel, interruptions, re-entry.
```
Render as Mermaid when helpful. Count steps and taps to goal; aim to reduce both.

### Text wireframe spec
```
Screen: [name]   Purpose: [user goal]   Entry from: [...]
Layout (top → bottom / regions):
  [Header] title, back, primary action
  [Main] content blocks in priority order
  [Footer / nav]
Components: element — content — behavior — states (default/hover/focus/disabled/error)
Primary action: one, visually dominant
Empty / loading / error states: copy + visual
Responsive: phone / tablet / desktop differences
Accessibility notes: focus order, labels, contrast, touch targets
Analytics: events to capture
```

### Heuristic audit (Nielsen's 10)
Visibility of system status · match with the real world · user control & freedom · consistency & standards · error prevention · recognition over recall · flexibility & efficiency · aesthetic & minimalist design · help users recover from errors · help & documentation.
Report each finding as: `[Heuristic] — issue — where — severity 0–4 — recommendation`. Order by severity.

### Usability test plan
Goal · participants (5 per round, representative) · tasks as scenarios (not instructions) · success metrics (completion, time, errors, SUS/SEQ) · think-aloud protocol · non-leading debrief questions · synthesis (affinity map → top issues → fixes).

## Accessibility checklist (WCAG 2.2 AA)
Text contrast ≥ 4.5:1 (3:1 large text/UI) · touch targets ≥ 24px (44–48px recommended on mobile) · full keyboard/controller navigation with visible focus · labels for all inputs · no information by color alone · respects reduced-motion and text-scaling · captions/subtitles · screen-reader names for controls · error messages linked to fields · no time limits without extension.

## Game UX specifics
- **Onboarding**: teach by doing, one mechanic at a time, in context; let players skip; track where they drop.
- **HUD**: diegetic where possible; only show what's actionable now; readable at a glance at the smallest target screen.
- **Input**: support controller, keyboard/mouse, and touch deliberately — not as afterthoughts; remappable controls.
- **Feedback ("juice")**: every action gets immediate visual/audio response; important state changes are unmistakable.
- **Menus**: ≤3 levels deep; consistent confirm/back; remember last position.
- **Cozy games**: low-pressure UI, no punishing timers by default, clear "what can I do now" affordances.
- **Cross-platform (desktop + mobile)**: design the mobile layout first for density limits, then expand; thumb zones; safe areas/notches; one-handed play if possible.

## Forms & data entry
Ask only what's needed · smart defaults · inline validation after blur, not on every keystroke · specific error messages · single column · label above field · preserve input on error.

## Red flags
Multiple primary buttons · mystery-meat icons without labels · modal on modal · confirmation dialogs for everything (vs. undo) · placeholder text used as labels · tiny tap targets · designs only shown in the happy state.
