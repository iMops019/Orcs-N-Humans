#pragma once

#include <string>
#include <vector>

namespace game::swarm {

struct DungeonDefinition {
    std::string id;
    std::string displayName;
    int baseXp = 100; // flat Overall XP for clearing this dungeon - see 01-VISION-AND-GOALS.md's formula
};

// Loaded from assets/data/dungeons.json. Only one dungeon and no
// difficulty tiers yet (that's a Phase 2 dungeon-select concern, see
// 04-ROADMAP.md) - this just proves dungeonBaseXp is real, data-driven
// content, not a hardcoded constant.
class DungeonTable {
public:
    bool loadFromFile(const std::string& path);
    const DungeonDefinition& first() const { return m_dungeons.front(); }

private:
    std::vector<DungeonDefinition> m_dungeons{DungeonDefinition{"goblin_camp", "Goblin Camp", 100}};
};

} // namespace game::swarm
