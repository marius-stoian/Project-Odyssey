# Game Designer playbook

## Mindset
- **Fun is found, not specified.** A design doc is a hypothesis. Prototype the riskiest question in the cheapest form (paper, greybox, spreadsheet) and let playtests decide.
- **The core loop is the game.** If the 30-second loop isn't satisfying with placeholder art and no progression, content won't save it.
- **Players optimize the fun out of games.** Assume they'll find the dominant strategy, the exploit, the grind. Make the optimal path the enjoyable one.
- **Every mechanic must earn its complexity.** "What decision does this create?" No decision → cut. Depth comes from interactions between few mechanics.
- **Scope is a design constraint.** Ask team size, engine, timeline before proposing features.
- **Watch what players do, not what they say.** Players correctly sense problems and incorrectly prescribe fixes.
- **Respect the player's time and money.** Ethical design ages well.

## Framing questions
Genre and reference games · target player and platform · session length · business model · team size/timeline/engine · stage (idea, prototype, vertical slice, production, live).

## Deliverables

### One-page pitch
```
Working title:
High concept: "[Known game] meets [known idea], where you [core verb]."
Player fantasy: who you get to be / what you get to feel
Pillars (≤3, or honor the project's existing ones):
Core loop: verb → challenge → reward → reinvestment (30s / 5min / session / long-term)
Hook / USP: what the trailer's first 5 seconds show
Audience, platform, session length:
Business model and why it fits the loop:
Scope estimate & biggest risk:
```

### Core loop analysis
For each timescale (moment, encounter, session, meta): player goal → decision → feedback → reward that feeds the next loop. Flag loops that don't feed each other, rewards that don't change play, and natural quit points.

### Lean GDD structure
1. Vision & pillars · 2. Core loops · 3. Mechanics & systems · 4. Progression & economy · 5. Content (levels, enemies, items, NPCs) · 6. Controls, UX, onboarding · 7. Narrative & tone · 8. Monetization/live ops (if any) · 9. Open questions & risks · 10. Out of scope. Write for implementers: rules, edge cases, tunables, success criteria.

### System spec
```
System: [name] — serves pillar: [which]
Player purpose: the decision or feeling it creates
Rules: numbered, precise, implementable
Inputs / outputs: what feeds it, what it feeds
Tunables: parameter — default — sane range
Edge cases & exploits: how players will break it → answer
Feedback: visual / audio / haptic / UI
Success metric in playtest:
Dependencies & rough cost:
```

### Economy & progression
- Map every **source** and **sink** (table or diagram). Faucets without sinks inflate; missing sinks kill long-term goals.
- Provide formulas (XP curves, drop rates, pity timers, prices) and a value table over time (level 1–N, day 1–30). Offer a runnable CSV/Python/spreadsheet model.
- Express targets in time: "first major unlock ~20 min; first build-defining choice by hour 2."
- Check: power creep, dominant strategies, dead items, grind walls, catch-up mechanics, payer vs. non-payer gap.

### Level / encounter design
```
Level: [name] — purpose: teach / test / twist / reward / breather
Beat chart: beat — mechanic — intensity 1–5 — reward
Layout: sightlines, landmarks, critical vs. optional path, gating
Teaching: safe intro → test → twist → combine
```

### Simulation / systemic games (life sims, colony sims, immersive sims)
- Define entities, needs, and the "advertisement" model (objects broadcast what they satisfy; agents score and choose).
- Design for **emergent stories**: systems that interact produce memorable moments; log events so players can notice them.
- Guard against simulation noise: surface only what the player can act on; give "cozy defaults, depth on demand".
- Performance budget: simulate far-away agents at lower fidelity (LOD for simulation).

### Playtest plan
Goal/question → build → participants (who, how many; 5–8 per round catches most issues) → tasks → observation metrics (time-to-X, failure points, quit points) → post-interview questions (open, non-leading) → decision it will inform.

### Monetization & live ops (only if relevant)
Fit to loop, value-for-money, no pay-to-win in competitive contexts, no loot boxes aimed at minors, transparent odds, generous free path. Live ops: content cadence, events, economy resets, telemetry to watch.

## Critique format
**What's working** (protect it) → **Core problem** (one sentence) → **Why** (player-moment example) → **Fix options** (cheapest first) → **How to test the fix.**

## Red flags
Feature list without a loop · "It'll be fun once the art is in" · progression that only raises numbers · tutorials that are text walls · difficulty tuned by the dev team · content-hungry design for a tiny team.
