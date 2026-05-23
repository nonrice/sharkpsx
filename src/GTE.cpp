#include "GTE.hpp"
#include "types.hpp"
#include "Panic.hpp"
#include "logging.hpp"

namespace pse {

void GTE::to_ctrl(u32 i, u32 x){
    assert(0 <= i && i <= 31);
    m_regs.write(static_cast<Regs::RegName>(i + 32), x);
}

void GTE::to_data(u32 i, u32 x){
    assert(0 <= i && i <= 31);
    m_regs.write(static_cast<Regs::RegName>(i), x);
}

u32 GTE::from_ctrl(u32 i){
    assert(0 <= i && i <= 31);
    return m_regs.read(static_cast<Regs::RegName>(i + 32));
}

u32 GTE::from_data(u32 i){
    assert(0 <= i && i <= 31);
    return m_regs.read(static_cast<Regs::RegName>(i));
}

void GTE::cmd(u32 x){
    process_instr(x);
}

bool GTE::get_flag(u8 i){
    return m_regs.get_flag(i);
}

void GTE::process_instr(u32 val){
    Instr i{val};

    assert(i.opcode < 64);

    GTE::OpHandlerPtr handler = m_op_table[i.opcode];

    if (handler == nullptr){
        Panic("unknown gte opcode");
    } else {
        std::invoke(handler, this, i);
    }
}

    
const std::array<GTE::OpHandlerPtr, 64> GTE::m_op_table = {{
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


constexpr u8 GTE::Regs::regname_ind(RegName r){
    return r & REGNAME_IND_MASK;
}

constexpr bool GTE::Regs::regname_is_pack(RegName r){
    return r & REGNAME_PACK;
}

constexpr bool GTE::Regs::regname_is_lo(RegName r){
    return r & REGNAME_LO;
}

constexpr bool GTE::Regs::regname_is_hi(RegName r){
    return r & REGNAME_HI;
}

constexpr bool GTE::Regs::regattr_is_u(u8 i){
    return attrs[i] & REGATTR_U;
}
constexpr bool GTE::Regs::regattr_is_16(u8 i){
    return attrs[i] & REGATTR_16;

}
constexpr bool GTE::Regs::regattr_can_read(u8 i){
    return !(attrs[i] & REGATTR_NORD);
}
constexpr bool GTE::Regs::regattr_can_write(u8 i){
    return !(attrs[i] & REGATTR_NOWR);
}

constexpr u32 GTE::Regs::read(GTE::Regs::RegName r){
    if (r == SXYP){
        return read(SXY2); // this is just how it is (see relevant redux test)
    }

    if (r == ORGB){
        calc_ORGB();
    }

    if (r == LZCR){
        calc_LZCR();
    }

    const u8 i = regname_ind(r);
    if (!regattr_can_read(i)){
        // throw Panic("trying to read from write only gte reg");
    }


    if (regname_is_pack(r)){
        PackedReg full_reg{raw[i]};
        u16 val;
        if (regname_is_lo(r)){
            val = full_reg.lo;
        } else {
            val = full_reg.hi;
        }

        if (regattr_is_u(i)){
            return val;
        } else {
            return static_cast<s16>(val);
        }
    } else {
        return raw[i];
    }
}

constexpr u64 GTE::Regs::read64(RegName r){
    u32 val = read(r);
    if (regattr_is_u(regname_ind(r))){
        return val;
    } else {
        return static_cast<s32>(val);
    }
}

constexpr void GTE::Regs::write(GTE::Regs::RegName r, u32 val){
    const u8 i = regname_ind(r);
    if (!regattr_can_write(i)){
        LOG_DBG("{}", i);
        // throw Panic("trying to write to read only gte reg");
    }

    if (r == SXYP){
        shift_SXYP();
        write(SXY2, val);
        return;
    }

    if (regname_is_pack(r)){
        PackedReg full_reg{raw[r]};
        if (regname_is_hi(r)){
            full_reg.hi = val;
        } else {
            full_reg.lo = val;
        }
        raw[i] = full_reg.val;
        return;
    }

    if (regattr_is_16(i)){
        u16 lo = val;
        if (regattr_is_u(i)){
            raw[i] = lo;
        } else {
            raw[i] = static_cast<s16>(lo);
        }

        if (r == IRGB){
            calc_IRGB();
        }
        return;
    }

    raw[i] = val;

    if (i == FLAG){
        calc_FLAG();
    }
}

constexpr void GTE::Regs::shift_SXYP(){
    raw[SXY0] = raw[SXY1];
    raw[SXY1] = raw[SXY2];
    raw[SXY2] = raw[SXYP];
}

constexpr void GTE::Regs::calc_ORGB(){
    s32 ir1 = raw[IR1];
    s32 ir2 = raw[IR2];
    s32 ir3 = raw[IR3];

    ir1 >>= 7;
    ir2 >>= 7;
    ir3 >>= 7;

    RGBReg r{ 0 };
    r.r = std::clamp(ir1, 0, 0x1f);
    r.g = std::clamp(ir2, 0, 0x1f);
    r.b = std::clamp(ir3, 0, 0x1f);

    raw[ORGB] = r.val;
}

constexpr void GTE::Regs::calc_IRGB(){
    RGBReg r{ raw[IRGB] };
    raw[IR1] = r.r << 7;
    raw[IR2] = r.g << 7;
    raw[IR3] = r.b << 7;

    // just ensure 0
    r.rest = 0;
    raw[IRGB] = r.val;

}

constexpr void GTE::Regs::calc_LZCR(){
    u32 lz = raw[LZCS] ^ (static_cast<s32>(raw[LZCS]) >> 31);
    raw[LZCR] = __builtin_clz(lz);
}

constexpr void GTE::Regs::calc_FLAG(){
    raw[FLAG] &= 0xFFFFF000;

    union SummaryBits {
        u32 val;
        bf32<23, 30> a;
        bf32<13, 18> b;
    };

    SummaryBits s{ raw[FLAG] };
    bool summary = s.a + s.b;

    if (summary){
        raw[FLAG] |= (1 << 31);
    }
}

constexpr bool GTE::Regs::get_flag(u8 i){
    return (raw[FLAG] >> i) & 1;
}

constexpr void GTE::Regs::set_flag(u8 i, bool val){
    raw[FLAG] = (raw[FLAG] & ~(1 << i)) | (val << i);
    calc_FLAG();
}

template <GTE::LimiterType T, u8 V>
u64 GTE::lim(u64 x){
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
        m_regs.set_flag(18, true);
    } else if constexpr (T == D){
        static_assert(1 <= V && V <= 2);
        m_regs.set_flag(15 - V, true);
    } else if constexpr (T == E){
        m_regs.set_flag(12, true);
    }

    return val;
}

template <GTE::LimiterType L>
u64 GTE::lim(u64 x){
    return lim<L, 0>(x);
}

template <u8 T>
u64 GTE::calc_test(u64 x){
    static_assert(1<=T && T<=4);
    
    s64 r = (1UL << 43) - 1;
    s64 l = -(1UL << 43);
    if constexpr (T == 4) {
        r = (1UL << 31) - 1;
        l = -(1UL << 31);
    }

    const s64 x_s = static_cast<s64>(x);
    if (x_s > r){
        if constexpr (T == 4){
            m_regs.set_flag(16, true);
        } else {
            m_regs.set_flag(31 - T, true);
        }
    } else if (x_s < l){
        if constexpr (T == 4){
            m_regs.set_flag(15, true);
        } else {
            m_regs.set_flag(28 - T, true);
        }
    }

    return x;
}

u64 GTE::divide(u64 p, u64 q){
    bool sat = false;
    u64 res = 0;
    if (q != 0){
        res = (((p << 17) / q) + 1) / 2;
    } else {
        sat = true;
    }

    if (res > 0x1FFFFULL){
        sat = true;
    }

    if (sat){
        res = 0x1FFFFULL;
        m_regs.set_flag(17, true);
        m_regs.set_flag(31, true);
    }

    return res;
}

// begin opcode implementation

#define LET const u64

#define REG(r) LET r = m_regs.read64(Regs::r)

// careful in branch!
#define MAT_REGS(r) \
    REG(r##11); REG(r##12); REG(r##13); \
    REG(r##21); REG(r##22); REG(r##23); \
    REG(r##31); REG(r##32); REG(r##33);
#define VEC_REGS(i) \
    REG(VX##i); REG(VY##i); REG(VZ##i)
#define TR_VEC_REGS() \
    REG(TRX); REG(TRY); REG(TRZ)

// need vaargs to allow apply lim template
#define WRITE(r, ...) m_regs.write(Regs::r, (__VA_ARGS__))

#define SHIFT_SZ2() \
    do { \
        REG(SZ0); REG(SZ1); REG(SZ2); \
        WRITE(SZX, SZ0); \
        WRITE(SZ0, SZ1); \
        WRITE(SZ1, SZ2); \
    } while (0)

#define SHIFT_SXY2() \
    do { \
        REG(SXY1); REG(SXY2); \
        WRITE(SXY0, SXY1); \
        WRITE(SXY1, SXY2); \
    } while (0)

template <u8 W>
static inline bool check_ovf(u64 x){
    u64 upper = static_cast<s64>(x) >> W;
    return !(upper == ~static_cast<u64>(0) || upper == 0);
}

void GTE::op_RTPS([[maybe_unused]] Instr i){
    MAT_REGS(R);
    VEC_REGS(0);
    TR_VEC_REGS();

    LET SSX =
        calc_test<1>((TRX << 12) + R11*VX0 + R12*VY0 + R13*VZ0);
    LET SSY =
        calc_test<2>((TRY << 12) + R21*VX0 + R22*VY0 + R23*VZ0);
    LET SSZ =
        calc_test<3>((TRZ << 12) + R31*VX0 + R32*VY0 + R33*VZ0);

    WRITE(IR1, lim<AS, 1>(SSX >> 12));
    WRITE(IR2, lim<AS, 2>(SSY >> 12));
    WRITE(IR3, lim<AS, 3>(SSZ >> 12));
    
    SHIFT_SZ2();
    WRITE(SZ2, lim<C>(SSZ >> 12));

    REG(OFX);
    REG(OFY);
    REG(IR1);
    REG(IR2);
    REG(SZ2);
    REG(H);
    REG(DQB);
    REG(DQA);
    LET div_res = divide(H, SZ2);
    LET SX = calc_test<4>(OFX + IR1 * div_res);
    LET SY = calc_test<4>(OFY + IR2 * div_res);
    LET P = calc_test<4>(DQB + DQA * div_res);

    SHIFT_SXY2();
    WRITE(SX2, SX);
    WRITE(SY2, SY);

    WRITE(MAC0, P);
    WRITE(MAC1, SSX >> 12);
    WRITE(MAC2, SSY >> 12);
    WRITE(MAC3, SSZ >> 12);
}

void GTE::op_NCLIP(Instr i) {}
void GTE::op_OP(Instr i) {}
void GTE::op_DPCS(Instr i) {}
void GTE::op_INTPL(Instr i) {}
void GTE::op_MVMVA(Instr i) {}
void GTE::op_NCDS(Instr i) {}
void GTE::op_CDP(Instr i) {}
void GTE::op_NCDT(Instr i) {}
void GTE::op_NCCS(Instr i) {}
void GTE::op_CC(Instr i) {}
void GTE::op_NCS(Instr i) {}
void GTE::op_NCT(Instr i) {}
void GTE::op_SQR(Instr i) {}
void GTE::op_DCPL(Instr i) {}
void GTE::op_DPCT(Instr i) {}
void GTE::op_AVSZ3(Instr i) {}
void GTE::op_AVSZ4(Instr i) {}
void GTE::op_RTPT(Instr i) {}
void GTE::op_GPF(Instr i) {}
void GTE::op_GPL(Instr i) {}
void GTE::op_NCCT(Instr i) {}


#undef REG 
#undef WRITE
#undef MAT_REGS
#undef VEC_REGS
#undef TR_VEC_REGS
#undef SHIFT_SZ2
#undef SHIFT_SXY2





}
