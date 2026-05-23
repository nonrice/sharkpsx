#pragma once

#include <array>

#include "types.hpp"
#include "BitField.hpp"

namespace pse {

// GTE design doc and psx-spx are extremely necessary 
// for understanding these!!!
class GTE {
public:
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


        union PackedReg {
            u32 val;
            bf32<0, 15> lo;
            bf32<16, 31> hi;
        };

        union RGBReg {
            u32 val;
            bf32<0, 4> r;
            bf32<5, 9> g;
            bf32<10, 14> b;
            bf32<15, 31> rest;
        };

        // We define the conventional mnemonics
        // Including those for packed regs, e.g. VX0, VY0. Flags indicates packing,
        // but index can be masked out.
        static constexpr u32 REGNAME_IND_MASK = 0x3F;
        static constexpr u32 REGNAME_PACK = 1 << 6;
        static constexpr u32 REGNAME_PACK_IS_HI = 1 << 7;
        static constexpr u32 REGNAME_HI = REGNAME_PACK_IS_HI | REGNAME_PACK;
        static constexpr u32 REGNAME_LO = REGNAME_PACK;
        enum RegName : u8 {
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
            RGB1 = 21,
            RGB2 = 22,
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
        constexpr bool regname_is_pack(RegName r);
        constexpr bool regname_is_lo(RegName r);
        constexpr bool regname_is_hi(RegName r);

        // Registers have certain attributes, these are defined and looked up by index
        // Importantly: 16 is diff from regname_lo. Attributes are for the *whole* register
        // while regname_lo just indicates an alias. For the upper empty bits, the spec sometimes
        // require the whole reg has consistent sign, so that is specified here.
        static constexpr u32 REGATTR_U = 1; // otherwise, will sign extend
        static constexpr u32 REGATTR_NOWR = 1 << 1; // exc with wonly
        static constexpr u32 REGATTR_NORD = 1 << 2;
        static constexpr u32 REGATTR_16 = 1 << 3; // actual value only low16
        static constexpr std::array<u8, NUM_REGS> attrs = {{
            [VZ0] = REGATTR_16,
            [VZ1] = REGATTR_16,
            [VZ2] = REGATTR_16,
            [RGBC] = REGATTR_U,
            [IR0] = REGATTR_16,
            [IR1] = REGATTR_16,
            [IR2] = REGATTR_16,
            [IR3] = REGATTR_16,
            [OTZ] = REGATTR_16 | REGATTR_U | REGATTR_NOWR,
            [SXYP] = REGATTR_NORD,
            [SZX] = REGATTR_16 | REGATTR_U,
            [SZ0] = REGATTR_16 | REGATTR_U,
            [SZ1] = REGATTR_16 | REGATTR_U,
            [SZ2] = REGATTR_16 | REGATTR_U,
            [RGB0] = REGATTR_U,
            [RGB1] = REGATTR_U,
            [RGB2] = REGATTR_U,
            [MAC0] = REGATTR_NOWR,
            [IRGB] = REGATTR_16 | REGATTR_U,
            [ORGB] = REGATTR_16 | REGATTR_U,
            [LZCS] = REGATTR_NORD,
            [LZCR] = REGATTR_NOWR,
            [R33] = REGATTR_16,
            [L33] = REGATTR_16,
            [LB3] = REGATTR_16,
            [H] = REGATTR_16,// so they litearlly lied that this is unsigned 
            [DQA] = REGATTR_16,
            [ZSF3] = REGATTR_16,
            [ZSF4] = REGATTR_16,
            [FLAG] = REGATTR_U | REGATTR_NOWR
        }};
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

    union Instr {
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
        AS_SF,
        AU,
        B,
        C,
        D,
        E
    };
    // Truncate fractions when using limiters!
    // They operate only on integers
    template <LimiterType L, u8 V>
    u64 lim(u64 x);
    template <LimiterType L>
    u64 lim(u64 x);

    template <u8 T>
    u64 calc_test(u64 x);

    // H/SZ is the only division, so this just implemetns that
    // Both are just u16
    u64 divide(u64 p, u64 q);

    using OpHandlerPtr = void (GTE::*)(Instr);
    static const std::array<OpHandlerPtr, 64> m_op_table;

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
