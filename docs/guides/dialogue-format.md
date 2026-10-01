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
-> Offer berries [if has(hero, berries, 1)] [else You have no berries] {take hero berries 1; opinion npc hero 5; remember npc "{hero} shared berries"} => thanks
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

When the player chooses **Talk** on a clan member, the game looks for the script that speaks for them. A script fits when one of its `@who` words is the person's **name** (the most specific), a **role** they have (`elder` is the oldest living member of the clan, `child` is anyone under 12), or their **kind** (`person`), and its `@when` holds. The most specific match wins, then the higher `@priority`, then the file name (US-162 adds barks and a seeded tie-break). A script marked `@bark` or `@pair` never answers Talk. When no script fits, Talk is the plain "You talk with ..." of before.

The game **pauses** while the conversation panel is open. It shows:

- the person's name and one **mood word**: how they feel about the hero (`warm`, `friendly`, `neutral`, `wary`, `hostile`), unless a need is pressing and they are not hostile, then `hungry`, `tired`, `cold` or `lonely`;
- what they say: every line of the node whose `[if]` holds, in order, as `Speaker: words`;
- the choices, numbered 1 to 5. Pick one with the mouse or the keys **1** to **5**. A choice with a false `[if]` is hidden, or greyed out with its `[else]` reason and cannot be picked. **Esc** walks away: the talk ends and no effect happens.

Choosing runs the choice's `{effects}` in order, then moves to its node, or ends the talk at `END`. Today the panel carries out `take hero <item> n`, `give hero <item> n`, `opinion npc hero n` (what the first thinks of the second changes by `n`, kept between -100 and 100) and `say`; the other verbs are read and checked but do nothing yet (`remember` and `flag` arrive with US-164). `opinion(npc, hero)` in a condition reads the real opinion.

## Not in this story yet

Barks and the seeded choice between scripts (US-162), small talk for people without a script and the `{smalltalk.topic}` lines (US-163), memories and flags (US-164) and bubbles between NPCs (US-165) come in the next stories. Until US-163 a `{smalltalk.hunt}` shows as written.