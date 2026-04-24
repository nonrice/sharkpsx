#include <array>
#include <algorithm>

#include "Panic.hpp"
#include "BIOSROM.hpp"

namespace pse {

BIOSROM::BIOSROM(){
    m_data = std::make_unique<std::array<u8, BIOSROM::SIZE>>();
}

u8 BIOSROM::read8(u32 addr){
    return (*m_data)[addr];
}

u16 BIOSROM::read16(u32 addr){
    return (*m_data)[addr] + ((*m_data)[addr+1] << 8);
}

u32 BIOSROM::read32(u32 addr){
    return (*m_data)[addr] +
        ((*m_data)[addr+1] << 8) +
        ((*m_data)[addr+2] << 16) +
        ((*m_data)[addr+3] << 24);
}

void BIOSROM::write8([[maybe_unused]] u32 addr, [[maybe_unused]] u8 val){
    throw Panic("Trying to write to BIOS ROM");
}

void BIOSROM::write16([[maybe_unused]] u32 addr, [[maybe_unused]] u16 val){
    throw Panic("Trying to write to BIOS ROM");
}

void BIOSROM::write32([[maybe_unused]] u32 addr, [[maybe_unused]] u32 val){
    throw Panic("Trying to write to BIOS ROM");
}

};
