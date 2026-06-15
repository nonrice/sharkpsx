#pragma once

#include <memory>

#include "MMIODevice.hpp"
#include "BitField.hpp"

namespace pse {

class GPU : public MMIODevice {
public:
    virtual u32 read(u32 offset) override;
    virtual void write(u32 offset, u32 val) override;

private:
    static constexpr usize VRAM_SIZE = 1024 * 1024; // 1mb
    std::unique_ptr<u8[]> m_vram;

    u32 rd_gpustat();
    u32 rd_gpuread();
    void wr_gp0(u32 val);
    void wr_gp1(u32 val);

    union Stat {
        u32 val;
        bf32<0, 3> texpg_x;
        b32<4> texpg_y;
        bf32<5, 6> sem_trans;
        bf32<7, 8> texpg_co;
        b32<9> dither;
        b32<10> disp_draw;
        b32<11> set_mask;
        b32<12> draw_mask;
        b32<13> interlace;
        b32<14> flip; // v1 only?
        b32<15> texpg_y2; // 2mb only
        b32<16> hres2;
        bf32<17, 18> hres1;
        b32<19> vres;
        b32<20> vidmode;
        b32<21> co_depth;
        b32<22> vinterlace;
        b32<23> disp;
        b32<24> irq1;
        b32<25> dma_req;
        b32<26> rdy_cmd;
        b32<27> rdy_vram;
        b32<28> rdy_dma;
        bf32<29, 30> dma_dir;
        b32<31> interlace_odd;
    };

    Stat m_stat;
    u32 m_disp_startx;
    u32 m_disp_starty;
    // these r not pixel coords btw
    u32 m_disp_x1;
    u32 m_disp_x2;
    u32 m_disp_y1;
    u32 m_disp_y2;
};

}
