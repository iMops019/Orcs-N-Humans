# Decisions Log

Newest first. Records *why*, not just *what* — see the engine repo's
own `DECISIONS-LOG.md` for the same convention.

## 2026-09-13 — Project started, engine consumed as a git submodule

New project: a Halls of Torment / Vampire Survivors-like on Dark X
Engine. Set up the same way as the Android port and the RTS (Orcs and
Towers) - a fresh repo with Dark X Engine pulled in as a
`DarkXEngine/` git submodule rather than forked/copied, so engine
improvements flow upstream instead of drifting per-project. Repo name
"Orcs N Humans" is the folder the user prepped for this - provisional,
not a locked title.

Root CMakeLists.txt duplicates the engine's `vcpkg.json`/
`CMakePresets.json` shape because this repo is now the outermost CMake
project (vcpkg manifest mode reads the top project's `vcpkg.json`, not
the submodule's). This is the same nested-`add_subdirectory()`
consumption pattern that motivated the engine repo's
`PROJECT_SOURCE_DIR` vs `CMAKE_SOURCE_DIR` fix (see that repo's commit
"Fix CMake include/asset paths to work when consumed as a git
submodule") - without that fix, landed the same day, every
`engine/...` include would have resolved against this repo's root
instead of the submodule's, and failed to compile.

## 2026-09-13 — Two separate XP economies, not one

The user described two related-but-distinct systems: in-run XP orbs
feeding a pick-1-of-4 level-up (classic VS/HoT), and a persistent
Overall character level earned at end-of-run that grants passive
points. Decided to keep these strictly separate in both data and
naming (Orb XP / in-run level vs. Overall XP / Overall level) rather
than one unified "XP" — conflating them is the most common way this
genre's progression math breaks (an in-run power spike bleeding into
permanent progression, or vice versa). See
`01-VISION-AND-GOALS.md`.

## 2026-09-13 — Overall XP formula: flat per-dungeon base + per-tier kill bonus

The user asked directly which of two approaches to use: sum a per-
enemy-tier value across all kills, or a flat reward per dungeon.
Recommended (and adopted as the Phase 0 starting point, not a locked
number) combining both - flat `dungeonBaseXp[dungeon][difficulty]` as
the main per-content-piece knob, plus a small `tierXpValue[tier]`
bonus per kill on top. Reasoning: a pure kill-sum rewards longer/
slower runs over efficient ones once enemy counts get into the
hundreds-to-thousands, and is hard to balance across dungeons with
different enemy density; a pure flat-per-dungeon number throws away
the point of having four enemy tiers at all. Both values are meant to
live in data files and get tuned once there's a playable loop - see
`01-VISION-AND-GOALS.md` for the full reasoning and the formula.

## 2026-09-13 — Placeholder enemy: one Meshy goblin sprite for all four tiers

To unblock building the tier/spawn/XP/loot *systems* without waiting
on real art, all four enemy tiers (Normal/Elite/Epic/Legendary) share
one small 2D Meshy-generated goblin sprite for Phase 0/1, differentiated
only by tint and scale. Real tiered art is a Phase 4 concern (see
`04-ROADMAP.md`).
