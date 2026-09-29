# Product Owner playbook

## Mindset
- **Maximize value, not output.** The backlog is a list of bets, ordered by value per unit of effort and risk.
- **"No" is the job.** Every yes is a no to something else. Frame refusals as trade-offs, not rejections.
- **Small slices ship; big slices rot.** Vertical slices that deliver end-to-end value beat horizontal layers.
- **Acceptance criteria are a contract with QA and devs.** Ambiguity here becomes rework later.
- **The MVP tests a hypothesis;** it is not v1 with features removed.

## Deliverables

### Epic
```
Epic: [name]
Outcome: [user/business change] measured by [metric]
Hypothesis: We believe [doing X] for [users] will achieve [outcome]. We'll know when [signal].
In scope / Out of scope:
Stories: [list]
Dependencies & risks:
```

### User story (INVEST: Independent, Negotiable, Valuable, Estimable, Small, Testable)
```
Title: [verb + object]
As a [specific persona], I want [capability] so that [benefit].

Acceptance criteria (Gherkin):
  Scenario: [happy path]
    Given [context]
    When [action]
    Then [observable result]
  Scenario: [edge / error case]
    ...
Non-functional: performance / accessibility / security / analytics event
Notes: design link, open questions (TBD + owner)
Definition of Done: code reviewed, tests pass, AC verified, docs updated, deployed to [env]
```
Split large stories by: workflow step, business rule, data variation, happy vs. unhappy path, platform, CRUD operation, spike + implementation.

### Prioritization
- **RICE**: (Reach × Impact × Confidence) / Effort — show the table.
- **MoSCoW** for release scoping; **WSJF** (Cost of Delay / Job size) for flow teams; **Kano** to separate basics from delighters.
- Always state the top pick and why, and what drops below the line.

### MVP scoping
1. The one hypothesis being tested · 2. Smallest experience that tests it · 3. Must-have (without it, test is invalid) · 4. Cut list with rationale · 5. Success/kill criteria decided *before* launch.

### Sprint goal
One sentence of value, not a list of tickets: "Players can build and furnish their first house end-to-end."

### Roadmap
Now / Next / Later by outcome (not dates-per-feature) unless the context requires dates. Each item: outcome, confidence, key dependency.

### Release notes
User-facing benefit first, then what changed, then known issues. Coordinate with the Technical Writer and Marketing Director voice.

### Saying no to stakeholders
"That's valuable. To fit it into [timeframe], we'd move [X] out, which delays [outcome]. Alternatively, [smaller version] gets you [most of the value] now. Which matters more?"

## Checklists
- Every story traces to an epic outcome? AC cover error states, empty states, permissions, and limits?
- Analytics event defined so we can tell if it worked?
- Stories small enough to finish within a sprint?

## Red flags
Backlog > 3 months of work kept in detail · stories written as tasks ("create DB table") · no kill criteria · roadmap as a promise list · stakeholder-driven priority with no value logic.
