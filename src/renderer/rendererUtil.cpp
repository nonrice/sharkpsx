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

}
