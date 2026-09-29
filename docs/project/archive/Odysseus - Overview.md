# Odysseus — Project Overview

> **Status:** Draft v0.1 · **Last updated:** 2026-09-29
> A living foundation document. Anything marked **(TBD)** is still open for decision.

---

## 1. Vision

**Odysseus** is a pixel-art life-simulation game set in a persistent 2D world. You control one character who lives, works, builds, makes friends and changes the world. The world keeps running around you: other inhabitants have needs, jobs, moods and memories of their own.

It combines three influences:

| Inspiration | What we take from it |
|---|---|
| **Animal Crossing** | A cozy, low-pressure daily rhythm: seasons, real-time days, collecting, decorating, and villagers you get attached to. |
| **The Sims** | A needs-driven character, relationships, careers, building homes, and objects that "advertise" what you can do with them. |
| **Dwarf Fortress** | A deep simulation underneath: materials, crafting chains, NPCs acting on their own, emergent stories, and a world with history. |

**In one sentence:** *A cozy life sim on the surface, with a living, systemic world underneath.*

### Design pillars

1. **Everything is interactable.** Any object can be used in several ways, depending on who is using it, what they're holding, and what the object is made of.
2. **The world lives without you.** NPCs follow their own needs and schedules. Things happen while you're somewhere else.
3. **Cozy first, deep on demand.** A new player can just fish and decorate. Someone who wants depth can dig into crafting chains, economies and relationships.
4. **Stories come from the systems.** Memorable moments are produced by systems interacting, not by scripts.
5. **One codebase, every screen.** Desktop and mobile are equally important, and both run from the same Java code.

---

## 2. Core Gameplay Loop

### Moment-to-moment (seconds)
Walk → look at an object → pick an interaction → watch the result (animation, item gained, need satisfied, reaction from an NPC).

### Daily loop (one in-game day)
```
Wake up ─► Take care of needs (eat, wash, rest)
        ─► Choose what to do today:
             • Gather   (forage, fish, mine, chop, farm)
             • Craft    (turn raw materials into goods)
             • Socialize (talk, gift, help, argue, befriend)
             • Work      (a job or trade for income)
             • Build/Decorate (home, garden, town)
        ─► Evening events / visit someone
        ─► Sleep (autosave, world simulation catch-up)
```

### Long-term loop (weeks and seasons)
- Improve your home, and later help grow the town.
- Build deep relationships: friendship, rivalry, romance, family.
- Master skills that unlock new interactions and recipes.
- Complete collections (fish, bugs, fossils, art, recipes).
- Shape the world: settlers arrive, shops open, landmarks go up, the town's history builds.

### Emergent layer (the Dwarf Fortress part)
- NPCs pursue their own goals and form opinions of each other and of you.
- Resources are finite in any one place, and ecosystems regrow.
- Events can spread through the world: a bad harvest raises prices, a fire damages buildings, a festival boosts everyone's mood.
- A **world chronicle** records notable events, so the town has a readable history.

---

## 3. Key Systems

### 3.1 Character & Needs
- **Needs** (they decay over time): Hunger, Energy, Hygiene, Social, Fun, Comfort.
- **Mood** comes from needs, recent memories and surroundings (e.g. a beautiful room or a messy one).
- **Traits** (e.g. *Green Thumb*, *Shy*, *Night Owl*) change how fast needs decay, which interactions are available, and how NPCs react.
- **Skills** go up with use: Cooking, Fishing, Carpentry, Gardening, Charisma, Mining, etc.

### 3.2 Interaction System (the core of the game)
Objects **advertise** actions (the Sims "smart object" model):

```
Object: Oak Tree
  Advertised actions:
    • Shake       (any)            → chance of fruit, bugs, or a wasp nest
    • Chop        (needs Axe)      → Oak Logs; the tree becomes a stump
    • Climb       (Fitness ≥ 2)    → view, hidden items
    • Sit under   (any)            → +Comfort, +Fun if reading
    • Carve name  (needs Knife)    → +Romance with a partner, marks the tree
```

- Actions are **data-driven** (defined in JSON/YAML, not hard-coded), so content can grow without changing engine code.
- Each action has **preconditions** (tool, skill, trait, time of day, relationship level), a **duration**, **effects** (on needs, items, world state or relationships) and an **animation**.
- NPCs use **the same interaction system** as the player. The only difference is whether a person or the AI picks the action.
- **Item-on-object** combinations (use item X on object Y) open up more options, e.g. water can + plant, bait + fishing spot, gift + NPC.

### 3.3 Materials & Crafting
- Items have **materials** (wood, stone, iron, cloth…) with properties such as flammable, heavy or valuable.
- **Crafting chains:** Tree → Logs → Planks → Chair. Ore → Ingot → Tool.
- **Workstations** (workbench, stove, loom, forge) unlock recipe groups.
- Quality tiers depend on skill level and material quality.

### 3.4 NPCs & Social Simulation
- Each NPC has needs, traits, skills, a daily schedule, a home and a job.
- **Utility-based AI:** every tick, each NPC scores the available actions against its current needs and goals and picks one.
- **Relationships** are tracked two ways: *Friendship* and *Romance*, plus tags such as Rival, Family or Coworker.
- **Memories:** NPCs remember important events ("You gave me my favorite flower", "You chopped down the tree I loved"), and those memories affect how they treat you.
- **Conversations** are built from topics plus the NPC's personality, not long scripted dialogue trees. A few hand-written story moments are layered on top.

### 3.5 World & Time
- A **tile-based 2D world** (top-down, 16×16 or 32×32 pixel tiles, **TBD**), split into chunks for streaming.
- **Time:** in-game days with a configurable length. Optionally synced to the real clock (Animal Crossing style) or free-running (Sims style), chosen per save.
- **Seasons & weather** affect crops, fish and bug spawns, NPC behavior and visuals.
- **Regions:** a home town to start, with wilderness, caves and neighboring settlements added over time.
- **Off-screen simulation:** areas far from the player run a cheaper, lower-detail simulation (level of detail for the simulation).

### 3.6 Economy
- A local currency, plus bartering with NPCs.
- Prices move with supply and demand in each settlement.
- Shops are run by NPCs with real inventories, not items that appear from nowhere.

### 3.7 Building & Decoration
- Place, rotate and move furniture on a grid.
- Build homes room by room (walls, floors, doors).
- A **room scoring** system (beauty, comfort, function) affects the mood of whoever lives there.

---

## 4. Platforms & Technology

### 4.1 Target platforms
| Platform | Priority | Notes |
|---|---|---|
| Windows / macOS / Linux | Primary (MVP) | Keyboard + mouse, and controller support |
| Android | Primary (MVP+1) | Touch controls, battery-aware |
| iOS | Secondary | Via RoboVM/MobiVM (see risks) |
| Web | Optional / later | Via TeaVM backend, if needed |

### 4.2 Recommended stack
| Concern | Choice | Why |
|---|---|---|
| Language | **Java 17** (LTS) | Required. Android and RoboVM compatibility limits which newer features we can use. |
| Framework | **libGDX** | A mature, cross-platform Java game framework: one codebase for desktop, Android, iOS and web. Large community. |
| Build | **Gradle** (via `gdx-liftoff`) | The standard libGDX multi-module setup. |
| Desktop backend | LWJGL3 | Default libGDX desktop backend. |
| Android backend | libGDX Android | Native support. |
| iOS backend | RoboVM (MobiVM fork) | The only realistic way to ship Java on iOS today. |
| ECS | **Artemis-odb** (or Ashley) | Fast entity-component-system for thousands of simulated entities. |
| Maps | **Tiled** (`.tmx`) + libGDX loaders | Industry-standard 2D level editor. |
| UI | **Scene2D.ui** + custom pixel skin | Built into libGDX, and scales well across screen sizes. |
| Data / content | JSON (libGDX `Json`) or Jackson | Data-driven items, actions, recipes, NPC definitions. |
| Saves | Versioned JSON (compressed) at first, Kryo later for speed | Human-readable while developing. |
| Pathfinding | **gdx-ai** (A*, steering, behavior trees) | Official libGDX AI extension. |
| Testing | JUnit 5 | The simulation core is tested without any rendering. |

**Alternative considered:** FXGL / JavaFX + GluonFX. Rejected for now because its mobile support is less proven and its 2D game tooling is less mature than libGDX.

### 4.3 Architecture principles
1. **Separate the simulation from the presentation.** The `core-sim` module contains pure Java with **no libGDX imports**. It runs headless, can be unit-tested, and could later run a server.
2. **Fixed-timestep simulation** (e.g. 20 ticks/sec), with rendering interpolated at whatever frame rate the device manages.
3. **Data-driven content.** Items, objects, actions, recipes and NPC archetypes live in `assets/data/`. Designers add content without writing Java.
4. **ECS for world entities**, with plain services for global systems (time, weather, economy, chronicle).
5. **Input abstraction.** Game "intents" (Move, Interact, OpenMenu) are mapped separately from each device's input (keyboard, gamepad, touch).
6. **Deterministic where practical.** Seeded RNG keeps bugs reproducible and leaves room for replays or multiplayer later.

### 4.4 Proposed module layout
```
odysseus/
├── core-sim/        # Pure Java simulation: ECS, needs, AI, interactions, economy, time
├── core-game/       # libGDX game layer: rendering, audio, UI, input, screens
├── desktop/         # LWJGL3 launcher
├── android/         # Android launcher + manifest
├── ios/             # RoboVM launcher (later)
├── tools/           # Content validators, asset packers, debug tools
├── assets/
│   ├── data/        # JSON: items, objects, actions, recipes, NPCs, dialogue topics
│   ├── maps/        # Tiled .tmx maps
│   ├── sprites/     # Texture atlases (packed)
│   ├── audio/
│   └── ui/          # Scene2D skins, fonts
└── docs/            # This file, design docs, ADRs
```

### 4.5 Cross-platform considerations
- **Resolution:** render the pixel art to a fixed low-resolution virtual screen (e.g. 480×270), then scale it up by whole numbers to keep pixels crisp. On mobile, letterbox or extend the view as needed.
- **Touch UX:** tap to move or interact, long-press to open the action menu, a virtual joystick as an option, and large touch targets.
- **Performance budget:** hold 60 FPS on mid-range Android phones. Simulation LOD keeps CPU use under control.
- **Lifecycle:** mobile can pause the app at any time, so autosave on pause and handle resumes gracefully.
- **Storage:** use libGDX `Gdx.files.local()` for saves. Cloud sync comes later.

---

## 5. Art & Audio Direction
- **Pixel art**, with a limited, cohesive palette (**TBD**: 16×16 vs 32×32 base tile size).
- Top-down, 3/4 perspective (like Stardew Valley or classic Pokémon).
- Characters are built from layers (body, hair, clothes, tool) so there can be many customizations and NPC variations.
- Expressive, readable animations: every interaction needs a clear visual signal.
- Audio: gentle, adaptive music that changes with time of day and season. Sound effects for every interaction.

---

## 6. Scope & Roadmap

### Phase 0 — Foundation (current)
- [ ] Finalize this overview
- [ ] Generate the libGDX project with `gdx-liftoff` (desktop + android)
- [ ] Set up the repo, CI (build + tests), code style
- [ ] Settle key decisions (tile size, time model, ECS library)

### Phase 1 — Vertical Slice / Prototype
- A character walks around a single tilemap
- 5–10 interactable objects with several actions each
- Basic needs (Hunger, Energy) with UI
- Day/night cycle
- Inventory + 3 simple recipes
- Save and load
- Runs on desktop **and** Android

### Phase 2 — Living Town (MVP)
- 5–8 NPCs with schedules, needs and utility AI
- Relationships + gifting + basic conversations
- Farming, fishing, foraging
- Home decoration
- Seasons + weather
- Shops + currency

### Phase 3 — Depth
- Material system + deeper crafting chains
- NPC memories + world chronicle
- Off-screen simulation LOD
- Building construction, town growth, new settlers
- Careers/jobs

### Phase 4 — Polish & Release
- iOS port
- Controller + accessibility options
- Localization
- Balancing, onboarding, performance tuning
- Store releases (Steam / itch.io, Google Play, App Store)

### Explicitly out of scope (for now)
- Multiplayer (but we keep the architecture open to it)
- Combat as a core mechanic
- Procedurally generated whole worlds (start with handcrafted maps that have procedural details)

---

## 7. Risks & Open Questions

| Risk / Question | Notes / Mitigation |
|---|---|
| **iOS support for Java** | RoboVM (MobiVM) is community-maintained. Ship desktop + Android first and prototype an iOS build early to confirm it works. |
| **Simulation performance on mobile** | Simulation LOD, a fixed tick rate, profiling from Phase 1 onward, and avoiding GC churn (object pools). |
| **Scope creep** (three genres combined!) | Keep the pillars strict. Each phase needs a playable, fun build before moving on. |
| **UI on small screens** | Design mobile UI alongside desktop UI from day one, not as an afterthought. |
| **Real-time clock vs. free time** | **TBD**: a per-save choice or one fixed model? |
| **Tile size / resolution** | **TBD**: 16×16 is faster to produce art for, 32×32 has more detail. |
| **Art pipeline** | **TBD**: in-house pixel artist, asset packs, or a mix? Tool: Aseprite recommended. |
| **Monetization** | **TBD**: premium (one purchase) is recommended. Avoid predatory mobile models. |

---

## 8. Glossary
- **Advertisement:** an action that an object offers to characters that could use it.
- **Utility AI:** decision-making where each option gets a score and the highest-scoring one is picked.
- **Simulation LOD:** simulating distant or unseen areas in less detail to save CPU.
- **Chronicle:** a log of significant world events that the player can read.
- **ECS:** Entity-Component-System, an architecture that keeps entities (IDs), components (data) and systems (logic) separate.

---

*Next step: review this draft, settle the **TBD** items, then scaffold the Gradle project for Phase 0.*
