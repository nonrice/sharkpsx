#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <mutex>

#include "App.hpp"
#include "logging.hpp"

namespace pse {

struct App::Impl {
    SDL_Window* win{nullptr};
    SDL_Texture* vram_tex{nullptr};
    SDL_Renderer* renderer{nullptr};

    std::mutex vram_buf_mx{};
    std::unique_ptr<u16[]> vram_buf; // MUST LOCK
    bool vram_buf_updated{false};

    std::unique_ptr<u16[]> vram_local_buf;
};

App::App() : m_imp(std::make_unique<Impl>()) {
    m_imp->vram_buf = std::make_unique<u16[]>(VRAM_SIZE);
    m_imp->vram_local_buf = std::make_unique<u16[]>(VRAM_SIZE);
}

App::~App() = default;

bool App::init(){
    if (!SDL_Init(SDL_INIT_VIDEO)){
        LOG_DBG("SDL could not init video");
        return false;
    }

    if ((m_imp->win = SDL_CreateWindow(
                                   "VRAM", VRAM_WIDTH, VRAM_HEIGHT,
                                   SDL_WINDOW_ALWAYS_ON_TOP
                                   )) == nullptr){
        LOG_DBG("SDL could not create window");
        return false;
    }

    m_imp->renderer = SDL_CreateRenderer(m_imp->win, nullptr);
    SDL_SetRenderVSync(m_imp->renderer, 1);

    m_imp->vram_tex = SDL_CreateTexture(
        m_imp->renderer,
        SDL_PIXELFORMAT_ABGR1555,
        SDL_TEXTUREACCESS_STREAMING,
        VRAM_WIDTH, VRAM_HEIGHT 
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

        bool upd_tex_ready = false;
        {
            std::lock_guard lock1(m_imp->vram_buf_mx);
            if (m_imp->vram_buf_updated){
                m_imp->vram_buf_updated = false;
                std::memcpy(m_imp->vram_local_buf.get(),
                            m_imp->vram_buf.get(),
                            VRAM_SIZE * sizeof(u16));
                upd_tex_ready = true;
            }
        }

        if (upd_tex_ready){
            SDL_UpdateTexture(m_imp->vram_tex,
                              nullptr,
                              m_imp->vram_local_buf.get(),
                              VRAM_WIDTH * sizeof(u16));
        }
        SDL_SetRenderDrawColor(m_imp->renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_imp->renderer);
        SDL_RenderTexture(m_imp->renderer, m_imp->vram_tex, NULL, NULL);
        SDL_RenderPresent(m_imp->renderer);
    }

    SDL_DestroyWindow(m_imp->win);
    SDL_Quit();
}

void App::vram_into_buf(const u16* p){
    std::lock_guard lock1(m_imp->vram_buf_mx);
    std::memcpy(m_imp->vram_buf.get(), p, VRAM_SIZE * sizeof(u16));
    m_imp->vram_buf_updated = true;
}


};
