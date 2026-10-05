# Story plans: M2b

Per-story plans for milestone M2b, merged from the former `docs/plans/US-<id>.md` files. Each story has its own section.

- [US-110](#us-110)
- [US-111](#us-111)
- [US-112](#us-112)
- [US-113](#us-113)
- [US-114](#us-114)
- [US-115](#us-115)

---

<a id="us-110"></a>

## Plan US-110: Give every death and feud a reason

Codex v1.6, prompt S-US-110. Design: [M2b story design](M2b-story-design.md) sections 1, 2, 6. Traces to STO-03, NA-02; owner decision D-18.

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/chronicle.{h,cpp}` | Simulation | Every entry gets an id, an `EventKind`, up to three people and the ids of its causes; `record()`, `find()`, `explainEvent()` |
| `src/sim/story.{h,cpp}`, `assets/data/sim/story.json` | Simulation | `StoryConfig` (lean autumn, cause windows, grudge limit), validated at load ("season.leanAutumnPercent") |
| `src/sim/person.h`, `memory.h` | Simulation | `Grudge {about, event, weight}`; a memory knows its event |
| `src/sim/world.{h,cpp}` | Simulation | Thefts are events (seen or not); the empty store names thieves and the failed harvest; deaths by hunger name the store and the thief; feuds name their strongest grudge; a lean autumn (`Stream::Story` is added for later stories); hash covers all of it |
| `src/sim/save.{h,cpp}` | Simulation | Save version 3 with an upgrade from 2 (and 1 -> 2 -> 3) |
| `apps/headless/main.cpp` | App | chronicle lines show `[#id]`; `--why <id>` prints an event and its causes |
| `tests/sim/story_test.cpp`, `save_test.cpp`, `run_headless_soak.cmake` | Tests | See below |

### Tests
| Scenario | Test |
|---|---|
| Death with a cause | `US-110 Death with a cause` (a barren land, one theft: "Garu died of hunger in the dry summer, after Tiru stole from the store."); `US-110 A death after an empty store names the store even without a thief` |
| Feud with a reason | `US-110 Feud with a reason`: "A feud broke out between X and Y over stolen meat." and its causes are thefts |
| Traceable | `US-110 Traceable` (a death lists the empty store, which lists the thefts; every cause is earlier than its effect); ctest `US-110 Traceable` runs the real `odysseus_headless --why <id>` |
| Also | `US-110 A failed harvest ...`, `US-110 Story data is validated`, `US-016 Old version` (a real version-2 file upgrades) |

Manual checks: none (headless). Read a real one: `odysseus_headless --seed 7 --years 100 --chronicle`, then `--why <id>` on a death.

---

<a id="us-111"></a>

## Plan US-111: Quarrel, blame and take revenge

Codex v1.6, prompt S-US-111. Design: [M2b story design](M2b-story-design.md) sections 2, 3, 4. Traces to SDC-02, GD-03; owner decision D-18.

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/world_story.cpp` (new) | Simulation | `World::encounters` (every morning each person meets someone; often a person they hold a grudge against), `quarrel`, `blame`, `takeRevenge`, `considerRevenge`, `injure`, `updateHealth`, `exile` |
| `src/sim/world.{h,cpp}` | Simulation | Feuds are records (people, starting event, since when, last revenge); deaths by a fight or a wound name who struck the blow; grieving kin blame the thief they know of or the killer; daily order: health, encounters, pairing, feuds, revenge |
| `src/sim/person.h`, `memory.h`, `ai.{h,cpp}` | Simulation | `Health`, `exiled`; new death causes (fight, wounds, sickness) and memory kinds; the unwell rest and do not work |
| `src/sim/story.{h,cpp}`, `assets/data/sim/story.json` | Simulation | Sections `quarrel`, `blame`, `revenge`, `health` (numbers only, validated at load) |
| `src/sim/save.cpp`, `report.{h,cpp}` | Simulation | Save version 3 carries feud records, health and exile; exiles are counted apart from the dead |
| `tests/sim/story_quarrel_test.cpp`, `story_helpers.h` | Tests | See below |

### Rules in one place
- **Quarrel.** Both must think at most `dislikeOpinion` (-10) of each other. Short-tempered (Hunger or Energy below 60): 40% chance a day, otherwise 6%. Each thinks 12 less of the other, keeps a (minor) memory and a grudge; the sentence names the reason ("over stolen meat", "again") and the mood ("while hungry").
- **Blame.** When someone close (partner, parent, child, sibling) dies, the grieving blame the thief whose theft they know of (a memory of it), the one who struck the blow in a fight, or (US-114) the leader of the hunt. -40 opinion, a memory kept for life, a grudge; the blame's cause is the death.
- **Revenge.** A feud older than 10 days where one side thinks -60 or worse: 8% a day (then 60 days rest). The one who hates more attacks. If the clan thinks -25 or worse of them on average, they are driven out (exile); otherwise it is a fight: strength is hunting skill and courage plus luck; the loser is hurt for 5-12 days, dies with 20% chance, the winner is hurt with 30%. Wounds can kill (1.5% a day) and are told ("died of the wounds taken in the fight with X"). Kin blame the killer.

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Quarrel: tired or hungry people who dislike each other may quarrel; both remember it and think less | `US-111 Quarrel` (opinions fall by exactly `opinionLoss`, both hold a Quarrel memory of the event), `US-111 Quarrel happens when short-tempered people meet` (barren land, hunger: "quarrelled over stolen meat while hungry") |
| Blame: close kin blame the person linked to the cause and remember it for life | `US-111 Blame` (a partner who saw the theft blames the thief when the other starves; major memory), `US-111 Blame for a hunt someone led, once per death` |
| Revenge: a fight (injury or death) or exile, and the chronicle tells which and why | `US-111 Revenge ends in a fight` (injury, recovery), `... can end in death, and the kin blame the killer`, `... can end in exile`, `US-111 A feud that keeps worsening ends in revenge on its own` |

Manual check: read a real story: `odysseus_headless --seed 7 --years 30 --chronicle --threshold 30` shows quarrels, a feud, an attack, blame and an exile in order (see the evidence file).

### Balance (30 seeds, 100 years)
No crash, no extinction: 17 to 50 people alive at the end; 1 to 14 exiled and 0 to 3 killed in fights per century.

---

<a id="us-112"></a>

## Plan US-112: Share food and nurse the sick

Codex v1.6, prompt S-US-112. Design: [M2b story design](M2b-story-design.md) sections 3 and 4. Traces to SDC-02; owner decision D-18.

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/world_care.cpp` (new) | Simulation | `updateHealth` (the unwell die or mend; sickness strikes the weak; carers are assigned), `assignCarers`, `sicken`, `injure`, `hurtOnHunt`, `share`, `shareFood`, `adopt`, `adoptOrphans`, `isOrphan`, `kill` |
| `src/sim/world.{h,cpp}` | Simulation | hunting can wound; the evening meal calls `shareFood` when someone went hungry; illness deaths name what weakened them; adoptive kin count as family (`closeKin`); carer, patient and guardian in the hash |
| `src/sim/person.h` | Simulation | `carer`, `nursing`, `guardian` |
| `src/sim/story.{h,cpp}`, `assets/data/sim/story.json` | Simulation | Sections `sickness`, `nursing`, `sharing`, `adoption` |
| `src/sim/save.cpp` | Simulation | the new person fields (version 3, upgrade from 2) |
| `tests/sim/story_care_test.cpp` | Tests | See below |

### Rules in one place
- **Sickness.** Each morning a well person may fall sick: 0.1% a day, plus 2.5% when Hunger is at or below 25, plus 2.5% when Warmth is. It lasts 3-9 days; the sick cannot gather, hunt or steal. A sickness kills with 1.2% a day (wounds 1%). Hunters are hurt on a small-game hunt with 0.1% chance per hour. All of it is told, with its reason ("fell sick, weakened by hunger", "was hurt hunting", "died of the sickness, weakened by hunger").
- **Nursing.** The best carer (well, at least 12, not busy; score = opinion + 40 for kin or a partner + 30 for a Kind person, at least 30) comes with an 80% chance a day. The patient mends one extra day each day, dies with 40% of the usual chance, remembers the carer with gratitude for life (+20 opinion), and the carer thinks a little better of them (+5).
- **Sharing.** After an evening meal that left someone hungry: those at Hunger 35 or below are helped, hungriest first, by the best giver (Hunger at least 40 and at least 20 better fed; score = opinion + 30 kin + 20 Kind + 30 for a child), once a day each, 15 Hunger points moved. The receiver remembers (for life if they were at 10 or below) and thinks 15 better. The same pair within 30 days is not told again.
- **Adoption.** A child under 16 whose known parents are dead or driven out is taken in by the best adult (well, at most 2 wards; score = opinion + 50 for a sibling + 20 Kind, at least 20). Both think 30 better; the child remembers it for life; the guardian counts as family (grief, no pairing).

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Sickness: weakened by cold, hunger or a hunting wound, sick or injured for some days, may die | `US-112 Sickness` (4 days exactly, the sick do not work), `US-112 The cold weakens too`, `US-112 A hunting wound`, `US-112 Sickness can kill` |
| Nursing: recovery likelier, the patient remembers the carer with gratitude | `US-112 Nursing` (partner comes, gratitude memory, faster recovery, "nursed by"), `US-112 Care saves lives` (the same patient survives with a carer and dies without) |
| Sharing and adoption: a relative or friend shares or adopts, the one helped remembers | `US-112 Sharing food` (the transfer, the event, the memory, repeats are silent), `US-112 Hungry days bring sharing`, `US-112 Adoption` (the orphan is taken in; "orphaned by the death of X, was taken in by Y") |

Manual check: read the seed-7 story ([evidence](../evidence/US-112/headless-seed7-100years-threshold30.txt)): sickness, nursing, sharing and adoption lines in context.

### Balance (100 seeds, 100 years)
No crash or extinction: 9 to 53 alive at year 100 (median 38). Courtship (US-113) and the final tuning pass will lift the low end.

---

<a id="us-113"></a>

## Plan US-113: Court and compete for a partner

Codex v1.6, prompt S-US-113. Design: [M2b story design](M2b-story-design.md) sections 3 and 4. Traces to SDC-02; owner decision D-18.

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/world_love.cpp` (new) | Simulation | `courtship` (the daily round: partings, then suitors), `part`, `jealousOf` (rivals), and the helpers they share |
| `src/sim/world.{h,cpp}` | Simulation | `pairUp` is replaced by `courtship`; `pair` returns the Pairing event's id, links it to the courtship behind it and makes rivals jealous; the new person fields join the hash |
| `src/sim/person.h` | Simulation | `courting`, `courtDays`, `courtEvent`, `courtPauseDay` |
| `src/sim/chronicle.h` | Simulation | Importances for Courtship, Rejection, Jealousy and Parting (the event kinds were reserved in US-110) |
| `src/sim/story.{h,cpp}`, `assets/data/sim/story.json` | Simulation | Sections `courtship`, `rivals`, `parting` |
| `src/sim/save.cpp` | Simulation | the new person fields (version 3, upgrade from 2) and their validation |
| `tests/sim/story_love_test.cpp` | Tests | See below |

### Rules in one place
- **Courtship.** Each morning an unpaired adult courts the unpaired adult of the other sex (not close kin) they like best, if they like them at least 20. They keep courting the same person until that person is taken, the liking fades, or the courtship ends. Starting a courtship is an event ("Tok began courting Maa."). Every day of it the loved one's opinion of the suitor rises by 2 (gifts, time together) and the suitor's own by 1. A loved one who thinks worse than 0 of the suitor turns them down at once.
- **Both must agree.** Two people pair only when each thinks at least `life.pairOpinion` of the other, the suitor is courting the other, and (with a 10% chance a morning, `life.pairPercent`) the loved one chooses them: among several suitors the one they like best, ties to the smaller id. The Pairing event's cause is the courtship (both, if both courted), and its sentence names whoever began first ("after Tok's courtship").
- **Rejection.** A turned-down suitor (or one who has courted 40 days in vain) gives up: a Rejection event, 15 points less liking, a memory (Rejection) that fades like any minor one, and a pause of 30 days before courting again.
- **Rivals.** When a pair forms, every other suitor of either partner who has courted at least 2 days grows jealous of the winner (Jealousy event; cause: the pairing and their own courtship; reason "over their love for X"). They think 25 points worse of the winner, hold a grudge, remember being passed over for life (Rejection), and stop courting for 30 days. With a 50% chance they quarrel at once, and the quarrel names the reason.
- **Parting.** Each morning, partners of whom either thinks less than -20 of the other part. A Parting event says why: the heaviest grudge between them ("parted after a bitter quarrel", "parted over stolen meat"), or "as their love faded". Both are free again after a pause of 30 days; a woman who is expecting keeps the child's father as it was.

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Courtship: the other's opinion rises; they pair only when both agree | `US-113 Courtship` (suitor begins courting, opinion rises each day, the pair forms only once both agree, the Pairing cites the courtship), `US-113 Unrequited love` (a loved one who thinks ill of the suitor turns them down: Rejection event and memory, no pairing) |
| Rivals: jealousy, possible quarrel, the one not chosen remembers | `US-113 Rivals` (the better-liked suitor wins, the other's Jealousy event with causes, Rejection memory, lower opinion, a quarrel that names the reason; none when the quarrel chance is 0) |
| Parting: opinions below the threshold, the chronicle records why | `US-113 Parting` (a bitter quarrel), `US-113 The one who falls out of love leaves` (one-sided, stolen meat), `US-113 Steady partners stay` |
| Rules of the world | `US-113 Courtship keeps the clan's rules` (20 years: nobody courts themselves, the dead, the paired or kin; partners agree on each other), `US-113 Saved courtships` (save and load in the middle of courting keeps the hash) |

Manual check: read the seed-7 story ([evidence](../evidence/US-113/headless-seed7-100years-threshold30.txt)): courtships, rejections, jealousy and partings in context; check the population is still healthy over 100 seeds.

### Balance
The 100-year soak on seeds 1-100 must show no crash and no extinction; if pairings became too rare the numbers in `story.json` are tuned (never a test weakened).

### Manual check results (2026-09-30)
Seed 7, 100 years, threshold 30 ([evidence](../evidence/US-113/headless-seed7-100years-threshold30.txt)): courtship lines lead to pairings, jealousy lines name the rival and the loved one; no partings occurred (partners rarely fall out). Seeds 1-10 (owner limit): 27 to 45 alive at year 100, no crash or extinction.

---

<a id="us-114"></a>

## Plan US-114: Teach the young and hunt together

Codex v1.6, prompt S-US-114. Design: [M2b story design](M2b-story-design.md) sections 3 and 4. Traces to SDC-02; owner decision D-18.

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/world_hunt.cpp` (new) | Simulation | `teaching` (apprentices, lessons, graduation), `huntMammoth` (party, roles, outcome, danger, rescue), `releaseTeaching` |
| `src/sim/world.{h,cpp}` | Simulation | a mammoth sighting calls `huntMammoth`; `bringDownMammoth` returns its event and can name the party; teaching runs each morning; deaths and exiles end apprenticeships; new fields in the hash |
| `src/sim/person.h` | Simulation | `master`, `apprentice`, `teachHunt`, `teachEvent` |
| `src/sim/memory.{h,cpp}` | Simulation | memory kinds `Heroism` and `Cowardice` (added at the end: saved numbers never change) |
| `src/sim/chronicle.{h,cpp}` | Simulation | importances; `joinNames` shared by the chronicle texts |
| `src/sim/story.{h,cpp}`, `assets/data/sim/story.json` | Simulation | Sections `teaching` and `hunt` |
| `src/sim/save.cpp` | Simulation | the new person fields (version 3, upgrade from 2) and their validation |
| `tests/sim/story_hunt_test.cpp` | Tests | See below |

### Rules in one place
- **Apprenticeship.** Each morning a well adult with a skill of at least 30 may take a youth of 12 to 15 who lacks a master, when the master's skill is at least 15 better and each thinks no worse than -10 of the other (20% a day; the best score of opinions plus 40 for kin). The skill taught is the master's better one of the two (hunting only from 15). Every day the apprentice gains 1 skill point (never above the master) and both think 2 better of each other. They graduate at 16, or when within 5 points of the master. Events: Apprentice, Graduation.
- **Hunting party.** When a hunter meets the year's mammoth, other well hunters (15 or older) may join (70%, +20 Brave, -30 Timid); with 3 to 5 in all a party hunts, otherwise the sighter hunts alone as before. Roles: the leader is the best hunter; each member's courage is skill + 15 Brave - 10 Timid + luck; the hero is the most courageous (at least 55), the coward the least (at most 30). The party's strength (average skill, leader, hero, coward) sets the chance of success; only a success brings the feast.
- **Danger and rescue.** Each member, except the coward who fled, is in danger with 25%. The hero (or the most courageous other) tries to save them (70%, +15 Brave): a Rescue event, the rescued remembers it for life and thinks 40 better of the rescuer, who may be hurt (30%). An unsaved member dies (60%) or is hurt.
- **Memories.** Every member remembers what the others did: the hero (Heroism, +), the coward (Cowardice, -), the rescuer (Rescue, for the rescued).

### Tests (`odysseus_sim_tests`)
| Scenario | Test |
|---|---|
| Apprenticeship: skill grows faster, they grow close | `US-114 Apprenticeship` (a master takes a youth, skill and opinion rise faster than without, graduation), `US-114 Saved apprenticeships` |
| Hunting party: 3 to 5, roles, outcome depends on the party, memories | `US-114 Hunting party` (size, leader, hero, coward, memories), `US-114 The party decides the hunt` (strength 100% brings the feast, 0% does not) |
| Rescue: the rescued owes a debt, the chronicle tells | `US-114 Rescue`, `US-114 Nobody to save them` |

Manual check: read the seed-7 story ([evidence](../evidence/US-114/headless-seed7-100years-threshold30.txt)): apprentices, hunting parties, heroes, cowards and rescues in context; population over seeds 1-10 stays healthy.

### Manual check results (2026-09-30)
Seed 7, 100 years, threshold 30 ([evidence](../evidence/US-114/headless-seed7-100years-threshold30.txt)): 205 apprenticeships, 192 graduations, 73 hunting parties (72 brought down a mammoth), 73 heroes, 2 cowards, 61 rescues. Seeds 1-10 (owner limit): 29 to 51 alive at year 100.

---

<a id="us-115"></a>

## Plan US-115: Tell the clan's story in episodes

Codex v1.6, prompt S-US-115. Design: [M2b story design](M2b-story-design.md) section 5. Traces to STO-02, STO-03, NA-02; owner decision D-18.

### Files
| File | Layer | What |
|---|---|---|
| `src/sim/episodes.{h,cpp}` (new) | Simulation | `findEpisodes` (link events, pick the best), `formatEpisode` (one paragraph), `formatStory` (episodes, then the lines with reasons) |
| `src/sim/story.{h,cpp}`, `assets/data/sim/story.json` | Simulation | Section `episodes` (minEvents 3, minImportance 60, minEventImportance 25, maxPerCentury 40, maxPeople 4) |
| `apps/headless/main.cpp` | Headless runner | `--story` |
| `tests/sim/story_episode_test.cpp`, `tests/sim/run_headless_soak.cmake` | Tests | See below |

No new state: episodes are a pure function of the chronicle, so the world hash and the save format do not change (old and new saves tell their stories alike).

### Rules in one place
- **Linking.** Events of importance 25 or more are the nodes. An event and each of its causes belong together. The hardship events of one season (Lean, StoreEmpty, Sharing, Nursing, Sickness, and hunger or cold deaths) are joined too, when that season has an empty store or a hunger death, so "the hard winter" is one episode.
- **Which groups.** At least 3 events, one of them important (60 or more). The score is the sum of importances, plus 20 per death and 10 per person. Only the best are told: at most 40 for every hundred years of the run; the rest stay in the full chronicle.
- **Shape.** The beginning is the first event (the cause), the turn the most important event in between, the end the last event.
- **Name.** The theme that weighs most decides: hardship ("The Hard Winter of year 74", "The Lean Autumn...", "The Hungry Spring...", "The Dry Summer..."), feud ("The Vengeance of Ura", "The Feud of Tok and Brak", "The Grief of Ura", "The Quarrel of ..."), hunting ("The Great Hunt of year 12"), sickness ("The Sickness of year 40"), love ("The Parting of ...", "The Rivals for ...", "The Courtship of ..."), teaching ("The Apprenticeship of Ban"); otherwise "The Story of" the person most involved.
- **Paragraph.** "The Hard Winter of year 2. It began in Summer, year 2: <the cause>. The turn came in Winter, year 2: <the turn> It ended in Spring, year 3: <the end> Those who lived it: A, B, C and D."
- **`--story`** prints the episodes, then the births, deaths, pairings, partings and feuds (importance at or above `--threshold`, default 50) with their reasons. `--chronicle` still prints every event.

### Tests
| Scenario | Test |
|---|---|
| Episode: linked events become one named paragraph (who, why, what came of it) | `US-115 Episode` (a lean autumn and an empty winter store: "The Hard Winter of year N", beginning, turn, end, people) |
| Fewer, bigger: at most 40 a century, each with people and causes, then the lines with reasons | `US-115 Fewer, bigger` (60 years, at most 24, sorted, deterministic), `US-115 The story, then the lines with reasons` |
| Both views: `--story` and `--chronicle` | ctest `US-115 Both views` (100 years: 1 to 40 episodes, lines with reasons after them, `--story` alone prints no raw events, `--chronicle` unchanged) |

Manual check: read the printed story of seed 7 ([evidence](../evidence/US-115/headless-seed7-100years-story.txt)): is it a story? (The owner judges at X-M2b.)

### Manual check results (2026-09-30)
Seed 7, 100 years: 40 episodes (11 Great Hunts, 10 Hard Winters, 9 Sicknesses, 6 Vengeances, 3 Lean Autumns, 1 Hungry Spring, and love and apprenticeship stories), followed by 300 lines of births, deaths, pairings and feuds with their reasons ([evidence](../evidence/US-115/headless-seed7-100years-story.txt)). Seeds 1-10 (owner limit): 29 to 51 alive at year 100; every seed tells the maximum 40 episodes.
