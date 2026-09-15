# Roadmap

## Phase 0 — Repo scaffolding (this pass)

- [x] New repo, Dark X Engine pulled in as a `DarkXEngine/` git
      submodule
- [x] Root CMake project consuming the submodule + a `game/` target
- [x] Minimal bootable `PlaceholderState` (window + clear + static
      "player"/"goblin" rects) proving the wiring compiles and runs -
      since superseded by Phase 1's `SwarmArenaState` and deleted
- [x] This docs folder
- [x] User confirms it builds and launches on their machine

## Phase 1 — Core survival loop

- [x] Swarm spawner/director: waves of goblins spawn around the
      player and path toward them (`SwarmArenaState` + `SwarmSystems` -
      straight-line seek, not full nav/pathfinding: an open arena with
      no obstacles doesn't need it, see `03-TECH-ARCHITECTURE.md`)
- [x] One placeholder attack (auto-fires at nearest enemy in range) and
      basic player movement/HP (WASD, ECS-free `PlayerState`)
- [x] Enemy tier system: Normal/Elite/Epic/Legendary as data
      (`assets/data/enemy_tiers.json`), driving spawn weight, HP/damage
      scaling, and per-tier orb/Overall XP value
- [x] XP orbs on kill -> in-run level bar -> pick-1-of-4 level-up
      screen (4 fixed placeholder boons - see `Boons.cpp`)
- [x] End-of-run summary screen: kills-by-tier, Overall XP awarded
      (see `01-VISION-AND-GOALS.md`'s formula), win (survive 3:00) and
      death both handled
- [x] Real Idle/Walk/Attack animation for player (peasant/Bow Character)
      and goblin, 16-facing baked sprite sequences rendered from Meshy's
      animated rigs - replaces the placeholder tinted rects both used
      through the rest of Phase 1 (see `DECISIONS-LOG.md`). Pulled
      forward from Phase 4's "real art" item below since the pipeline
      to produce it landed first.
- [ ] Stress-test toward ~1,000 concurrent enemies with F3's DebugHud
      (F6 spawns a 100-enemy burst for this); only then decide if/what
      needs batching (see `03-TECH-ARCHITECTURE.md`) - **user to run
      this pass and report back**

## Phase 2 — Town + Overall progression

- [ ] Persistent save: Overall level, Overall XP, passive points,
      unspent/spent state
- [ ] Town hub state (reuse as much of the ARPG's Idle Hub shape as
      makes sense - it's the same "screen between runs" role)
- [ ] Passive Tree data model (nodes, connections, costs) + a first
      pass UI to view and spend points
- [ ] Dungeon select screen wired to `dungeonBaseXp` per dungeon/
      difficulty

## Phase 3 — Gear

- [ ] Basic gear slots + a small item data model (far shallower than
      the ARPG's affix system by design - see `01-VISION-AND-GOALS.md`)
- [ ] First few niche/unique items with a build-defining effect each
- [ ] Loot drops tied into the enemy tier system (Elites/Epics/
      Legendaries drop better odds)

## Phase 4 — Content + real art

- [x] Replace the placeholder goblin with real animated enemy art
      (Idle/Walk/Attack, 16 facings - see Phase 1 above and
      `DECISIONS-LOG.md`). **Not yet done:** distinct art per tier -
      Elite/Epic/Legendary still differentiate via tint/scale over the
      same Normal-tier goblin model, not separate Meshy models.
- [ ] More dungeons, more enemy variety, Passive Tree fleshed out past
      its first-pass shape
- [ ] Balance pass on the Overall XP formula and tier values once
      there's actual playtime to tune against

Later phases (further content, polish, platform questions) get added
here as Phase 3-4 land — deliberately not planned out in detail yet
per `02-COLLABORATION-WORKFLOW.md`'s "keep planning light."
