#pragma once

#include <random>

#include "engine/ecs/Entity.h"
#include "engine/physics/Vec2.h"

#include "PlayerState.h"
#include "TierData.h"

namespace engine::ecs {
class Registry;
}

namespace engine::render {
class SpriteClipSet;
}

namespace game::swarm::systems {

// Base (Normal-tier) goblin stats before a tier's multipliers are
// applied - see TierDefinition. Placeholder numbers (see
// Orcs N Humans Game Docs/DECISIONS-LOG.md); tunable once there's a
// playable loop.
constexpr float kEnemyBaseHp = 10.0f;
constexpr float kEnemyBaseDamage = 5.0f;
constexpr float kEnemyMoveSpeed = 90.0f; // px/s, same for every tier for now
constexpr float kEnemyBaseRadius = 12.0f;

constexpr float kProjectileSpeed = 520.0f; // px/s
constexpr float kProjectileRadius = 5.0f;
constexpr float kProjectileMaxRange = 900.0f;

constexpr float kPlayerRadius = 16.0f;
constexpr float kContactInvulnSeconds = 0.6f;

constexpr float kOrbPickupRadius = 26.0f;
constexpr float kOrbMagnetRadius = 150.0f;
constexpr float kOrbMagnetSpeed = 420.0f;

// Spawns `count` enemies just outside [0,arenaWidth]x[0,arenaHeight],
// each on a random point around the arena's perimeter, tier rolled via
// `tiers`. They start with zero velocity - updateEnemySeek gives them
// one on the next tick. Each gets an engine::render::SpriteAnimator
// component pointed at `enemyClips` (loopState=Idle, visualScale baked
// in from the tier's own scale multiplier) - the caller owns loading
// enemyClips (see EnemyAnimState) and ticking every spawned entity's
// animator (engine::render::systems::advanceSpriteAnimators).
void spawnEnemyWave(engine::ecs::Registry& registry, const TierTable& tiers, std::mt19937& rng, int count,
                     float arenaWidth, float arenaHeight, const engine::render::SpriteClipSet& enemyClips);

// Every enemy's Velocity is set to move straight toward the player at
// kEnemyMoveSpeed - no pathfinding needed, this is an open arena with
// no obstacles (see 03-TECH-ARCHITECTURE.md). Caller still has to run
// engine::ecs::systems::applyVelocity (and, for crowd separation,
// resolveCircleCollisions) afterward - this only sets direction. Also
// drives each enemy's SpriteAnimator facing/loopState - they always
// face their seek direction (no separate "idle, not seeking" state
// exists for an enemy that never stops chasing).
void updateEnemySeek(engine::ecs::Registry& registry, const PlayerState& player);

// What updatePlayerAttack found/did this frame. hasTarget/targetDirection
// are reported whenever an enemy is in attackRange, independent of
// whether the cooldown actually let a shot fire this frame - callers
// use this to keep the player continuously facing its target (Halls of
// Torment-style) rather than only snapping to face it on the exact
// frame a shot fires.
struct AttackUpdateResult {
    bool fired = false;
    bool hasTarget = false;
    engine::physics::Vec2 targetDirection{0.0f, 0.0f};
};

// Ticks the player's attack cooldown and, when ready, fires a straight-
// line Projectile at the nearest enemy within attackRange.
AttackUpdateResult updatePlayerAttack(engine::ecs::Registry& registry, PlayerState& player, float dt);

// Call AFTER engine::ecs::systems::applyVelocity has moved everything
// this frame. Resolves hits at the projectiles' new positions: damages
// the first enemy touched, destroys the projectile, and - if that kill
// drops the enemy to 0 HP - increments player.killsByTier and spawns
// an XpOrb in its place. Also despawns any projectile past
// kProjectileMaxRange (dt is only used for that distance accounting).
void updateProjectiles(engine::ecs::Registry& registry, PlayerState& player, const TierTable& tiers, float dt);

// Call AFTER applyVelocity. Damages the player when an enemy's
// collision circle touches theirs, respecting the post-hit
// invulnerability window. Also ticks that window down by dt. Returns
// the enemy entity that landed the hit this frame, or kInvalidEntity if
// none did - callers use this to trigger that enemy's Attack one-shot.
engine::ecs::Entity updateContactDamage(engine::ecs::Registry& registry, PlayerState& player, float dt);

// Call BEFORE applyVelocity - sets Velocity on any orb within magnet
// range so applyVelocity actually moves it toward the player this
// frame. Also immediately collects (destroys + credits player.orbXp)
// any orb already within pickup range, regardless of movement.
void updateXpOrbs(engine::ecs::Registry& registry, PlayerState& player, float dt);

} // namespace game::swarm::systems
