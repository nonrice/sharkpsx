#include "Bus.hpp"
#include "Panic.hpp"
#include "logging.hpp"

namespace pse {

Bus::Bus(CPU* cpu, Device* ram, Device* bios_rom) :
    m_cpu(cpu), m_ram(ram), m_bios_rom(bios_rom) {};

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

void Bus::write8(u32 addr, u8 val){
    MemAccess m = map_addr(addr);
    m_write_addr = addr;
    m.dev->write8(m.addr, val);
}
void Bus::write16(u32 addr, u16 val){
    MemAccess m = map_addr(addr);
    m_write_addr = addr;
    m.dev->write16(m.addr, val);
}
void Bus::write32(u32 addr, u32 val){
    MemAccess m = map_addr(addr);
    m_write_addr = addr;
    m.dev->write32(m.addr, val);
}

Bus::MemAccess Bus::map_addr(u32 addr){
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
    }

    LOG_DBG("Unmapped addr " HEX32, addr);
    if (addr < 0x1f000000){
        throw Panic("accessing unmapped address");
    }

    //throw Panic("accessing unmapped address");

    return MemAccess {
        .dev = &m_dummy,
        .addr = 0
    };
}

};
