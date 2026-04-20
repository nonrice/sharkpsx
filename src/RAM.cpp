#include <array>
#include <algorithm>

#include "RAM.hpp"

namespace pse {

RAM::RAM(){
    m_data = std::make_unique<std::array<u8, RAM::SIZE>>();
}

u8 RAM::read8(u32 addr){
    return (*m_data)[addr];
}

u16 RAM::read16(u32 addr){
    return (*m_data)[addr] + ((*m_data)[addr+1] << 8);
}

u32 RAM::read32(u32 addr){
    return (*m_data)[addr] +
        ((*m_data)[addr+1] << 8) +
        ((*m_data)[addr+2] << 16) +
        ((*m_data)[addr+3] << 24);
}

void RAM::write8(u32 addr, u8 val){
    (*m_data)[addr] = val;
}

void RAM::write16(u32 addr, u16 val){
    (*m_data)[addr] = val & 0xFF;
    (*m_data)[addr+1] = (val >> 8) & 0xFF;
}

void RAM::write32(u32 addr, u32 val){
    (*m_data)[addr] = val & 0xFF;
    (*m_data)[addr+1] = (val >> 8) & 0xFF;
    (*m_data)[addr+2] = (val >> 16) & 0xFF;
    (*m_data)[addr+3] = (val >> 24) & 0xFF;
}

};
