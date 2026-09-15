cmake_minimum_required(VERSION 3.21)
project(OrcsNHumans CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_image CONFIG REQUIRED)
find_package(SDL3_mixer CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# Tell the shared GameEditor tool (built below, as part of the
# DarkXEngine submodule) where THIS game's own data lives, so its
# baked-in asset paths point at Orcs N Humans/assets rather than the
# engine checkout's own assets/ - see
# DarkXEngine/tools/GameEditor/CMakeLists.txt's comment on
# DARKX_GAME_ROOT_DIR for why that distinction matters (this is the
# fix for games mixing content via one shared engine checkout).
set(DARKX_GAME_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Dark X Engine, consumed as a git submodule (DarkXEngine/) rather than
# copied in - engine changes land upstream in that repo and get pulled
# in here via `git submodule update --remote` when wanted. Its own root
# CMakeLists.txt brings in the `engine`, `anim`, and `game_ui` static
# libraries plus the shared GameEditor tool (reused as-is for authoring
# this game's data - see Orcs N Humans Game Docs/03-TECH-ARCHITECTURE.md).
# Resolving engine paths via PROJECT_SOURCE_DIR rather than
# CMAKE_SOURCE_DIR (see DarkXEngine/engine/CMakeLists.txt's own comment)
# is what makes this nested add_subdirectory() work at all.
add_subdirectory(DarkXEngine)

add_subdirectory(game)
