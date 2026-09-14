#include "DungeonData.h"

#include <nlohmann/json.hpp>

#include "engine/save/SaveFile.h"

namespace game::swarm {

bool DungeonTable::loadFromFile(const std::string& path) {
    nlohmann::json data;
    if (!engine::save::readJson(path, data) || !data.contains("dungeons")) {
        return false; // keep the built-in default from the member initializer
    }

    std::vector<DungeonDefinition> loaded;
    for (const auto& entry : data.at("dungeons")) {
        DungeonDefinition dungeon;
        dungeon.id = entry.value("id", "");
        dungeon.displayName = entry.value("displayName", dungeon.id);
        dungeon.baseXp = entry.value("baseXp", 100);
        if (!dungeon.id.empty()) {
            loaded.push_back(std::move(dungeon));
        }
    }

    if (loaded.empty()) {
        return false;
    }

    m_dungeons = std::move(loaded);
    return true;
}

} // namespace game::swarm
