/*
 * Notable behaviors:
 *
 * Branch delay slot: Regardless of branch result, next instruction after branch is always executed. The branch occurs after. Two consecutive branches are not allowed.
 
 * Load delay:
 * Loads have a lag of 1 instruction. So the register doesn't update until after the next instruction.
 *
 * Multiplication/division:
 * These instructions do not complete in 1 cycle, but they do run concurrently. 
 *
 * Based on some specific number sizes, the number of cycles they take vary. Results only go into the HI/LO registers.
 *
 * If accessing one of those before the operation completes, the CPU stalls (nothing is performed each cycle) until the operation is done and results recorded
 *
 * Starting a new mult/div while one in progress invalidates the old one, and all subsequent behavior will be as if only the latest mult/div actually existed.
 *
 * Store delay:
 * After consulting PSX discord, this shouldn't be important for most games and can be assumed nonexistent
 *
 */

#pragma once

#include <cstdint>
#include <cassert>
#include <array>
#include <string>

#include "types.hpp"

namespace pse {

class Bus;

class CPU {
public:
    CPU(Bus* bus);

    void tick();
    void set_pc(u32 pc);

private:
    friend class Debugger;

    Bus* m_bus;

    struct Instr {
        u32 val;
        constexpr u8 primary_opcode() const noexcept {
            return static_cast<u8>(val >> 26);
        }

        constexpr u8 secondary_opcode() const noexcept {
            return static_cast<u8>(val & 0x3F);
        }

        constexpr u8 rs() const noexcept {
            return static_cast<u8>((val >> 21) & 0x1F);
        }

        constexpr u8 rt() const noexcept {
            return static_cast<u8>((val >> 16) & 0x1F);
        }

        constexpr u8 rd() const noexcept {
            return static_cast<u8>((val >> 11) & 0x1F);
        }

        constexpr u8 imm5() const noexcept {
            return static_cast<u8>((val >> 6) & 0x1F);
        }

        constexpr u16 imm16() const noexcept {
            return static_cast<u16>(val & 0xFFFF);
        }

        constexpr s16 imm16_signed() const noexcept {
            return static_cast<s16>(val & 0xFFFF);
        }

        constexpr u32 imm26() const noexcept {
            return val & 0x3FFFFFF;
        }

        constexpr s32 imm26_signed() const noexcept {
            return static_cast<s32>(val & 0x3FFFFFF);
        }
    };

    enum class Mode {
        User,
        Kernel
    };
    Mode m_mode;

    struct COP0 {
        static constexpr usize NUM_REGS = 64;
        std::array<u32, NUM_REGS> regs;

        enum RegName {
            BPC = 3,
            BDA = 5,
            TAR = 6,
            DCIC = 7,
            BadA = 8,
            BDAM = 9,
            BPCM = 11,
            SR = 12,
            CAUSE = 13,
            EPC = 14,
            PRID = 15
        };
    };
    COP0 m_cop0;

    enum ExcCode {
        OVF = 0x0c,
        RI = 0x0A,
        ADEL = 0x04,
        ADES = 0x05,
        SYS = 0x08
    };

    // op should immediately return after calling this
    void trigger_exception(ExcCode e);

    // the only cop0 command!!!
    void rfe();

    static constexpr usize NUM_REGS = 32;
    std::array<u32, NUM_REGS> m_regs;
    static constexpr usize REG_RA = 31;
    void reset_reg0();

    // we keep the actual pc of the cur instruction
    // is useful for exns and reasoning in general
    u32 m_cur_pc;
    u32 m_next_pc;
    void increment_pc();

    // set when in the bds, thus pc incr prior correctly sets NEXT pc
    // i guess technically also set when exiting the branch instr
    bool m_is_branching;
    u32 m_branch_pc;
    u32 calc_rel_branch_pc(s16 d);
    constexpr void set_branch(u32 branch_pc);

    u32 m_rem_halt;
    void halt_for(u32 cycles);

    // To handle register load delays, queue of 2
    struct Load {
        u32 data;
        usize reg;
        bool valid;
    };
    std::array<Load, 2> m_loads;
    void tick_load();
    void set_load(u32 data, usize reg);

    u32 m_hi, m_lo;
    u32 m_hi_buf, m_lo_buf;
    u32 m_multdiv_rem_cycles;
    bool m_multdiv_active;
    bool multdiv_ensure_halt();
    void tick_multdiv();

    // intercept BIOS putchar call to print to (our) stdout
    bool detect_putchar();

    void process_instr(u32 instr);

    // util for mem ops bc Instr is private... should it be?? idk
    u32 get_effective_addr(Instr i);

    using OpHandlerPtr = void (CPU::*)(CPU::Instr);
    static const std::array<CPU::OpHandlerPtr, 64> m_primary_op_table;
    static const std::array<CPU::OpHandlerPtr, 64> m_secondary_op_table;
 
    void op_BcondZ(Instr i);
    void op_J(Instr i);
    void op_JAL(Instr i);
    void op_BEQ(Instr i);
    void op_BNE(Instr i);
    void op_BLEZ(Instr i);
    void op_BGTZ(Instr i);
    void op_ADDI(Instr i);
    void op_ADDIU(Instr i);
    void op_SLTI(Instr i);
    void op_SLTIU(Instr i);
    void op_ANDI(Instr i);
    void op_ORI(Instr i);
    void op_XORI(Instr i);
    void op_LUI(Instr i);
    void op_COP0(Instr i);
    void op_COP1(Instr i);
    void op_COP2(Instr i);
    void op_COP3(Instr i);
    void op_LB(Instr i);
    void op_LH(Instr i);
    void op_LWL(Instr i);
    void op_LW(Instr i);
    void op_LBU(Instr i);
    void op_LHU(Instr i);
    void op_LWR(Instr i);
    void op_SB(Instr i);
    void op_SH(Instr i);
    void op_SWL(Instr i);
    void op_SW(Instr i);
    void op_SWR(Instr i);
    void op_LWC0(Instr i);
    void op_LWC1(Instr i);
    void op_LWC2(Instr i);
    void op_LWC3(Instr i);
    void op_SWC0(Instr i);
    void op_SWC1(Instr i);
    void op_SWC2(Instr i);
    void op_SWC3(Instr i);
    void op_SLL(Instr i);
    void op_SRL(Instr i);
    void op_SRA(Instr i);
    void op_SLLV(Instr i);
    void op_SRLV(Instr i);
    void op_SRAV(Instr i);
    void op_JR(Instr i);
    void op_JALR(Instr i);
    void op_SYSCALL(Instr i);
    void op_BREAK(Instr i);
    void op_MFHI(Instr i);
    void op_MTHI(Instr i);
    void op_MFLO(Instr i);
    void op_MTLO(Instr i);
    void op_MULT(Instr i);
    void op_MULTU(Instr i);
    void op_DIV(Instr i);
    void op_DIVU(Instr i);
    void op_ADD(Instr i);
    void op_ADDU(Instr i);
    void op_SUB(Instr i);
    void op_SUBU(Instr i);
    void op_AND(Instr i);
    void op_OR(Instr i);
    void op_XOR(Instr i);
    void op_NOR(Instr i);
    void op_SLT(Instr i);
    void op_SLTU(Instr i);   
};

}
