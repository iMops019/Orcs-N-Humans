#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdio>
#include <memory>

#include "engine/assets/AssetManager.h"
#include "engine/input/InputMap.h"
#include "engine/render/DebugHud.h"
#include "engine/render/Renderer.h"
#include "engine/scene/StateStack.h"

#include "PlaceholderState.h"

// Phase 0 entry point - proves the DarkXEngine submodule builds and
// runs standalone. No save system, audio, or menu stack yet (those
// come with the states that actually need them - see
// Orcs N Humans Game Docs/04-ROADMAP.md). Structurally this mirrors
// Dark X Engine's own game/main.cpp (see that repo's git history,
// "Bane of the Outcasts removed") minus everything BotO-specific.
int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    const int windowWidth = 1920;
    const int windowHeight = 1080;

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);

    SDL_Window* window = SDL_CreateWindow("Orcs N Humans", windowWidth, windowHeight, SDL_WINDOW_OPENGL);
    if (window == nullptr) {
        std::printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    engine::render::Renderer renderer;
    if (!renderer.init(window, windowWidth, windowHeight)) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    engine::render::DebugHud debugHud;
    debugHud.init(renderer, "C:\\Windows\\Fonts\\arial.ttf");

    {
        engine::assets::AssetManager assets(renderer);
        engine::input::InputMap input;
        input.bindKey(SDL_SCANCODE_ESCAPE, "Quit");

        engine::scene::StateStack stateStack;
        stateStack.push(std::make_unique<game::PlaceholderState>(windowWidth, windowHeight), renderer, assets);

        Uint64 previousTicks = SDL_GetPerformanceCounter();
        const Uint64 frequency = SDL_GetPerformanceFrequency();

        bool running = true;
        SDL_Event event;
        while (running) {
            input.beginFrame();
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                }
                if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat && event.key.scancode == SDL_SCANCODE_F3) {
                    debugHud.toggle();
                }
                input.handleEvent(event);
                stateStack.handleEvent(event);
            }
            input.update();

            if (input.wasPressed("Quit")) {
                running = false;
            }

            const Uint64 currentTicks = SDL_GetPerformanceCounter();
            const float deltaTime = static_cast<float>(currentTicks - previousTicks) / static_cast<float>(frequency);
            previousTicks = currentTicks;

            stateStack.update(deltaTime, input);

            renderer.beginFrame();
            renderer.clear();
            stateStack.render(renderer);
            debugHud.render(renderer, deltaTime);
            renderer.present();
        }

        while (!stateStack.empty()) {
            stateStack.pop();
        }
    }

    debugHud.shutdown();
    renderer.shutdown();
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
