#include <cassert>
#include <functional>
#include <cmath>
#include <sstream>
#include <format>
#include <iostream>
#include <algorithm>
 
#include "types.hpp"
#include "CPU.hpp"
#include "Bus.hpp"
#include "Panic.hpp"

namespace pse {

CPU::CPU(Bus* bus) : m_bus(bus) {
    std::fill(m_regs.begin(), m_regs.end(), 0);

    m_rem_halt = 0;

    m_cur_pc = 0;
    m_next_pc = 4;

    m_is_branching = false;
    m_branch_pc = 0;

    m_loads[0].valid = false;
    m_loads[1].valid = false;

    m_hi = 0;
    m_lo = 0;
    m_hi_buf = 0;
    m_lo_buf = 0;
    m_multdiv_active = false;

    //TODO implement usermode restrictions
    m_mode = CPU::Mode::Kernel;
}

constexpr std::array<CPU::OpHandlerPtr, 64> CPU::m_primary_op_table = {{
    [0x00] = nullptr,          [0x01] = &CPU::op_BcondZ, [0x02] = &CPU::op_J,     [0x03] = &CPU::op_JAL,
    [0x04] = &CPU::op_BEQ,     [0x05] = &CPU::op_BNE,    [0x06] = &CPU::op_BLEZ,  [0x07] = &CPU::op_BGTZ,
    [0x08] = &CPU::op_ADDI,    [0x09] = &CPU::op_ADDIU,  [0x0A] = &CPU::op_SLTI,  [0x0B] = &CPU::op_SLTIU,
    [0x0C] = &CPU::op_ANDI,    [0x0D] = &CPU::op_ORI,    [0x0E] = &CPU::op_XORI,  [0x0F] = &CPU::op_LUI,
    [0x10] = &CPU::op_COP0,    [0x11] = &CPU::op_COP1,   [0x12] = &CPU::op_COP2,  [0x13] = &CPU::op_COP3,
    [0x20] = &CPU::op_LB,      [0x21] = &CPU::op_LH,     [0x22] = &CPU::op_LWL,   [0x23] = &CPU::op_LW,
    [0x24] = &CPU::op_LBU,     [0x25] = &CPU::op_LHU,    [0x26] = &CPU::op_LWR,   [0x28] = &CPU::op_SB,
    [0x29] = &CPU::op_SH,      [0x2A] = &CPU::op_SWL,    [0x2B] = &CPU::op_SW,    [0x2E] = &CPU::op_SWR,
    [0x30] = &CPU::op_LWC0,    [0x31] = &CPU::op_LWC1,   [0x32] = &CPU::op_LWC2,  [0x33] = &CPU::op_LWC3,
    [0x38] = &CPU::op_SWC0,    [0x39] = &CPU::op_SWC1,   [0x3A] = &CPU::op_SWC2,  [0x3B] = &CPU::op_SWC3
}};

constexpr std::array<CPU::OpHandlerPtr, 64> CPU::m_secondary_op_table = {{
    [0x00] = &CPU::op_SLL,     [0x02] = &CPU::op_SRL,    [0x03] = &CPU::op_SRA,   [0x04] = &CPU::op_SLLV,
    [0x06] = &CPU::op_SRLV,    [0x07] = &CPU::op_SRAV,   [0x08] = &CPU::op_JR,    [0x09] = &CPU::op_JALR,
    [0x0C] = &CPU::op_SYSCALL, [0x0D] = &CPU::op_BREAK,  [0x10] = &CPU::op_MFHI,  [0x11] = &CPU::op_MTHI,
    [0x12] = &CPU::op_MFLO,    [0x13] = &CPU::op_MTLO,   [0x18] = &CPU::op_MULT,  [0x19] = &CPU::op_MULTU,
    [0x1A] = &CPU::op_DIV,     [0x1B] = &CPU::op_DIVU,   [0x20] = &CPU::op_ADD,   [0x21] = &CPU::op_ADDU,
    [0x22] = &CPU::op_SUB,     [0x23] = &CPU::op_SUBU,   [0x24] = &CPU::op_AND,   [0x25] = &CPU::op_OR,
    [0x26] = &CPU::op_XOR,     [0x27] = &CPU::op_NOR,    [0x2A] = &CPU::op_SLT,   [0x2B] = &CPU::op_SLTU
}};

void CPU::set_pc(u32 pc){
    m_cur_pc = pc;
    m_next_pc = m_cur_pc + 4;
}

void CPU::tick_multdiv(){
    if (!m_multdiv_active){
        return;
    }

    assert(m_multdiv_rem_cycles != 0);

    m_multdiv_rem_cycles -= 1;
    if (m_multdiv_rem_cycles == 0){
        m_multdiv_active = false;
        m_hi = m_hi_buf;
        m_lo = m_lo_buf;
    }
}

void CPU::tick_load(){
    const Load& l0 = m_loads[0];
    if (l0.valid){
        m_regs[l0.reg] = l0.data;
    }

    m_loads[0] = m_loads[1];
    m_loads[1].valid = false;
}

void CPU::set_load(u32 data, usize reg){
    m_loads[1] = {
        .data = data,
        .reg = reg,
        .valid = true
    };
}

void CPU::reset_reg0(){
    m_regs[0] = 0;
}

void CPU::tick(){
    if (m_rem_halt > 0){
        m_rem_halt -= 1;
        return;
    }

    increment_pc();
    reset_reg0();
    if (!detect_putchar()){
        process_instr(m_bus->read32(m_cur_pc));
    }
    tick_load();
    tick_multdiv();
}

bool CPU::detect_putchar(){
    u32 addr = m_cur_pc & 0x1FFFFFFF;
    if ((addr == 0xA0 && m_regs[9] == 0x3C) || (addr = 0xB0 && m_regs[9] == 0x3d)){
        std::cout << static_cast<char>(m_regs[4]);
        m_cur_pc = m_regs[CPU::REG_RA];
        return true;
    }

    return false;
}

void CPU::process_instr(u32 instr){
    CPU::Instr i{instr};

    CPU::OpHandlerPtr handler_ptr;
    if (i.primary_opcode() == 0){
        handler_ptr = m_secondary_op_table[i.secondary_opcode()];
    } else {
        handler_ptr = m_primary_op_table[i.primary_opcode()];
    }

    if (handler_ptr == nullptr){
        trigger_exception(ExcCode::RI);
    } else {
        std::invoke(handler_ptr, this, i);
    }
}

inline u32 CPU::calc_rel_branch_pc(s16 d){
    return static_cast<u32>(
        static_cast<s32>(m_cur_pc) + 4 + 4 * static_cast<s32>(d)
    );
}

constexpr void CPU::set_branch(u32 branch_pc){
    assert(!m_is_branching);

    m_is_branching = true;
    m_branch_pc = branch_pc;
}

void CPU::increment_pc(){
    m_cur_pc = m_next_pc;

    if (m_is_branching){
        m_next_pc = m_branch_pc;
        m_is_branching = false;
    } else {
        m_next_pc = m_cur_pc + 4;
    }
}

void CPU::trigger_exception(CPU::ExcCode e) {
    throw Panic("got exception");
}

void CPU::op_BcondZ(CPU::Instr i){
    switch (i.rt()){
        case 0x00: // BLTZ
            if (static_cast<s32>(m_regs[i.rs()]) < 0){
                set_branch(calc_rel_branch_pc(i.imm16_signed()));
            }
            break;
        case 0x01: // BGEZ
            if (static_cast<s32>(m_regs[i.rs()]) >= 0){
                set_branch(calc_rel_branch_pc(i.imm16_signed()));
            }
            break;
        case 0x10: // BLTZAL
            if (static_cast<s32>(m_regs[i.rs()]) < 0){
                set_branch(calc_rel_branch_pc(i.imm16_signed()));
            }
            m_regs[REG_RA] = m_cur_pc + 8;
            break;
        case 0x11: // BGEZAL
            if (static_cast<s32>(m_regs[i.rs()]) >= 0){
                set_branch(calc_rel_branch_pc(i.imm16_signed()));
            }
            m_regs[REG_RA] = m_cur_pc + 8;
            break;
    }
}

void CPU::op_J(CPU::Instr i){
    set_branch((m_next_pc & 0xF0000000) + (i.imm26() * 4));
}

void CPU::op_JAL(CPU::Instr i){
    set_branch((m_next_pc & 0xF0000000) + (i.imm26() * 4));
    m_regs[CPU::REG_RA] = m_cur_pc + 8;
}


void CPU::op_BEQ(CPU::Instr i){
    if (m_regs[i.rs()] == m_regs[i.rt()]){
        set_branch(calc_rel_branch_pc(i.imm16_signed()));
    }
}

void CPU::op_BNE(CPU::Instr i){
    if (m_regs[i.rs()] != m_regs[i.rt()]){
        set_branch(calc_rel_branch_pc(i.imm16_signed()));
    }
}

void CPU::op_BLEZ(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs()]) <= 0){
        set_branch(calc_rel_branch_pc(i.imm16_signed()));
    }
}

void CPU::op_BGTZ(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs()]) > 0){
        set_branch(calc_rel_branch_pc(i.imm16_signed()));
    }
}

static bool add_overflows(u32 a, u32 b){
    s32 s_a = static_cast<s32>(a);
    s32 s_b = static_cast<s32>(b);
    u32 r = a + b;
    u32 s_r = static_cast<s32> (r);

    return (s_a>=0 && s_b>=0 && s_r<0) || (s_a<0 && s_b<0 && s_r>=0);
}

void CPU::op_ADDI(CPU::Instr i){
    u32 a = m_regs[i.rs()];
    u32 b = static_cast<u32>(i.imm16_signed());

    if (add_overflows(a, b)){
        trigger_exception(CPU::ExcCode::OVF);
    } else {
        m_regs[i.rd()] = a + b;
    }
}

void CPU::op_ADDIU(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rs()] + static_cast<u32>(i.imm16());
}

void CPU::op_SLTI(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs()]) < static_cast<s32>(i.imm16_signed())){
        m_regs[i.rt()] = 1;
    } else {
        m_regs[i.rt()] = 0;
    }
}

void CPU::op_SLTIU(CPU::Instr i){
    if (m_regs[i.rs()] < static_cast<u32>(static_cast<s32>(i.imm16_signed()))){
        m_regs[i.rt()] = 1;
    } else {
        m_regs[i.rt()] = 0;
    }
}

void CPU::op_ANDI(CPU::Instr i){
    m_regs[i.rt()] = m_regs[i.rs()] & static_cast<u32>(i.imm16());
}

void CPU::op_ORI(CPU::Instr i){
    m_regs[i.rt()] = m_regs[i.rs()] | static_cast<u32>(i.imm16());
}

void CPU::op_XORI(CPU::Instr i){
    m_regs[i.rt()] = m_regs[i.rs()] ^ static_cast<u32>(i.imm16());
}

void CPU::op_LUI(CPU::Instr i){
    m_regs[i.rt()] = i.rt() << 16;
}

void CPU::op_COP0(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_COP1(CPU::Instr i) {
    throw Panic ("unimplemented opcode");
}
void CPU::op_COP2(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_COP3(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};

u32 CPU::get_effective_addr(CPU::Instr i){
    return static_cast<u32>(static_cast<u32>(i.imm16()) + m_regs[i.rs()]);
}

void CPU::op_LB(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    u32 data = static_cast<u32>(static_cast<s32>(static_cast<s8>(
                    m_bus->read8(addr)
                )));
    
    set_load(data, i.rt());
}

void CPU::op_LH(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    if (addr % 2 != 0){
        trigger_exception(CPU::ExcCode::ADEL);
        return;
    }

    u32 data = static_cast<u32>(static_cast<s32>(static_cast<s16>(
                    m_bus->read16(addr)
                )));

    set_load(data, i.rt());
}

void CPU::op_LWL(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    u32 val = m_regs[i.rt()];
    u32 src = m_bus->read32(addr_base);
    switch (offset){
        case 0: val = (val & 0xFFFFFF00) | (src >> 24); break;
        case 1: val = (val & 0xFFFF0000) | (src >> 16); break;
        case 2: val = (val & 0xFF000000) | (src >> 8); break;
        case 3: val = src;
    }

    m_regs[i.rt()] = val;
}

void CPU::op_LW(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    if (addr % 4 != 0){
        trigger_exception(CPU::ExcCode::ADEL);
        return;
    }

    u32 data = m_bus->read32(addr);

    set_load(data, i.rt());
}

void CPU::op_LBU(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    u32 data = static_cast<u32>(m_bus->read8(addr));
    
    set_load(data, i.rt());
}

void CPU::op_LHU(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    if (addr % 2 != 0){
        trigger_exception(CPU::ExcCode::ADEL);
        return;
    }
    
    u32 data = static_cast<u32>(m_bus->read16(addr));
    
    set_load(data, i.rt());
}

void CPU::op_LWR(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    u32 val = m_regs[i.rt()];
    u32 src = m_bus->read32(addr_base);
    switch (offset){
        case 0: val = src; break;
        case 1: val = (val & 0x000000FF) | (src << 8); break;
        case 2: val = (val & 0x0000FFFF) | (src << 16); break;
        case 3: val = (val & 0x00FFFFFF) | (src << 24); break;
    }

    m_regs[i.rt()] = val;
}

void CPU::op_SB(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    m_bus->write8(addr, static_cast<u8>(m_regs[i.rt()] & 0xFF));
}

void CPU::op_SH(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    if (addr % 2 != 0){
        trigger_exception(CPU::ExcCode::ADES);
        return;
    }
    
    m_bus->write16(addr, static_cast<u16>(m_regs[i.rt()] & 0xFFFF));
}

void CPU::op_SWL(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    u32 val = m_bus->read32(addr_base);
    u32 src = m_regs[i.rt()];
    switch (offset){
        case 0: val = (val & 0xFFFFFF00) | (src >> 24); break;
        case 1: val = (val & 0xFFFF0000) | (src >> 16); break;
        case 2: val = (val & 0xFF000000) | (src >> 8); break;
        case 3: val = src;
    }

    m_bus->write32(addr_base, val);
};

void CPU::op_SW(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    if (addr % 4 != 0){
        trigger_exception(CPU::ExcCode::ADES);
        return;
    }
    
    m_bus->write32(addr, m_regs[i.rt()]);
}

void CPU::op_SWR(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    u32 val = m_bus->read32(addr_base);
    u32 src = m_regs[i.rt()];
    switch (offset){
        case 0: val = src; break;
        case 1: val = (val & 0x000000FF) | (src << 8); break;
        case 2: val = (val & 0x0000FFFF) | (src << 16); break;
        case 3: val = (val & 0x00FFFFFF) | (src << 24); break;
    }

    m_bus->write32(addr_base, val);
};

void CPU::op_LWC0(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_LWC1(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_LWC2(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_LWC3(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_SWC0(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_SWC1(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_SWC2(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_SWC3(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};

void CPU::op_SLL(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rt()] << i.imm5();
}

void CPU::op_SRL(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rt()] >> i.imm5();
}

void CPU::op_SRA(CPU::Instr i){
    m_regs[i.rd()] = static_cast<u32>(
        static_cast<s32>(m_regs[i.rt()]) >> i.imm5()
    );
}

void CPU::op_SLLV(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rt()] << (m_regs[i.rs()] & 0x1F);
}

void CPU::op_SRLV(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rt()] >> (m_regs[i.rs()] & 0x1F);
}

void CPU::op_SRAV(CPU::Instr i){
    m_regs[i.rd()] = static_cast<u32>(
        static_cast<s32>(m_regs[i.rt()]) >> (m_regs[i.rs()] & 0x1F)
    );
}

void CPU::op_JR(CPU::Instr i){
    set_branch(m_regs[i.rs()]);
}

void CPU::op_JALR(CPU::Instr i){
    set_branch(m_regs[i.rs()]);
    m_regs[i.rd()] = m_cur_pc + 8;
}

void CPU::op_SYSCALL(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_BREAK(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};

void CPU::halt_for(u32 cycles){
    m_rem_halt += cycles;
}

bool CPU::multdiv_ensure_halt(){
    if (m_multdiv_rem_cycles != 0){
        halt_for(m_multdiv_rem_cycles);
        return true;
    }
    return false;
}

void CPU::op_MFHI(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        m_regs[i.rd()] = m_hi;
    }
}

void CPU::op_MTHI(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        m_hi = m_regs[i.rs()];
    }
}

void CPU::op_MFLO(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        m_regs[i.rd()] = m_lo;
    }
}

void CPU::op_MTLO(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        m_lo = m_regs[i.rs()];
    }
}

void CPU::op_MULTU(CPU::Instr i){
    u64 a = static_cast<u64>(m_regs[i.rs()]);
    u64 b = static_cast<u64>(m_regs[i.rt()]);

    if (a <= 0x7FF){
        m_multdiv_rem_cycles = 6;
    } else if (a <= 0xFFFFF){
        m_multdiv_rem_cycles = 9;
    } else {
        m_multdiv_rem_cycles = 13;
    }

     u64 res = a * b;
     m_lo_buf = static_cast<u32>(res & 0xFFFFFFFF);
     m_hi_buf = static_cast<u32>(res >> 32);
}

void CPU::op_MULT(CPU::Instr i){
    s64 a = static_cast<s64>(m_regs[i.rs()]);
    s64 b = static_cast<s64>(m_regs[i.rt()]);

    if (std::abs(a) <= 0x7FF){
        m_multdiv_rem_cycles = 6;
    } else if (std::abs(a) <= 0xFFFFF){
        m_multdiv_rem_cycles = 9;
    } else {
        m_multdiv_rem_cycles = 13;
    }

    s64 res = a * b;
    m_lo_buf = static_cast<u32>(res & 0xFFFFFFFF);
    m_hi_buf = static_cast<u32>((res >> 32) & 0xFFFFFFFF);
}

void CPU::op_DIVU(CPU::Instr i){
    m_multdiv_rem_cycles = 36;

    u32 a = m_regs[i.rs()];
    u32 b = m_regs[i.rt()];

    if (b == 0){
        m_hi_buf = a;
        m_lo_buf = 0xFFFFFFFF;
        return;
    }

    m_lo_buf = a / b;
    m_hi_buf = a % b;
}

void CPU::op_DIV(CPU::Instr i){
    m_multdiv_rem_cycles = 36;

    s32 a = static_cast<s32>(m_regs[i.rs()]);
    s32 b = static_cast<s32>(m_regs[i.rt()]);

    if (b == 0){
        if (a >= 0){
            m_hi_buf = a;
            m_lo_buf = static_cast<u32>(-1);
        } else {
            m_hi_buf = a;
            m_lo_buf = 1;
        }

        return;
    }

    // negating int min
    if (b == -1 && a == 0x80000000i){
        m_hi_buf = 0;
        m_lo_buf = 0x80000000;

        return;
    }

    m_lo_buf = static_cast<u32>(a / b);
    m_hi_buf = static_cast<u32>(a % b);
}

void CPU::op_ADD(CPU::Instr i){
    u32 a = m_regs[i.rs()];
    u32 b = m_regs[i.rt()];

    if (add_overflows(a, b)){
        trigger_exception(CPU::ExcCode::OVF);
    } else {
        m_regs[i.rd()] = a+b;
    }
}

void CPU::op_ADDU(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rs()] + m_regs[i.rt()];
}

static bool sub_overflows(u32 a, u32 b) {
    s32 s_a = static_cast<s32>(a);
    s32 s_b = static_cast<s32>(b);
    u32 r = a - b;
    s32 s_r = static_cast<s32>(r);

    return (s_a>=0 && s_b<0 && s_r<0) || (s_a<0 && s_b>=0 && s_r>=0);
}

void CPU::op_SUB(CPU::Instr i){
    u32 a = m_regs[i.rs()];
    u32 b = m_regs[i.rt()];

    if (sub_overflows(a, -b)){
        trigger_exception(CPU::ExcCode::OVF);
    } else {
        m_regs[i.rd()] = a+b;
    }
}

void CPU::op_SUBU(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rs()] - m_regs[i.rt()];
}

void CPU::op_AND(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rs()] & m_regs[i.rt()];
}

void CPU::op_OR(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rs()] | m_regs[i.rt()];
}

void CPU::op_XOR(CPU::Instr i){
    m_regs[i.rd()] = m_regs[i.rs()] ^ m_regs[i.rt()];
}

void CPU::op_NOR(CPU::Instr i){
    m_regs[i.rd()] = ~(m_regs[i.rs()] | m_regs[i.rt()]);
}

void CPU::op_SLT(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs()]) < static_cast<s32>(m_regs[i.rt()])){
        m_regs[i.rd()] = 1;
    } else {
        m_regs[i.rd()] = 0;
    }
}

void CPU::op_SLTU(CPU::Instr i){
    if (m_regs[i.rs()] < m_regs[i.rt()]){
        m_regs[i.rd()] = 1;
    } else {
        m_regs[i.rd()] = 0;
    }
}


}

