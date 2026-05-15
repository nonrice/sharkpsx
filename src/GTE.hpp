#pragma once

#include <array>

#include "types.hpp"
#include "BitField.hpp"

namespace pse {

// the GTE design doc is very necessary for understanding naming, etc here
class GTE {
public:

private:
    struct Regs {
        static constexpr usize NUM_REGS = 64;
        std::array<u32, NUM_REGS> raw; 

        union Pack16 {
            u32 val;
            bf32<0, 15> lo;
            bf32<16, 31> hi;
        };


        static constexpr u32 REGNAME_IND_MASK = 0xFF;
        static constexpr u32 REGNAME_16 = 1 << 9;
        static constexpr u32 REGNAME_16_IS_HI = 1 << 10;
        static constexpr u32 REGNAME_16HI = REGNAME_16_IS_HI | REGNAME_16;
        static constexpr u32 REGNAME_16LO = REGNAME_16;
        // there are more signed regs than unsigned
        // Since we do math in u64, these determine when to sign extend
        // Math is done in u64 because some results will go over 2^32
        // this way, can can have absolutely no casting in opcode implementations
        // which should be nicer since there are a ton of things to implement
        // casting would be yet another source of bugs
        static constexpr u32 REGNAME_U = 1 << 11; 
        static constexpr u32 REGNAME_RONLY = 1 << 12;
        static constexpr u32 REGNAME_WONLY = 1 << 13;


        enum RegName : u32 {
            VXY0 = 0,
            VX0 = 0 | REGNAME_16LO, VY0 = 0 | REGNAME_16HI,
            VZ0 = 1 | REGNAME_16LO,
            VXY1 = 2,
            VX1 = 2 | REGNAME_16LO, VY1 = 2 | REGNAME_16HI,
            VZ1 = 3 | REGNAME_16LO,
            VXY2 = 4,
            VX2 = 4 | REGNAME_16LO, VY2 = 4 | REGNAME_16HI,
            VZ2 = 5 | REGNAME_16LO,
            RGBC = 6 | REGNAME_U,
            OTZ = 7 | REGNAME_16LO | REGNAME_U | REGNAME_RONLY,
            IR0 = 8 | REGNAME_16LO,
            IR1 = 9 | REGNAME_16LO,
            IR2 = 10 | REGNAME_16LO,
            IR3 = 11 | REGNAME_16LO,
            SXY0 = 12,
            SX0 = 12 | REGNAME_16LO, SY0 = 12 | REGNAME_16HI,
            SXY1 = 13,
            SX1 = 13 | REGNAME_16LO, SY1 = 13 | REGNAME_16HI,
            SXY2 = 14,
            SX2 = 14 | REGNAME_16LO, SY2 = 14 | REGNAME_16HI,
            SXYP = 15 | REGNAME_WONLY,
            SXP = 15 | REGNAME_16LO | REGNAME_WONLY, SYP = 15 | REGNAME_16HI | REGNAME_WONLY,
            SZ0 = 16 | REGNAME_16LO | REGNAME_U,
            SZ1 = 17 | REGNAME_16LO | REGNAME_U,
            SZ2 = 18 | REGNAME_16LO | REGNAME_U,
            SZ3 = 19 | REGNAME_16LO | REGNAME_U,
            RGB0 = 20 | REGNAME_U,
            RGB1 = 21 | REGNAME_U,
            RGB2 = 22 | REGNAME_U,
            // 23 is unused
            MAC0 = 24 | REGNAME_RONLY,
            MAC1 = 25,
            MAC2 = 26,
            MAC3 = 27,
            IRGB = 28 | REGNAME_16LO | REGNAME_U,
            ORGB = 29 | REGNAME_16LO | REGNAME_U,
            LZCS = 30 | REGNAME_WONLY,
            LZCR = 31 | REGNAME_RONLY,
            // begin control regs
            RT11 = 32 | REGNAME_16LO, RT12 = 32 | REGNAME_16HI,
            RT13 = 33 | REGNAME_16LO, RT21 = 33 | REGNAME_16HI,
            RT22 = 34 | REGNAME_16LO, RT23 = 34 | REGNAME_16HI,
            RT31 = 35 | REGNAME_16LO, RT32 = 35 | REGNAME_16HI,
            RT33 = 36 | REGNAME_16LO,
            TRX = 37,
            TRY = 38,
            TRZ = 39,
            L11 = 40 | REGNAME_16LO, L12 = 40 | REGNAME_16HI,
            L13 = 41 | REGNAME_16LO, L21 = 41 | REGNAME_16HI,
            L22 = 42 | REGNAME_16LO, L23 = 42 | REGNAME_16HI,
            L31 = 43 | REGNAME_16LO, L32 = 43 | REGNAME_16HI,
            L33 = 44 | REGNAME_16LO,
            RBK = 45,
            GBK = 46,
            BBK = 47,
            LR1 = 48 | REGNAME_16LO, LR2 = 48 | REGNAME_16HI,
            LR3 = 49 | REGNAME_16LO, LG1 = 49 | REGNAME_16HI,
            LG2 = 50 | REGNAME_16LO, LG3 = 50 | REGNAME_16HI,
            LB1 = 51 | REGNAME_16LO, LB2 = 51 | REGNAME_16HI,
            LB3 = 52 | REGNAME_16LO,
            RFC = 53,
            GFC = 54,
            BFC = 55,
            OFX = 56,
            OFY = 57,
            H = 58 | REGNAME_16LO | REGNAME_U,
            DQA = 59 | REGNAME_16LO,
            DQB = 60,
            ZSF3 = 61 | REGNAME_16LO,
            ZSF4 = 62 | REGNAME_16LO,
            FLAG = 63 | REGNAME_U
        };
        constexpr u32 regname_ind(RegName r);
        constexpr bool regname_is_16(RegName r);
        constexpr bool regname_is_16lo(RegName r);
        constexpr bool regname_is_16hi(RegName r);
        constexpr bool regname_is_u(RegName r);
        constexpr bool regname_eq(RegName a, RegName b);

        // direct interface for raw array
        // NO side effects, literally just raw[i & 0xFF] (because we use upper
        // bits for attributes)
        constexpr void set(RegName r, u32 val);
        constexpr u32 get(RegName r);

        constexpr bool get_flag(u8 i);
        constexpr void set_flag(u8 i, bool val);

        // TODO perm/noperm variants?
        // reads and writes may have side effects
        constexpr u32 read(RegName r);
        // Always use read64 in opcode implementation, for math
        // This way, no manual casting logic is required
        constexpr u64 read64(RegName r);
        constexpr void write(RegName r, u32 val);

        // rw sideffs
        constexpr void shift_SXYP();
        constexpr void calc_IRGB();
        constexpr void calc_ORGB();
        constexpr void calc_LZCS();

        // these are  NOT a builtin sideff, must call
        constexpr void shift_SXY2();
        constexpr void shift_SZ3();
        constexpr void shift_RGBC();
    };

    Regs m_regs;

    struct Instr {
        u32 val;

        bf32<0, 5> opcode;
        bf32<10, 10> lm;
        bf32<13, 14> trans_vec;
        bf32<15, 16> mult_vec;
        bf32<17, 18> mult_mat;
        bf32<19, 19> sf;
    };

    enum LimiterType {
        AS,
        AU,
        B,
        C,
        D,
        E
    };

    template <LimiterType L, u8 V>
    u64 apply_lim(u64 x);

    using OpHandlerPtr = void (GTE::*)(Instr);
    static const std::array<OpHandlerPtr, 32> m_op_table;

    void process_instr(u32 val);

    void op_RTPS(Instr i);
    void op_NCLIP(Instr i);
    void op_OP(Instr i);
    void op_DPCS(Instr i);
    void op_INTPL(Instr i);
    void op_MVMVA(Instr i);
    void op_NCDS(Instr i);
    void op_CDP(Instr i);
    void op_NCDT(Instr i);
    void op_NCCS(Instr i);
    void op_CC(Instr i);
    void op_NCS(Instr i);
    void op_NCT(Instr i);
    void op_SQR(Instr i);
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
