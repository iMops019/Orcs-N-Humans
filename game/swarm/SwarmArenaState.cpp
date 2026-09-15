#include "SwarmArenaState.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "engine/assets/AssetManager.h"
#include "engine/ecs/Components.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/Systems.h"
#include "engine/input/InputMap.h"
#include "engine/physics/Vec2.h"
#include "engine/render/AnimationSystems.h"
#include "engine/render/Renderer.h"

#include "Boons.h"
#include "Components.h"
#include "SwarmSystems.h"

namespace game::swarm {

namespace {
constexpr float kRunClearSeconds = 180.0f; // survive this long -> "Cleared!" (see DECISIONS-LOG.md, Phase 1 scope)
constexpr float kInitialSpawnDelay = 1.0f;

const char* tierName(EnemyTier tier) {
    switch (tier) {
        case EnemyTier::Normal: return "Normal";
        case EnemyTier::Elite: return "Elite";
        case EnemyTier::Epic: return "Epic";
        case EnemyTier::Legendary: return "Legendary";
        default: return "?";
    }
}
} // namespace

SwarmArenaState::SwarmArenaState(int windowWidth, int windowHeight)
    : m_windowWidth(windowWidth), m_windowHeight(windowHeight) {}

SwarmArenaState::~SwarmArenaState() = default;

void SwarmArenaState::onEnter(engine::render::Renderer& renderer, engine::assets::AssetManager& assets) {
    // Character clips (SpriteClipSet::loadClip below) bypass AssetManager
    // entirely - see DirectionalAnimation's own header comment - but the
    // arrow prop is a single static texture, so it's the one thing here
    // that actually goes through it.
    m_arrowTexture = assets.loadTexture("assets/textures/props/arrow.png");

    m_textReady = m_text.init() && m_text.loadFont("C:\\Windows\\Fonts\\arial.ttf", 20);

    m_tiers.loadFromFile("assets/data/enemy_tiers.json");
    m_dungeons.loadFromFile("assets/data/dungeons.json");

    // Baked 16-facing frame sequences rendered directly from Meshy's
    // animated rig (see Orcs N Humans Game Docs/DECISIONS-LOG.md for why
    // this replaced an earlier 2D-cutout-rig curve-extraction attempt -
    // it broke down on any pose with real depth, like the bow draw).
    // Idle/Walk loop; Attack is a one-shot played over the loop.
    m_playerClips.loadClip(static_cast<int>(PlayerAnimState::Idle), "assets/textures/characters_animated/peasant_idle",
                            renderer, 12.0f, true);
    m_playerClips.loadClip(static_cast<int>(PlayerAnimState::Walk), "assets/textures/characters_animated/peasant_walk",
                            renderer, 16.0f, true);
    m_playerClips.loadClip(static_cast<int>(PlayerAnimState::Attack),
                            "assets/textures/characters_animated/peasant_bow_attack", renderer, 16.0f, false);
    m_playerAnim.clips = &m_playerClips;
    m_playerAnim.loopState = static_cast<int>(PlayerAnimState::Idle);
    m_playerAnim.fallbackState = static_cast<int>(PlayerAnimState::Idle);
    // Baked frames were originally rendered at 512px, cropped to
    // ~150-230px tall content, with 0.35 tuned to read sensibly against
    // the swarm's enemy/projectile sizes - then downscaled to 220px
    // source resolution to fix a slow-load hang (see DECISIONS-LOG.md),
    // which shrinks on-screen size by that same ratio unless compensated
    // for. 0.35 * (512/220) restores the original on-screen size:
    // eyeball-tuned once, still adjust freely.
    m_playerAnim.visualScale = 0.35f * (512.0f / 220.0f);

    // Same pipeline, goblin's own baked sequences - the enemy Attack
    // clip is whatever melee-reading pose was picked from Meshy's
    // animation list ("Shield Push Left"), triggered on each contact
    // hit rather than on a cooldown timer (see updateContactDamage's
    // caller in update()) since enemies don't have a discrete "swing"
    // action, only continuous contact damage.
    m_enemyClips.loadClip(static_cast<int>(EnemyAnimState::Idle), "assets/textures/characters_animated/goblin_idle",
                           renderer, 10.0f, true);
    m_enemyClips.loadClip(static_cast<int>(EnemyAnimState::Walk), "assets/textures/characters_animated/goblin_walk",
                           renderer, 14.0f, true);
    m_enemyClips.loadClip(static_cast<int>(EnemyAnimState::Attack), "assets/textures/characters_animated/goblin_attack",
                           renderer, 16.0f, false);

    resetRun();
}

void SwarmArenaState::resetRun() {
    m_registry = std::make_unique<engine::ecs::Registry>();

    m_player = PlayerState{};
    m_player.x = static_cast<float>(m_windowWidth) / 2.0f;
    m_player.y = static_cast<float>(m_windowHeight) / 2.0f;

    // Reset per-run playback state, but not m_playerAnim.clips/
    // fallbackState/visualScale - those are onEnter()-time config, not
    // per-run state.
    m_playerAnim.loopState = static_cast<int>(PlayerAnimState::Idle);
    m_playerAnim.loopTime = 0.0f;
    m_playerAnim.stopOnce();
    m_playerAnim.onceTime = 0.0f;
    m_playerAnim.facingX = 0.0f;
    m_playerAnim.facingY = 1.0f;

    m_runTimeSeconds = 0.0f;
    m_spawnTimer = kInitialSpawnDelay;
    m_phase = RunPhase::Playing;
    m_summaryOutcome.clear();
    m_overallXpAwarded = 0;
}

void SwarmArenaState::beginSummary(const std::string& outcome) {
    m_summaryOutcome = outcome;

    int xp = m_dungeons.first().baseXp;
    for (int i = 0; i < kTierCount; ++i) {
        const EnemyTier tier = static_cast<EnemyTier>(i);
        xp += m_player.killsByTier[i] * m_tiers.get(tier).overallXpValue;
    }
    m_overallXpAwarded = xp;

    m_phase = RunPhase::Summary;
}

void SwarmArenaState::applyBoonPick(int index) {
    const auto& boons = allBoons();
    if (index < 0 || static_cast<std::size_t>(index) >= boons.size()) {
        return;
    }
    boons[index].apply(m_player);

    m_player.orbXp -= m_player.orbXpToNextLevel;
    m_player.level += 1;
    m_player.orbXpToNextLevel = 20 + (m_player.level - 1) * 15;

    m_phase = RunPhase::Playing;
}

void SwarmArenaState::handleEvent(const SDL_Event& event) {
    if (event.type != SDL_EVENT_KEY_DOWN || event.key.repeat) {
        return;
    }

    if (m_phase == RunPhase::LevelUp) {
        switch (event.key.scancode) {
            case SDL_SCANCODE_1: applyBoonPick(0); break;
            case SDL_SCANCODE_2: applyBoonPick(1); break;
            case SDL_SCANCODE_3: applyBoonPick(2); break;
            case SDL_SCANCODE_4: applyBoonPick(3); break;
            default: break;
        }
        return;
    }

    if (m_phase == RunPhase::Summary) {
        if (event.key.scancode == SDL_SCANCODE_RETURN || event.key.scancode == SDL_SCANCODE_SPACE) {
            resetRun();
        }
        return;
    }

    // Playing - a debug stress-test burst (see 04-ROADMAP.md's "stress-
    // test toward ~1,000 concurrent enemies" item), not meant to survive
    // past prototyping.
    if (event.key.scancode == SDL_SCANCODE_F6) {
        systems::spawnEnemyWave(*m_registry, m_tiers, m_rng, 100, static_cast<float>(m_windowWidth),
                                 static_cast<float>(m_windowHeight), m_enemyClips);
    }
}

void SwarmArenaState::update(float deltaTime, const engine::input::InputMap& input) {
    if (m_phase != RunPhase::Playing) {
        return; // frozen during the level-up pause and the end-of-run summary
    }

    m_runTimeSeconds += deltaTime;

    engine::physics::Vec2 move{0.0f, 0.0f};
    if (input.isHeld("MoveUp")) move.y -= 1.0f;
    if (input.isHeld("MoveDown")) move.y += 1.0f;
    if (input.isHeld("MoveLeft")) move.x -= 1.0f;
    if (input.isHeld("MoveRight")) move.x += 1.0f;
    move = normalized(move);
    m_player.x += move.x * m_player.moveSpeed * deltaTime;
    m_player.y += move.y * m_player.moveSpeed * deltaTime;
    m_player.x = std::clamp(m_player.x, systems::kPlayerRadius, static_cast<float>(m_windowWidth) - systems::kPlayerRadius);
    m_player.y = std::clamp(m_player.y, systems::kPlayerRadius, static_cast<float>(m_windowHeight) - systems::kPlayerRadius);

    // Facing only updates while actually moving - holding still keeps
    // whatever direction was last faced, matching
    // DirectionalAnimation::currentFrame()'s screen-space (dx,dy)
    // convention (see its own header comment).
    const bool playerMoving = move.x != 0.0f || move.y != 0.0f;
    if (playerMoving) {
        m_playerAnim.facingX = move.x;
        m_playerAnim.facingY = move.y;
    }
    m_playerAnim.loopState = static_cast<int>(playerMoving ? PlayerAnimState::Walk : PlayerAnimState::Idle);

    systems::updateEnemySeek(*m_registry, m_player);
    const systems::AttackUpdateResult attack = systems::updatePlayerAttack(*m_registry, m_player, deltaTime);
    // A target in range overrides movement facing (Halls of Torment-
    // style: face what you're shooting at, not necessarily where
    // you're walking) - reported every frame a target exists, not just
    // the frame a shot fires, so facing tracks the target continuously
    // rather than snapping only at each 0.6s shot. Deliberately NOT
    // gated on "not mid-attack": this game's attack auto-fires far more
    // often than the ~2s Attack clip takes to play out, so playingOnce()
    // is true almost continuously once the swarm picks up - gating
    // facing on it froze facing for the rest of the run the first time
    // combat started. The Attack clip has all 16 facings rendered too,
    // so there's no visual cost to keeping facing live while it plays.
    if (attack.hasTarget) {
        m_playerAnim.facingX = attack.targetDirection.x;
        m_playerAnim.facingY = attack.targetDirection.y;
    }
    if (attack.fired) {
        m_playerAnim.playOnce(static_cast<int>(PlayerAnimState::Attack));
    }
    systems::updateXpOrbs(*m_registry, m_player, deltaTime); // sets magnet velocity - before applyVelocity moves it

    // No ECS registry entry for the player, so advanceSpriteAnimators()
    // (which ticks every SpriteAnimator IN a Registry) doesn't reach
    // m_playerAnim - tick its two clocks by hand instead.
    m_playerAnim.loopTime += deltaTime;
    if (m_playerAnim.playingOnce()) {
        m_playerAnim.onceTime += deltaTime;
        if (m_playerAnim.onceFinished()) {
            m_playerAnim.stopOnce();
        }
    }

    engine::ecs::systems::applyVelocity(*m_registry, deltaTime);
    engine::ecs::systems::resolveCircleCollisions(*m_registry); // jostles overlapping enemies apart

    systems::updateProjectiles(*m_registry, m_player, m_tiers, deltaTime); // reads post-move positions
    const engine::ecs::Entity attacker =
        systems::updateContactDamage(*m_registry, m_player, deltaTime); // reads post-move positions
    auto& enemyAnimators = m_registry->poolFor<engine::render::SpriteAnimator>();
    if (attacker != engine::ecs::kInvalidEntity && enemyAnimators.has(attacker)) {
        enemyAnimators.get(attacker).playOnce(static_cast<int>(EnemyAnimState::Attack));
    }

    // Ticks loopTime/onceTime for every enemy's SpriteAnimator (every
    // enemy always has one - see spawnEnemyWave). Doesn't auto-clear a
    // finished one-shot (see AnimationSystems.h/SpriteAnimator.h's own
    // comments on why the caller owns that) - the loop right after does.
    // m_playerAnim isn't in the registry (see resetRun()'s comment), so
    // it's ticked by hand separately, above.
    engine::render::systems::advanceSpriteAnimators(*m_registry, deltaTime);
    for (engine::render::SpriteAnimator& animator : enemyAnimators.components()) {
        if (animator.playingOnce() && animator.onceFinished()) {
            animator.stopOnce();
        }
    }

    m_spawnTimer -= deltaTime;
    if (m_spawnTimer <= 0.0f) {
        const int waveSize = 1 + static_cast<int>(m_runTimeSeconds / 12.0f);
        systems::spawnEnemyWave(*m_registry, m_tiers, m_rng, waveSize, static_cast<float>(m_windowWidth),
                                 static_cast<float>(m_windowHeight), m_enemyClips);
        m_spawnTimer = std::max(0.25f, 1.1f - m_runTimeSeconds * 0.004f);
    }

    if (m_player.orbXp >= m_player.orbXpToNextLevel) {
        m_phase = RunPhase::LevelUp;
        return;
    }

    if (m_player.hp <= 0.0f) {
        beginSummary("You Died");
    } else if (m_runTimeSeconds >= kRunClearSeconds) {
        beginSummary("Cleared!");
    }
}

void SwarmArenaState::render(engine::render::Renderer& renderer) {
    renderer.fillRect(0, 0, m_windowWidth, m_windowHeight, 24, 20, 28, 255);

    auto& transforms = m_registry->poolFor<engine::ecs::Transform>();

    for (const engine::ecs::Entity orb : m_registry->poolFor<XpOrb>().entities()) {
        const auto& t = transforms.get(orb);
        renderer.fillRect(static_cast<int>(t.x) - 5, static_cast<int>(t.y) - 5, 10, 10, 235, 220, 90, 255);
    }

    auto& velocities = m_registry->poolFor<engine::ecs::Velocity>();
    for (const engine::ecs::Entity shot : m_registry->poolFor<Projectile>().entities()) {
        const auto& t = transforms.get(shot);
        if (m_arrowTexture != nullptr) {
            // The arrow art's own rest pose already points along
            // screen-right (angle 0) - see assets/textures/props/arrow.png's
            // own render notes - so the shot's raw Velocity angle IS the
            // rotation needed, no base-angle offset to subtract.
            const engine::ecs::Velocity& v = velocities.get(shot);
            const float angleDegrees = std::atan2(v.y, v.x) * (180.0f / 3.14159265f);
            constexpr float kArrowLength = 34.0f;
            constexpr float kArrowThickness = 7.0f;
            renderer.drawTextureRotated(m_arrowTexture, t.x - kArrowLength / 2.0f, t.y - kArrowThickness / 2.0f,
                                         static_cast<int>(kArrowLength), static_cast<int>(kArrowThickness), angleDegrees,
                                         engine::render::Camera{});
        } else {
            renderer.fillRect(static_cast<int>(t.x) - 3, static_cast<int>(t.y) - 3, 6, 6, 235, 235, 235, 255);
        }
    }

    auto& enemyAnimators = m_registry->poolFor<engine::render::SpriteAnimator>();
    for (const engine::ecs::Entity enemy : m_registry->poolFor<EnemyTag>().entities()) {
        const auto& t = transforms.get(enemy);
        const EnemyTier tier = m_registry->getComponent<EnemyTag>(enemy).tier;
        const TierDefinition& def = m_tiers.get(tier);

        const engine::render::SpriteAnimator& animator = enemyAnimators.get(enemy);
        const engine::render::ResolvedSpriteFrame frame = engine::render::resolveSpriteFrame(animator);
        engine::render::Texture* texture =
            frame.clip != nullptr ? frame.clip->currentFrame(animator.facingX, animator.facingY, frame.sampleTime)
                                   : nullptr;
        if (texture != nullptr) {
            int contentW = 0, contentH = 0;
            frame.clip->contentSize(contentW, contentH);
            const float drawW = static_cast<float>(contentW) * animator.visualScale;
            const float drawH = static_cast<float>(contentH) * animator.visualScale;
            renderer.drawTexture(texture, t.x - drawW / 2.0f, t.y - drawH / 2.0f, static_cast<int>(drawW),
                                  static_cast<int>(drawH), engine::render::Camera{}, def.tint.r, def.tint.g, def.tint.b,
                                  255);
        } else {
            // Art failed to load - fall back to the original placeholder rect.
            const int half = static_cast<int>(systems::kEnemyBaseRadius * def.scale);
            renderer.fillRect(static_cast<int>(t.x) - half, static_cast<int>(t.y) - half, half * 2, half * 2, def.tint.r,
                               def.tint.g, def.tint.b, 255);
        }
    }

    const bool flash = m_player.invulnTimer > 0.0f && std::fmod(m_player.invulnTimer, 0.2f) > 0.1f;
    const engine::render::ResolvedSpriteFrame playerFrame = engine::render::resolveSpriteFrame(m_playerAnim);
    engine::render::Texture* playerTexture =
        playerFrame.clip != nullptr ? playerFrame.clip->currentFrame(m_playerAnim.facingX, m_playerAnim.facingY,
                                                                       playerFrame.sampleTime)
                                     : nullptr;
    if (playerTexture != nullptr) {
        int contentW = 0, contentH = 0;
        playerFrame.clip->contentSize(contentW, contentH);
        const float drawW = static_cast<float>(contentW) * m_playerAnim.visualScale;
        const float drawH = static_cast<float>(contentH) * m_playerAnim.visualScale;
        const Uint8 tintR = flash ? 235 : 255;
        const Uint8 tintG = flash ? 90 : 255;
        const Uint8 tintB = flash ? 90 : 255;
        renderer.drawTexture(playerTexture, m_player.x - drawW / 2.0f, m_player.y - drawH / 2.0f,
                              static_cast<int>(drawW), static_cast<int>(drawH), engine::render::Camera{}, tintR, tintG,
                              tintB, 255);
    } else {
        // Art failed to load (missing folder, bad build) - fall back to
        // the original placeholder rect rather than drawing nothing.
        const int playerHalf = static_cast<int>(systems::kPlayerRadius);
        renderer.fillRect(static_cast<int>(m_player.x) - playerHalf, static_cast<int>(m_player.y) - playerHalf,
                           playerHalf * 2, playerHalf * 2, flash ? 235 : 90, flash ? 90 : 170, flash ? 90 : 230, 255);
    }

    drawHud(renderer);

    if (m_phase == RunPhase::LevelUp) {
        drawLevelUpOverlay(renderer);
    } else if (m_phase == RunPhase::Summary) {
        drawSummaryOverlay(renderer);
    }
}

void SwarmArenaState::drawHud(engine::render::Renderer& renderer) {
    if (!m_textReady) {
        return;
    }

    constexpr SDL_Color kWhite{235, 235, 240, 255};
    constexpr SDL_Color kDim{170, 170, 180, 255};

    // HP bar
    const int barX = 16;
    const int barY = 16;
    const int barW = 220;
    const int barH = 18;
    renderer.fillRect(barX, barY, barW, barH, 60, 30, 30, 255);
    const float hpFrac = m_player.maxHp > 0.0f ? std::clamp(m_player.hp / m_player.maxHp, 0.0f, 1.0f) : 0.0f;
    renderer.fillRect(barX, barY, static_cast<int>(barW * hpFrac), barH, 210, 70, 70, 255);

    char buf[128];
    std::snprintf(buf, sizeof(buf), "HP %d / %d", static_cast<int>(std::max(0.0f, m_player.hp)),
                  static_cast<int>(m_player.maxHp));
    m_text.draw(renderer, buf, static_cast<float>(barX + 6), static_cast<float>(barY + 1), kWhite);

    // XP bar
    const int xpBarY = barY + barH + 6;
    renderer.fillRect(barX, xpBarY, barW, barH, 40, 40, 55, 255);
    const float xpFrac = m_player.orbXpToNextLevel > 0
                              ? std::clamp(static_cast<float>(m_player.orbXp) / static_cast<float>(m_player.orbXpToNextLevel), 0.0f, 1.0f)
                              : 0.0f;
    renderer.fillRect(barX, xpBarY, static_cast<int>(barW * xpFrac), barH, 90, 140, 230, 255);
    std::snprintf(buf, sizeof(buf), "Lv %d  (%d / %d xp)", m_player.level, m_player.orbXp, m_player.orbXpToNextLevel);
    m_text.draw(renderer, buf, static_cast<float>(barX + 6), static_cast<float>(xpBarY + 1), kWhite);

    // Run timer + kill counts, top-right
    std::snprintf(buf, sizeof(buf), "%d:%02d / %d:00", static_cast<int>(m_runTimeSeconds) / 60,
                  static_cast<int>(m_runTimeSeconds) % 60, static_cast<int>(kRunClearSeconds) / 60);
    m_text.draw(renderer, buf, static_cast<float>(m_windowWidth - 160), 16.0f, kWhite);

    std::snprintf(buf, sizeof(buf), "N:%d  E:%d  Ep:%d  L:%d", m_player.killsByTier[0], m_player.killsByTier[1],
                  m_player.killsByTier[2], m_player.killsByTier[3]);
    m_text.draw(renderer, buf, static_cast<float>(m_windowWidth - 220), 44.0f, kDim);

    m_text.draw(renderer, "WASD move - F6 spawn burst - F3 perf", 16.0f,
                static_cast<float>(m_windowHeight - 28), kDim, 0.8f);
}

void SwarmArenaState::drawLevelUpOverlay(engine::render::Renderer& renderer) {
    renderer.fillRect(0, 0, m_windowWidth, m_windowHeight, 10, 10, 15, 170);

    if (!m_textReady) {
        return;
    }

    constexpr SDL_Color kWhite{235, 235, 240, 255};
    constexpr SDL_Color kDim{180, 180, 190, 255};

    const std::string title = "LEVEL UP - choose one";
    m_text.draw(renderer, title, static_cast<float>(m_windowWidth) / 2.0f - 110.0f,
                static_cast<float>(m_windowHeight) / 2.0f - 160.0f, kWhite, 1.3f);

    const auto& boons = allBoons();
    const int cardW = 260;
    const int cardH = 120;
    const int gap = 24;
    const int totalW = static_cast<int>(boons.size()) * cardW + (static_cast<int>(boons.size()) - 1) * gap;
    int x = m_windowWidth / 2 - totalW / 2;
    const int y = m_windowHeight / 2 - cardH / 2;

    for (std::size_t i = 0; i < boons.size(); ++i) {
        renderer.fillRect(x, y, cardW, cardH, 45, 45, 60, 255);
        char header[32];
        std::snprintf(header, sizeof(header), "[%zu]", i + 1);
        m_text.draw(renderer, header, static_cast<float>(x + 10), static_cast<float>(y + 10), kDim);
        m_text.draw(renderer, boons[i].name, static_cast<float>(x + 10), static_cast<float>(y + 36), kWhite);
        m_text.draw(renderer, boons[i].description, static_cast<float>(x + 10), static_cast<float>(y + 66), kDim, 0.85f);
        x += cardW + gap;
    }
}

void SwarmArenaState::drawSummaryOverlay(engine::render::Renderer& renderer) {
    renderer.fillRect(0, 0, m_windowWidth, m_windowHeight, 10, 10, 15, 200);

    if (!m_textReady) {
        return;
    }

    constexpr SDL_Color kWhite{235, 235, 240, 255};
    constexpr SDL_Color kGold{230, 190, 90, 255};

    const float centerX = static_cast<float>(m_windowWidth) / 2.0f - 140.0f;
    float y = static_cast<float>(m_windowHeight) / 2.0f - 140.0f;

    m_text.draw(renderer, m_summaryOutcome, centerX, y, kWhite, 1.5f);
    y += 48.0f;

    char buf[128];
    for (int i = 0; i < kTierCount; ++i) {
        const EnemyTier tier = static_cast<EnemyTier>(i);
        std::snprintf(buf, sizeof(buf), "%s kills: %d", tierName(tier), m_player.killsByTier[i]);
        m_text.draw(renderer, buf, centerX, y, kWhite);
        y += 26.0f;
    }

    y += 10.0f;
    std::snprintf(buf, sizeof(buf), "Overall XP awarded: %d", m_overallXpAwarded);
    m_text.draw(renderer, buf, centerX, y, kGold);
    y += 40.0f;

    m_text.draw(renderer, "Press Enter to run again", centerX, y, kWhite, 0.9f);
}

} // namespace game::swarm
