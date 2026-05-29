/*
 * Notable behaviors:
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
#include <optional>

#include "types.hpp"
#include "BitField.hpp"
#include "DummyDevice.hpp"
#include "GTE.hpp"

namespace pse {

class Bus;

class CPU {
public:
    CPU(Bus* bus);

    void tick();
    void set_pc(u32 pc);

    // necessary for debugging UGH!@!
    union Instr {
        u32 val;
        bf32<26, 31> primary;
        bf32<21, 25> rs;
        bf32<21, 25> cop_primary;
        bf32<16, 20> rt;
        bf32<11, 15> rd;
        bf32<6, 10> imm5;
        bf32<0, 5> secondary;
        bf32<0, 15> imm16;
        bf32<0, 25> imm26;
    };


private:
    friend class BasicDebug;

    Bus* m_bus;
    DummyDevice m_dummy_dev;//use as icache

    GTE m_gte;
    bool gte_ensure_halt();

    struct COP0 {
        static constexpr usize NUM_REGS = 64;

        enum Reg : usize {
            BPC = 3,
            BDA = 5,
            BADA = 6,
            SR = 12,
            CAUSE = 13,
            EPC = 14,
            PRID = 15
        };

        union Cause {
            u32 val;
            bf32<2, 6> exc_code;
            bf32<8, 9> sw;
            bf32<10, 15> ip;
            bf32<28, 29> ce;
            bf32<31, 31> bd;
        };

        union Status {
            u32 val;
            bf32<0, 0> iec;
            bf32<1, 1> kuc;
            bf32<2, 2> iep;
            bf32<3, 3> kup;
            bf32<4, 4> ieo;
            bf32<5, 5> kuo;
            bf32<8, 15> im;
            bf32<16, 16> isc;
            bf32<22, 22> bev;
        };

        enum Ie : bool {
            DISABLE = 0,
            ENABLE = 1
        };
        enum Ku : bool {
            KERNEL = 0,
            USER = 1
        };
        void push_system_state(Ie ie, Ku ku);
        void pop_system_state(); //result is never used lmao

        std::array<u32, NUM_REGS> regs;
    };

    COP0 m_cop0;

    enum ExcCode : u32{
        INT = 0x00,
        OVF = 0x0c,
        RI = 0x0A,
        ADEL = 0x04,
        ADES = 0x05,
        SYS = 0x08,
        BP = 0x09
    };

    // op should immediately return after calling this
    void trigger_exception(ExcCode e, std::optional<u32> bad_addr = std::nullopt);

    // cop0 methods
    void rfe();

    // regs blah
    static constexpr usize NUM_REGS = 32;
    enum Reg : usize {
        RA = 31,
        SP = 29
    };
    std::array<u32, NUM_REGS> m_regs;
    void reset_reg0();
    bool m_reg_pending_write;
    u32 m_reg_pending_buf;
    u32 m_reg_pending_num;
    // in general case, you MUST use this to mutate regs
    // this supports load canceling which supposedly, is frequent
    void write_reg(u32 num, u32 val);
    void tick_reg_writeback();
    

    // we keep the actual pc of the cur instruction
    // is useful for exns and reasoning in general
    u32 m_cur_pc;
    u32 m_next_pc;
    void increment_pc();

    // Important behavior-Branch delay slot: the next instruction after a branch *always* executes
    // Jump occurs after it
    //
    // This is just a tag for whether to process a branch, it doesn't specify we are in delayslot
    bool m_is_branching;
    bool m_in_bds;
    u32 m_branch_pc;
    u32 calc_rel_branch(s16 d);
    constexpr void set_branch(u32 branch_pc);
    constexpr void set_branch_not_taken(); // need to know for deciding exn cause

    u32 m_rem_halt;
    void halt_for(u32 cycles);

    // Load delays: A load from memory, and loads from COPs have a lag of 1 instr
    // In psx-spx cop they debunk that cop loads should take 2 instrs?
    // Thats nice since this load queue can now handle both
    //
    // It's just a queue of 2
    struct Load {
        u32 val;
        usize reg;
        bool valid;
    };
    std::array<Load, 2> m_loads;
    void tick_load();
    void set_load(u32 data, usize reg);

    // Multiplication/division
    // These instructions don't finish immediately, instead run concurrently
    //
    // Depending on number size, the number of cycles changes
    //
    // Trying to read from hi/lo halts until the op is done
    //
    // Also, starting a new op while another is running voids the old one
    // So basically we immediatley calc the result then swap it in when the 
    // operation should be done
    //
    // Actually, i dont think these cycle specific timings really matter
    // since the processor halts when accessing hi/lo mid-operation it's the
    // same behavior as just instantly completing these. I've ignored other timings
    // like memory reads and it has been fine.
    u32 m_hi, m_lo;
    u32 m_hi_buf, m_lo_buf;
    u32 m_multdiv_rem_cycles;
    bool m_multdiv_active;
    static constexpr u32 DIV_NUM_CYCLES = 36;
    void multdiv_set_res(u32 lo, u32 hi, u32 cycles);
    bool multdiv_ensure_halt();
    void tick_multdiv();

    // intercept BIOS putchar call to print to (our) stdout
    bool detect_putchar();

    void process_instr(u32 instr);

    // util for mem ops bc Instr is private... should it be?? idk
    u32 get_effective_addr(Instr i);
    Device* get_mem_device();//give dummy if isolate cache is on

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
