#pragma once

#include <memory>
#include <random>
#include <string>

#include "engine/render/SpriteAnimator.h"
#include "engine/render/TextRenderer.h"
#include "engine/scene/GameState.h"

#include "DungeonData.h"
#include "PlayerState.h"
#include "TierData.h"

namespace engine::ecs {
class Registry;
}

namespace game::swarm {

// Phase 1's whole survival loop in one state (see
// Orcs N Humans Game Docs/04-ROADMAP.md): swarm spawner, combat,
// XP-orb pickup, the pick-1-of-4 level-up pause, and an end-of-run
// summary, all handled internally via m_phase rather than as separate
// GameStates - there is no town/menu flow yet for a StateStack
// transition to land in (that's Phase 2), so splitting this up now
// would be premature - see DECISIONS-LOG.md.
class SwarmArenaState : public engine::scene::GameState {
public:
    SwarmArenaState(int windowWidth, int windowHeight);
    ~SwarmArenaState() override;

    void onEnter(engine::render::Renderer& renderer, engine::assets::AssetManager& assets) override;
    void handleEvent(const SDL_Event& event) override;
    void update(float deltaTime, const engine::input::InputMap& input) override;
    void render(engine::render::Renderer& renderer) override;

private:
    enum class RunPhase { Playing, LevelUp, Summary };

    // Cast to int for SpriteAnimator::loopState/onceState/fallbackState -
    // see PlayerAnimState's loadClip() calls in onEnter() for which
    // baked facing-sequence folder (assets/textures/characters_animated/)
    // each state maps to.
    enum class PlayerAnimState { Idle, Walk, Attack };

    void resetRun();
    void beginSummary(const std::string& outcome);
    void applyBoonPick(int index);

    void drawHud(engine::render::Renderer& renderer);
    void drawLevelUpOverlay(engine::render::Renderer& renderer);
    void drawSummaryOverlay(engine::render::Renderer& renderer);

    int m_windowWidth;
    int m_windowHeight;

    std::unique_ptr<engine::ecs::Registry> m_registry; // unique_ptr so resetRun() can cheaply swap in a fresh one
    TierTable m_tiers;
    DungeonTable m_dungeons;
    PlayerState m_player;
    engine::render::SpriteClipSet m_playerClips;
    engine::render::SpriteAnimator m_playerAnim;
    // Shared by every spawned enemy's own SpriteAnimator component (see
    // spawnEnemyWave) - one set of GPU textures for the whole swarm,
    // not one per entity.
    engine::render::SpriteClipSet m_enemyClips;
    // The player's arrow projectile - a single static prop (not
    // directional/animated), loaded via AssetManager like any other
    // one-off texture, rotated per-shot at draw time to match its
    // Velocity instead of needing 16 baked facings.
    engine::render::Texture* m_arrowTexture = nullptr;
    std::mt19937 m_rng{std::random_device{}()};

    RunPhase m_phase = RunPhase::Playing;
    float m_runTimeSeconds = 0.0f;
    float m_spawnTimer = 0.0f;

    std::string m_summaryOutcome;
    int m_overallXpAwarded = 0;

    engine::render::TextRenderer m_text;
    bool m_textReady = false;
};

} // namespace game::swarm
