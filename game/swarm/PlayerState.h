#pragma once

#include <array>

#include "Components.h"

namespace game::swarm {

// The player is plain game state, not an ECS entity - it's a
// singleton with no need for component-pool storage, and every system
// below already takes it as an explicit parameter alongside the
// Registry (see SwarmSystems.h). Enemies/projectiles/orbs are the
// things that actually benefit from ECS (many, homogeneous, iterated
// in bulk).
struct PlayerState {
    float x = 0.0f;
    float y = 0.0f;

    float hp = 100.0f;
    float maxHp = 100.0f;
    float moveSpeed = 220.0f; // px/s

    float attackDamage = 8.0f;
    float attackCooldown = 0.6f; // seconds between shots
    float attackRange = 260.0f;  // px
    float attackTimer = 0.0f;    // counts down to 0, then a shot is fired

    float invulnTimer = 0.0f; // brief i-frames after taking contact damage

    // In-run progression (Orb XP) - resets every run. See
    // 01-VISION-AND-GOALS.md's "Two separate XP economies".
    int level = 1;
    int orbXp = 0;
    int orbXpToNextLevel = 20; // matches the 20 + (level-1)*15 curve at level 1 - see SwarmArenaState::applyBoonPick

    // Per-tier kill counts this run, for the end-of-run Overall XP
    // formula (dungeonBaseXp + sum(killCount[tier] * overallXpValue[tier])).
    std::array<int, kTierCount> killsByTier{};
};

} // namespace game::swarm
