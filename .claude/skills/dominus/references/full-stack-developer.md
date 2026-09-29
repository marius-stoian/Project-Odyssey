# Full Stack Developer playbook

## Mindset
- **Boring technology wins.** Choose well-understood tools unless the novelty is the product. Every new technology spends an "innovation token."
- **Design for change, not for imagined scale.** Clear boundaries and simple code outlive clever abstractions. Scale when metrics say so.
- **The data model is the foundation.** UI and APIs change often; migrations of bad schemas are expensive. Get entities, relationships, and ownership right first.
- **Own it end to end.** From schema to API to UI to deploy to monitoring — a feature isn't done until it runs in production and you can see it working.
- **Security and privacy are defaults, not features.**

## Architecture approach
Scope: the design *inside* an application. For solution-level questions — multiple systems, vendors, build vs. buy, cloud platform, integration patterns, enterprise quality attributes, cost models — lead with the Solution Architect playbook (`solution-architect.md`) and implement its decisions here.

1. Clarify: users & load, data sensitivity, platforms, team skills, budget, deadlines, existing stack.
2. Identify components and boundaries (client, API, domain/services, persistence, integrations, jobs).
3. Choose the simplest architecture that meets requirements — usually a modular monolith before microservices.
4. Define contracts: API shapes, events, data schemas, error model, versioning.
5. Plan non-functionals: authN/authZ, validation, caching, observability, backups, CI/CD, environments.
6. Record key choices as ADRs (see Technical Writer playbook).

### Architecture summary template
```
Context: what the system does, for whom, key constraints
Components: [name] — responsibility — tech — owns data?
Data model: core entities and relationships (Mermaid ER diagram when useful)
API surface: main endpoints/operations
Flows: 1–3 critical sequence walkthroughs
Non-functional: performance targets, security, availability, observability
Deployment: environments, pipeline, hosting, rollback
Risks & open decisions (TBD)
```

## Layers — what "good" looks like
- **Frontend:** component-driven, state kept close to where it's used, server state separated from UI state, accessible markup, responsive, loading/empty/error states for every view, performance budget (bundle size, LCP/INP).
- **API:** consistent resource naming, validation at the boundary, typed contracts (OpenAPI/GraphQL schema/TS types), pagination, idempotency for writes that may retry, meaningful errors with stable codes, versioning strategy.
- **Domain logic:** framework-independent where practical, pure functions for rules, explicit transactions.
- **Persistence:** normalized by default, indexes for real query patterns, migrations versioned and reversible, no N+1 queries, backups tested by restoring.
- **Integration:** timeouts, retries with backoff, circuit breakers for flaky dependencies, webhooks verified.
- **Ops:** 12-factor config, structured logs, metrics, traces, health checks, feature flags for risky releases, zero-downtime deploys.

## Security baseline (OWASP-minded)
Parameterized queries · output encoding · authZ checked server-side on every request · least privilege · secrets out of code (env/secret manager) · dependency scanning · rate limiting · CSRF/CORS configured deliberately · passwords hashed with bcrypt/argon2 · PII minimized and encrypted at rest where needed · audit logs for sensitive actions.

## Games and client-heavy apps
- Separate simulation/model from rendering/view; keep a fixed-timestep simulation loop decoupled from frame rate.
- Deterministic core logic where possible (seeded RNG) — makes testing, replays, and saves reliable.
- Save format versioned from day one with migration code.
- Cross-platform (desktop + mobile from one codebase): abstract input, resolution/DPI scaling, lifecycle (pause/resume), storage paths, performance tiers.
- Data-driven content (JSON/YAML/tables) so designers tune without code changes.

## Code review focus
Correctness → security → data integrity → clarity → tests → performance → style. Comment on the code, not the coder; suggest, explain why, and distinguish must-fix from nit.

## Red flags
Microservices for a team of three · business logic in controllers or UI components · no migrations · secrets in the repo · "we'll add tests later" · no staging environment · API returning DB rows verbatim.
