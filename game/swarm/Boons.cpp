#include "Boons.h"

#include <algorithm>

namespace game::swarm {

namespace {

void applySharperArrows(PlayerState& player) {
    player.attackDamage *= 1.2f;
}

void applyQuickDraw(PlayerState& player) {
    player.attackCooldown = std::max(0.15f, player.attackCooldown * 0.85f);
}

void applyThickHide(PlayerState& player) {
    player.maxHp += 20.0f;
    player.hp += 20.0f;
}

void applySwiftBoots(PlayerState& player) {
    player.moveSpeed *= 1.15f;
}

} // namespace

const std::array<Boon, 4>& allBoons() {
    static const std::array<Boon, 4> boons{
        Boon{"Sharper Arrows", "+20% attack damage", &applySharperArrows},
        Boon{"Quick Draw", "-15% attack cooldown", &applyQuickDraw},
        Boon{"Thick Hide", "+20 max HP", &applyThickHide},
        Boon{"Swift Boots", "+15% move speed", &applySwiftBoots},
    };
    return boons;
}

} // namespace game::swarm
