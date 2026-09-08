#include "Bus.hpp"
#include "Panic.hpp"
#include "logging.hpp"

namespace pse {

Bus::Bus(
        CPU* cpu,
        Device* ram,
        Device* scratch,
        Device* bios_rom,
        Device* redux,
        Device* gpu,
        Device* sio0,
        Device* intc 
        ) :
    m_cpu(cpu),
    m_ram(ram),
    m_scratch(scratch),
    m_bios_rom(bios_rom),
    m_redux(redux),
    m_gpu(gpu),
    m_sio0(sio0),
    m_intc(intc),
    m_tty(nullptr),
    m_buserr(false) {
}

u8 Bus::read8(u32 addr) {
    MemAccess m = map_addr(addr);
    m_read_addr = addr;
    return m.dev->read8(m.addr);
}

u16 Bus::read16(u32 addr) {
    MemAccess m = map_addr(addr);
    m_read_addr = addr;
    return m.dev->read16(m.addr);
}

u32 Bus::read32(u32 addr) {
    MemAccess m = map_addr(addr);
    m_read_addr = addr;
    return m.dev->read32(m.addr);
}

void Bus::write8(u32 addr, u32 val){
    MemAccess m = map_addr(addr);
    m_write_addr = addr;
    m.dev->write8(m.addr, val);
}
void Bus::write16(u32 addr, u32 val){
    MemAccess m = map_addr(addr);
    m_write_addr = addr;
    m.dev->write16(m.addr, val);
}
void Bus::write32(u32 addr, u32 val){
    MemAccess m = map_addr(addr);
    m_write_addr = addr;
    m.dev->write32(m.addr, val);
}

void Bus::set_tty(std::ostream* tty){
    m_tty = tty;
}

void Bus::flush_tty(){
    if (m_tty){
        m_tty->flush();
    }
}

void Bus::putchar(char ch){
    if (m_tty){
        (*m_tty) << ch;
    }
}

Bus::MemAccess Bus::map_addr(u32 addr){
    if (addr == 0xFFFFFFFF){
        throw Panic("Accessing 0xFFFFFFFF, crash triggered");
    }

    // this little shit needs to go first
    // (cache registers)
    if (addr >= 0xFFFE0000){
        return MemAccess {
            .dev = &m_dummy,
            .addr = 0
        };
    }

    // throw everything else into kuseg
    addr &= 0x1FFFFFFF;
    if (addr < 0x00800000){
        return MemAccess {
            .dev = m_ram,
            .addr = addr & 0x1FFFFF
        };
    } else if (addr >= 0x1FC00000){
        return MemAccess {
            .dev = m_bios_rom,
            .addr = addr - 0x1FC00000
        };
    } else if (addr >= 0x1F802080 && addr <= 0x1F802084){
        return MemAccess {
            .dev = m_redux,
            .addr = addr - 0x1F802080
        };
    } else if (addr >= 0x1F801810 && addr <= 0x1f801814){
        return MemAccess {
            .dev = m_gpu,
            .addr = addr - 0x1f801810
        };
    } else if (addr >= 0x1F800000 && addr <= 0x1F800400){
        return MemAccess {
            .dev = m_scratch,
            .addr = addr - 0x1f800000
        };
    } else if (addr >= 0x1F801040 && addr <= 0x1f80104E){
        return MemAccess {
            .dev = m_sio0,
            .addr = addr - 0x1f801040
        };
    } else if (addr >= 0x1f801070 && addr <= 0x1f801074){
        return MemAccess {
            .dev = m_intc,
            .addr = addr - 0x1f801070
        };
    }

    m_buserr = true;
    LOG_DBG("Unmapped addr " HEX32, addr);
    if (addr < 0x1f000000){
        LOG_DBG("Addr: HEX32", addr);
        throw Panic("accessing unmapped address");
    }

    //throw Panic("accessing unmapped address");

    return MemAccess {
        .dev = &m_dummy,
        .addr = 0
    };
}

bool Bus::get_buserr(){
    bool r = m_buserr;
    m_buserr = false;
    return r;
}

};

