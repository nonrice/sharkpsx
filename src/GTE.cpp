#include "GTE.hpp"
#include "types.hpp"
#include "Panic.hpp"

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


constexpr u32 GTE::Regs::regname_ind(RegName r){
    return r & REGNAME_IND_MASK;
}

constexpr bool GTE::Regs::regname_is_16(RegName r){
    return r & REGNAME_16;
}

constexpr bool GTE::Regs::regname_is_16lo(RegName r){
    return r & REGNAME_16LO;
}

constexpr bool GTE::Regs::regname_is_16hi(RegName r){
    return r & REGNAME_16HI;
}

constexpr bool GTE::Regs::regname_is_u(RegName r){
    return r & REGNAME_U;
}

constexpr bool GTE::Regs::regname_eq(RegName a, RegName b){
    return regname_ind(a) == regname_ind(b);
}

constexpr u32 GTE::Regs::get(RegName r){
    return raw[regname_ind(r)];
}

constexpr void GTE::Regs::set(RegName r, u32 val){
    raw[regname_ind(r)] = val;
}

constexpr u32 GTE::Regs::read(GTE::Regs::RegName r){
    if (regname_is_16(r)) {
        u16 val;
        if (r & REGNAME_16HI){
            val = get(r) >> 16;
        } else {
            val = get(r) & 0xFFFF;
        }

        if (regname_is_u(r)){
            return val;
        } else {
            return static_cast<s16>(val);
        }
    } else {
        if (regname_is_u(r)){
            return get(r);
        } else {
            return static_cast<s32>(get(r));
        }
    }
}

constexpr u64 GTE::Regs::read64(RegName r){
    u32 val = read(r);
    if (regname_is_u(r)){
        return val;
    } else {
        return static_cast<s32>(val);
    }
}

constexpr void GTE::Regs::write(GTE::Regs::RegName r, u32 val){
    if (r == FLAG){
        Panic("Trying to write to GTE FLAG");
    }

    if (r == SXYP){
        shift_SXYP();
        return;
    }

    if (r & REGNAME_16) {
        Pack16 reg{ get(r) };
        if (r & REGNAME_16HI){
            reg.lo = val;
        } else {
            reg.hi = val;
        }
        set(r, reg.val);

    } else {
        set(r, val);
    }
}

constexpr void GTE::Regs::shift_SXYP(){
    shift_SXY2();
    set(SXY2, get(SXYP));
}

constexpr void GTE::Regs::shift_SXY2(){
    set(SXY0, get(SXY1));
    set(SXY1, get(SXY2));
}

constexpr void GTE::Regs::shift_SZ3(){
    set(SZ0, get(SZ1));
    set(SZ1, get(SZ2));
    set(SZ2, get(SZ3));
}

constexpr void GTE::Regs::shift_RGBC(){
    set(RGB0, get(RGB1));
    set(RGB1, get(RGB2));
    set(RGB2, get(RGBC));
}

constexpr bool GTE::Regs::get_flag(u8 i){
    return (get(FLAG) >> i) & 1;
}

constexpr void GTE::Regs::set_flag(u8 i, bool val){
    set(FLAG, (get(FLAG) & ~(1 << i)) | (val << i));
}

template <GTE::LimiterType T, u8 V>
u64 GTE::apply_lim(u64 x){
    s64 l=0, r;
    if constexpr (T == AS){
        l = -(1<<15);
        r = (1<<15) - 1;
    } else if constexpr (T == AU){
        r = (1<<15) - 1;
    } else if constexpr (T == B){
        r = (1<<8) - 1;
    } else if constexpr (T == C){
        r = (1<<16) - 1;
    } else if constexpr (T == D){
        l = -(1 << 10);
        r = (1 << 10) - 1; 
    } else if constexpr (T == E){
        r = (1 << 12) - 1;
    } else {
        assert(false);
    }

    s64 x_s = x;
    s64 val = std::clamp(x_s, l, r);

    bool fail = false;
    if (val != x_s){
        fail = true;
    }

    if (!fail){
        return val;
    }

    if constexpr (T == AS || T == AU){
        static_assert(1 <= V && V <= 3);
        m_regs.set_flag(25 - V, true);
    } else if constexpr (T == B){
        static_assert(1 <= V && V <= 3);
        m_regs.set_flag(22 - V, true);
    } else if constexpr (T == C){
        static_assert(V == 1);
        m_regs.set_flag(18, true);
    } else if constexpr (T == D){
        static_assert(1 <= V && V <= 2);
        m_regs.set_flag(15 - V, true);
    } else if constexpr (T == E){
        static_assert(V == 1);
        m_regs.set_flag(12, true);
    }

    return val;
}

// begin opcode implementation

#define 







}
