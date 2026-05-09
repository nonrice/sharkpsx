#pragma once

#include <array>

#include "types.hpp"
#include "BitField.hpp"

namespace pse {

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

        static constexpr u32 REGNAME_PACK16 = 1 << 9;
        static constexpr u32 REGNAME_PACK16_IS_HI = 1 << 10;
        static constexpr u32 REGNAME_PACK16_HI = REGNAME_PACK16_IS_HI | REGNAME_PACK16;
        static constexpr u32 REGNAME_PACK16_LO = REGNAME_PACK16;

        enum RegName : u32 {
            VXY0 = 0,
            VX0 = 0 | REGNAME_PACK16_LO, VY0 = 0 | REGNAME_PACK16_HI,
            VZ0 = 1,
            VX1 = 2 | REGNAME_PACK16_LO, VY1 = 2 | REGNAME_PACK16_HI,
            VZ1 = 3,
            VX2 = 4 | REGNAME_PACK16_LO, VY2 = 4 | REGNAME_PACK16_HI,
            VZ2 = 5,
            RGBC = 6,
            OTZ = 7,
            IR0 = 8,
            IR1 = 9,
            IR2 = 10,
            IR3 = 11,
            SXY0 = 12,
            SX0 = 12 | REGNAME_PACK16_LO, SY0 = 12 | REGNAME_PACK16_HI,
            SXY1 = 13,
            SX1 = 13 | REGNAME_PACK16_LO, SY1 = 13 | REGNAME_PACK16_HI,
            SXY2 = 14,
            SX2 = 14 | REGNAME_PACK16_LO, SY2 = 14 | REGNAME_PACK16_HI,
            SXYP = 15,
            SXP = 15 | REGNAME_PACK16_LO, SYP = 15 | REGNAME_PACK16_HI,
            SZ0 = 16,
            SZ1 = 17,
            SZ2 = 18,
            SZ3 = 19,
            RGB0 = 20,
            RGB1 = 21,
            RGB2 = 22,
            // 23 is unused
            MAC0 = 24,
            MAC1 = 25,
            MAC2 = 26,
            MAC3 = 27,
            IRGB = 28,
            ORGB = 29,
            LZCS = 30,
            LZCR = 31,
            // begin control regs
            RT11 = 32 | REGNAME_PACK16_LO, RT12 = 32 | REGNAME_PACK16_HI,
            RT13 = 33 | REGNAME_PACK16_LO, RT21 = 33 | REGNAME_PACK16_HI,
            RT22 = 34 | REGNAME_PACK16_LO, RT23 = 34 | REGNAME_PACK16_HI,
            RT31 = 35 | REGNAME_PACK16_LO, RT32 = 35 | REGNAME_PACK16_HI,
            RT33 = 36,
            TRX = 37,
            TRY = 38,
            TRZ = 39,
            L11 = 40 | REGNAME_PACK16_LO, L12 = 40 | REGNAME_PACK16_HI,
            L13 = 41 | REGNAME_PACK16_LO, L21 = 41 | REGNAME_PACK16_HI,
            L22 = 42 | REGNAME_PACK16_LO, L23 = 42 | REGNAME_PACK16_HI,
            L31 = 43 | REGNAME_PACK16_LO, L32 = 43 | REGNAME_PACK16_HI,
            L33 = 44,
            RBK = 45,
            GBK = 46,
            BBK = 47,
            LR1 = 48 | REGNAME_PACK16_LO, LR2 = 48 | REGNAME_PACK16_HI,
            LR3 = 49 | REGNAME_PACK16_LO, LG1 = 49 | REGNAME_PACK16_HI,
            LG2 = 50 | REGNAME_PACK16_LO, LG3 = 50 | REGNAME_PACK16_HI,
            LB1 = 51 | REGNAME_PACK16_LO, LB2 = 51 | REGNAME_PACK16_HI,
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
        
        union Flag {
            u32 val;

            // naming comes from the psy q doc!!
            b32<12> ir0_sat;
            b32<13> sy2_sat;
            b32<14> sx2_sat;
            b32<15> mac0_neg;
            b32<16> mac0_pos;
            b32<17> div_ovf;
            b32<18> sz3_otz_sat;
            b32<19> r_sat;
            b32<20> g_sat;
            b32<21> b_sat;
            b32<22> ir3_sat;
            b32<23> ir2_sat;
            b32<24> ir1_sat;
            b32<25> mac3_neg;
            b32<26> mac2_neg;
            b32<27> mac1_neg;
            b32<28> mac3_pos;
            b32<29> mac2_pos;
            b32<30> mac1_pos;
            b32<31> err;
        };

        constexpr void shift_SXY();
        constexpr void shift_SZ();

        constexpr u32 read(RegName r);
        // does have special push queue sideeff for SX/YP, hence write
        constexpr void write(RegName r, u32 val);
    };

    struct Instr {
        u32 val;

        bf32<0, 5> opcode;
        bf32<10, 10> lm;
        bf32<13, 14> trans_vec;
        bf32<15, 16> mult_vec;
        bf32<17, 18> mult_mat;
        bf32<19, 19> sf;
    };

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
