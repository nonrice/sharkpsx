#include <cstdlib>

#include "GPU.hpp"
#include "Panic.hpp"
#include "logging.hpp"

// turn off gpu log
#define LOG_DBG(...)

namespace pse {

GPU::GPU(Renderer& r, IntCtl* intc) :
    m_renderer(r),
    m_intc(intc)
{
    m_cmd.state = CmdParse::START;
    m_gpuread_res = 0xFFFFFFFF;
}

void GPU::tick(){
    m_clk += 1;
    if (m_clk % 883000 == 0){ //roughly 1/60s of gpu clock time
        m_renderer.vblank();
        m_intc->set_interrupt(IntCtl::VBLANK);
    }

    if (!m_cmdbuf.empty()){
        process_cmd();
    }
}

u32 GPU::read(u32 offset){
    switch (offset){
        case 0:
            return rd_gpuread();
        case 4:
            return rd_gpustat();
        default:
            throw Panic("illegal GPU read");
    }
}

void GPU::write(u32 offset, u32 val){
    switch (offset){
        case 0:
            wr_gp0(val);
            return;
        case 4:
            wr_gp1(val);
            return;
        default:
            throw Panic("illegal GPU write");
    }
}

u32 GPU::rd_gpustat(){
    return m_stat.val;
}

u32 GPU::rd_gpuread(){
    if (m_cmd.state == CmdParse::VCBLIT_INPROGRESS){
        if (!m_renderer.blit_cv(&m_cmd.cvblit, 1, &m_gpuread_res)){
            m_cmd.state = CmdParse::START;
            m_stat.rdy_vram = false;
        }
    }
    return m_gpuread_res;
}

void GPU::wr_gp0(u32 val){
    LOG_DBG("wr gp0:" HEX32, val);

    if (m_cmd.state == CmdParse::START && (val >> 24) == 0){
        return;
    }

    if (m_cmdbuf.full()){
        throw Panic("I am full");
    } else {
        m_cmdbuf.push(val);
    }
}

void GPU::process_cmd(){
    assert(!m_cmdbuf.empty());
    u32 val = m_cmdbuf.pop();

    if (m_cmd.state == CmdParse::START){
        u8 opcode = val >> 24;
        switch ((opcode >> 5) & 0x7) {
            case 0:
                switch (opcode){
                    case 0x02:
                        m_cmd.state = CmdParse::QUICKRECT_COLOR;
                        break;
                    case 0xe1:
                        gp0_e1(val);
                        break;
                    case 0xe2:
                        gp0_e2(val);
                        break;
                    case 0xe3:
                        gp0_e3(val);
                        break;
                    case 0xe4:
                        gp0_e4(val);
                        break;
                    case 0xe5:
                        gp0_e5(val);
                        break;
                    case 0xe6:
                        gp0_e6(val);
                        break;
                    default:
                        LOG_DBG("unspported gpu mmio (gp0)");
                        // throw Panic("unsupported gpu mmio (gp0)");
                }
                break;
            case 1:
                m_cmd.polygon.noblend = get_b32<24>(val);
                m_cmd.polygon.trans = get_b32<25>(val);
                m_cmd.polygon.tex = get_b32<26>(val);
                m_cmd.polygon.quad = get_b32<27>(val);
                m_cmd.polygon.gouraud = get_b32<28>(val);

                m_cmd.state = CmdParse::POLYGON_COLOR;
                m_cmd.state_ind = 0;
                break;
            case 3:
                m_cmd.rect.noblend = get_b32<24>(val);
                m_cmd.rect.trans = get_b32<25>(val);
                m_cmd.rect.tex = get_b32<26>(val);
                m_cmd.rect.sz = get_bf32<27, 28>(val);

                LOG_DBG("Rect: " HEX32 " rect sz: {}", val, m_cmd.rect.sz);

                m_cmd.state = CmdParse::RECT_COLOR;
                break;
            case 5:
                m_cmd.state = CmdParse::CVBLIT_START;
                break;
            case 6:
                m_cmd.state = CmdParse::VCBLIT_START;
                break;
        }
    }

    if (m_cmd.state != CmdParse::START){
        switch (m_cmd.state){
            // quickrect
            case CmdParse::QUICKRECT_COLOR:
                m_cmd.quickrect.color.val = val;
                m_cmd.state = CmdParse::QUICKRECT_TOPLEFT;
                break;
            case CmdParse::QUICKRECT_TOPLEFT:
                m_cmd.quickrect.topleft.val = val;
                m_cmd.state = CmdParse::QUICKRECT_DIMS;
                break;
            case CmdParse::QUICKRECT_DIMS:
                m_cmd.quickrect.dims.val = val;
                m_cmd.state = CmdParse::START;
                m_renderer.draw_quickrect({}, m_cmd.quickrect);
                break;

            // polygon
            case CmdParse::POLYGON_COLOR:
                m_cmd.polygon.col[m_cmd.state_ind].val = val;
                m_cmd.state = CmdParse::POLYGON_VERT;
                break;
            case CmdParse::POLYGON_VERT:
                m_cmd.polygon.vert[m_cmd.state_ind].val = val;
                if (m_cmd.polygon.tex){
                    m_cmd.state = CmdParse::POLYGON_UV;
                } else {
                    m_cmd.state = CmdParse::POLYGON_COLOR;
                    m_cmd.state_ind += 1;

                    if (m_cmd.state_ind == 3 + m_cmd.polygon.quad){
                        m_cmd.state = CmdParse::POLYGON_DONE;
                    }
                }
                break;
            case CmdParse::POLYGON_UV:
                m_cmd.polygon.uv[m_cmd.state_ind].val = val;
                if (m_cmd.polygon.gouraud){
                    m_cmd.state = CmdParse::POLYGON_COLOR;
                } else {
                    m_cmd.state = CmdParse::POLYGON_VERT;
                }
                m_cmd.state_ind += 1;

                if (m_cmd.state_ind == 3 + m_cmd.polygon.quad){
                    m_cmd.state = CmdParse::POLYGON_DONE;
                }
                break;

            // cpu to vram blit
            case CmdParse::CVBLIT_START:
                m_cmd.state = CmdParse::CVBLIT_SRC;
                break;
            case CmdParse::CVBLIT_SRC:
                LOG_DBG("src " HEX32, val);
                m_cmd.cvblit.cur.val = m_cmd.cvblit.src.val = val;
                m_cmd.state = CmdParse::CVBLIT_DIMS;
                break;
            case CmdParse::CVBLIT_DIMS:
                m_cmd.cvblit.dims.val = val;
                m_cmd.state = CmdParse::CVBLIT_INPROGRESS;
                break;
            case CmdParse::CVBLIT_INPROGRESS:
                //TODO revisit,add buffering bc this is super slow:(
                if (!m_renderer.blit_cv(&m_cmd.cvblit, 1, &val)){
                    m_cmd.state = CmdParse::START;
                }
                break;

            //vram to cpu blit
            case CmdParse::VCBLIT_START:
                m_cmd.state = CmdParse::VCBLIT_SRC;
                break;
            case CmdParse::VCBLIT_SRC:
                m_cmd.vcblit.cur.val = m_cmd.vcblit.src.val = val;
                m_cmd.state = CmdParse::VCBLIT_DIMS;
                break;
            case CmdParse::VCBLIT_DIMS:
                m_cmd.vcblit.dims.val = val;
                m_cmd.state = CmdParse::VCBLIT_INPROGRESS;
                m_stat.rdy_vram = true;
                break;
            case CmdParse::VCBLIT_INPROGRESS:
                //stenzek says: nothing happens
                LOG_DBG("what ther helly");
                break;

            //rect
            case CmdParse::RECT_COLOR:
                m_cmd.rect.col.val = val;
                m_cmd.state = CmdParse::RECT_SRC;
                break;
            case CmdParse::RECT_SRC:
                m_cmd.rect.src.val = val;
                if (m_cmd.rect.tex){
                    m_cmd.state = CmdParse::RECT_UV;
                } else if (m_cmd.rect.sz == Renderer::Rect::VAR){
                    m_cmd.state = CmdParse::RECT_DIMS;
                } else {
                    m_cmd.state = CmdParse::RECT_DONE;
                }
                break;
            case CmdParse::RECT_UV:
                m_cmd.rect.uv.val = val;
                if (m_cmd.rect.sz == Renderer::Rect::VAR){
                    m_cmd.state = CmdParse::RECT_DIMS;
                } else {
                    m_cmd.state = CmdParse::RECT_DONE;
                }
                break;
            case CmdParse::RECT_DIMS:
                m_cmd.rect.dims.val = val;
                m_cmd.state = CmdParse::RECT_DONE;
                break;
            default:
                break;
        }
    }

    switch (m_cmd.state){
        case CmdParse::POLYGON_DONE:
            m_cmd.state = CmdParse::START;
            m_renderer.draw_polygon({}, m_cmd.polygon);
            break;
        case CmdParse::RECT_DONE:
            m_cmd.state = CmdParse::START;
            m_renderer.draw_rect({}, m_cmd.rect);
            break;
        default:
            break;

    }
}

void GPU::wr_gp1(u32 val){
    u8 opcode = val >> 24;
    switch (opcode){
        case 0x00:
            gp1_00(val);
            break;
        case 0x01:
            gp1_01(val);
            break;
        case 0x02:
            gp1_02(val);
            break;
        case 0x03:
            gp1_03(val);
            break;
        case 0x04:
            gp1_04(val);
            break;
        case 0x05: 
            gp1_05(val);
            break;
        case 0x06:
            gp1_06(val);
            break;
        case 0x07:
            gp1_07(val);
            break;
        case 0x08:
            gp1_08(val);
            break;
        default: {
            if (opcode >= 0x10 && opcode <= 0x1F){
                gp1_10(val);
                break;
            }

            LOG_DBG(HEX32, val);
            throw Panic("unsupported gpu mmio");
        } 
    }
}

void GPU::gp0_e1(u32 val){
    m_stat.texpg_x = get_bf32<0, 3>(val);
    m_stat.texpg_y = get_b32<4>(val);
    m_stat.sem_trans = get_bf32<5, 6>(val);
    m_stat.texpg_co = get_bf32<7, 8>(val);
    m_stat.draw_disp = get_b32<10>(val);
    m_stat.texpg_y2 = get_b32<11>(val);
    m_texrect_xflip = get_b32<12>(val);
    m_texrect_yflip = get_b32<13>(val);

}
void GPU::gp0_e2(u32 val){
    m_texwin_xm = get_bf32<0, 4>(val);
    m_texwin_ym = get_bf32<5, 9>(val);
    m_texwin_xo = get_bf32<10, 14>(val);
    m_texwin_yo = get_bf32<15, 19>(val);
    m_gp1e2_val = val & ((1 << 19) - 1);

}
void GPU::gp0_e3(u32 val){
    m_draw_x1 = get_bf32<0, 9>(val);
    m_draw_y1 = get_bf32<10, 18>(val);
    m_gp1e3_val = val & ((1 << 18) - 1);

}
void GPU::gp0_e4(u32 val){
    m_draw_x2 = get_bf32<0, 9>(val);
    m_draw_y2 = get_bf32<10, 18>(val);
    m_gp1e4_val = val & ((1 << 18) - 1);

}
void GPU::gp0_e5(u32 val){
    m_draw_xo =
        static_cast<s32>(get_bf32<0, 10>(val)) << 22 >> 22;
    // basically, sign extend the 10th bit
    m_draw_yo =
        static_cast<s32>(get_bf32<11, 21>(val)) << 22 >> 22;

    m_gp1e5_val = val & ((1 << 21) - 1);

}
void GPU::gp0_e6(u32 val){
    m_stat.set_mask = get_b32<0>(val);
    m_stat.draw_mask = get_b32<1>(val);
}

void GPU::gp1_00(u32 val){
    gp1_01(0);
    gp1_02(0);
    gp1_03(1);
    gp1_04(0);
    gp1_05(0);
    gp1_06(0x200 + ((0x200 + 256*10) << 12));
    gp1_07(0x010 + ((0x010 + 240) << 12));
    gp1_08(0);
    gp0_e1(0);
    gp0_e2(0);
    gp0_e3(0);
    gp0_e4(0);
    gp0_e5(0);
    gp0_e6(0);
    
    m_stat.rdy_cmd = true;
    m_stat.rdy_vram = false;
    m_stat.rdy_dma = true;

    assert(m_stat.val == 0x14802000);
}

void GPU::gp1_01(u32 val){
    while (!m_cmdbuf.empty()){
        m_cmdbuf.pop();
    }
}

void GPU::gp1_02(u32 val){
    m_stat.irq1 = 0;
}
void GPU::gp1_03(u32 val){
    m_stat.disp = val & 1;

}
void GPU::gp1_04(u32 val){
    m_stat.dma_dir = val & 0x3;

}
void GPU::gp1_05(u32 val){
    m_disp_startx = get_bf32<0, 9>(val);
    m_disp_starty = get_bf32<10, 18>(val); // assume 1mb ram
}

void GPU::gp1_06(u32 val){
    m_disp_x1 = get_bf32<0, 11>(val);
    m_disp_x2 = get_bf32<12, 23>(val);

}
void GPU::gp1_07(u32 val){
    m_disp_y1 = get_bf32<0, 9>(val);
    m_disp_y2 = get_bf32<10, 19>(val);

}
void GPU::gp1_08(u32 val){
    m_stat.hres1 = get_bf32<0, 1>(val);
    m_stat.vres = get_b32<2>(val);
    m_stat.vidmode = get_b32<3>(val);
    m_stat.co_depth = get_b32<4>(val);
    m_stat.vinterlace = get_b32<5>(val);
    // must always update this if interlace is 0
    if (m_stat.vinterlace == 0){
        m_stat.interlace_field = 1;
    }
    m_stat.hres2 = get_b32<6>(val);
    m_stat.flip = get_b32<7>(val);

}

void GPU::gp1_10(u32 val){
    u32 regnum = val & ((1 << 24) - 1);

    switch (regnum % 8){
        case 2:
            m_gpuread_res = m_gp1e2_val;
            break;
        case 3:
            m_gpuread_res = m_gp1e3_val;
            break;
        case 4:
            m_gpuread_res = m_gp1e4_val;
            break;
        case 5:
            m_gpuread_res = m_gp1e5_val;
            break;
    }
}

}
