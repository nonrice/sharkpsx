#include "SWRenderer.hpp"
#include "rendererUtil.hpp"
#include "logging.hpp"

namespace pse {

SWRenderer::SWRenderer(OnVBlankType f) :
    m_on_vblank(f),
    m_vram(std::make_unique<u16[]>(VRAM_SIZE))
{}

void SWRenderer::vblank(){
    m_on_vblank(m_vram.get());
}

void SWRenderer::draw_quickrect(DrawState s, QuickRect a){
    Color16 c = color16_from_24(a.color);
    c.m = 0;

    LOG_DBG("{} {} {} {}", a.topleft.x, a.topleft.y, a.dims.x, a.dims.y);
    for (u16 y=0; y<a.dims.y; y++){
        for (u16 x=0; x<a.dims.x; x++){
            m_vram[to_flat(a.topleft.x + x, a.topleft.y + y)] = c.val;
        }
    }
}

void SWRenderer::draw_polygon(DrawState s, Polygon a){
    LOG_DBG("{} {}", a.vert[2].x, a.vert[2].y);
}

bool SWRenderer::blit_cv(Blit* a, usize sz, const u32* d){
    for (usize i=0; i<sz; i++){
        m_vram[to_flat(a->cur.x, a->cur.y)] = d[i];
        blit_incr(a);
        if (!blit_valid(a)){
            return false;
        }
    }
    return blit_valid(a);
}


bool SWRenderer::blit_vc(Blit* a, usize sz, u32* d){
    for (usize i=0; i<sz; i++){
        d[i] = m_vram[to_flat(a->cur.x, a->cur.y)];
        blit_incr(a);
        if (!blit_valid(a)){
            return false;
        }
    }
    return blit_valid(a);
}


}
