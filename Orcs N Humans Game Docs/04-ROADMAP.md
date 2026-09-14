# Roadmap

## Phase 0 — Repo scaffolding (this pass)

- [x] New repo, Dark X Engine pulled in as a `DarkXEngine/` git
      submodule
- [x] Root CMake project consuming the submodule + a `game/` target
- [x] Minimal bootable `PlaceholderState` (window + clear + static
      "player"/"goblin" rects) proving the wiring compiles and runs
- [x] This docs folder
- [ ] User confirms it builds and launches on their machine

## Phase 1 — Core survival loop

- [ ] Swarm spawner/director: waves of goblins spawn around the
      player and path toward them (reuse engine nav/steering)
- [ ] One placeholder attack (auto-fires at nearest enemy) and basic
      player movement/HP
- [ ] Enemy tier system: Normal/Elite/Epic/Legendary as data, driving
      spawn weight, HP/damage scaling, and (stub) loot/XP value
      differences
- [ ] XP orbs on kill -> in-run level bar -> pick-1-of-4 level-up
      screen (even with a tiny placeholder boon list)
- [ ] End-of-run summary screen: kills-by-tier, Overall XP awarded
      (see `01-VISION-AND-GOALS.md`'s formula), win/death both handled
- [ ] Stress-test toward ~1,000 concurrent enemies with F3's DebugHud;
      only then decide if/what needs batching (see
      `03-TECH-ARCHITECTURE.md`)

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

- [ ] Replace the placeholder goblin with real tiered enemy art
      (Meshy pipeline or hand-authored)
- [ ] More dungeons, more enemy variety, Passive Tree fleshed out past
      its first-pass shape
- [ ] Balance pass on the Overall XP formula and tier values once
      there's actual playtime to tune against

Later phases (further content, polish, platform questions) get added
here as Phase 3-4 land — deliberately not planned out in detail yet
per `02-COLLABORATION-WORKFLOW.md`'s "keep planning light."
