# Decisions Log

Newest first. Records *why*, not just *what* — see the engine repo's
own `DECISIONS-LOG.md` for the same convention.

## 2026-09-14 — Fixed the animation load hang; real arrow projectile; bow in the player's hands

Three follow-ups to the entry directly below, all same-session.

**Load hang, fixed.** Launching after the player/goblin animation wiring landed a black screen that eventually got Windows' "Not Responding" treatment. Root cause was two compounding problems: the baked clips were wildly oversampled (Idle alone had 101 frames × 16 facings, at full 512px when the actual cropped content was only ~150-250px), and `engine::render::DirectionalAnimation::loadFromDirectory` decoded every one of those frames TWICE (see the engine repo's own `DECISIONS-LOG.md` for that fix) - together, ~8,600 full-resolution PNG decodes synchronously in `onEnter()` before the window ever pumped a message. Fixed on both sides: thinned every clip down to 12-16 frames per facing (still smooth at this genre's typical playback speed) and re-rendered at 220px instead of 512px (`assets/textures/characters_animated` dropped from 453MB to ~20MB), plus the engine's single-decode-pass fix. Load is now near-instant. **Side effect caught and fixed the same pass:** `visualScale` for both the player and enemies was tuned against the old 512px source, so the resolution cut alone shrank on-screen character size by the same ratio (512/220) - multiplied both scale constants back up by that exact ratio to restore the original on-screen size without needing to re-tune by eye.

**Arrow projectile.** The player's shot was still the placeholder white pixel rect from Phase 1. Replaced with a real arrow, rendered from an existing (previously reference-only) Meshy shortbow-arrow model via `render_isometric_sprite.py`, oriented via the same empirical-probe approach as character facings so its rest pose points along a clean screen-right (0°) axis - the shot's own `Velocity` angle (`atan2(vy,vx)`) then IS the rotation to apply, no base-angle correction needed. Lives at `assets/textures/props/arrow.png`, drawn via `Renderer::drawTextureRotated` (continuous rotation, not a 16-facing bucket - a flying projectile needs to point exactly where it's going, not the nearest of 16 directions).

**Bow now visibly in the player's hands**, across all three clips (Idle/Walk/Attack), not just conjured during the draw. This was the open question from the entry below ("where's the bow") - turned out to have nothing to do with the earlier CharacterRig-era bow-splitting attempt (`reference/meshy/Meshy_AI_medieval_shortbow_3d_manualsplit.blend`, from before this game pivoted away from that pipeline entirely): Meshy's ANIMATION-clip exports simply never include held equipment, confirmed by listing every object in the Archery Shot FBX - just the skinned body. Fixed by rigidly attaching the separately-modeled bow to the `LeftHand` bone at render time (new prop-attachment feature in `render_facing_sequence.py`, see the engine repo's own `DECISIONS-LOG.md` for the mechanism and the scale gotcha that cost a few blind iterations - short version: the armature's baked-in 0.01 unit scale means the prop's own scale parameter has to be much larger than intuition suggests, 58 not ~0.6, to end up bow-sized). All three peasant clips re-rendered with the bow attached; the goblin doesn't carry one.

## 2026-09-14 — Real Idle/Walk/Attack animation for player and goblin, via baked facing sequences (not the CharacterRig curve rig)

Picks up directly from the entry below ("First two CharacterRig
entries") - posed clips are done, but NOT via the CharacterRig/2D-
Animator curve-authoring path that entry expected. Full technical
account (the two approaches tried, the exact facing-angle math, the
known one-shot-restart rough edge) lives in the engine repo's own
`DECISIONS-LOG.md`, "Animated-clip authoring: two approaches tried, one
adopted" - this entry covers the game-specific decisions on top of it.

**Why the switch:** the curve-rig approach (rotate/offset each body-part
sprite per bone) broke down hard on the peasant's bow-draw attack - a
2D cutout can only rotate around one screen-plane axis, but drawing a
bow swings the whole arm through real depth. Since Meshy already hands
over a FULLY posed, animated 3D mesh per clip, the actual fix was to
stop approximating it: render the real posed mesh directly, once per
frame, once per facing (16-point compass, reusing
`engine::render::DirectionalAnimation`'s existing folder convention -
built engine-side years before this game existed, never fed by
anything until now). Zero approximation error, and it sidesteps the
whole rotation-instability class of problem entirely.

**Real bug found along the way, worth remembering:** `peasant_rigged.fbx`
(the STATIC export the sprite art/`character_rigs.json` attach points
were built from) and the separately-exported ANIMATED FBXs (`peasant_idle.fbx`
etc.) do NOT share an identical rest skeleton, even though both claim
to be "the same rig" - confirmed by directly comparing a joint's rest
position between the two files and finding a large, clearly-not-
rounding-noise discrepancy. Apparently Meshy's separate export/rig
passes aren't guaranteed pixel-identical. Irrelevant now that the
curve-rig path is unused for these clips, but worth knowing if that
path ever gets revived for hand-authored poses.

**Clips chosen:**
- **Peasant** (`assets/textures/characters_animated/peasant_idle|walk|bow_attack/`) - Idle, Walk, and an Archery Shot used as the Attack (this is the Bowman class's first weapon, so a bow draw is the actually-correct attack pose here, not a placeholder pick).
- **Goblin** (`.../goblin_idle|walk|attack/`) - Idle and Walk are straightforward; nothing in Meshy's animation list read as a goblin-appropriate attack, so the user picked "Shield Push Left" on visual merit alone and it's used as Attack regardless of literal name.

**Gameplay wiring** (`game/swarm/SwarmArenaState.*`/`SwarmSystems.*`):
placeholder tinted rects for both the player and enemies are gone,
replaced with the real baked sprites via `SpriteAnimator`/
`SpriteClipSet`/`resolveSpriteFrame` (engine components that existed
but had zero live callers before this - see the engine log entry).
Enemies are ECS entities, so they get a real `SpriteAnimator` component
per spawn and get ticked in a batch via
`engine::render::systems::advanceSpriteAnimators`; the player is
deliberately NOT an ECS entity (`PlayerState`'s own long-standing
design, see the "Phase 1 built" entry below) so `m_playerAnim` lives as
a plain `SwarmArenaState` member, ticked by hand instead.

**Player attack-facing is Halls of Torment-style, not movement-locked**:
`updatePlayerAttack` now reports the nearest in-range enemy's direction
every frame (not just on the frame a shot fires), and that overrides
movement-direction facing whenever a target exists - so strafing around
an enemy keeps the player visually facing it, matching the genre
convention the user asked for by name. A future "Auto Attack on/off"
settings toggle was raised as a want but not built - noted for later,
doesn't change today's targeting behavior either way.

**Enemy Attack is contact-triggered, not cooldown-based** - by design,
per the user's explicit call: enemies don't have a discrete "swing"
action the way the player's ranged auto-fire does, only continuous
contact damage, so `updateContactDamage` now reports which enemy (if
any) actually landed a hit each frame, and that entity's Attack one-
shot plays on top of its Walk loop.

**Bug found and fixed the same session**: the player's attack-facing
was initially gated on `!m_playerAnim.playingOnce()` (mirroring a
reference pattern from a discrete-melee-swing game) - but this game's
attack auto-fires every `attackCooldown` (0.6s) at any target in range,
far more often than the ~2s Attack clip takes to play out, so
`playingOnce()` stayed true almost continuously once the swarm picked
up. Facing froze the instant combat started and never moved again for
the rest of the run. Fixed by simply not gating movement/target facing
on attack state at all - the Attack clip has all 16 facings rendered
too, so there was never a real reason to.

**Known rough edge, not fixed**: an Attack one-shot re-triggered faster
than its own playback length restarts from frame 0 instead of
finishing - true for both the player (0.6s cooldown vs. ~2s clip) and
the goblin (each contact hit vs. the Shield Push clip's own length)
once combat gets busy. A clip-fps/cadence tuning question, not a
correctness bug - left for a later pass.

**Tooling housekeeping**: `tools/BlenderScripts/extract_animation_clip.py`
(the abandoned curve-rig extractor) and its `assets/data/animations.json`
output stay in the repo - the cutout-puppet approach is still the right
tool for hand-authored poses or heavy per-part gear/tint variation,
just not for these Meshy-sourced clips - but `animations.json` itself
was emptied back out (the 3 curve-based clips it briefly held are
superseded by the baked sequences above) rather than left as dead,
confusing data.

## 2026-09-14 — First two CharacterRig entries: goblin and the Bow Character body

`assets/data/character_rigs.json` now has real rigs for both the
goblin (enemy) and the peasant body (Bow Character base) - 10 bones
each (torso, head, upper-arm/forearm x2, thigh/shin x2), art at
`assets/textures/characters/<name>/`. Built from Meshy's rigged FBX
exports via `DarkXEngine/tools/BlenderScripts/segment_rigged_character.py`
(see that engine repo's own DECISIONS-LOG for the two real Blender-
scripting bugs found getting it working - an `--background` operator-
context gotcha, and a parent/armature-deletion scale bug).

Every bone's `anchorX/anchorY` and `attachX/attachY` are **computed**,
not eyeballed - the script projects each bucket's primary skeleton
joint (shoulder/elbow/hip/knee/neck) through the exact same camera the
parts were rendered with (`bpy_extras.object_utils.world_to_camera_view`),
so the numbers are pixel-accurate to the source geometry. Verified two
ways: read `game/anim/AnimationSampler.cpp`/`ClipPlayer.cpp` to confirm
the exact runtime formula (world position = summed `attachX/Y` up the
parent chain; `anchorX/Y` is the pivot fraction within each part's own
image), then reimplemented that formula in a throwaway Python compositor
and rendered both rigs back into single images - both reassembled into
correctly-proportioned, recognizable standing characters.

**Deliberately not done yet:** actual posed clips (Idle/Walk/Attack -
keyframing the curves on top of this bind pose) need visual, iterative
authoring in the 2D Animator's Clips tab, which isn't a blind-JSON task
the way the rig skeleton itself was. Waiting on the user's Game Editor
rebuild (see the user's own account of *why* it needs rebuilding -
multiple game sessions had ended up sharing one Dark X Engine checkout
instead of each game's own submodule copy, mixing content across
games/games' Game Editor tabs) before that work happens. Draw order
(which bone draws in front of which) is also an unverified first guess
- easy to fix visually once the Editor is usable again.

**Where this data lives, on purpose:** `Orcs N Humans`'s own `assets/`,
never `DarkXEngine/assets/` (even though the Game Editor tool itself is
hardwired via CMake to read/write whichever engine checkout it's built
from - see `GameEditor`'s `DARKX_SOURCE_ASSETS_DIR`). Character rig/
sprite data is per-game content, the same category `assets/data/*.json`
game data (removed from the engine repo entirely, see that repo's
"Bane of the Outcasts removed") already established shouldn't live in
the shared engine repo.

## 2026-09-13 — Phase 1 built: one state, not a StateStack of them

## 2026-09-13 — Phase 1 built: one state, not a StateStack of them

The whole survival loop (spawner, combat, XP orbs, the pick-1-of-4
level-up pause, and the end-of-run summary) lives in one
`SwarmArenaState`, switching internally between `Playing`/`LevelUp`/
`Summary` via a private enum, rather than three separate `GameState`s
pushed/popped on the `StateStack`. There is no town/menu flow yet for
a real state transition to land in - that only arrives in Phase 2 - so
splitting this up now would just be premature structure with nowhere
useful to go; "press Enter on the summary screen" currently just calls
`resetRun()` in place. Expect this to get split apart once Phase 2's
town hub exists and an actual `NextState`-driven transition out of a
finished run is possible.

Enemies use no pathfinding, just a normalized vector straight at the
player (`SwarmSystems::updateEnemySeek`) - this is an open arena with
no obstacles, so the engine's nav/flow-field system (built for the RTS,
see `project_rts-supercharge` in the Dark X Engine memory) would be
solving a problem this game doesn't have yet. Revisit if/when dungeons
get terrain.

Projectiles are fire-and-forget straight lines locked to the target's
position at the moment of firing, not homing entities tracking a
stored target - `engine::ecs::Entity` has no generation counter (see
`engine/ecs/Entity.h`), so a stored target id could go stale (or even
get reused) before a slow shot arrives; this also matches how the
genre's projectiles read anyway (Vampire Survivors' knife, this game's
arrow).

Enemy HP/damage/spawn-weight/XP values are loaded from
`assets/data/enemy_tiers.json` at runtime (`TierTable::loadFromFile`),
falling back to the same numbers hardcoded if the file is missing or
malformed - so a bad/missing data file never prevents the game from
booting, and the numbers from `01-VISION-AND-GOALS.md`'s table are
real, tunable data rather than a promise made only in docs.

The stress-test-toward-1,000-enemies roadmap item is **not** done by
Claude - per `02-COLLABORATION-WORKFLOW.md`, feel/performance-in-play
is the user's pass, run with F3's DebugHud open and F6 (spawns a
100-enemy burst) held down as needed.

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
Recommended combining both - flat `dungeonBaseXp[dungeon][difficulty]`
as the main per-content-piece knob, plus a small `tierXpValue[tier]`
bonus per kill on top. Reasoning: a pure kill-sum rewards longer/
slower runs over efficient ones once enemy counts get into the
hundreds-to-thousands, and is hard to balance across dungeons with
different enemy density; a pure flat-per-dungeon number throws away
the point of having four enemy tiers at all. **Confirmed by the user
2026-09-13.** The formula shape is now locked; `dungeonBaseXp` and
`tierXpValue`'s actual numbers are not - both live in data files and
get tuned once there's a playable loop. See `01-VISION-AND-GOALS.md`
for the full reasoning and the formula.

## 2026-09-13 — Enemy tiers also scale HP and damage, not just loot/XP

Originally the four enemy tiers (Normal/Elite/Epic/Legendary) only
drove loot odds and Overall XP value in the design doc; the user
confirmed they should also scale enemy **HP and damage**, each tier
multiplying a shared base stat block. Adopted starting-point
multipliers (front-loaded toward HP over damage, so a Legendary reads
as a tanky detour rather than an instant-death spike near the swarm):
Normal 1×/1×, Elite 4×/1.5×, Epic 12×/2.5×, Legendary 40×/4× (HP/
damage). Not balanced - a Phase 1/4 tuning target once there's a
playable loop. Overall XP per tier deliberately does NOT scale 1:1
with the HP multiplier, so the flat per-dungeon base stays the
dominant term in the XP formula above. See
`01-VISION-AND-GOALS.md`'s Enemy tiers section for the full table.

## 2026-09-13 — Placeholder enemy: one Meshy goblin sprite for all four tiers

To unblock building the tier/spawn/XP/loot *systems* without waiting
on real art, all four enemy tiers (Normal/Elite/Epic/Legendary) share
one small 2D Meshy-generated goblin sprite for Phase 0/1, differentiated
only by tint and scale. Real tiered art is a Phase 4 concern (see
`04-ROADMAP.md`).
