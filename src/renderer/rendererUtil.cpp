#include "rendererUtil.hpp"

namespace pse {

Renderer::Color16 color16_from_24(Renderer::Color24 c){
    Renderer::Color16 r;
    r.r = c.r >> 3;
    r.g = c.g >> 3;
    r.b = c.b >> 3;
    r.m = 0;

    return r;
}

usize to_flat(u16 x, u16 y){
    return Renderer::VRAM_WIDTH * y + x;
}

void blit_incr(Renderer::Blit* a){
    u16 cx = a->cur.x;
    u16 cy = a->cur.y;
    u16 l = a->src.x;
    u16 r = l + a->dims.x;
    u16 b = a->src.y + a->dims.y;

    cx += 1;
    if (cx == r){
        cx = l;
        cy += 1;
    }

    a->cur.x = cx;
    a->cur.y = cy;
}

bool blit_valid(const Renderer::Blit* a){
    return a->cur.y >= a->cur.y + a->dims.y;
}

}
