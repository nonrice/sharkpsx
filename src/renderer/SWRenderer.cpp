#include "SWRenderer.hpp"
#include "rendererUtil.hpp"
#include "logging.hpp"

// turn off log
#define LOG_DBG(...)

namespace pse {

SWRenderer::SWRenderer(OnVBlankType f) :
    m_on_vblank(f),
    m_vram(std::make_unique<u16[]>(VRAM_SIZE))
{}

void SWRenderer::vblank(){
    if (m_dirty){
        m_on_vblank(m_vram.get());
        m_dirty = false;
    }
}

void SWRenderer::draw_quickrect(DrawState s, QuickRect a){
    m_dirty = true;

    Color16 c = color16_from_24(a.color);
    c.m = 0;

    LOG_DBG("{} {} {} {}", a.topleft.x, a.topleft.y, a.dims.x, a.dims.y);
    for (u16 y=0; y<a.dims.y; y++){
        for (u16 x=0; x<a.dims.x; x++){
            m_vram[to_flat(a.topleft.x + x, a.topleft.y + y)] = c.val;
        }
    }
}

void SWRenderer::draw_rect(DrawState s, Rect a){
    m_dirty = true;

    LOG_DBG("rect type {}", a.sz);
    switch (a.sz){
        case Rect::PIX:
            m_vram[to_flat(a.src.x, a.src.y)] =
                color16_from_24(a.col).val;
            break;
        default:
            LOG_DBG("Unsupported rect");
    }
}

void SWRenderer::draw_polygon(DrawState s, Polygon a){
    LOG_DBG("{} {}", a.vert[2].x, a.vert[2].y);
}

bool SWRenderer::blit_cv(Blit* a, usize sz, const u32* d){
    m_dirty = true;

    for (usize i=0; i<sz; i++){
        Pack16_32 val{ d[i] };
        for (u16 p : { val.hi.get(), val.lo.get() }){ 
            LOG_DBG("blitcv {} {} " HEX16, a->cur.x, a->cur.y, p);
            m_vram[to_flat(a->cur.x, a->cur.y)] = p;
            blit_incr(a);
            if (!blit_valid(a)){
                return false;
            }
        }
    }

    return blit_valid(a);
}


bool SWRenderer::blit_vc(Blit* a, usize sz, u32* d){
    for (usize i=0; i<sz; i++){
        for (u8 s=16; s>=0; s-=16){
            LOG_DBG("blitvc {} {}", a->cur.x, a->cur.y);
            d[i] |= m_vram[to_flat(a->cur.x, a->cur.y)] << s;
            blit_incr(a);
            if (!blit_valid(a)){
                return false;
            }
        }
    }
    return blit_valid(a);
}


}
