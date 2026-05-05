#include <memory>
#include <numeric>
#include <cassert>

#include "MemBlockDevice.hpp"
#include "types.hpp"

namespace pse {
    MemBlockDevice::MemBlockDevice(usize size) :
        m_size(size), m_data(std::make_unique_for_overwrite<u8[]>(size))
    {
        //fill with patterend data to see uninitialized
        std::iota(m_data.get(), m_data.get() + size, 0);
    }

    usize MemBlockDevice::get_size(){
        return m_size;
    }

    u8 MemBlockDevice::read8(u32 addr){
        assert(addr < m_size);
        return m_data[addr];
    }

    u16 MemBlockDevice::read16(u32 addr){
        assert(addr+1 < m_size);
        return m_data[addr] + (m_data[addr+1] << 8);//promoted to u32 anyways
    }

    u32 MemBlockDevice::read32(u32 addr){
        assert(addr+3 < m_size);
        return m_data[addr] +
            (m_data[addr+1] << 8) +
            (m_data[addr+2] << 16) +
            (m_data[addr+3] << 24);
    }

    void MemBlockDevice::write8(u32 addr, u8 val){
        assert(addr < m_size);
        m_data[addr] = val;
    }

    void MemBlockDevice::write16(u32 addr, u16 val){
        assert(addr+1 < m_size);
        m_data[addr] = val & 0xFF;
        m_data[addr+1] = (val >> 8) & 0xFF;
    }

    void MemBlockDevice::write32(u32 addr, u32 val){
        assert(addr+3 < m_size);
        m_data[addr] = val & 0xFF;
        m_data[addr+1] = (val >> 8) & 0xFF;
        m_data[addr+2] = (val >> 16) & 0xFF;
        m_data[addr+3] = (val >> 24) & 0xFF;
    }
}
