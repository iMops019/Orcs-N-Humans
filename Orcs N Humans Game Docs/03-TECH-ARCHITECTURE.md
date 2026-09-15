# Tech Architecture

## Repo shape

```
Orcs N Humans/                  (this repo)
  DarkXEngine/                  git submodule -> github.com/iMops019/Dark-X-Engine
    engine/                     ECS, renderer, input, nav, physics, audio, save, ...
    game/anim/, game/ui/        reusable 2D Animator + UI widget/theme layer
    tools/GameEditor/           shared authoring tool - reused as-is for this game's data
  game/                         this game's own source (states, data model, main.cpp)
  assets/                       this game's own art/audio/data (not yet created)
  Orcs N Humans Game Docs/      this folder
  CMakeLists.txt                add_subdirectory(DarkXEngine); add_subdirectory(game)
  vcpkg.json, CMakePresets.json duplicated from the engine repo (same deps: SDL3 stack,
                                 nlohmann-json, imgui) - this repo is the outer CMake
                                 project now, so vcpkg manifest mode reads *this* file
```

Why a submodule and not a fork/copy: engine improvements (renderer,
ECS, nav, the swept-collision system, sprite batching work) benefit
every Dark X Engine game, this one included, and should flow one
direction — upstream into the engine repo — not get duplicated and
drift. See the engine repo's `DECISIONS-LOG.md` ("Bane of the Outcasts
removed") for the same reasoning applied to the ARPG project, and its
09-ANDROID-PORT.md for the identical shape used by the Android port.

The engine's own `engine/CMakeLists.txt` (and `game/anim`, `game/ui`,
`tools/GameEditor`) resolve their include paths and baked-in asset
paths via `PROJECT_SOURCE_DIR` rather than `CMAKE_SOURCE_DIR`
specifically so this nested `add_subdirectory()` works correctly — see
that repo's commit "Fix CMake include/asset paths to work when
consumed as a git submodule."

## What's reused vs. what's new

Reused wholesale from the engine: the OpenGL renderer, ECS, input
mapping, nav/pathfinding, physics/collision (swept move + wall-slide),
save system, tilemap renderer, 2D Animator, UI widget/theme layer, and
the Game Editor (as the data-authoring tool for this game's enemies,
items, and dungeons, the same way it authors the ARPG's).

New to this game: the swarm spawner/director, the enemy tier system
(Normal/Elite/Epic/Legendary), the dual XP economy (in-run orbs vs.
Overall level — see `01-VISION-AND-GOALS.md`), the Passive Tree data
model + UI, the pick-1-of-4 level-up screen, and this game's own (much
simpler than the ARPG's) gear/item model.

## Performance target: enemies on screen

The genre's whole identity is screen-filling swarms — the working
target is **on the order of 1,000 simultaneous enemies**, which the
current renderer was not stress-tested against (its batching exists,
per `07-RENDERER-REDESIGN.md` in the engine docs, but the ARPG project
never spawned anywhere near this many entities at once). This is the
one area where "reused as-is" is not a safe assumption — expect this
project to be the forcing function for sprite-batching/instancing work
that was already on the engine's own roadmap (see engine repo's
`project_engine-layer-push` context) rather than a nice-to-have.
Measure before optimizing, same rule as everywhere else in this
engine: get the swarm spawner working correctly first, profile with
F3's DebugHud, and only then decide what actually needs batching.

## Character animation: baked 16-facing frame sequences, not sprite-sheet-per-frame math

Player (peasant/Bow Character) and goblin both have real Idle/Walk/
Attack animation, rendered directly from Meshy's animated rigs rather
than authored by hand: `DarkXEngine/tools/BlenderScripts/render_facing_sequence.py`
poses the actual 3D mesh per sampled frame, once per one of
`engine::render::DirectionalAnimation`'s 16 compass facings, straight
to PNG (`assets/textures/characters_animated/<clip>/<COMPASS>/`) - see
`DECISIONS-LOG.md` for why an earlier attempt (2D-cutout-rig rotate/
offset curves, `game/anim/CharacterRig`) was abandoned in favor of
this. Loaded/played via `engine::render::SpriteClipSet`/`SpriteAnimator`
in `SwarmArenaState`; enemies get a real `SpriteAnimator` ECS component
per spawn, the player (deliberately not an ECS entity) keeps its own as
a plain state member. Tier differentiation (Elite/Epic/Legendary) is
still tint + scale over the one Normal-tier goblin model, not separate
art per tier - that part of the original placeholder-art plan still
holds.
