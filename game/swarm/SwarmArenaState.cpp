#include "SwarmArenaState.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "engine/ecs/Components.h"
#include "engine/ecs/Registry.h"
#include "engine/ecs/Systems.h"
#include "engine/input/InputMap.h"
#include "engine/physics/Vec2.h"
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
    (void)assets; // no textures yet - Phase 1 draws placeholder rects only, see 01-VISION-AND-GOALS.md

    m_textReady = m_text.init() && m_text.loadFont("C:\\Windows\\Fonts\\arial.ttf", 20);

    m_tiers.loadFromFile("assets/data/enemy_tiers.json");
    m_dungeons.loadFromFile("assets/data/dungeons.json");

    resetRun();
}

void SwarmArenaState::resetRun() {
    m_registry = std::make_unique<engine::ecs::Registry>();

    m_player = PlayerState{};
    m_player.x = static_cast<float>(m_windowWidth) / 2.0f;
    m_player.y = static_cast<float>(m_windowHeight) / 2.0f;

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
                                 static_cast<float>(m_windowHeight));
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

    systems::updateEnemySeek(*m_registry, m_player);
    systems::updatePlayerAttack(*m_registry, m_player, deltaTime);
    systems::updateXpOrbs(*m_registry, m_player, deltaTime); // sets magnet velocity - before applyVelocity moves it

    engine::ecs::systems::applyVelocity(*m_registry, deltaTime);
    engine::ecs::systems::resolveCircleCollisions(*m_registry); // jostles overlapping enemies apart

    systems::updateProjectiles(*m_registry, m_player, m_tiers, deltaTime); // reads post-move positions
    systems::updateContactDamage(*m_registry, m_player, deltaTime);        // reads post-move positions

    m_spawnTimer -= deltaTime;
    if (m_spawnTimer <= 0.0f) {
        const int waveSize = 1 + static_cast<int>(m_runTimeSeconds / 12.0f);
        systems::spawnEnemyWave(*m_registry, m_tiers, m_rng, waveSize, static_cast<float>(m_windowWidth),
                                 static_cast<float>(m_windowHeight));
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

    for (const engine::ecs::Entity shot : m_registry->poolFor<Projectile>().entities()) {
        const auto& t = transforms.get(shot);
        renderer.fillRect(static_cast<int>(t.x) - 3, static_cast<int>(t.y) - 3, 6, 6, 235, 235, 235, 255);
    }

    for (const engine::ecs::Entity enemy : m_registry->poolFor<EnemyTag>().entities()) {
        const auto& t = transforms.get(enemy);
        const EnemyTier tier = m_registry->getComponent<EnemyTag>(enemy).tier;
        const TierDefinition& def = m_tiers.get(tier);
        const int half = static_cast<int>(systems::kEnemyBaseRadius * def.scale);
        renderer.fillRect(static_cast<int>(t.x) - half, static_cast<int>(t.y) - half, half * 2, half * 2, def.tint.r,
                           def.tint.g, def.tint.b, 255);
    }

    const int playerHalf = static_cast<int>(systems::kPlayerRadius);
    const bool flash = m_player.invulnTimer > 0.0f && std::fmod(m_player.invulnTimer, 0.2f) > 0.1f;
    if (flash) {
        renderer.fillRect(static_cast<int>(m_player.x) - playerHalf, static_cast<int>(m_player.y) - playerHalf,
                           playerHalf * 2, playerHalf * 2, 235, 90, 90, 255);
    } else {
        renderer.fillRect(static_cast<int>(m_player.x) - playerHalf, static_cast<int>(m_player.y) - playerHalf,
                           playerHalf * 2, playerHalf * 2, 90, 170, 230, 255);
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
