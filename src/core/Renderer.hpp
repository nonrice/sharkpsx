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

    u16 read_vram(u16 x, u16 y);
    void write_vram(u16 x, u16 y);

    struct DrawState {

    };

    union Vec2 {
        u32 val;

        bf32<0, 15> x;
        bf32<16, 31> y;
    };

    union Color24 {
        u32 val;

        bf32<0, 7> r;
        bf32<8, 15> g;
        bf32<16, 23> b;
        bf32<24, 31> c;
    };

    union Color16 {
        u16 val;

        bf16<0, 4> r;
        bf16<5, 9> g;
        bf16<10, 14> b;
        b16<15> m;
    };

    union UV {
        u32 val;

        bf32<16, 31> c;
        bf32<0, 7> u;
        bf32<8, 15> v;
    };

    virtual void vblank() = 0;

    struct QuickRect {
        Color24 color;
        Vec2 topleft;
        Vec2 dims;
    };
    virtual void draw_quickrect(DrawState s, QuickRect a) = 0;

    struct Polygon {
        Color24 col[4];
        Vec2 vert[4];
        UV uv[4];
        bool noblend; // or modulate
        bool trans; // or opaque
        bool tex; // or not
        bool quad; // or tri
        bool gouraud; // or flat
    };
    virtual void draw_polygon(DrawState s, Polygon a) = 0;

    struct Blit {
        Vec2 src;
        Vec2 dims;
        Vec2 cur;
    };
    virtual bool blit_cv(Blit* a, usize sz, const u32* d) = 0;
    virtual bool blit_vc(Blit* a, usize sz, u32* d) = 0;
};

}
