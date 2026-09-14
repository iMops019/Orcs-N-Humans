#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <random>
#include <string>

#include "Components.h"

namespace game::swarm {

struct TierDefinition {
    std::string id;
    float hpMultiplier = 1.0f;
    float damageMultiplier = 1.0f;
    float spawnWeight = 1.0f;
    int orbXpValue = 1;     // in-run XP orb value on kill (resets every run)
    int overallXpValue = 1; // persistent Overall XP contribution on kill
    float scale = 1.0f;     // placeholder-art size multiplier - see Orcs N Humans Game Docs/01-VISION-AND-GOALS.md
    SDL_Color tint{255, 255, 255, 255};
};

// Data-driven per-tier stat/loot/XP multipliers, loaded from
// assets/data/enemy_tiers.json (falls back to the same starting-point
// numbers from Orcs N Humans Game Docs/01-VISION-AND-GOALS.md if the
// file is missing, so the game never fails to boot over a missing/bad
// data file). Not balanced - a tuning target once there's a playable
// loop, per that doc.
class TierTable {
public:
    // Always leaves every tier populated (falls back to built-in
    // defaults on any missing/malformed entry) - returns false only to
    // tell the caller the file itself couldn't be used, not that the
    // table is unusable.
    bool loadFromFile(const std::string& path);

    const TierDefinition& get(EnemyTier tier) const { return m_tiers[static_cast<std::size_t>(tier)]; }

    // Weighted-random tier pick for a newly spawned enemy, using each
    // tier's spawnWeight.
    EnemyTier rollWeightedTier(std::mt19937& rng) const;

private:
    void applyDefaults();

    std::array<TierDefinition, kTierCount> m_tiers;
};

} // namespace game::swarm
