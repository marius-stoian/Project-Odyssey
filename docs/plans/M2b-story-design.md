# Story engine design (M2b: Kill Gate 1 retry)

Architect's and Game Designer's design for M2b, shared by US-110..US-115. Codex v1.6; requirements v1.6 (STO-02, STO-03, SDC-02, GD-03, NA-02); owner decision D-18 (the story pivot after Kill Gate 1). Builds on [M2-clan-design.md](M2-clan-design.md); nothing there is thrown away.

## Why the M2 chronicle was "not really a story"
It listed *what* happened (births, deaths, pairings, a feud) but never *why*, and every line stood alone. A story needs causes, people who matter to each other, and a shape (a beginning, a turn, an end). M2b adds exactly those three things and nothing else.

## Principles (unchanged Charter rules)
Simulation layer only (`src/sim/`, `apps/headless/`), integers only, seeded PCG32 streams (one new stream, `Story`), no wall clock, no unordered iteration, numbers as data in `assets/data/sim/story.json`, errors naming file and field, saves versioned (save version 3, upgrading versions 2 and 1).

## 1. The event log: every notable event has an id, people and causes (US-110)
The `Chronicle` already holds the clan's sentences. Each entry now also carries:
- `id`: its position in the log, never reused (the chronicle only grows);
- `kind` (`EventKind`: Birth, Death, Pairing, Parting, Feud, Peace, Theft, StoreEmpty, Lean, Mammoth, Gift, Quarrel, Blame, Revenge, Exile, Sickness, Injury, Recovery, Nursing, Sharing, Adoption, Courtship, Jealousy, Rejection, Apprentice, Graduation, HuntParty, Rescue, Hero, Coward, plus Note for free text);
- `who`, `other`, `aux`: up to three people (a PersonId or -1). Meaning depends on the kind and is documented next to the enum (for Blame: who = the one who blames, other = the one blamed, aux = the dead person);
- `causes`: the ids of the earlier events that led to this one.

The sentence is written when the event happens, with its reason inside it. `Chronicle::add(date, importance, text)` stays (kind Note) for tests and free text; `Chronicle::record(...)` is the full form and returns the id. `--chronicle` prints every event above a threshold with its id; `--why <id>` prints an event and its causes recursively ("Traceable").

## 2. How causes are chosen
- **Theft**: every theft is an event (Theft, importance 35), witnessed or not; a witness remembers it (Memory keeps the event id).
- **Store runs empty**: causes = the thefts of the last `causeWindowDays` (up to 4) and the lean season if there was one. The sentence names the thieves when there are any.
- **Death by hunger**: causes = the store-empty event and the thefts behind it. "Winter, year 74: Hano died of hunger in the hard winter, after Brak stole from the store." Without a theft: "... when the store ran empty." Death by cold, illness, wounds, fights and childbirth name their cause event the same way (illness: the sickness event; childbirth: the birth).
- **Grudges**: whenever an event makes person A think worse of person B (a theft A saw, a quarrel, blame, jealousy, a fight, a story heard through gossip), A gets a `Grudge {about, event, weight}` (at most 16 per person; the weakest and oldest go first). A feud starts when both think badly of each other (opinion <= feudOpinion, as before); its causes are the strongest grudges between the two, and its sentence names the strongest one: "A feud broke out between Tok and Brak over stolen meat." The same reason phrases (stolen food, a bitter quarrel, the death of X, a rival in love, a fight) are used everywhere, in one function.
- **Blame** (US-111): grieving kin blame the person linked to the cause (the thief whose theft emptied the store, the leader of the hunt); the event's cause is the death; it becomes a grudge and a lifelong memory.

## 3. Health (US-112)
`Person::health` = Well, Sick or Injured, with days left, the onset event and the carer. Weakness (cold, hunger) makes sickness likelier each morning; hunts and fights cause wounds. The sick cannot work; each day they may recover or die (illness / wound), more likely without a carer. Nursing by a kind or close person raises recovery and makes the patient remember the carer (Memory kind Nursing, opinion up).

## 4. The new interactions (D-18) and their data
All in `story.json`, one section each, read into `StoryConfig`:

| Section | Interaction | Story |
|---|---|---|
| `season` | lean autumn (harvest fails now and then): the root of many stories | US-110 |
| `quarrel` | irritable, mutually disliking people quarrel when they meet (encounters happen every morning, one per person) | US-111 |
| `blame`, `revenge` | grief looks for someone to blame; worsening feuds end in a fight (injury or death) or exile | US-111 |
| `health`, `nursing`, `sharing`, `adoption` | sickness, care, sharing food in famine, orphans taken in | US-112 |
| `courtship`, `rivals`, `parting` | courting instead of instant pairing; jealous rivals; partners who drift apart | US-113 |
| `teaching`, `hunt` | apprentices; hunting parties of 3-5 with a leader, a hero, a coward and rescues | US-114 |
| `episodes` | how episodes are found, how many are told | US-115 |

Every interaction is decided with the `Story` random stream (hunts use the existing Hunting stream) and changes opinions, memories (each has an event id) and grudges. All new state is in the world hash and the save.

## 5. Episodes: how the story gets its shape (US-115)
Episodes are *derived*, never stored: a pure function of the chronicle, so old and new saves tell stories alike.
1. Build a graph: entries are nodes; an edge joins an entry to each of its causes. Hardship events of one season (Lean, StoreEmpty, hunger/cold deaths, Sharing, Nursing, Sickness) are joined too, so "the hard winter" is one episode.
2. Each connected group with at least `minEvents` events, one of them important (>= `minImportance`), is an episode. Groups are scored (sum of importance, extra for deaths and for people who appear often) and only the best `maxPerCentury` per 100 years are told ("at most about 40 episodes a century"); the rest still appear in the full chronicle.
3. Shape: the beginning is the earliest event (the cause), the turn the most important event after it, the end the last event. The name comes from the dominant kinds: "The Hard Winter of year 74", "The Feud of Tok and Brak", "The Vengeance of Ura", "The Great Hunt of year 12", "The Courtship of Maa", "The Sickness of year 40", "The Apprenticeship of Ban"; the fallback is "The Story of <most involved person>".
4. The paragraph is short: name, who (the people involved), why (the beginning) and what came of it (the turn and the end), each in the event's own sentence.

`odysseus_headless --story` prints the episodes first, then the lines with reasons (births, deaths, pairings, feuds at or above the threshold); `--chronicle` still prints every event.

## 6. Saves and the world hash
Save version 3 adds: chronicle entry fields (kind, who, other, aux, causes); per person: grudges, health, healthDays, healthEvent, carer, courting, courtDays, master, apprentice, guardian, exiled, hunt roles counters; per memory: event id; world: lean year flag, the `Story` stream. `upgradeFrom2` gives old saves defaults (Note entries, no grudges, everyone well); `upgradeFrom1` chains into it. The hash covers all of it.

## 7. Keeping the chronicle readable
Importance decides what is printed: births 85, deaths 90, feuds 75, pairings 70, store empty 70, revenge/exile 80, blame 60, rescue/hero/coward 55-65, quarrels 30, courtship 35, sharing 30, nursing 30, thefts 35, gifts 10 (all code constants next to the others, as before). The episode limit keeps the story short even when the log holds tens of thousands of small events.

## 8. Tuning and safety
The clan must survive its own drama. After each story: the 100-year soak on seeds 1-20 (no crash; no extinction; population stays healthy); the determinism test; a save/load round trip in the middle of a run. Numbers that break this are fixed in `story.json`, never by weakening a test.
