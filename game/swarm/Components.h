#pragma once

namespace game::swarm {

enum class EnemyTier { Normal, Elite, Epic, Legendary, Count };

constexpr int kTierCount = static_cast<int>(EnemyTier::Count);

// Cast to int for engine::render::SpriteAnimator::loopState/onceState -
// shared between SwarmSystems.cpp (spawns/drives it) and
// SwarmArenaState.cpp (loads the clips, draws it), so it lives here
// rather than nested in either.
enum class EnemyAnimState { Idle, Walk, Attack };

// Game-specific ECS components (see engine/ecs/Components.h's own
// comment on why these live here, not in the engine) - every swarm
// enemy, projectile, and XP orb is a plain engine::ecs::Entity built
// from these plus the engine's own Transform/Velocity/CollisionRadius.

struct EnemyTag {
    EnemyTier tier = EnemyTier::Normal;
};

struct Health {
    float current = 0.0f;
    float max = 0.0f;
};

// Damage an enemy deals to the player on contact - see
// SwarmSystems::updateContactDamage. Not a general damage-over-time
// component; just this one interaction.
struct ContactDamage {
    float damage = 0.0f;
};

// A fired shot: travels in a straight line (its Velocity, set once at
// spawn) and deals `damage` to the first enemy it touches. Homing
// toward a stored target entity was deliberately skipped - the target
// could die (or the id could even be reused - Entity has no generation
// counter, see engine/ecs/Entity.h) before the shot arrives, and
// straight-line "fire and forget" shots are the norm for this genre
// anyway (Vampire Survivors' knife, this game's arrow).
struct Projectile {
    float damage = 0.0f;
    float traveledDistance = 0.0f; // despawn once this exceeds a max range - see kProjectileMaxRange
};

struct XpOrb {
    int value = 0;
};

} // namespace game::swarm
