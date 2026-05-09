#include "GTE.hpp"
#include "types.hpp"

namespace pse {

const std::array<GTE::OpHandlerPtr, 32> GTE::m_op_table = {{
    [0x00] = &GTE::op_RTPS,
    [0x02] = &GTE::op_RTPT,
    [0x04] = &GTE::op_MVMVA,
    [0x06] = &GTE::op_DCPL,
    [0x07] = &GTE::op_DPCS,
    [0x08] = &GTE::op_DPCT,
    [0x09] = &GTE::op_INTPL,
    [0x0A] = &GTE::op_SQR,
    [0x0C] = &GTE::op_NCS,
    [0x0D] = &GTE::op_NCT,
    [0x0E] = &GTE::op_NCDS,
    [0x0F] = &GTE::op_NCDT,
    [0x10] = &GTE::op_NCCS,
    [0x11] = &GTE::op_NCCT,
    [0x12] = &GTE::op_CDP,
    [0x13] = &GTE::op_CC,
    [0x14] = &GTE::op_NCLIP,
    [0x15] = &GTE::op_AVSZ3,
    [0x16] = &GTE::op_AVSZ4,
    [0x17] = &GTE::op_OP,
    [0x19] = &GTE::op_GPF,
    [0x1A] = &GTE::op_GPL
}};

constexpr u32 GTE::Regs::read(GTE::Regs::RegName r){
    if (r & REGNAME_PACK16) {
        if (r & REGNAME_PACK16_IS_HI){
            return raw[r] >> 16;
        } else {
            return raw[r] & 0xFFFF;
        }
    } else {
        return raw[r];
    }
}

constexpr void GTE::Regs::shift_SXY(){
    raw[SXY0] = raw[SXY1];
    raw[SXY1] = raw[SXY2];
}

constexpr void GTE::Regs::shift_SZ(){
    raw[SZ0] = raw[SZ1];
    raw[SZ1] = raw[SZ2];
    raw[SZ2] = raw[SZ3];
}

constexpr void GTE::Regs::write(GTE::Regs::RegName r, u32 val){
    if (r == SXYP){
        shift_SXY();
        return;
    }

    if (r & REGNAME_PACK16) {
        Pack16 reg{raw[r]};
        if (r & REGNAME_PACK16_IS_HI){
            reg.lo = val;
        } else {
            reg.hi = val;
        }
        raw[r] = reg.val;

    } else {
        raw[r] = val;
    }
}

void GTE::op_MVMVA(GTE::Instr i){


}








}
