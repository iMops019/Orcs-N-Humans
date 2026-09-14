#pragma once

#include <array>
#include <string>

#include "PlayerState.h"

namespace game::swarm {

// The pick-1-of-4 level-up boon list (see 01-VISION-AND-GOALS.md's
// "in-run XP" section). A tiny placeholder roster on purpose - four
// stat boosts, all four always offered - just enough to prove the
// level-up loop end to end; a real, larger pool (new skills, not just
// stat boosts) is later content work, not a Phase 1 concern.
struct Boon {
    std::string name;
    std::string description;
    void (*apply)(PlayerState&);
};

// Always the same 4, in the same order - see the struct comment above
// for why that's fine for now.
const std::array<Boon, 4>& allBoons();

} // namespace game::swarm
