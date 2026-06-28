#pragma once

#include "BitField.hpp"
#include "types.hpp"

namespace pse {

class Renderer {
public:
    static constexpr usize VRAM_SIZE = 1024 * 512; // 1mb
    static constexpr usize VRAM_WIDTH = 1024;
    static constexpr usize VRAM_HEIGHT = 1024;

    virtual ~Renderer() = default;

    struct DrawState {

    };

    union Vec2 {
        u32 val;

        bf32<0, 15> x;
        bf32<16, 31> y;
    };

    union Color24 {
        u32 val;

        bf32<0, 7> c;
        bf32<8, 15> b;
        bf32<16, 23> g;
        bf32<24, 31> r;
    };

    union Color16 {
        u16 val;

        bf16<0, 4> r;
        bf16<5, 9> g;
        bf16<10, 14> b;
        b16<15> m;
    };

    virtual void vblank() = 0;

    struct QuickRect {
        Color24 color;
        Vec2 topleft;
        Vec2 dims;
    };
    virtual void draw_quickrect(DrawState s, QuickRect a) = 0;
};

}
