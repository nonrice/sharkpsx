#pragma once

#include "types.hpp"

#include <memory>

namespace pse {

class App {
public:
    static constexpr u32 VRAM_WIDTH = 1024;
    static constexpr u32 VRAM_HEIGHT = 512;
    static constexpr u32 VRAM_SIZE = VRAM_WIDTH * VRAM_HEIGHT;

    App();
    ~App();

    bool init();
    void run();

    void vram_into_buf(const u16* p);

private:
    struct Impl;

    // so we can hide SDL from core
    // no need to do this anymore
    std::unique_ptr<Impl> m_imp;

};

};
