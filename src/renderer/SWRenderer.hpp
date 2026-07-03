#pragma once

#include <functional>
#include <memory>

#include "Renderer.hpp"

namespace pse {

class SWRenderer : public Renderer {
public:
    using OnVBlankType = std::function<void(u16[])>;
    SWRenderer(OnVBlankType f);

    virtual void vblank() override;
    virtual void draw_quickrect(DrawState s, QuickRect a) override;
    virtual void draw_polygon(DrawState s, Polygon a) override;
    virtual bool blit_cv(Blit* a, usize sz, const u32* d) override;
    virtual bool blit_vc(Blit* a, usize sz, u32* d) override;
    virtual void draw_rect(DrawState s, Rect a) override;
private:
    OnVBlankType m_on_vblank;
    std::unique_ptr<u16[]> m_vram;

    bool m_dirty{ false };
};

}
