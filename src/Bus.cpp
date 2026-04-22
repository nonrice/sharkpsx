#include "Bus.hpp"
#include "Panic.hpp"

namespace pse {

Bus::Bus(CPU* cpu, Device* ram) : m_cpu(cpu), m_ram(ram) {};

u8 Bus::read8(u32 addr) {
    MemAccess m = map_addr(addr);
    return m.dev->read8(m.addr);
}

u16 Bus::read16(u32 addr) {
    MemAccess m = map_addr(addr);
    return m.dev->read16(m.addr);
}

u32 Bus::read32(u32 addr) {
    MemAccess m = map_addr(addr);
    return m.dev->read32(m.addr);
}

void Bus::write8(u32 addr, u8 val){
    MemAccess m = map_addr(addr);
    m.dev->write8(m.addr, val);
}
void Bus::write16(u32 addr, u16 val){
    MemAccess m = map_addr(addr);
    m.dev->write16(m.addr, val);
}
void Bus::write32(u32 addr, u32 val){
    MemAccess m = map_addr(addr);
    m.dev->write32(m.addr, val);
}

Bus::MemAccess Bus::map_addr(u32 addr){
    addr &= 0x1FFFFFFF;
    
    if (addr < 0x00800000){
        return MemAccess {
            .dev = m_ram,
            .addr = addr & 0x1FFFFF
        };
    }

    throw Panic("accessing unmapped address");

    return MemAccess {
        .dev = nullptr,
        .addr = 0
    };
}

};
