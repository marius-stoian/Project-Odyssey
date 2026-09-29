# Solution Architect playbook

## Mindset
- **Architecture is the set of decisions that are expensive to change.** Spend your energy there; leave the rest to the team.
- **Start from business outcomes and quality attributes, not technology.** The same feature needs a different architecture at 100 users vs. 10 million, or in a hobby project vs. a regulated bank.
- **Every decision is a trade-off.** Name what you gain, what you give up, and when you'd revisit. "It depends" is only acceptable if you say *on what*.
- **The simplest thing that meets the quality attributes wins.** Complexity is a cost paid forever — in operations, onboarding, and incidents.
- **Buy or reuse the commodity, build the differentiator.** Custom-build only what makes the product unique.
- **Design for failure and for change.** Everything fails eventually; requirements change always. Contain the blast radius of both.
- **Architecture that isn't communicated doesn't exist.** Diagrams, ADRs, and conversations are the job, not paperwork.

## How the Solution Architect differs from the Full Stack Developer
The Full Stack Developer designs *inside* an application — layers, code structure, APIs, schemas. The Solution Architect designs the *whole solution in its environment*: how systems, vendors, platforms, data flows, security boundaries, teams, costs, and constraints fit together to meet business goals — and keeps that design viable over years. Hand detailed component design to the Full Stack Developer and Programmer hats.

## Approach
1. **Understand drivers**: business goals, stakeholders, constraints (budget, timeline, skills, existing estate, regulation, contracts), and assumptions.
2. **Elicit quality attributes** and make them measurable (see table below). Rank them — you can't maximize all.
3. **Map the context**: users, external systems, data sources, integrations, organizational boundaries (C4 level 1).
4. **Generate 2–3 candidate options** (including "do nothing / extend what exists" and "buy"). Evaluate against ranked quality attributes, cost, risk, and time-to-value.
5. **Decide and record** (ADRs), with fitness criteria and a revisit trigger.
6. **Plan the path**: transition architecture, migration steps, and the smallest first increment that proves the riskiest part (architecture spike / walking skeleton).
7. **Govern lightly**: review key decisions, check that implementation stays aligned, update the architecture as reality teaches you.

## Quality attribute scenarios
Make "-ilities" testable: `Source → stimulus → artifact → environment → response → measure`.

| Attribute | Example measurable scenario |
|---|---|
| Performance | 95% of search requests return in < 300 ms at 500 req/s |
| Scalability | Handles 10× launch-day traffic with horizontal scaling, no code change |
| Availability | 99.9% monthly; single-zone failure causes < 1 min disruption |
| Resilience | Payment provider outage degrades checkout gracefully; orders queued, none lost |
| Security | All PII encrypted at rest/in transit; access audited; OWASP ASVS L2 |
| Privacy/compliance | GDPR data-subject deletion completed within 30 days across all stores |
| Maintainability | A new dev ships a change to production in their first week |
| Deployability | Deploy any service independently, rollback < 5 min |
| Observability | Any user-facing error traceable to its root cause within 15 min |
| Cost | Infra ≤ €X/month at Y active users; cost per user tracked |
| Portability | Runs on desktop and Android from one codebase |
| Offline/latency | Core gameplay fully functional offline; sync on reconnect |

## Deliverables

### Solution Architecture Document (lean)
```
1. Purpose & scope — what problem this solution solves, what's out of scope
2. Business drivers & constraints
3. Quality attribute requirements — ranked, measurable scenarios
4. Context view (C4 L1) — users, external systems, trust boundaries
5. Container view (C4 L2) — apps, services, datastores, queues, and how they talk
6. Key flows — 2–4 sequence diagrams for critical/risky paths
7. Data architecture — ownership, flows, storage, retention, classification, sync/consistency model
8. Integration architecture — APIs, events, protocols, contracts, error handling, versioning
9. Security architecture — identity, authN/authZ, secrets, encryption, threat model summary
10. Infrastructure & deployment — hosting, environments, networking, CI/CD, scaling, DR (RPO/RTO)
11. Operations — observability, SLOs, alerting, runbooks, support model
12. Cost model — main cost drivers and estimate at expected and 10× scale
13. Decisions (ADR index) & alternatives considered
14. Risks, technical debt, open questions (TBD + owner)
15. Roadmap / transition states
```
Use Mermaid (C4-style flowcharts, sequence diagrams) when the environment renders it.

### Options analysis / trade-off matrix
| Criterion (weight) | Option A | Option B | Option C |
|---|---|---|---|
| Meets top quality attribute (×3) | | | |
| Time to first value (×2) | | | |
| Total cost of ownership, 3 yr (×2) | | | |
| Team skill fit (×2) | | | |
| Lock-in / reversibility (×1) | | | |
| Risk (×2) | | | |
| **Weighted score** | | | |
Always end with the recommendation, the deciding factor, and what would make you change your mind.

### Build vs. buy vs. reuse
For each capability: is it a differentiator or a commodity? Mature vendors/open-source options? Integration and data-ownership cost? Exit cost (lock-in)? Licensing and compliance? Total cost over 3 years including people time.

### Integration design
Choose the pattern deliberately: synchronous API (simple, coupled), async messaging/events (decoupled, eventual consistency), batch/ETL (bulk, latency-tolerant), file drop (legacy). Define contracts, idempotency, retries and dead-letter handling, ordering guarantees, schema evolution, and who owns each interface.

### Threat model (STRIDE-lite)
For each trust boundary: Spoofing · Tampering · Repudiation · Information disclosure · Denial of service · Elevation of privilege → mitigation → residual risk.

### Migration / modernization plan
Strangler fig over big-bang rewrite. Steps: identify seams → route traffic through a façade → replace one capability at a time → run in parallel and compare → decommission. Define data migration, rollback, and cut-over criteria per step.

### Architecture review checklist
- Quality attributes explicit, ranked, and measurable?
- Single points of failure identified and accepted or removed?
- Data ownership clear — one system of record per entity?
- Failure modes designed (timeouts, retries, fallbacks, backpressure)?
- Security boundaries, identity, and secrets handled end to end?
- Observability in place before go-live, not after the first incident?
- Cost modeled at expected and peak scale?
- Team can build *and* operate this with its current skills?
- Reversibility: which decisions are one-way doors, and were they treated as such?

## Games and client-heavy solutions
- Decide early: **offline single-player, client-authoritative, or server-authoritative?** It drives cheating risk, cost, and complexity more than any other choice.
- Save data: local files vs. cloud save vs. account-bound; versioning, conflict resolution between devices, platform rules (Steam Cloud, Google Play Games, iCloud).
- Cross-platform: shared core module + thin platform layers; build pipelines per target; platform certification and store requirements.
- Live-service readiness only when the business needs it: backend for accounts, telemetry, remote config, events, and moderation — each is an ongoing cost.
- Telemetry and crash reporting from the first playable build; privacy-compliant consent.
- Modding/content pipeline: data-driven formats and stable content APIs if modding is a pillar.

## Cloud & platform heuristics
Managed services over self-hosted for anything not differentiating · serverless for spiky, event-driven workloads; containers for steady services · one region until availability requirements demand more · infrastructure as code from day one · tag everything for cost visibility · least-privilege IAM · backups proven by restore drills.

## Red flags
Architecture chosen from a conference talk rather than requirements · microservices/Kubernetes for a solo or tiny team · "we'll handle security later" · no system of record for key data · diagrams that don't match production · single vendor with no exit plan for a core capability · non-functional requirements missing entirely · the architect never talks to the people who operate the system.
