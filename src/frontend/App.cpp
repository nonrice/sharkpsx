#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "App.hpp"
#include "logging.hpp"

namespace pse {

struct App::Impl {
    SDL_Window* win{nullptr};
    SDL_Texture* vram_tex{nullptr};
    SDL_Renderer* renderer{nullptr};
};

App::App() : m_imp(std::make_unique<Impl>()) {}

App::~App() = default;

bool App::init(){
    if (!SDL_Init(SDL_INIT_VIDEO)){
        LOG_DBG("SDL could not init video");
        return false;
    }

    if ((m_imp->win = SDL_CreateWindow("VRAM", 1024, 512, 0)) == nullptr){
        LOG_DBG("SDL could not create window");
        return false;
    }

    m_imp->renderer = SDL_CreateRenderer(m_imp->win, nullptr);
    SDL_SetRenderVSync(m_imp->renderer, 1);

    m_imp->vram_tex = SDL_CreateTexture(
        m_imp->renderer,
        SDL_PIXELFORMAT_ABGR1555,
        SDL_TEXTUREACCESS_STREAMING,
        1024, 512
    );

    return true;
}

void App::run(){
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        SDL_UpdateWindowSurface(m_imp->win);
    }

    SDL_DestroyWindow(m_imp->win);
    SDL_Quit();
}


};
