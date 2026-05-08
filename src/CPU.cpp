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
#include "logging.hpp"

namespace pse {

CPU::CPU(Bus* bus) : m_bus(bus) {
    // well, we need to start somehwere...
    m_cur_pc = 0;
    m_next_pc = 4;

    // emulation state, not cpu
    m_reg_pending_write = false;

    m_rem_halt = 0;

    m_is_branching = false;
    m_branch_pc = 0;
    m_in_bds = false;

    m_loads[0].valid = false;
    m_loads[1].valid = false;

    m_hi = 0;
    m_lo = 0;
    m_hi_buf = 0;
    m_lo_buf = 0;
    m_multdiv_active = false;
    // end
    
    reset_reg0();
    COP0::Status sr{0};
    sr.bev = true;
    m_cop0.regs[COP0::SR] = sr.val;
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

void CPU::write_reg(u32 num, u32 val){
    m_reg_pending_write = true;
    m_reg_pending_buf = val;
    m_reg_pending_num = num;
}

// only use direct write reg for these

void CPU::reset_reg0(){
    m_regs[0] = 0;//!
}

void CPU::tick_load(){
    const Load& l0 = m_loads[0];
    if (l0.valid){
        m_regs[l0.reg] = l0.val;
    }

    m_loads[0] = m_loads[1];
    m_loads[1].valid = false;
}

void CPU::tick_reg_writeback(){
    if (!m_reg_pending_write){
        return;
    }

    m_reg_pending_write = false;
    m_regs[m_reg_pending_num] = m_reg_pending_buf;
}


//----------//

void CPU::set_load(u32 data, usize reg){
    // this is how to correctly implement consecutive loads to the
    // same reg...
    //
    // idrk why it works like this but thats just how
    // somewhat related to load cancelling (see write_reg)
    if (m_loads[0].reg == reg){
        m_loads[0].valid = false;
    }

    m_loads[1] = {
        .val = data,
        .reg = reg,
        .valid = true
    };
}

void CPU::tick(){
    tick_multdiv();
    if (m_rem_halt > 0){
        m_rem_halt -= 1;
        return;
    }

    detect_putchar();
    reset_reg0();
    process_instr(m_bus->read32(m_cur_pc));
    tick_load();
    tick_reg_writeback(); // tick AFTER load to implement load cancel
    if (m_rem_halt > 0){
        // when halt is triggered by instr, we do not increment
        return;
    }
    increment_pc();
}

bool CPU::detect_putchar(){
    static constexpr u32 RAM_MASK = 0x1FFFFFFF;

    u32 pc_masked = m_cur_pc & RAM_MASK;
    if ((pc_masked == 0xA0 && m_regs[9] == 0x3C) || 
            (pc_masked == 0xB0 && m_regs[9] == 0x3D))
    {
        std::cout << static_cast<char>(m_regs[4]);
        std::cout.flush();
        return true;
        
    }

    return false;
}

void CPU::process_instr(u32 instr){
    CPU::Instr i{instr};

    CPU::OpHandlerPtr handler_ptr;
    if (i.primary == 0){
        handler_ptr = m_secondary_op_table[i.secondary];
    } else {
        handler_ptr = m_primary_op_table[i.primary];
    }

    if (handler_ptr == nullptr){
        trigger_exception(ExcCode::RI);
    } else {
        std::invoke(handler_ptr, this, i);
    }
}

inline u32 CPU::calc_rel_branch(s16 d){
    // spec says pc+4 instead of next pc, but this actually is
    // what happens irl. Anyways next pc is pc+4, when not in the bds
    //
    // However, this approach also correctly implements branch in the bds
    //
    // nextpc is the dest for the original branch, which is actually supposed
    // to be the base for the bds branch
    return static_cast<u32>(
        static_cast<s32>(m_next_pc) + 4 * static_cast<s32>(d)
    );
}

constexpr void CPU::set_branch(u32 branch_pc){
    assert(!m_is_branching);

    m_is_branching = true;
    m_branch_pc = branch_pc;
}

constexpr void CPU::set_branch_not_taken(){
    assert(!m_is_branching);
    
    m_is_branching = true;
    m_branch_pc = m_cur_pc + 8;
}

void CPU::increment_pc(){
    m_cur_pc = m_next_pc;

    if (m_in_bds){
        m_in_bds = false;
    }

    if (m_is_branching){
        m_next_pc = m_branch_pc;
        m_is_branching = false;
        m_in_bds = true;
    } else {
        m_next_pc = m_cur_pc + 4;
    }
}

void CPU::trigger_exception(CPU::ExcCode e, std::optional<u32> bad_addr) {
    LOG_DBG("Exception " HEX8, static_cast<u8>(e));

    COP0::Cause cause{0};
    cause.exc_code = e;

    u32 epc;

    if (m_in_bds){
        m_in_bds = false;
        epc = m_cur_pc - 4;
        cause.bd = true;
    } else {
        epc = m_cur_pc;
    }

    if (bad_addr){
        m_cop0.regs[COP0::BADA] = *bad_addr;
    }

    COP0::Status sr{m_cop0.regs[COP0::SR]};
    u32 exn_vec;
    if (sr.bev){
        exn_vec = 0xBFC00180;
    } else {
        exn_vec = 0x80000080;
    }

    // not set in cause... ip, sw, ce
    m_cop0.regs[COP0::CAUSE] = cause.val;
    m_cop0.regs[COP0::EPC] = epc;
    m_cop0.push_system_state(COP0::Ie::DISABLE, COP0::Ku::KERNEL);

    set_pc(exn_vec);
}

void CPU::rfe(){
    m_cop0.pop_system_state();
}

void CPU::COP0::push_system_state(CPU::COP0::Ie ie, CPU::COP0::Ku ku){
    Status sr{regs[COP0::SR]};

    sr.ieo = sr.iep;
    sr.kuo = sr.kup;
    sr.iep = sr.iec;
    sr.kup = sr.kuc;

    sr.iec = ie;
    sr.iec = ku;

    regs[COP0::SR] = sr.val;
}

void CPU::COP0::pop_system_state(){
    Status sr{regs[COP0::SR]};

    sr.iec = sr.iep;
    sr.kuc = sr.kup;
    sr.kup = sr.kuo;
    sr.iep = sr.ieo;
}

void CPU::op_BcondZ(CPU::Instr i){
    switch (i.rt){
        case 0x00: // BLTZ
            if (static_cast<s32>(m_regs[i.rs]) < 0){
                set_branch(calc_rel_branch(i.imm16));
                return;
            }
            break;
        case 0x01: // BGEZ
            if (static_cast<s32>(m_regs[i.rs]) >= 0){
                set_branch(calc_rel_branch(i.imm16));
                return;
            }
            break;
        case 0x10: // BLTZAL
            //unconditionally set RA
            // Note write reg wont update the reg until after the opcode
            //
            // This is actually good, because the spec is, if rs=RA, then 
            // use the original value of ra
            write_reg(Reg::RA, m_cur_pc + 8);
            if (static_cast<s32>(m_regs[i.rs]) < 0){
                set_branch(calc_rel_branch(i.imm16));
                return;
            }
            break;
        case 0x11: // BGEZAL
            write_reg(Reg::RA, m_cur_pc + 8); // see bltzal
            if (static_cast<s32>(m_regs[i.rs]) >= 0){
                set_branch(calc_rel_branch(i.imm16));
                return;
            }
            break;
    }

    // goes for all of above 4 (since returns if taken)
    set_branch_not_taken();
}

void CPU::op_J(CPU::Instr i){
    set_branch((m_next_pc & 0xF0000000) + (i.imm26 * 4));
}

void CPU::op_JAL(CPU::Instr i){
    set_branch((m_next_pc & 0xF0000000) + (i.imm26 * 4));
    write_reg(Reg::RA, m_cur_pc + 8);
}


void CPU::op_BEQ(CPU::Instr i){
    if (m_regs[i.rs] == m_regs[i.rt]){
        set_branch(calc_rel_branch(i.imm16));
    } else {
        set_branch_not_taken();
    }
}

void CPU::op_BNE(CPU::Instr i){
    if (m_regs[i.rs] != m_regs[i.rt]){
        set_branch(calc_rel_branch(i.imm16)); 
    } else {
        set_branch_not_taken();
    }
}

void CPU::op_BLEZ(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs]) <= 0){
        set_branch(calc_rel_branch(i.imm16));
    } else {
        set_branch_not_taken();
    }
}

void CPU::op_BGTZ(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs]) > 0){
        set_branch(calc_rel_branch(i.imm16));
    } else {
        set_branch_not_taken();
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
    u32 a = m_regs[i.rs];
    u32 b = static_cast<u32>(static_cast<s32>(
                static_cast<s16>(i.imm16)));

    if (add_overflows(a, b)){
        trigger_exception(CPU::ExcCode::OVF);
    } else {
        write_reg(i.rt, a + b);
    }
}

void CPU::op_ADDIU(CPU::Instr i){
    write_reg(i.rt, m_regs[i.rs] + static_cast<s16>(i.imm16));
}

void CPU::op_SLTI(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs]) < static_cast<s32>(
                static_cast<s16>(i.imm16))){
        write_reg(i.rt, 1);
    } else {
        write_reg(i.rt, 0);
    }
}

void CPU::op_SLTIU(CPU::Instr i){
    if (m_regs[i.rs] < static_cast<u32>(static_cast<s32>(
                    static_cast<s16>(i.imm16)))){
        write_reg(i.rt, 1);
    } else {
        write_reg(i.rt, 0);
    }
}

void CPU::op_ANDI(CPU::Instr i){
    write_reg(i.rt, m_regs[i.rs] & i.imm16);
}

void CPU::op_ORI(CPU::Instr i){
    write_reg(i.rt, m_regs[i.rs] | i.imm16);
}

void CPU::op_XORI(CPU::Instr i){
    write_reg(i.rt, m_regs[i.rs] ^ i.imm16);
}

void CPU::op_LUI(CPU::Instr i){
    write_reg(i.rt, i.imm16 << 16);
}

void CPU::op_COP0(CPU::Instr i) {
    switch (i.cop_primary) {
        case 0x00: // MFC0
            set_load(m_cop0.regs[i.rd], i.rt);
            return;
        case 0x02: // CFC0
            // control regs start at 32
            set_load(m_cop0.regs[32 + i.rd], i.rt);
            return;
        case 0x04: // MTC0
            m_cop0.regs[i.rd] = m_regs[i.rt];
            return;
        case 0x06: // CTC0
            m_cop0.regs[32 + i.rd] = m_regs[i.rt];
            return;
        case 0x10:
            if (i.secondary == 0x10){
                rfe();
                return;
            }
            break;
    };

    throw Panic("unimplemented cop0 opcode");
}

void CPU::op_COP1([[maybe_unused]] CPU::Instr i) {
    throw Panic ("There is no cop1 dummy");
}

void CPU::op_COP2(CPU::Instr i) {
    throw Panic("gte opcodes are not implemnted");
}

void CPU::op_COP3([[maybe_unused]] CPU::Instr i) {
    throw Panic("No cop 3");
}

u32 CPU::get_effective_addr(CPU::Instr i){
    return static_cast<u32>(
            static_cast<s16>(i.imm16) + m_regs[i.rs]);
}

void CPU::op_LB(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    u32 data = static_cast<u32>(static_cast<s32>(static_cast<s8>(
                    get_mem_device()->read8(addr)
                )));
    
    set_load(data, i.rt);
}

void CPU::op_LH(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    if (addr % 2 != 0){
        trigger_exception(CPU::ExcCode::ADEL);
        return;
    }

    u32 data = static_cast<u32>(static_cast<s32>(static_cast<s16>(
                    get_mem_device()->read16(addr)
                )));

    set_load(data, i.rt);
}

void CPU::op_LWL(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    bool extend = (m_loads[0].valid && m_loads[0].reg == i.rt);

    u32 val;
    if (extend){
        val = m_loads[0].val;
    } else {
        val = m_regs[i.rt];
    }

    u32 src = get_mem_device()->read32(addr_base);
    switch (offset){
        case 0: val = (val & 0x00FFFFFF) | (src << 24); break;
        case 1: val = (val & 0x0000FFFF) | (src << 16); break;
        case 2: val = (val & 0x000000FF) | (src << 8); break;
        case 3: val = src;
    }

    if (extend){
        m_loads[0].val = val;

        // this still does need to be delayed, so we move it to the back
        //
        // at this point there should be nothing at the back, since the only way
        // for that to happen is for *this* opcode to put it there. We set to false
        // just in case (possibly against uninitizlied stuff maybe)
        m_loads[1].valid = false;
        std::swap(m_loads[0], m_loads[1]);
    } else {
        set_load(val, i.rt);
    }
}

Device* CPU::get_mem_device(){
    COP0::Status sr{m_cop0.regs[COP0::SR]};
    if (sr.isc){
        return &m_dummy_dev;
    } else {
        return m_bus; 
    }
}

void CPU::op_LW(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    if (addr % 4 != 0){
        trigger_exception(CPU::ExcCode::ADEL);
        return;
    }

    u32 data = get_mem_device()->read32(addr);

    set_load(data, i.rt);
}

void CPU::op_LBU(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    u32 data = static_cast<u32>(get_mem_device()->read8(addr));
    
    set_load(data, i.rt);
}

void CPU::op_LHU(CPU::Instr i){
    u32 addr = get_effective_addr(i);
    if (addr % 2 != 0){
        trigger_exception(CPU::ExcCode::ADEL);
        return;
    }
    
    u32 data = static_cast<u32>(get_mem_device()->read16(addr));
    
    set_load(data, i.rt);
}

void CPU::op_LWR(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    bool extend = (m_loads[0].valid && m_loads[0].reg == i.rt);

    u32 val;
    if (extend){
        val = m_loads[0].val;
    } else {
        val = m_regs[i.rt];
    }

    u32 src = get_mem_device()->read32(addr_base);
    switch (offset){
        case 0: val = src; break;
        case 1: val = (val & 0xFF000000) | (src >> 8); break;
        case 2: val = (val & 0xFFFF0000) | (src >> 16); break;
        case 3: val = (val & 0xFFFFFF00) | (src >> 24); break;
    }

    if (extend){
        m_loads[0].val = val;

        m_loads[1].valid = false;
        std::swap(m_loads[0], m_loads[1]); // (see lwl)
    } else {
        set_load(val, i.rt);
    }
}

void CPU::op_SB(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    get_mem_device()->write8(addr, m_regs[i.rt]);
}

void CPU::op_SH(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    if (addr % 2 != 0){
        trigger_exception(CPU::ExcCode::ADES);
        return;
    }
    
    get_mem_device()->write16(addr, m_regs[i.rt]);
}

void CPU::op_SWL(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    u32 val = get_mem_device()->read32(addr_base);
    u32 src = m_regs[i.rt];
    switch (offset){
        case 0: val = (val & 0x00FFFFFF) | (src << 24); break;
        case 1: val = (val & 0x0000FFFF) | (src << 16); break;
        case 2: val = (val & 0x000000FF) | (src << 8); break;
        case 3: val = src;
    }

    get_mem_device()->write32(addr_base, val);
};

void CPU::op_SW(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    if (addr % 4 != 0){
        trigger_exception(CPU::ExcCode::ADES);
        return;
    }
    
    get_mem_device()->write32(addr, m_regs[i.rt]);
}

void CPU::op_SWR(CPU::Instr i) {
    u32 addr = get_effective_addr(i);

    u32 offset = addr % 4;
    u32 addr_base = addr - offset;

    u32 val = get_mem_device()->read32(addr_base);
    u32 src = m_regs[i.rt];
    switch (offset){
        case 0: val = src; break;
        case 1: val = (val & 0xFF000000) | (src >> 8); break;
        case 2: val = (val & 0xFFFF0000) | (src >> 16); break;
        case 3: val = (val & 0xFFFFFF00) | (src >> 24); break;
    }

    get_mem_device()->write32(addr_base, val);
};

void CPU::op_LWC0(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_LWC1([[maybe_unused]] CPU::Instr i) {
    throw Panic("theres no cop1");
};
void CPU::op_LWC2(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_LWC3([[maybe_unused]] CPU::Instr i) {
    throw Panic("theres no cop3");
};
void CPU::op_SWC0(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_SWC1([[maybe_unused]] CPU::Instr i) {
    throw Panic("there's no cop1");
};
void CPU::op_SWC2(CPU::Instr i) {
    throw Panic("unimplemented opcode");
};
void CPU::op_SWC3([[maybe_unused]] CPU::Instr i) {
    throw Panic("there's no cop3");
};

void CPU::op_SLL(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rt] << i.imm5);
}

void CPU::op_SRL(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rt] >> i.imm5);
}

void CPU::op_SRA(CPU::Instr i){
    m_regs[i.rd] = static_cast<u32>(
        static_cast<s32>(m_regs[i.rt]) >> i.imm5
    );
}

void CPU::op_SLLV(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rt] << (m_regs[i.rs] & 0x1F));
}

void CPU::op_SRLV(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rt] >> (m_regs[i.rs] & 0x1F));
}

void CPU::op_SRAV(CPU::Instr i){
    m_regs[i.rd] = static_cast<u32>(
        static_cast<s32>(m_regs[i.rt]) >> (m_regs[i.rs] & 0x1F)
    );
}

void CPU::op_JR(CPU::Instr i){
    set_branch(m_regs[i.rs]);
}

void CPU::op_JALR(CPU::Instr i){
    set_branch(m_regs[i.rs]);
    write_reg(i.rd, m_cur_pc + 8);
}

void CPU::op_SYSCALL([[maybe_unused]] CPU::Instr i) {
    trigger_exception(ExcCode::SYS);
};
void CPU::op_BREAK([[maybe_unused]] CPU::Instr i) {
    trigger_exception(ExcCode::BP);
};

void CPU::halt_for(u32 cycles){
    m_rem_halt += cycles;
}

bool CPU::multdiv_ensure_halt(){
    if (m_multdiv_active){
        halt_for(m_multdiv_rem_cycles);
        return true;
    }
    return false;
}

void CPU::op_MFHI(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        write_reg(i.rd, m_hi);
    }
}

void CPU::op_MTHI(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        m_hi = m_regs[i.rs];
    }
}

void CPU::op_MFLO(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        write_reg(i.rd, m_lo);
    }
}

void CPU::op_MTLO(CPU::Instr i){
    if (!multdiv_ensure_halt()){
        m_lo = m_regs[i.rs];
    }
}

void CPU::multdiv_set_res(u32 lo, u32 hi, u32 cycles){
    m_multdiv_rem_cycles = cycles;
    m_hi_buf = hi;
    m_lo_buf = lo;
    m_multdiv_active = true;
}

void CPU::op_MULTU(CPU::Instr i){
    u64 a = static_cast<u64>(m_regs[i.rs]);
    u64 b = static_cast<u64>(m_regs[i.rt]);

    u32 cycles;
    if (a <= 0x7FF){
        cycles = 6;
    } else if (a <= 0xFFFFF){
        cycles = 9;
    } else {
        cycles = 13;
    }

     u64 res = a * b;
     multdiv_set_res(
             static_cast<u32>(res & 0xFFFFFFFF),
             static_cast<u32>(res >> 32),
             cycles
         );
}

void CPU::op_MULT(CPU::Instr i){
    s64 a = static_cast<s64>(m_regs[i.rs]);
    s64 b = static_cast<s64>(m_regs[i.rt]);

    u32 cycles;
    if (std::abs(a) <= 0x7FF){
        cycles = 6;
    } else if (std::abs(a) <= 0xFFFFF){
        cycles = 9;
    } else {
        cycles = 13;
    }

    s64 res = a * b;
    multdiv_set_res(
            static_cast<u32>(res & 0xFFFFFFFF),
            static_cast<u32>((res >> 32) & 0xFFFFFFFF),
            cycles
        );
}

void CPU::op_DIVU(CPU::Instr i){
    u32 a = m_regs[i.rs];
    u32 b = m_regs[i.rt];

    if (b == 0){
        multdiv_set_res(
                0xFFFFFFFF,
                a,
                DIV_NUM_CYCLES);
        return;
    }

    multdiv_set_res(a/b, a%b, DIV_NUM_CYCLES);
}

void CPU::op_DIV(CPU::Instr i){
    s32 a = static_cast<s32>(m_regs[i.rs]);
    s32 b = static_cast<s32>(m_regs[i.rt]);

    if (b == 0){
        if (a >= 0){
            multdiv_set_res(
                    static_cast<u32>(-1),
                    a,
                    DIV_NUM_CYCLES);
        } else {
            multdiv_set_res(
                    1,
                    a,
                    DIV_NUM_CYCLES);
        }

        return;
    }

    // negating int min
    if (b == -1 && a == 0x80000000i){
        multdiv_set_res(
                0x80000000,
                0,
                DIV_NUM_CYCLES);
        return;
    }
    
    multdiv_set_res(
            static_cast<u32>(a / b),
            static_cast<u32>(a % b),
            DIV_NUM_CYCLES
        );
}

void CPU::op_ADD(CPU::Instr i){
    u32 a = m_regs[i.rs];
    u32 b = m_regs[i.rt];

    if (add_overflows(a, b)){
        trigger_exception(CPU::ExcCode::OVF);
    } else {
        write_reg(i.rd, a+b);
    }
}

void CPU::op_ADDU(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rs] + m_regs[i.rt]);
}

static bool sub_overflows(u32 a, u32 b) {
    s32 s_a = static_cast<s32>(a);
    s32 s_b = static_cast<s32>(b);
    u32 r = a - b;
    s32 s_r = static_cast<s32>(r);

    return (s_a>=0 && s_b<0 && s_r<0) || (s_a<0 && s_b>=0 && s_r>=0);
}

void CPU::op_SUB(CPU::Instr i){
    u32 a = m_regs[i.rs];
    u32 b = m_regs[i.rt];

    if (sub_overflows(a, -b)){
        trigger_exception(CPU::ExcCode::OVF);
    } else {
        write_reg(i.rd, a+b);
    }
}

void CPU::op_SUBU(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rs] - m_regs[i.rt]);
}

void CPU::op_AND(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rs] & m_regs[i.rt]);
}

void CPU::op_OR(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rs] | m_regs[i.rt]);
}

void CPU::op_XOR(CPU::Instr i){
    write_reg(i.rd, m_regs[i.rs] ^ m_regs[i.rt]);
}

void CPU::op_NOR(CPU::Instr i){
    write_reg(i.rd, ~(m_regs[i.rs] | m_regs[i.rt]));
}

void CPU::op_SLT(CPU::Instr i){
    if (static_cast<s32>(m_regs[i.rs]) < static_cast<s32>(m_regs[i.rt])){
        write_reg(i.rd, 1);
    } else {
        write_reg(i.rd, 0);
    }
}

void CPU::op_SLTU(CPU::Instr i){
    if (m_regs[i.rs] < m_regs[i.rt]){
        write_reg(i.rd, 1);
    } else {
        write_reg(i.rd, 0);
    }
}


}

