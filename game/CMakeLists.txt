# The Orcs N Humans executable. Phase 1: the core swarm survival loop
# (spawner, combat, XP orbs, pick-1-of-4 level-up, end-of-run summary) -
# see Orcs N Humans Game Docs/04-ROADMAP.md.
add_executable(OrcsNHumans
    main.cpp
    swarm/TierData.cpp
    swarm/DungeonData.cpp
    swarm/Boons.cpp
    swarm/SwarmSystems.cpp
    swarm/SwarmArenaState.cpp
)

target_link_libraries(OrcsNHumans PRIVATE
    engine
)
