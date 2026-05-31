#pragma once

#include <array>

#include "types.hpp"
#include "BitField.hpp"

namespace pse {

// GTE design doc and psx-spx are extremely necessary 
// for understanding these!!!
//
// For naming we use the psq one, other docs may use different conventions,
// notably for the limiters.
class GTE {
public:
    GTE();

    // return the number of cycles to halt
    u32 get_rem_cycles();
    bool nothing_inflight();
    void tick();
    void to_ctrl(u32 i, u32 x);
    void to_data(u32 i, u32 x);
    u32 from_ctrl(u32 i);
    u32 from_data(u32 i);
    void cmd(u32 x);
    bool get_flag(u8 i);

private:
    struct Regs {
        static constexpr usize NUM_REGS = 64;
        std::array<u32, NUM_REGS> raw; 

        union RGBReg {
            u32 val;
            bf32<0, 4> r;
            bf32<5, 9> g;
            bf32<10, 14> b;
            bf32<15, 31> rest; 
        };

        union ColorReg {
            u32 val;
            bf32<0, 7> r;
            bf32<8, 15> g;
            bf32<16, 23> b;
            bf32<24, 31> c;
        };

        // We define the conventional mnemonics
        // Including those for packed regs, e.g. VX0, VY0. Flags indicates packing,
        // but index can be masked out.
        static constexpr u32 REGNAME_IND_MASK = 0x3F;
        static constexpr u32 REGNAME_PACK16 = 1 << 6;
        static constexpr u32 REGNAME_PACK_IS_HI = 1 << 7;
        static constexpr u32 REGNAME_HI = REGNAME_PACK_IS_HI | REGNAME_PACK16;
        static constexpr u32 REGNAME_LO = REGNAME_PACK16;
        static constexpr u32 REGNAME_PACK8 = 1 << 8;
        static constexpr u32 REGNAME_A = REGNAME_PACK8;
        static constexpr u32 REGNAME_B = REGNAME_PACK8 | (1 << 9);
        static constexpr u32 REGNAME_C = REGNAME_PACK8 | (2 << 9);
        static constexpr u32 REGNAME_D = REGNAME_PACK8 | (3 << 9);


        enum RegName : u32 {
            VXY0 = 0,
            VX0 = 0 | REGNAME_LO, VY0 = 0 | REGNAME_HI,
            VZ0 = 1, 
            VXY1 = 2,
            VX1 = 2 | REGNAME_LO, VY1 = 2 | REGNAME_HI,
            VZ1 = 3,
            VXY2 = 4,
            VX2 = 4 | REGNAME_LO, VY2 = 4 | REGNAME_HI,
            VZ2 = 5,
            RGBC = 6,
            R = 6 | REGNAME_A, G = 6 | REGNAME_B, B = 6 | REGNAME_C, C = 6 | REGNAME_D,  
            OTZ = 7,
            IR0 = 8,
            IR1 = 9,
            IR2 = 10,
            IR3 = 11,
            SXY0 = 12,
            SX0 = 12 | REGNAME_LO, SY0 = 12 | REGNAME_HI,
            SXY1 = 13,
            SX1 = 13 | REGNAME_LO, SY1 = 13 | REGNAME_HI,
            SXY2 = 14,
            SX2 = 14 | REGNAME_LO, SY2 = 14 | REGNAME_HI,
            SXYP = 15,
            SXP = 15 | REGNAME_LO, SYP = 15 | REGNAME_HI,
            SZX = 16, // DO NOT USE SZX/SZ1/... naming
            SZ0 = 17,
            SZ1 = 18,
            SZ2 = 19,
            RGB0 = 20,
            R0 = 20 | REGNAME_A, G0 = 20 | REGNAME_B, B0 = 20 | REGNAME_C, C0 = 20 | REGNAME_D,  
            RGB1 = 21,
            R1 = 21 | REGNAME_A, G1 = 21 | REGNAME_B, B1 = 21 | REGNAME_C, C1 = 21 | REGNAME_D,  
            RGB2 = 22,
            R2 = 22 | REGNAME_A, G2 = 22 | REGNAME_B, B2 = 22 | REGNAME_C, C2 = 22 | REGNAME_D,  
            //23 is unused
            MAC0 = 24,
            MAC1 = 25,
            MAC2 = 26,
            MAC3 = 27,
            IRGB = 28,
            ORGB = 29,
            LZCS = 30,
            LZCR = 31,
            // begin control regs
            // true regs for mats (e.g. R11R12) omitted because... i don't want to
            // and no one uses them
            R11 = 32 | REGNAME_LO, R12 = 32 | REGNAME_HI,
            R13 = 33 | REGNAME_LO, R21 = 33 | REGNAME_HI,
            R22 = 34 | REGNAME_LO, R23 = 34 | REGNAME_HI,
            R31 = 35 | REGNAME_LO, R32 = 35 | REGNAME_HI,
            R33 = 36,
            TRX = 37,
            TRY = 38,
            TRZ = 39,
            L11 = 40 | REGNAME_LO, L12 = 40 | REGNAME_HI,
            L13 = 41 | REGNAME_LO, L21 = 41 | REGNAME_HI,
            L22 = 42 | REGNAME_LO, L23 = 42 | REGNAME_HI,
            L31 = 43 | REGNAME_LO, L32 = 43 | REGNAME_HI,
            L33 = 44,
            RBK = 45,
            GBK = 46,
            BBK = 47,
            LR1 = 48 | REGNAME_LO, LR2 = 48 | REGNAME_HI,
            LR3 = 49 | REGNAME_LO, LG1 = 49 | REGNAME_HI,
            LG2 = 50 | REGNAME_LO, LG3 = 50 | REGNAME_HI,
            LB1 = 51 | REGNAME_LO, LB2 = 51 | REGNAME_HI,
            LB3 = 52,
            RFC = 53,
            GFC = 54,
            BFC = 55,
            OFX = 56,
            OFY = 57,
            H = 58,
            DQA = 59,
            DQB = 60,
            ZSF3 = 61,
            ZSF4 = 62,
            FLAG = 63
        };
        constexpr u8 regname_ind(RegName r);
        constexpr bool regname_is_pack16(RegName r);
        constexpr bool regname_is_lo(RegName r);
        constexpr bool regname_is_hi(RegName r);
        constexpr bool regname_is_pack8(RegName r);
        constexpr bool regname_is_a(RegName r);
        constexpr bool regname_is_b(RegName r);
        constexpr bool regname_is_c(RegName r);
        constexpr bool regname_is_d(RegName r);

        // Registers have certain attributes, these are defined and looked up by index
        // Importantly: 16 is diff from regname_lo. Attributes are for the *whole* register
        // while regname_lo just indicates an alias. For the upper empty bits, the spec sometimes
        // require the whole reg has consistent sign, so that is specified here.
        static constexpr u32 REGATTR_U = 1; // otherwise, will sign extend
        static constexpr u32 REGATTR_NOWR = 1 << 1; // exc with wonly
        static constexpr u32 REGATTR_NORD = 1 << 2;
        static constexpr u32 REGATTR_16 = 1 << 3; // actual value only low16
        static constexpr std::array<u8, NUM_REGS> attrs = []{
            std::array<u8, NUM_REGS> a{};
            a[VZ0] = REGATTR_16;
            a[VZ1] = REGATTR_16;
            a[VZ2] = REGATTR_16;
            a[RGBC] = REGATTR_U;
            a[IR0] = REGATTR_16;
            a[IR1] = REGATTR_16;
            a[IR2] = REGATTR_16;
            a[IR3] = REGATTR_16;
            a[OTZ] = REGATTR_16 | REGATTR_U | REGATTR_NOWR;
            a[SXYP] = REGATTR_NORD;
            a[SZX] = REGATTR_16 | REGATTR_U;
            a[SZ0] = REGATTR_16 | REGATTR_U;
            a[SZ1] = REGATTR_16 | REGATTR_U;
            a[SZ2] = REGATTR_16 | REGATTR_U;
            a[RGB0] = REGATTR_U;
            a[RGB1] = REGATTR_U;
            a[RGB2] = REGATTR_U;
            a[MAC0] = REGATTR_NOWR;
            a[IRGB] = REGATTR_16 | REGATTR_U;
            a[ORGB] = REGATTR_16 | REGATTR_U;
            a[LZCS] = REGATTR_NORD;
            a[LZCR] = REGATTR_NOWR;
            a[R33] = REGATTR_16;
            a[L33] = REGATTR_16;
            a[LB3] = REGATTR_16;
            // So; any read write to H externally IS signed;
            // but internally; which just means when H is used for division;
            // it is NOT sign extended when read.
            a[H] = REGATTR_16; 
            a[DQA] = REGATTR_16;
            a[ZSF3] = REGATTR_16;
            a[ZSF4] = REGATTR_16;
            a[FLAG] = REGATTR_U | REGATTR_NOWR;
            return a;
        }();
        constexpr bool regattr_is_u(u8 i);
        constexpr bool regattr_can_read(u8 i); // more useful than is_ronly
        constexpr bool regattr_can_write(u8 i);
        constexpr bool regattr_is_16(u8 i);


        // typical method for interacting with flag
        constexpr bool get_flag(u8 i);
        constexpr void set_flag(u8 i, bool val);

        // reads and writes may have side effects
        constexpr u32 read(RegName r);
        // Always use read64 in opcode implementation (it's just a simple wrapper), for math
        // This way, no manual casting logic is required
        constexpr u64 read64(RegName r);
        constexpr void write(RegName r, u32 val);

        // rw sideffs, these are literally baked into register wirings
        constexpr void shift_SXYP();
        constexpr void calc_IRGB();
        constexpr void calc_ORGB();
        constexpr void calc_LZCR();
        constexpr void calc_FLAG();
    };

    Regs m_regs;
    bool m_instr_inflight;
    u32 m_instr_rem_cycles;
    u32 m_instr_val;

    union Instr {
        u32 val;

        bf32<0, 5> opcode;
        bf32<10, 10> lm;
        bf32<13, 14> cv;
        bf32<15, 16> v;
        bf32<17, 18> mx;
        bf32<19, 19> sf;

    }; 
    
    enum LimType {
        AS,
        AS_SF,
        AU,
        B,
        C,
        D,
        E
    };
    // Truncate fractions when using limiters!
    // They operate only on integers
    template <LimType L, u8 V>
    u64 lim(u64 x);
    template <LimType L>
    u64 lim(u64 x);

    template <u8 T>
    u64 calc_test(u64 x);

    // H/SZ is the only division, so this just implemetns that
    // Both are just u16
    u64 divide(u64 p, u64 q);

    enum Lm {
        LM_NEG = 0,
        LM_ZERO = 1
    };
    enum Mx {
        MX_R = 0,
        MX_L = 1,
        MX_LR = 2
    };
    enum Vec {
        V_V0 = 0,
        V_V1 = 1,
        V_V2 = 2,
        V_IR = 3
    };
    enum Cv {
        CV_TR = 0,
        CV_BK = 1,
        CV_Z = 3
    };
    enum Sf : u32 {
        SF_LG = 0,
        SF_SM = 1
    };
    constexpr void mvmva(u8 sf, u8 mx, u8 v, u8 cv, u8 lm, bool rtp = false);

    using OpHandlerPtr = void (GTE::*)(Instr);
    static const std::array<OpHandlerPtr, 64> m_op_table;
    static const std::array<u32, 64> m_op_times;

    void process_instr(u32 val);


    constexpr void rtp(u8 sf, u8 v); // extra v arg so rtpt easy
    void op_RTPS(Instr i);
    void op_NCLIP(Instr i);
    void op_OP(Instr i);
    constexpr void intpl_common(u8 sf, u8 lm);
    constexpr void dpc(u8 sf, u8 lm, bool use_rgb0 = false);
    void op_DPCS(Instr i);
    void op_INTPL(Instr i);
    void op_MVMVA(Instr i);
    constexpr void ncd(u8 sf, u8 lm, u8 v);
    void op_NCDS(Instr i);
    constexpr void cdp(u8 sf, u8 lm);
    void op_CDP(Instr i);
    void op_NCDT(Instr i);
    constexpr void ncc(u8 sf, u8 lm, u8 v);
    void op_NCCS(Instr i);
    constexpr void cc(u8 sf, u8 lm);
    void op_CC(Instr i);
    constexpr void nc(u8 sf, u8 lm, u8 v);
    void op_NCS(Instr i);
    void op_NCT(Instr i);
    void op_SQR(Instr i);
    constexpr void dcpl(u8 sf, u8 lm);
    void op_DCPL(Instr i);
    void op_DPCT(Instr i);
    void op_AVSZ3(Instr i);
    void op_AVSZ4(Instr i);
    void op_RTPT(Instr i);
    void op_GPF(Instr i);
    void op_GPL(Instr i);
    void op_NCCT(Instr i);
};

}
