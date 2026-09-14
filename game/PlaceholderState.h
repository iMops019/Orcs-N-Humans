#pragma once

#include "engine/scene/GameState.h"

namespace game {

// Phase 0 stand-in for the real gameplay state (the swarm arena lands
// in Phase 1 - see Orcs N Humans Game Docs/04-ROADMAP.md). Proves the
// engine submodule's render/input/scene systems all link and run
// end to end: clears the screen and draws a couple of static rects
// (a "player" square and a handful of "goblin" squares) with no logic
// behind them yet.
class PlaceholderState : public engine::scene::GameState {
public:
    PlaceholderState(int windowWidth, int windowHeight);

    void render(engine::render::Renderer& renderer) override;

private:
    int m_windowWidth;
    int m_windowHeight;
};

} // namespace game
