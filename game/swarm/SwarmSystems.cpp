#include "SwarmSystems.h"

#include <algorithm>
#include <vector>

#include "engine/ecs/Components.h"
#include "engine/ecs/Registry.h"
#include "engine/physics/Vec2.h"

namespace game::swarm::systems {

using engine::ecs::CollisionRadius;
using engine::ecs::Entity;
using engine::ecs::Registry;
using engine::ecs::Transform;
using engine::ecs::Velocity;
using engine::physics::Vec2;

void spawnEnemyWave(Registry& registry, const TierTable& tiers, std::mt19937& rng, int count, float arenaWidth,
                     float arenaHeight) {
    std::uniform_real_distribution<float> perimeterDist(0.0f, 1.0f);
    constexpr float kSpawnMargin = 48.0f; // spawn just outside the visible arena, not right at the edge

    for (int i = 0; i < count; ++i) {
        // A point on the perimeter of a rectangle expanded by the
        // margin, picked by walking `t` around its total border length -
        // keeps spawns evenly spread across all four sides rather than
        // clustering on whichever side a naive pick favors.
        const float w = arenaWidth + kSpawnMargin * 2.0f;
        const float h = arenaHeight + kSpawnMargin * 2.0f;
        const float perimeter = 2.0f * (w + h);
        float t = perimeterDist(rng) * perimeter;

        float x = 0.0f;
        float y = 0.0f;
        if (t < w) {
            x = t - kSpawnMargin;
            y = -kSpawnMargin;
        } else if (t < w + h) {
            t -= w;
            x = arenaWidth + kSpawnMargin;
            y = t - kSpawnMargin;
        } else if (t < w + h + w) {
            t -= (w + h);
            x = arenaWidth - t + kSpawnMargin;
            y = arenaHeight + kSpawnMargin;
        } else {
            t -= (w + h + w);
            x = -kSpawnMargin;
            y = arenaHeight - t + kSpawnMargin;
        }

        const EnemyTier tier = tiers.rollWeightedTier(rng);
        const TierDefinition& def = tiers.get(tier);

        const Entity enemy = registry.createEntity();
        registry.addComponent(enemy, Transform{x, y});
        registry.addComponent(enemy, Velocity{0.0f, 0.0f});
        registry.addComponent(enemy, CollisionRadius{kEnemyBaseRadius * def.scale});
        registry.addComponent(enemy, Health{kEnemyBaseHp * def.hpMultiplier, kEnemyBaseHp * def.hpMultiplier});
        registry.addComponent(enemy, EnemyTag{tier});
        registry.addComponent(enemy, ContactDamage{kEnemyBaseDamage * def.damageMultiplier});
    }
}

void updateEnemySeek(Registry& registry, const PlayerState& player) {
    auto& transforms = registry.poolFor<Transform>();
    auto& velocities = registry.poolFor<Velocity>();
    auto& tags = registry.poolFor<EnemyTag>();

    for (const Entity entity : tags.entities()) {
        const Transform& t = transforms.get(entity);
        const Vec2 toPlayer = normalized(Vec2{player.x, player.y} - Vec2{t.x, t.y});
        Velocity& v = velocities.get(entity);
        v.x = toPlayer.x * kEnemyMoveSpeed;
        v.y = toPlayer.y * kEnemyMoveSpeed;
    }
}

void updatePlayerAttack(Registry& registry, PlayerState& player, float dt) {
    player.attackTimer -= dt;
    if (player.attackTimer > 0.0f) {
        return;
    }

    auto& transforms = registry.poolFor<Transform>();
    auto& tags = registry.poolFor<EnemyTag>();

    Entity nearest = engine::ecs::kInvalidEntity;
    float nearestDistSq = player.attackRange * player.attackRange;
    for (const Entity entity : tags.entities()) {
        const Transform& t = transforms.get(entity);
        const float distSq = distanceSquared(Vec2{player.x, player.y}, Vec2{t.x, t.y});
        if (distSq <= nearestDistSq) {
            nearestDistSq = distSq;
            nearest = entity;
        }
    }

    if (nearest == engine::ecs::kInvalidEntity) {
        return; // nothing in range - keep the cooldown at 0 and try again next frame
    }

    const Transform& targetTransform = transforms.get(nearest);
    const Vec2 direction = normalized(Vec2{targetTransform.x, targetTransform.y} - Vec2{player.x, player.y});

    const Entity shot = registry.createEntity();
    registry.addComponent(shot, Transform{player.x, player.y});
    registry.addComponent(shot, Velocity{direction.x * kProjectileSpeed, direction.y * kProjectileSpeed});
    registry.addComponent(shot, Projectile{player.attackDamage, 0.0f});

    player.attackTimer = player.attackCooldown;
}

void updateProjectiles(Registry& registry, PlayerState& player, const TierTable& tiers, float dt) {
    auto& projectiles = registry.poolFor<Projectile>();
    auto& transforms = registry.poolFor<Transform>();
    auto& enemyTags = registry.poolFor<EnemyTag>();
    auto& radii = registry.poolFor<CollisionRadius>();
    auto& healths = registry.poolFor<Health>();

    std::vector<Entity> projectilesToDestroy;
    std::vector<Entity> enemiesToKill;

    // Snapshot both dense lists up front - killing an enemy swap-removes
    // it from enemyTags' pool (see ComponentPool::remove()), which would
    // otherwise skip or repeat entries if taken mid-loop. Enemies are
    // only actually destroyed after this whole function is done
    // iterating (the enemiesToKill pass below), so these snapshots stay
    // valid to read from throughout.
    const std::vector<Entity> projectileEntities = projectiles.entities();
    const std::vector<Entity> enemyEntities = enemyTags.entities();

    for (const Entity shot : projectileEntities) {
        Projectile& proj = projectiles.get(shot);
        const Transform& shotTransform = transforms.get(shot);
        proj.traveledDistance += kProjectileSpeed * dt;

        bool hit = false;
        for (const Entity enemy : enemyEntities) {
            Health& health = healths.get(enemy);
            if (health.current <= 0.0f) {
                continue; // already killed by another shot this frame, pending removal below
            }

            const Transform& enemyTransform = transforms.get(enemy);
            const float radius = kProjectileRadius + radii.get(enemy).radius;
            if (distanceSquared(Vec2{shotTransform.x, shotTransform.y}, Vec2{enemyTransform.x, enemyTransform.y}) >
                radius * radius) {
                continue;
            }

            health.current -= proj.damage;
            hit = true;
            if (health.current <= 0.0f) {
                enemiesToKill.push_back(enemy);
            }
            break; // one enemy per shot - no piercing yet
        }

        if (hit || proj.traveledDistance >= kProjectileMaxRange) {
            projectilesToDestroy.push_back(shot);
        }
    }

    for (const Entity enemy : enemiesToKill) {
        const EnemyTier tier = enemyTags.get(enemy).tier;
        const TierDefinition& def = tiers.get(tier);
        player.killsByTier[static_cast<std::size_t>(tier)] += 1;

        const Transform& deathPos = transforms.get(enemy);
        const Entity orb = registry.createEntity();
        registry.addComponent(orb, Transform{deathPos.x, deathPos.y});
        registry.addComponent(orb, Velocity{0.0f, 0.0f});
        registry.addComponent(orb, XpOrb{def.orbXpValue});

        registry.destroyEntity(enemy);
    }

    for (const Entity shot : projectilesToDestroy) {
        registry.destroyEntity(shot);
    }
}

void updateContactDamage(Registry& registry, PlayerState& player, float dt) {
    player.invulnTimer = std::max(0.0f, player.invulnTimer - dt);
    if (player.invulnTimer > 0.0f) {
        return;
    }

    auto& transforms = registry.poolFor<Transform>();
    auto& contactDamages = registry.poolFor<ContactDamage>();
    auto& radii = registry.poolFor<CollisionRadius>();

    for (const Entity enemy : contactDamages.entities()) {
        const Transform& t = transforms.get(enemy);
        const float radius = kPlayerRadius + radii.get(enemy).radius;
        if (distanceSquared(Vec2{player.x, player.y}, Vec2{t.x, t.y}) > radius * radius) {
            continue;
        }

        player.hp -= contactDamages.get(enemy).damage;
        player.invulnTimer = kContactInvulnSeconds;
        break; // one hit per invulnerability window, regardless of how many enemies are touching
    }
}

void updateXpOrbs(Registry& registry, PlayerState& player, float dt) {
    (void)dt; // orb motion is a snap-to-velocity magnet, not itself time-integrated here - applyVelocity handles that

    auto& transforms = registry.poolFor<Transform>();
    auto& velocities = registry.poolFor<Velocity>();
    auto& orbs = registry.poolFor<XpOrb>();

    std::vector<Entity> collected;
    for (const Entity orb : orbs.entities()) {
        const Transform& t = transforms.get(orb);
        const float distSq = distanceSquared(Vec2{player.x, player.y}, Vec2{t.x, t.y});

        if (distSq <= kOrbPickupRadius * kOrbPickupRadius) {
            player.orbXp += orbs.get(orb).value;
            collected.push_back(orb);
            continue;
        }

        Velocity& v = velocities.get(orb);
        if (distSq <= kOrbMagnetRadius * kOrbMagnetRadius) {
            const Vec2 toPlayer = normalized(Vec2{player.x, player.y} - Vec2{t.x, t.y});
            v.x = toPlayer.x * kOrbMagnetSpeed;
            v.y = toPlayer.y * kOrbMagnetSpeed;
        } else {
            v.x = 0.0f;
            v.y = 0.0f;
        }
    }

    for (const Entity orb : collected) {
        registry.destroyEntity(orb);
    }
}

} // namespace game::swarm::systems
