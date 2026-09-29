# Business Analyst playbook

## Mindset
- **The stated request is a solution; find the problem.** Ask "why?" until you reach a business outcome (revenue, cost, risk, compliance, experience). Five whys, gently.
- **Requirements are discovered, not collected.** Stakeholders describe symptoms and workarounds. Observe the actual process; the gap between what people say and do is where the real requirement hides.
- **Every requirement must be testable, traceable, and owned.** If QA can't verify it, it's a wish. If it doesn't trace to a goal, it's scope creep. If nobody owns it, it won't be decided.
- **Name the "as-is" before the "to-be."** You can't improve a process you haven't mapped honestly, including exceptions and manual workarounds.
- **Stakeholders disagree; surface it early.** Conflicting requirements found in UAT cost 100× more than ones found in a workshop.

## Elicitation toolkit
- Stakeholder interviews (open → probing → confirming questions), workshops, observation/job shadowing, document and data analysis, surveys, prototyping, reviewing existing systems and support tickets.
- Good questions: *What triggers this? What happens next? What goes wrong? What do you do when it does? How often? Who else touches this? How do you know it worked? What would you stop doing if this existed?*

## Deliverables

### Problem / opportunity statement
```
Problem: [who] struggles with [what] when [situation], resulting in [measurable impact].
Goal: [outcome] measured by [metric] from [baseline] to [target] by [date].
Constraints: budget, time, regulation, tech, people.
Assumptions: [list] — each with how we'll validate it.
Out of scope: [explicit list].
```

### Stakeholder analysis
| Stakeholder | Role | Interest | Influence (H/M/L) | Impact on them | What they need from us | Engagement |
|---|---|---|---|---|---|---|
Plot on a power/interest grid: manage closely / keep satisfied / keep informed / monitor.

### Business Requirements Document (lean)
1. Background & problem · 2. Business objectives & success metrics · 3. Scope (in / out) · 4. Stakeholders · 5. Current state (as-is) · 6. Future state (to-be) · 7. Business requirements (BR-01…) · 8. Functional requirements (FR-01…, traced to BRs) · 9. Non-functional requirements (performance, security, availability, accessibility, localization, compliance) · 10. Business rules · 11. Data requirements · 12. Assumptions, constraints, dependencies · 13. Risks · 14. Open questions (owner, due date) · 15. Glossary

Requirement format: `FR-07 — The system shall [behavior] when [condition] so that [value]. Priority: Must/Should/Could/Won't. Source: [stakeholder]. Traces to: BR-02. Acceptance: [verifiable criterion].`

### Process map (as-is / to-be)
Use swimlanes per actor. Text form:
```
[Actor] → step → (decision?) → yes: step / no: step → [handoff to Actor] → ... → end state
Pain points: ⚠ numbered, with frequency and cost
```
Render as a Mermaid flowchart when the environment supports it.

### Gap analysis
| Capability | Current state | Desired state | Gap | Impact | Recommended action | Priority |

### Requirements traceability matrix
| Business goal | BR | FR | User story | Test case | Status |

## Checklists
- Non-functional requirements asked about explicitly? (Nobody volunteers them.)
- Every business rule has an example, including edge cases and exceptions?
- Data: sources, owners, quality, retention, privacy (GDPR etc.) covered?
- Every "etc.", "user-friendly", "fast", "flexible" replaced with something measurable?
- Glossary agreed? (Half of all requirement defects are two people using one word differently.)

## Red flags
- Requirements written as UI designs ("add a dropdown").
- "All users" as a persona. No success metric. No out-of-scope list.
- One stakeholder speaking for everyone. Sign-off without reading.
