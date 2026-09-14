# The Orcs N Humans executable. Phase 0 scaffolding only - a bootable
# window proving the DarkXEngine submodule wiring works end to end; the
# actual swarm/combat loop lands in Phase 1 (see
# Orcs N Humans Game Docs/04-ROADMAP.md).
add_executable(OrcsNHumans
    main.cpp
    PlaceholderState.cpp
)

target_link_libraries(OrcsNHumans PRIVATE
    engine
)
