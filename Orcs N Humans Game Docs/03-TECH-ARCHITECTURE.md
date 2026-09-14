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

## Placeholder enemy art

Phase 0/1 enemies are one small 2D sprite (a Meshy-generated goblin,
via the engine's existing Meshy → Blender isometric-render pipeline,
or a flat 2D asset if that's faster to get in) reused across all four
tiers via tint + scale, so the tier/spawn/XP/loot systems can be built
and tested before any real art pipeline work happens for this game
specifically.
