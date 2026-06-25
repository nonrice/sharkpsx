#pragma once

#include <memory>
#include <functional>

#include "MMIODevice.hpp"
#include "BitField.hpp"

namespace pse {

class GPU : public MMIODevice {
public:
    GPU();

    virtual u32 read(u32 offset) override;
    virtual void write(u32 offset, u32 val) override;

    using OnVBlankType = std::function<void(u16*)>;

    void set_on_vblank(OnVBlankType f);

    void tick();
private:
    OnVBlankType m_on_vblank{};

    static constexpr usize VRAM_SIZE = 1024 * 512; // 1mb
    std::unique_ptr<u16[]> m_vram;
    static constexpr u32 VRAM_WIDTH = 1024;

    constexpr usize to_flat(u32 x, u32 y) const;

    u32 rd_gpustat();
    u32 rd_gpuread();
    void wr_gp0(u32 val);
    void wr_gp1(u32 val);

    u32 m_clk;

    union Stat {
        u32 val;
        bf32<0, 3> texpg_x;
        b32<4> texpg_y;
        bf32<5, 6> sem_trans;
        bf32<7, 8> texpg_co;
        b32<9> dither;
        b32<10> draw_disp;
        b32<11> set_mask;
        b32<12> draw_mask;
        b32<13> interlace_field;
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
    u32 m_disp_start;
    // these r not pixel coords btw
    u32 m_disp_x1;
    u32 m_disp_x2;
    u32 m_disp_y1;
    u32 m_disp_y2;

    u32 m_draw_x1;
    u32 m_draw_y1;
    u32 m_gp1e3_val;

    u32 m_draw_x2;
    u32 m_draw_y2;
    u32 m_gp1e4_val;

    u32 m_draw_xo;
    u32 m_draw_yo;
    u32 m_gp1e5_val;

    u32 m_texwin_xm;
    u32 m_texwin_ym;
    u32 m_texwin_xo;
    u32 m_texwin_yo;
    u32 m_gp1e2_val;

    bool m_texrect_xflip, m_texrect_yflip;

    u32 m_gpuread_res;

    void gp0_e1(u32 val);
    void gp0_e2(u32 val);
    void gp0_e3(u32 val);
    void gp0_e4(u32 val);
    void gp0_e5(u32 val);
    void gp0_e6(u32 val);

    void gp1_00(u32 val);
    void gp1_01(u32 val);
    void gp1_02(u32 val);
    void gp1_03(u32 val);
    void gp1_04(u32 val);
    void gp1_05(u32 val);
    void gp1_06(u32 val);
    void gp1_07(u32 val);
    void gp1_08(u32 val);
    void gp1_10(u32 val);
};

}
