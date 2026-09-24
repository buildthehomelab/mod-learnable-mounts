# mod-learnable-mounts

An [AzerothCore](https://www.azerothcore.org/) module (WotLK 3.3.5a) that fixes mount items which
still summon the mount straight from your bags. After this module, using one of them teaches the
mount, it shows up in the Mounts tab of the spellbook, and the item is used up, the same as every
other mount item in 3.3.5.

By default it converts every such item in stock AzerothCore, found by checking each item_template
row for a mount spell (Mounts skill line, 777) that is cast on use instead of taught:

| Item | Name | Mount spell | |
|---|---|---|---|
| [13325](https://www.wowhead.com/item=13325) | Fluorescent Green Mechanostrider | 17458 | never released |
| [1041](https://www.wowhead.com/item=1041) | Horn of the Black Wolf | 578 | vanilla racial mount, removed before 3.0 |
| [1133](https://www.wowhead.com/item=1133) | Horn of the Winter Wolf | 581 | vanilla racial mount, removed before 3.0 |
| [1134](https://www.wowhead.com/item=1134) | Horn of the Gray Wolf | 459 | vanilla racial mount, removed before 3.0 |
| [5663](https://www.wowhead.com/item=5663) | Horn of the Red Wolf | 579 | vanilla racial mount, removed before 3.0 |
| [2413](https://www.wowhead.com/item=2413) | Palomino | 471 | vanilla racial mount, removed before 3.0 |
| [2415](https://www.wowhead.com/item=2415) | White Stallion | 468 | vanilla racial mount, removed before 3.0 |
| [5874](https://www.wowhead.com/item=5874) | Harness: Black Ram | 6896 | vanilla racial mount, removed before 3.0 |
| [5875](https://www.wowhead.com/item=5875) | Harness: Blue Ram | 6897 | vanilla racial mount, removed before 3.0 |
| [8583](https://www.wowhead.com/item=8583) | Horn of the Skeletal Mount | 8980 | vanilla racial mount, removed before 3.0 |
| [8589](https://www.wowhead.com/item=8589) | Old Whistle of the Ivory Raptor | 10795 | vanilla racial mount, removed before 3.0 |
| [8590](https://www.wowhead.com/item=8590) | Old Whistle of the Obsidian Raptor | 10798 | vanilla racial mount, removed before 3.0 |
| [14062](https://www.wowhead.com/item=14062) | Kodo Mount | 18363 | never released |
| [16339](https://www.wowhead.com/item=16339) | Commander's Steed | 16082 | old PvP rank reward |
| [25596](https://www.wowhead.com/item=25596) | Peep's Whistle | 32345 | developer joke item |
| [29225](https://www.wowhead.com/item=29225) | zzoldSwift Warstrider | 35028 | deprecated copy; Swift Warstrider already teaches this spell |

That scan used the WotLK Classic client's Mounts skill line. The module checks each spell against
your server's own 3.3.5 data at startup, so an item whose spell isn't a mount there is skipped with
an error in the log rather than changed.

## Why the items are broken

In 3.0 Blizzard converted every mount item to this layout:

| | spell 1 | spell 2 |
|---|---|---|
| Green Mechanostrider (13321) | 55884 Learning, on use, 1 charge | 17453 mount, trigger 6 "learn" |
| Fluorescent Green Mechanostrider (13325), stock AC | 17458 mount, on use | — |

None of the items above could be obtained in retail by 3.0, so nobody converted them. Their mount
spells *are* in the client's Mounts skill line (for 17458, SkillLineAbility row 18724, skill 777),
so once a character knows the spell the client lists it with the other mounts. The item rows were
the only thing missing.

## Changes

Each listed item gets the stock layout above: spell 1 becomes 55884 with one charge (category 330,
3 s category cooldown, the same values as other mount items) and spell 2 becomes the mount spell
with trigger 6. The core's `Player::CastItemUseSpell` then teaches spell 2 and consumes the item.
If the item has no description, it gets "Teaches you how to summon this mount."

Copies players already carry change too, since the template is edited in place. Their first use
teaches the mount and destroys the item.

An item is skipped, with an error in the log, unless it has exactly one spell: an on-use spell
that applies a mount aura and is in the Mounts skill line. Items that already use 55884 are skipped
with an info line.

## Why a module and not an SQL update

mod-individual-progression rewrites many item_template rows back to vanilla in
`data/sql/world/base/vanilla_item_changes.sql`, and the DB updater re-runs that file whenever IP
changes it. Editing the loaded template at startup can't be undone that way, and the core builds
item tooltips from the template anyway.

## Installation

1. Put this folder in your AzerothCore `modules/` directory.
2. Re-run CMake and rebuild the worldserver.
3. Copy `conf/mod_learnable_mounts.conf.dist` to `mod_learnable_mounts.conf` next to your other
   module configs.
4. Players delete their client's `Cache` folder (or at least `Cache/WDB`) to see the new tooltip.
   The server teaches the mount either way.

On startup the log shows one line per converted item, for example:

```
mod-learnable-mounts: item 13325 (Fluorescent Green Mechanostrider) now teaches mount spell 17458
```

## Configuration

| Key | Default | |
|-----|:------:|---|
| `LearnableMounts.Enable` | 1 | 0 = leave every item as the DB has it |
| `LearnableMounts.Items` | the 16 items above | comma-separated item ids to convert |

Changes take effect on the next worldserver restart.
