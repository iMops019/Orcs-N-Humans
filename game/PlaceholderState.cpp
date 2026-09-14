#include "PlaceholderState.h"

#include "engine/render/Renderer.h"

namespace game {

PlaceholderState::PlaceholderState(int windowWidth, int windowHeight)
    : m_windowWidth(windowWidth), m_windowHeight(windowHeight) {}

void PlaceholderState::render(engine::render::Renderer& renderer) {
    renderer.fillRect(0, 0, m_windowWidth, m_windowHeight, 24, 20, 28, 255);

    const int centerX = m_windowWidth / 2;
    const int centerY = m_windowHeight / 2;
    renderer.fillRect(centerX - 16, centerY - 16, 32, 32, 90, 170, 230, 255); // player

    const int goblinOffsets[4][2] = {{-160, -90}, {160, -90}, {-160, 90}, {160, 90}};
    for (const auto& offset : goblinOffsets) {
        renderer.fillRect(centerX + offset[0] - 12, centerY + offset[1] - 12, 24, 24, 90, 200, 90, 255); // goblin
    }
}

} // namespace game
