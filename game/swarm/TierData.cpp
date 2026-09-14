#include "TierData.h"

#include <nlohmann/json.hpp>

#include "engine/save/SaveFile.h"

namespace game::swarm {

namespace {

int tierIndexFromId(const std::string& id) {
    if (id == "normal") return static_cast<int>(EnemyTier::Normal);
    if (id == "elite") return static_cast<int>(EnemyTier::Elite);
    if (id == "epic") return static_cast<int>(EnemyTier::Epic);
    if (id == "legendary") return static_cast<int>(EnemyTier::Legendary);
    return -1;
}

} // namespace

void TierTable::applyDefaults() {
    m_tiers[static_cast<int>(EnemyTier::Normal)] = {
        "normal", 1.0f, 1.0f, 100.0f, 1, 1, 1.0f, SDL_Color{90, 200, 90, 255}};
    m_tiers[static_cast<int>(EnemyTier::Elite)] = {
        "elite", 4.0f, 1.5f, 12.0f, 4, 5, 1.3f, SDL_Color{90, 140, 230, 255}};
    m_tiers[static_cast<int>(EnemyTier::Epic)] = {
        "epic", 12.0f, 2.5f, 3.0f, 10, 15, 1.7f, SDL_Color{190, 90, 220, 255}};
    m_tiers[static_cast<int>(EnemyTier::Legendary)] = {
        "legendary", 40.0f, 4.0f, 0.5f, 25, 50, 2.2f, SDL_Color{230, 180, 60, 255}};
}

bool TierTable::loadFromFile(const std::string& path) {
    applyDefaults();

    nlohmann::json data;
    if (!engine::save::readJson(path, data) || !data.contains("tiers")) {
        return false;
    }

    for (const auto& entry : data.at("tiers")) {
        const std::string id = entry.value("id", "");
        const int index = tierIndexFromId(id);
        if (index < 0) {
            continue; // unknown tier id - skip rather than fail the whole load
        }

        TierDefinition& tier = m_tiers[index];
        tier.id = id;
        tier.hpMultiplier = entry.value("hpMultiplier", tier.hpMultiplier);
        tier.damageMultiplier = entry.value("damageMultiplier", tier.damageMultiplier);
        tier.spawnWeight = entry.value("spawnWeight", tier.spawnWeight);
        tier.orbXpValue = entry.value("orbXpValue", tier.orbXpValue);
        tier.overallXpValue = entry.value("overallXpValue", tier.overallXpValue);
        tier.scale = entry.value("scale", tier.scale);

        if (entry.contains("tint") && entry.at("tint").is_array() && entry.at("tint").size() >= 3) {
            const auto& tint = entry.at("tint");
            tier.tint = SDL_Color{
                static_cast<Uint8>(tint.at(0).get<int>()),
                static_cast<Uint8>(tint.at(1).get<int>()),
                static_cast<Uint8>(tint.at(2).get<int>()),
                255};
        }
    }

    return true;
}

EnemyTier TierTable::rollWeightedTier(std::mt19937& rng) const {
    float totalWeight = 0.0f;
    for (const TierDefinition& tier : m_tiers) {
        totalWeight += tier.spawnWeight;
    }

    std::uniform_real_distribution<float> dist(0.0f, totalWeight > 0.0f ? totalWeight : 1.0f);
    float roll = dist(rng);

    for (std::size_t i = 0; i < m_tiers.size(); ++i) {
        roll -= m_tiers[i].spawnWeight;
        if (roll <= 0.0f) {
            return static_cast<EnemyTier>(i);
        }
    }
    return EnemyTier::Normal; // float rounding fallback
}

} // namespace game::swarm
