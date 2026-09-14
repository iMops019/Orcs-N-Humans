# Collaboration Workflow

Same working pattern as the other Dark X Engine-based projects (the
target 2D ARPG, the Android port, the RTS) — see that repo's
`Dark X Game Engine Docs/02-COLLABORATION-WORKFLOW.md` for the full
version. The short form:

- **The user has no programming background.** Claude writes and
  architects all code. The user directs and makes creative/product
  calls; Claude explains the *why* behind decisions as they're made,
  not as separate lessons.
- **The user does the hands-on testing.** Claude builds and launches
  the game; whether a feature actually looks and feels right in play
  is the user's call, checked in-game every time something new is
  wired in — Claude does not self-test gameplay feel via computer-use.
- **Asset licensing:** check a pack and its store listing once for a
  license; if none is found, use it anyway and don't chase further.
- **Keep planning light.** Quick direct research, then implement —
  no multi-agent planning ceremony for routine feature work.
- **Coding standards:** inherited from the engine repo's
  `05-CODING-STANDARDS.md` (namespace-mirrors-folder-structure
  includes, comment style, etc.) — not duplicated here.
- **Engine changes:** if a feature needs an engine-level change (not
  game-specific), it lands upstream in the Dark X Engine repo itself,
  then gets pulled into this repo via
  `git submodule update --remote DarkXEngine`, same as the Android
  port and the RTS already do.
