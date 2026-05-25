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
    [0x01] = &GTE::op_RTPS,
    [0x06] = &GTE::op_NCLIP,
    [0x0C] = &GTE::op_OP,
    [0x10] = &GTE::op_DPCS,
    [0x11] = &GTE::op_INTPL,
    [0x12] = &GTE::op_MVMVA,
    [0x13] = &GTE::op_NCDS,
    [0x14] = &GTE::op_CDP,
    [0x16] = &GTE::op_NCDT,
    [0x1B] = &GTE::op_NCCS,
    [0x1C] = &GTE::op_CC,
    [0x1E] = &GTE::op_NCS,
    [0x20] = &GTE::op_NCT,
    [0x28] = &GTE::op_SQR,
    [0x29] = &GTE::op_DCPL,
    [0x2A] = &GTE::op_DPCT,
    [0x2D] = &GTE::op_AVSZ3,
    [0x2E] = &GTE::op_AVSZ4,
    [0x30] = &GTE::op_RTPT,
    [0x3D] = &GTE::op_GPF,
    [0x3E] = &GTE::op_GPL,
    [0x3F] = &GTE::op_NCCT,
}};


constexpr u8 GTE::Regs::regname_ind(RegName r){
    return r & REGNAME_IND_MASK;
}

constexpr bool GTE::Regs::regname_is_pack16(RegName r){
    return r & REGNAME_PACK16;
}

constexpr bool GTE::Regs::regname_is_lo(RegName r){
    return regname_is_pack16(r) && !(r & REGNAME_PACK_IS_HI);
}

constexpr bool GTE::Regs::regname_is_hi(RegName r){
    return regname_is_pack16(r) && (r & REGNAME_PACK_IS_HI);
}

constexpr bool GTE::Regs::regname_is_pack8(RegName r) {
    return r & REGNAME_PACK8;
}

constexpr bool GTE::Regs::regname_is_a(RegName r){
    return regname_is_pack8(r) && ((r >> 9) == 0);
}

constexpr bool GTE::Regs::regname_is_b(RegName r){
    return regname_is_pack8(r) && ((r >> 9) == 1);
}

constexpr bool GTE::Regs::regname_is_c(RegName r){
    return regname_is_pack8(r) && ((r >> 9) == 2);
}

constexpr bool GTE::Regs::regname_is_d(RegName r){
    return regname_is_pack8(r) && ((r >> 9) == 3);
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


    if (regname_is_pack16(r)){
        Pack16_32 full_reg{raw[i]};
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
    } else if (regname_is_pack8(r)){
        // don't care about sign since this is literally just color
        Pack8_32 full_reg{ raw[i] };
        if (regname_is_a(r)){
            return full_reg.a;
        } else if (regname_is_b(r)){
            return full_reg.b;
        } else if (regname_is_c(r)){
            return full_reg.c;
        } else {
            return full_reg.d;
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
        // throw Panic("trying to write to read only gte reg");
    }

    if (r == SXYP){
        shift_SXYP();
        write(SXY2, val);
        return;
    }

    if (regname_is_pack16(r)){
        Pack16_32 full_reg{ raw[i] };
        if (regname_is_hi(r)){
            full_reg.hi = val;
        } else {
            full_reg.lo = val;
        }
        raw[i] = full_reg.val;
    } else if (regname_is_pack8(r)){
        Pack8_32 full_reg{ raw[i] };
        if (regname_is_a(r)){
            full_reg.a = val;
        } else if (regname_is_b(r)){
            full_reg.b = val;
        } else if (regname_is_c(r)){
            full_reg.c = val;
        } else {
            full_reg.d = val;
        }
        raw[i] = full_reg.val;
    } else if (regattr_is_16(i)){ // 16, but actually now is just 1 val
        u16 lo = val;
        if (regattr_is_u(i)){
            raw[i] = lo;
        } else {
            raw[i] = static_cast<s16>(lo);
        }

        if (r == IRGB){
            calc_IRGB();
        }
    } else {
        raw[i] = val;

        if (i == FLAG){
            calc_FLAG();
        }
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

template <GTE::LimType T, u8 V>
u64 GTE::lim(u64 x){
    s64 l=0, r;
    if constexpr (T == AS || T == AS_SF){
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

    // as_sf: see gte rtps...
    s64 x_s = x;
    if constexpr (T == AS_SF){
        x_s >>= 12;
    }

    s64 val = std::clamp(x_s, l, r);

    bool fail = false;
    if (val != x_s){
        fail = true;
    }

    if constexpr (T == AS_SF){
        val = std::clamp(static_cast<s64>(x), l, r);
    }

    if (!fail){
        return val;
    }

    if constexpr (T == AS || T == AS_SF || T == AU){
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

template <GTE::LimType L>
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

#define READ(r) \
    m_regs.read64(Regs::r)

#define WRITE(r, ...) \
    m_regs.write(Regs::r, (__VA_ARGS__))

#define SF_SHIFT(x) (x ? 12 : 0)

#define SHIFT_SZ2() \
    WRITE(SZ0, READ(SZ1)); \
    WRITE(SZ1, READ(SZ2))

#define SHIFT_RGB2() \
    WRITE(RGB0, READ(RGB1)); \
    WRITE(RGB1, READ(RGB2));

#define PUSH_COLOR(r, g, b) \
    SHIFT_RGB2(); \
    WRITE(R2, lim<B, 1>(r)); \
    WRITE(G2, lim<B, 2>(g)); \
    WRITE(B2, lim<B, 3>(b)); \
    WRITE(C2, READ(C));

#define REG(r) \
    const u64 r = READ(r)

#define MAC_INTO_IR(lm) \
    if (lm == LM_NEG){ \
        WRITE(IR1, lim<AS, 1>(READ(MAC1))); \
        WRITE(IR2, lim<AS, 2>(READ(MAC2))); \
        WRITE(IR3, lim<AS, 3>(READ(MAC3))); \
    } else { \
        WRITE(IR1, lim<AU, 1>(READ(MAC1))); \
        WRITE(IR2, lim<AU, 2>(READ(MAC2))); \
        WRITE(IR3, lim<AU, 3>(READ(MAC3))); \
    }

#define WRITE_MAC1(x) \
    WRITE(MAC1, calc_test<1>((x)))

#define WRITE_MAC2(x) \
    WRITE(MAC2, calc_test<2>((x)))

#define WRITE_MAC3(x) \
    WRITE(MAC3, calc_test<3>((x)))

// for reading mats
// wont need these in opcode impls
// since why aren't u using mvmva then
#define _READ_VEC_END(var, name, x, y, z); \
    var##1 = READ(name##x); \
    var##2 = READ(name##y); \
    var##3 = READ(name##z)

#define _READ_VEC_MID(var, name_l, name_r, x, y, z); \
    var##1 = READ(name_l##x##name_r); \
    var##2 = READ(name_l##y##name_r); \
    var##3 = READ(name_l##z##name_r)

#define _READ_VEC_BEG(var, name, x, y, z); \
    var##1 = READ(x##name); \
    var##2 = READ(y##name); \
    var##3 = READ(z##name)

#define _READ_ROW(var, name) \
    do { \
        var##1 = READ(name##1); \
        var##2 = READ(name##2); \
        var##3 = READ(name##3); \
    } while (0)

#define _READ_MAT(m, r1, r2, r3) \
    do { \
        _READ_ROW(a1, m##r1); \
        _READ_ROW(a2, m##r2); \
        _READ_ROW(a3, m##r3); \
    } while (0)



constexpr void GTE::mvmva(u8 sf, u8 mx, u8 v, u8 cv, u8 lm, bool rtp){
    u64 a11, a12, a13, a21, a22, a23, a31, a32, a33; // mat
    u64 b1, b2, b3; // vec
    u64 c1, c2, c3; // const vec

    switch (mx){
        case MX_R:
            _READ_MAT(R, 1, 2, 3);
            break;
        case MX_L:
            _READ_MAT(L, 1, 2, 3);
            break;
        case MX_LR:
            _READ_MAT(L, R, G, B);
            break;
    }

    assert(sf == SF_LG || sf == SF_SM);

    switch (v){
        case V_V0:
            _READ_VEC_MID(b, V, 0, X, Y, Z);
            break;
        case V_V1:
            _READ_VEC_MID(b, V, 1, X, Y, Z);
            break;
        case V_V2:
            _READ_VEC_MID(b, V, 2, X, Y, Z);
            break;
        case V_IR:
            _READ_VEC_END(b, IR, 1, 2, 3);
            break;
    }

    switch (cv){
        case CV_TR:
            _READ_VEC_END(c, TR, X, Y, Z);
            break;
        case CV_BK:
            _READ_VEC_BEG(c, BK, R, G, B);
            break;
        case CV_Z:
            c1 = 0; c2 = 0; c3 = 0;
            break;
        default:
            throw Panic("unknown cv");
    }

    if (lm != LM_NEG && lm != LM_ZERO){
        throw Panic("unknown lm");
    }

    WRITE_MAC1(((c1 << 12) + a11*b1 + a12*b2 + a13*b3) >> SF_SHIFT(sf));
    WRITE_MAC2(((c2 << 12) + a21*b1 + a22*b2 + a23*b3) >> SF_SHIFT(sf));
    WRITE_MAC3(((c3 << 12) + a31*b1 + a32*b2 + a33*b3) >> SF_SHIFT(sf));

    if (lm == LM_NEG){ 
        WRITE(IR1, lim<AS, 1>(READ(MAC1)));
        WRITE(IR2, lim<AS, 2>(READ(MAC2)));
        if (rtp){
            WRITE(IR3, lim<AS_SF, 3>(READ(MAC3)));//FUCK!!! can't use he macro
        } else {
            WRITE(IR3, lim<AS, 3>(READ(MAC3)));
        }
    } else { 
        WRITE(IR1, lim<AU, 1>(READ(MAC1)));
        WRITE(IR2, lim<AU, 2>(READ(MAC2)));
        WRITE(IR3, lim<AU, 3>(READ(MAC3)));
    }
}

constexpr void GTE::rtp(u8 sf, u8 v){
    mvmva(sf, MX_R, v, CV_TR, LM_NEG, true);

    SHIFT_SZ2();
    WRITE(SZ2, READ(MAC3) >> (12 - SF_SHIFT(sf)));

    REG(OFX); 
    REG(OFY); 
    REG(IR1); 
    REG(IR2); 
    REG(SZ2); 
    REG(H); 
    REG(DQB); 
    REG(DQA); 
    u64 div_res = divide(H, SZ2); 
    u64 SX = calc_test<4>(OFX + IR1 * div_res); 
    u64 SY = calc_test<4>(OFY + IR2 * div_res); 
    u64 P = calc_test<4>(DQB + DQA * div_res); 
    WRITE(IR0, lim<E>(P)); 
 
    Pack16_32 sxy_new{};
    sxy_new.lo = lim<D, 1>(SX >> 16);
    sxy_new.hi = lim<D, 2>(SY >> 16);
    WRITE(SXYP, sxy_new.val);

    WRITE(MAC0, P); 
}

void GTE::op_RTPS([[maybe_unused]] Instr i){
    rtp(i.sf, V_V0);
}

void GTE::op_NCLIP(Instr i) {}
void GTE::op_OP(Instr i) {}
void GTE::op_DPCS(Instr i) {}
void GTE::op_INTPL(Instr i) {}

void GTE::op_MVMVA(Instr i){
    mvmva(i.sf, i.mx, i.v, i.cv, i.lm);
}

constexpr void GTE::ncd(u8 sf, u8 lm, u8 v){
    mvmva(sf, MX_L, v, CV_Z, lm);
    cdp(sf, lm);
}

void GTE::op_NCDS(Instr i){
    ncd(i.sf, i.lm, V_V0);
}

constexpr void GTE::cdp(u8 sf, u8 lm){
    mvmva(sf, MX_LR, V_IR, CV_BK, lm);

    WRITE_MAC1((READ(IR1) * READ(R)) << 4);
    WRITE_MAC2((READ(IR2) * READ(G)) << 4);
    WRITE_MAC3((READ(IR3) * READ(B)) << 4);

    const u64 m1 = READ(MAC1);
    const u64 m2 = READ(MAC2);
    const u64 m3 = READ(MAC3);
    REG(IR0);
    REG(RFC);
    REG(GFC);
    REG(BFC);
    WRITE_MAC1((m1 + 
                // this itself is probably another mac computation..
                // oh well
                // They don't show the shifting in psx-spx (or anywhere
                // for that matter for some reason)
                ((IR0*((RFC << 12) - m1)) >> SF_SHIFT(sf))
                ) >> SF_SHIFT(sf));
    WRITE_MAC2((m2 + 
                ((IR0*((GFC << 12) - m2)) >> SF_SHIFT(sf))
                ) >> SF_SHIFT(sf));
    WRITE_MAC3((m3 + 
                ((IR0*((BFC << 12) - m3)) >> SF_SHIFT(sf))
                ) >> SF_SHIFT(sf));

    PUSH_COLOR(READ(MAC1) >> 4, READ(MAC2) >> 4, READ(MAC3) >> 4);
    MAC_INTO_IR(lm);
}

void GTE::op_CDP(Instr i){
    cdp(i.sf, i.lm);
}

void GTE::op_NCDT(Instr i){
    ncd(i.sf, i.lm, V_V0);
    ncd(i.sf, i.lm, V_V1);
    ncd(i.sf, i.lm, V_V2);
}

constexpr void GTE::ncc(u8 sf, u8 lm, u8 v){
    mvmva(sf, MX_L, v, CV_Z, lm);
    cc(sf, lm);
}

void GTE::op_NCCS(Instr i) {
    ncc(i.sf, i.lm , V_V0);
}

constexpr void GTE::cc(u8 sf, u8 lm){
    mvmva(sf, MX_LR, V_IR, CV_BK, lm);

    WRITE_MAC1((READ(IR1) * READ(R)) << 4 >> SF_SHIFT(sf));
    WRITE_MAC2((READ(IR2) * READ(G)) << 4 >> SF_SHIFT(sf));
    WRITE_MAC3((READ(IR3) * READ(B)) << 4 >> SF_SHIFT(sf));

    PUSH_COLOR(READ(MAC1) >> 4, READ(MAC2) >> 4, READ(MAC3) >> 4);
    MAC_INTO_IR(lm);
}

void GTE::op_CC(Instr i) {
    cc(i.sf, i.lm);
}

constexpr void GTE::nc(u8 sf, u8 lm, u8 v){
    mvmva(sf, MX_L, v, CV_Z, lm);
    mvmva(sf, MX_LR, V_IR, CV_BK, lm);

    PUSH_COLOR(READ(MAC1), READ(MAC2), READ(MAC3));
}

void GTE::op_NCS(Instr i){
    nc(i.sf, i.lm, V_V0);
}

void GTE::op_NCT(Instr i) {
    nc(i.sf, i.lm, V_V0);
    nc(i.sf, i.lm, V_V1);
    nc(i.sf, i.lm, V_V2);
}

void GTE::op_SQR(Instr i) {}
void GTE::op_DCPL(Instr i) {}
void GTE::op_DPCT(Instr i) {}
void GTE::op_AVSZ3(Instr i) {}
void GTE::op_AVSZ4(Instr i) {}

void GTE::op_RTPT(Instr i) {
    rtp(i.sf, V_V0);
    rtp(i.sf, V_V1);
    rtp(i.sf, V_V2);
}

void GTE::op_GPF(Instr i) {}
void GTE::op_GPL(Instr i) {}

void GTE::op_NCCT(Instr i) {
    ncc(i.sf, i.lm , V_V0);
    ncc(i.sf, i.lm , V_V1);
    ncc(i.sf, i.lm , V_V2);
}


#undef REG 
#undef WRITE
#undef MAT_REGS
#undef VEC_REGS
#undef TR_VEC_REGS
#undef SHIFT_SZ2
#undef SHIFT_SXY2





}
