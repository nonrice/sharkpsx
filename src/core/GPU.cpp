#include "GPU.hpp"
#include "Panic.hpp"
#include "logging.hpp"

namespace pse {

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
    throw Panic("unsupported gpu mmio");
    return 0;
}

void GPU::wr_gp0(u32 val){
    throw Panic("unsupported gpu mmio");
    return;
}

void GPU::wr_gp1(u32 val){
    LOG_DBG("hello gpu");
    u8 opcode = val >> 24;
    switch (opcode){
        case 0x00:
            m_stat.irq1 = 0;
            m_stat.disp = 0;
            m_stat.dma_dir = 0;
            m_disp_startx = 0;
            m_disp_starty = 0;
            m_disp_x1 = 0x200;
            m_disp_x2 = 0x200 + 256*10;
            m_disp_y1 = 0x10;
            m_disp_y2 = 0x10 + 240;
            m_stat.vidmode = 0;
            // TODO reset rendering attrs
            break;
        case 0x01:
            break; // no cmdbuffer for now
        case 0x02:
            m_stat.irq1 = 0;
            break;
        case 0x03:
            m_stat.disp = val & 1;
            break;
        case 0x04:
            m_stat.dma_dir = val & 0x3;
            break;
        case 0x05:
            m_disp_startx = bf32<0, 9>{ val };
            m_disp_starty = bf32<10, 18>{ val }; // assume 1mb ram
            break;
        case 0x06:
            m_disp_x1 = bf32<0, 11>{val};
            m_disp_x2 = bf32<12, 23>{val};
            break;
        case 0x07:
            m_disp_y1 = bf32<0, 9>{val};
            m_disp_y2 = bf32<10, 19>{val};
            break;
        case 0x08:
            m_stat.hres1 = bf32<0, 1>{val};
            m_stat.vres = b32<2>{val};
            m_stat.vidmode = b32<3>{val};
            m_stat.co_depth = b32<4>{val};
            m_stat.vinterlace = b32<5>{val};
            m_stat.hres2 = b32<6>{val};
            m_stat.flip = b32<7>{val};
            break;
        default: {
            if (opcode >= 0x10 && opcode <= 0x1F){
                u32 regnum = val & 0x8;

                // TODO...
                return;
            }

            throw Panic("unsupported gpu mmio");
        } 
    }
}



}
