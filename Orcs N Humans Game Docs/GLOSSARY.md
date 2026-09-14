# Glossary

Terms specific to this game. Engine-level vocabulary (ECS, GameState,
StateStack, ...) is covered in the engine repo's own `GLOSSARY.md`.

- **Orb / XP Orb** — the in-run pickup dropped by dying enemies, fills
  the in-run level bar. Reset every run — never persists.
- **In-run level / level-up** — leveling up *within a single run* via
  Orb XP; triggers the pick-1-of-4 boon screen. Not the same as Overall
  level.
- **Overall level / Overall XP** — the persistent, account-wide
  character level, advanced by XP granted at the end of each run
  (regardless of outcome). Each Overall level-up grants passive
  points.
- **Passive Point** — earned from Overall level-ups, spent in town on
  the Passive Tree. The primary long-term build-crafting currency.
- **Passive Tree** — the persistent, town-side skill/stat tree that
  passive points are spent on. The game's central build-crafting
  system (see `01-VISION-AND-GOALS.md`).
- **Tier (Normal / Elite / Epic / Legendary)** — every enemy's
  classification; drives spawn weight, HP/damage scaling, loot odds,
  and Overall XP contribution.
- **Dungeon** — one selectable piece of content (its own base Overall
  XP value and difficulty), entered from town, ending in either a
  clear or a death.
- **Pick-1-of-4** — the level-up choice screen shown on every in-run
  level-up: new skill / upgrade a skill / stat boost, genre-standard
  shape (Vampire Survivors / Halls of Torment).
