# Orcs N Humans — Docs Index

**Status:** Phase 0 — repo scaffolding just landed, no gameplay yet.

A Halls of Torment / Vampire Survivors-like built on the **Dark X
Engine** (consumed here as a git submodule, `DarkXEngine/`). Read
these in order:

- [01-VISION-AND-GOALS.md](01-VISION-AND-GOALS.md) — what the game is,
  what drives it, what "done" looks like for early milestones
- [02-COLLABORATION-WORKFLOW.md](02-COLLABORATION-WORKFLOW.md) — how
  the user and Claude work together on this project
- [03-TECH-ARCHITECTURE.md](03-TECH-ARCHITECTURE.md) — the submodule
  structure, what's reused from the engine, swarm-scale performance
  targets
- [04-ROADMAP.md](04-ROADMAP.md) — phased build-out plan
- [DECISIONS-LOG.md](DECISIONS-LOG.md) — why things are the way they
  are, in the order they were decided
- [GLOSSARY.md](GLOSSARY.md) — this game's own vocabulary

### Where the engine comes from

Source repo: `https://github.com/iMops019/Dark-X-Engine` (also checked
out standalone at `C:\Users\conov\Documents\Dark X Engine`), pulled in
here as the `DarkXEngine/` git submodule. That repo's own
`Dark X Game Engine Docs/` folder covers every engine system in depth
(`03-TECH-ARCHITECTURE.md`, `05-CODING-STANDARDS.md`,
`DECISIONS-LOG.md`, `GLOSSARY.md`) — worth reading for depth, not
required to start. Engine changes land upstream in that repo, then get
pulled into this one with `git submodule update --remote`.

### Who you're working with

- **The user has no programming background and does not write code.**
  Claude is the technical lead and writes/architects all of it. The
  user directs, makes product/creative calls, and learns through
  Claude explaining the reasoning as decisions are made.
- **The user does the hands-on testing.** Claude builds and launches;
  the "does it look/feel right" pass in the actual game is the user's,
  every time a piece is wired in.
- **Asset licensing:** check a pack and its store listing once for a
  license; if none is found, use it anyway and don't chase further.
- **Keep planning light.** Quick direct research, then implement. No
  multi-agent planning ceremony.
