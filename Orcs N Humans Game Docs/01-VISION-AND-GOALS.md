# Vision & Goals

## The pitch

A **Halls of Torment / Vampire Survivors-like**: the player stands in
an arena-style dungeon, enemies swarm in from every side in growing
numbers, movement and positioning are the core skill, and most attacks
fire automatically off the player's chosen skills rather than being
manually aimed one at a time. Where this game diverges from the
genre's usual "one run, one meta-currency" shape is that **two
systems, not one, drive long-term progression**:

1. **Passive Trees** — the primary build-crafting layer, and the
   thing meant to make each character (or each run) feel structurally
   different from the last. This is the system the whole game is built
   around, not a bolt-on.
2. **Gear** — deliberately basic compared to an ARPG (no deep affix
   rolling à la the target 2D ARPG project). Gear's job is to surface
   rare, **niche, build-defining pieces** — items that do one unusual
   thing well (Halls of Torment's own equipment design is the direct
   reference point) — not to be a itemization treadmill in its own
   right.

## Two separate XP economies

These must stay conceptually and mechanically separate — conflating
them is the most common way this genre's progression math breaks:

- **In-run XP (orbs).** Enemies drop XP orbs on death, exactly like
  Vampire Survivors/Halls of Torment. Picking them up fills an in-run
  level bar. Each level-up pauses the game and offers **a pick-1-of-4**
  choice (new skill, upgrade an existing skill, or a stat boost — the
  genre-standard boon list). This bar and every choice made from it is
  **reset at the end of the run** — it only ever describes "how strong
  is this one attempt," never anything persistent.
- **Overall XP (meta-progression).** At the end of a run, regardless
  of how it ended (cleared or died), the player is granted a chunk of
  XP toward their **persistent Overall character level**. Each Overall
  level-up grants **passive points**, spent back in town on the
  Passive Tree. This is the real long-term progression — it is what
  makes the *next* run start stronger, not the in-run boon picks.

## Overall XP formula (confirmed)

The user's own framing named two options: sum up a per-tier value for
every enemy killed (Normal/Elite/Epic/Legendary each worth some
number, added together), or a flat reward per dungeon
("Dungeon 1 = 100xp, Dungeon 2 = 200xp"). **Confirmed: do both,
combined** —

```
overallXpAwarded = dungeonBaseXp[dungeonId][difficultyTier]
                  + killCount[Normal]    * tierXpValue[Normal]
                  + killCount[Elite]     * tierXpValue[Elite]
                  + killCount[Epic]      * tierXpValue[Epic]
                  + killCount[Legendary] * tierXpValue[Legendary]
```

Why not either alone:

- **Pure kill-sum** is the shakier of the two on its own once the game
  is throwing hundreds-to-a-thousand enemies on screen — a run that
  drags on longer (better survival, or just a slower player) racks up
  a bigger raw kill count than a run that clears the same content
  faster and more skillfully, which rewards the wrong thing and is
  brutal to balance across dungeons that have different enemy density.
- **Pure flat-per-dungeon** is clean and easy to balance dungeon-to-
  dungeon, but throws away the tier system entirely — killing a
  Legendary would feel the same as killing a Normal, which undercuts
  the whole point of having four enemy tiers.
- **Both together** keeps the flat per-dungeon number as the main
  "how much is this piece of content worth" knob (one number per
  dungeon per difficulty, trivial to tune in a data file), while the
  tier kill bonuses stay small enough to be a *bonus for playing well*
  (going for Elites/Epics/Legendaries, not just fleeing Normals) rather
  than the dominant term that a runaway kill count could otherwise
  become.

The formula shape is locked; the actual `dungeonBaseXp` and
`tierXpValue` numbers are not — both are meant to live in data files
(matching the engine's existing data-driven item/affix philosophy) so
they get tuned by feel once there's a playable loop, not designed on
paper. See [DECISIONS-LOG.md](DECISIONS-LOG.md).

## Enemy tiers

Every enemy in the game is one of **Normal / Elite / Epic /
Legendary**. The same four-tier ladder drives four things at once:

- **HP and damage scaling** (confirmed 2026-09-13) — each tier
  multiplies a shared base enemy stat block. Starting-point
  multipliers (data-driven, tunable, not balanced yet):

  | Tier      | HP ×  | Damage × | Spawn weight       |
  |-----------|-------|----------|--------------------|
  | Normal    | 1×    | 1×       | common (the swarm) |
  | Elite     | 4×    | 1.5×     | uncommon           |
  | Epic      | 12×   | 2.5×     | rare                |
  | Legendary | 40×   | 4×       | very rare / capped per run |

  These are intentionally front-loaded toward HP over damage — a
  Legendary should feel like a tanky, dangerous detour from the swarm,
  not an instant-death spike that punishes just being near it while
  fighting Normals.
- **Overall XP contribution** (above) — deliberately *not* scaled 1:1
  with the HP multiplier (a 40×-HP Legendary is not worth 40× the XP)
  so the flat per-dungeon base stays the dominant term and tier kills
  stay a bonus, not the whole economy.
- **Loot rarity odds** — higher tiers roll better on the (Phase 3)
  loot table.
- **(Later) visual distinctiveness** — see the placeholder note below.

For Phase 0/1, all four tiers share **one placeholder sprite** (a
small 2D Meshy-generated goblin), differentiated by tint/scale only
(which conveniently also doubles as the tier's visual HP-scaling
tell), so the tier *system* (stats, spawning, XP, loot-table hookup)
can be built and proven before any real art exists. See
[03-TECH-ARCHITECTURE.md](03-TECH-ARCHITECTURE.md).

## What "done" looks like early on

Phase 1 is done when: a player can enter one dungeon, get swarmed by
goblins across all four tiers, kill them for in-run XP orbs, hit at
least one pick-1-of-4 level-up, survive or die, and land back in town
having earned Overall XP. No gear, no Passive Tree UI yet — just the
survival loop and the XP pipeline in both directions working end to
end. See [04-ROADMAP.md](04-ROADMAP.md) for the full phase breakdown.
