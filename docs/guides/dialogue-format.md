# Dialogue format guide

How to write a conversation as plain text (US-160, SDC-04, ADR-019). One conversation per file, `assets/data/dialogue/<name>.dlg`, written in any text editor. The game reads the folder when it starts and again when you press F5, and a mistake is reported as `dialogue/<name>.dlg:<line>: message`. A file with a mistake is left out and the rest load. The Editor's graph (M9) reads and writes the very same files, and keeps your `#` notes.

Conditions (`[if ...]`) and effects (`{...}`) are the language of the interaction files: see `docs/guides/interaction-data.md` for every function and effect verb.

## A whole file

```text
# The elder at the clan fire (US-160). Anything after a # at the start of a line is a note for you; the game and the Editor keep it.
@who elder
@when opinion(npc, hero) >= -20
@priority 10

=== start
Elder: The fire is low tonight, {hero}.   [if time == night]
Elder: You walk like a hunter today.
-> Ask about the hunt => hunt
# Giving a berry is kind: it costs one and raises the elder's opinion a little.
-> Offer berries [if has(hero, berries, 1)] [else You have no berries] {take hero berries 1; opinion npc hero 5; remember npc "{hero} shared berries" 20} => thanks
-> Leave => END

=== hunt
Elder: {smalltalk.hunt}
-> Back => start

=== thanks
Elder: The clan remembers kindness.
-> Back => start
```

## The parts

| Part | What it is |
|---|---|
| `# a note` | A note for you. It must be a whole line starting with `#`. It stays attached to the line, choice or node that follows it. |
| `@who elder` | Who this conversation is for: a placed character's id or name, a role (`elder`, `hunter`, `child`) or a kind. More than one word is allowed. |
| `@when expression` | The conversation may be used only while this is true (`@when opinion(npc, hero) >= -20`). |
| `@priority 10` | When several conversations fit, the highest priority wins. Default 0. |
| `@bark hello` | Marks a short greeting of one line (a bark) of that kind, spoken as a speech bubble. |
| `@pair elder child` | Marks a conversation between two clan members, shown as speech bubbles. |
| `=== start` | A node: one stop in the conversation. The conversation begins at `start`, or at the first node when there is none. |
| `Elder: words` | Something the character says. `Elder` is one word; add `[if condition]` to say it only sometimes. Lines come before the choices of their node. |
| `-> words => node` | A choice for the player. It leads to the node named after `=>`, or ends the conversation with `END`. |
| `[if condition]` | On a choice: shown only while it is true. |
| `[else reason]` | On a choice with an `[if]`: while the condition is false the choice is shown greyed out with this reason, instead of hidden. |
| `{effect; effect}` | On a choice: what happens when it is chosen (`take hero berries 1; opinion npc hero 5; remember npc "..."`), in order. |

Headers (`@...`) come before the first node. The order of `[if]`, `[else]` and `{}` on a choice does not matter when you write it; the Editor writes them in this order, and so does the example above.

## Words in what people say

A `{token}` in a line or a choice is filled in when it is shown: `{hero}` and `{hero.name}` (the hero's name), `{npc}` and `{npc.name}` (the one speaking), `{target.name}`, `{season}`, `{time}`, and `{smalltalk.topic}`: a line made up from what the character remembers, heard or feels about that topic (`{smalltalk.hunt}`).

## Limits and checks

- At most **5 choices** in a node (the panel shows 5, keys 1 to 5). A node with more is a mistake.
- A choice must lead to a node that exists: `dialogue/elder-fire.dlg:9: unknown node "hunts"`.
- A node nobody can reach loads, with a warning.
- Every shipped `.dlg` file is read and written back by the tests, and must come back as the same text, `#` notes included.

## Talking: who speaks, and the panel (US-161)

When the player chooses **Talk** on a clan member, the game looks for the script that speaks for them. A script fits when one of its `@who` words is the person's **name** (the most specific), a **role** they have (`elder` is the oldest living member of the clan, `hunter` and `gatherer` are the grown members with the best hunting and gathering skill, `child` is anyone under 12), or their **kind** (`person`), and its `@when` holds. The most specific match wins (a script for `Ama` beats one for all `elder`s), then the higher `@priority`; scripts still tied are chosen between by the game's seeded stream, so the same game always picks the same one. A script marked `@bark` or `@pair` never answers Talk. Use `@when mood(npc) == wary` or `@when opinion(npc, hero) < -30` to write what someone says to a hero they dislike. When no script fits, Talk is the plain "You talk with ..." of before.

The game **pauses** while the conversation panel is open. It shows:

- the person's name and one **mood word**: how they feel about the hero (`warm`, `friendly`, `neutral`, `wary`, `hostile`), unless a need is pressing and they are not hostile, then `hungry`, `tired`, `cold` or `lonely`;
- what they say: every line of the node whose `[if]` holds, in order, as `Speaker: words`;
- the choices, numbered 1 to 5. Pick one with the mouse or the keys **1** to **5**. A choice with a false `[if]` is hidden, or greyed out with its `[else]` reason and cannot be picked. **Esc** walks away: the talk ends and no effect happens.

Choosing runs the choice's `{effects}` in order, then moves to its node, or ends the talk at `END`. The panel carries out `take hero <item> n`, `give hero <item> n`, `opinion npc hero n` (what the first thinks of the second changes by `n`, kept between -100 and 100), `say`, and the three that make a conversation matter later, described in the next section. `opinion(npc, hero)` in a condition reads the real opinion.

## Greetings (US-162)

A script with `@bark greet` is a **greeting**: one short line said in a bubble over someone's head, not a conversation. When the hero passes within **3 m** of a clan member who thinks well of them (opinion 0 or more), the game chooses a greeting for that person the same way it chooses a conversation (name, then role, then kind, then `@priority`, then the seeded stream), and shows the first line of its first node whose `[if]` holds for 3 seconds. Each person greets at most **once a minute**, even if the hero stays beside them. A greeting needs no choices and no `-> ...` line:

```text
@who elder
@when opinion(npc, hero) >= 0
@priority 10
@bark greet

=== start
Elder: The fire keeps you well, {hero}.
```

Shipped examples: `greet-elder.dlg`, and two plain ones for anyone, `greet-friend.dlg` and `greet-friend-warm.dlg` (equally fitting, so the seeded stream chooses).

## Being remembered: `remember`, `flag` and `chronicle` (US-164)

What is said can matter later. Three effects write into the world, in a choice's `{...}`:

| Effect | What it does | Example |
|---|---|---|
| `remember npc "text" [feeling]` | The person remembers that the hero did something, with a feeling from -100 (hateful) to 100 (wonderful), **10** when you leave it out. It is a real memory: a good feeling is a *Gift*, a bad one a *Quarrel*; from 60 either way it is kept for life, otherwise it fades after a while. People pass memories on when they talk, at **half strength** and marked as heard, so a few days later the friends of the person you insulted have heard it too (and think a little less of you). The `text` says what happened, as a short past clause with `{hero}`/`{npc}`, and small talk tells it as "the day Voll shared berries". | `remember npc "{hero} shared berries" 20` |
| `flag name [value]` | A story note: the whole number `value`, **1** when you leave it out (0 is the same as never set). Read it back in a condition with `flag(name)`, in any script or interaction file: `-> Ask for the key [if flag(promised-hunt)] => key`. Flags are saved with the game and start empty in a new run. Names are words, hyphens allowed. | `flag promised-hunt` or `flag trust 3` |
| `chronicle "line"` | Writes a line in the clan's chronicle, so the clan's story tells what the talk led to. Say why in the line. `{hero}` and `{npc}` are allowed. | `chronicle "{hero} promised {npc} a hunt"` |

A choice that does all three (a promise worth remembering):

```text
=== start
Talker: Will you hunt for me?
-> Promise a hunt {flag promised-hunt; chronicle "{hero} promised {npc} a hunt"; remember npc "{hero} promised a hunt" 30} => thanks
-> Not now => END
```

The generated small talk ("Be quiet") uses the same effects: the person's opinion of the hero falls by 10 and they remember it with a feeling of -40.

## Small talk: what people say when no script fits (US-163)

When Talk finds no script for a person, they make small talk: one plain, short line made up from what they remember, what they heard, what they need and the season, and two answers: **Thank you** (nothing happens) and **Be quiet** (the person's opinion of the hero falls by 10). The same lines fill `{smalltalk.<topic>}` in your own scripts: `Elder: {smalltalk.hunt}` says a line of the topic `hunt`, chosen when the node is shown and kept while it stays on screen.

The lines are templates in `assets/data/dialogue/smalltalk.json`, read at start and again on F5 (a mistake is named as `dialogue/smalltalk.json:<line>: message` and the old lines stay in use):

```json
{
  "topics": {
    "memory": [
      "I keep thinking about {memory.what}.",
      "{memory.what}, {memory.when}. I have not forgotten.",
      { "text": "Why do you ask? {memory.what}, {memory.when}. That is all.", "mood": ["wary", "hostile"] }
    ],
    "people": [ "They say {gossip.what}. It leaves me {gossip.feeling}.", ... ],
    "needs": [ "I could do with some {need.name}.", ... ],
    "season": [ "It is {season}. The days go by.", ... ],
    "hero": [ "{hero}, what brings you here?", ... ],
    "hunt": [ "The deer are thin this {season}.", ... ]
  }
}
```

- **Topics.** `memory`, `people`, `needs`, `season` and `hero` must be there, each with **at least 3 templates**; you may add topics of your own (like `hunt`) for `{smalltalk.<topic>}`. Without a topic asked for, the game weighs the topics by the person: a pressing need, a fresh memory, gossip they have heard, the season, the hero. Asking for `memory`, `people` or `needs` when the person has nothing to say about it falls back to the season.
- **A template** is a quoted sentence (any mood) or `{ "text": "...", "mood": ["warm", "friendly"] }`, used only while the person feels one of those ways. The moods are `warm`, `friendly`, `neutral`, `wary`, `hostile` (from their opinion of the hero) and `hungry`, `tired`, `cold`, `lonely` (a pressing need). Keep each template **plain and short**: 140 characters at most.
- **Tokens**, filled when it is said. Everywhere: `{season}` (spring...), `{hero}` (the hero's name), `{npc}` and `{npc.name}` (the speaker). In `memory`: `{memory.what}` ("a wolf at the fire", "Bo blaming Ama", "you giving me a gift"), `{memory.who}` (who did it) and `{memory.when}` (today, yesterday, 3 days ago, a long time ago). In `people`: `{gossip.what}`, `{gossip.who}`, `{gossip.about}` and `{gossip.feeling}` (angry, uneasy, unsure, glad, delighted: how the heard memory sits with them). In `needs`: `{need.name}` (food, rest, warmth, company). A token used in the wrong topic, or one that does not exist, is a mistake with its line.
- **Variety.** A person does not say a line they said in their last three, and nobody says the same line more than twice in fifty. Add templates to widen it.

## Clan members talking to each other (US-165)

When the social simulation makes two clan members **talk**, **quarrel**, **court**, **share** or **give** a gift, and both are within **12 m** of the hero, the game shows it: a short exchange of speech bubbles, the first person's line, then the other's, each over the speaker's head for **3 seconds** and gone when the next line begins. The outcome is the simulation's own (opinions, memories, the chronicle); the bubbles show it. One exchange plays at a time and a few wait their turn; the hero's own talk has the panel, not bubbles; what happened before a save is not played again.

What they say comes from, in this order:
1. A **`@pair` script** that fits them. `@pair first second` names the two, as `@who` does (a name, a role or a kind): the one who starts matches `first`, the one who answers matches `second`, and the lines are said by the speaker with that word. `@bark <kind>` says which event it is for: `talk` (also when there is no `@bark`), `quarrel`, `courtship` (also a pairing), `sharing` or `gift`. The two best matches win (a name beats a role beats a kind, added together), then `@priority`, then the seeded stream. `{partner}` is the one spoken to, `{npc}` the speaker. Up to 6 lines, shown in order; a line with an `[if]` is left out while it is false. Shipped: `pair-elder-child.dlg`.
2. Else two short lines from the topic `social.<kind>` of `smalltalk.json` (`social.talk`, `social.quarrel`, `social.courtship`, `social.sharing`, `social.gift`), one for each, written from the speaker's side: here `{hero}` is the one spoken to.

```text
@bark talk
@pair elder child

=== start
elder: Come here, little one.
child: Yes, elder!
```

## Not in this story yet

Nothing of the dialogue stories remains: the owner reads the whole of M8 at its exit review (X-M8).

## Talking to placed NPCs (US-265)

A placed person of a level (see `docs/guides/npc-data.md`) speaks the script its `player` dialogue names. In that script `opinion(npc, hero)` reads what it thinks of the hero, `mood(npc)` its attitude word (friendly, wary, hostile...), and the effects `opinion npc hero N` and `remember npc "text" N` change its opinion and give it a memory; `{npc}` and `{hero}` fill in as usual. Name the script in the class, kind or NPC (`"dialogues": { "player": "npc-trader.dlg" }`) and give it an `@who` nobody has, so it is not chosen for a clan member.
