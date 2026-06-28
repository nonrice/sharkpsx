#pragma once

#include <functional>

#include "Renderer.hpp"

namespace pse {

class SWRenderer : public Renderer {
public:
    using OnVBlankType = std::function<void(u16[])>;
    SWRenderer(OnVBlankType f);

    virtual void vblank() override;
    virtual void draw_quickrect(DrawState s, QuickRect a) override;
private:
    OnVBlankType m_on_vblank;
    std::unique_ptr<u16[]> m_vram;
};

}
