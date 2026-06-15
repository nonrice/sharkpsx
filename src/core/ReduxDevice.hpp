#pragma once

#include "MMIODevice.hpp"
#include "types.hpp"
#include "Panic.hpp"
#include "Bus.hpp"

namespace pse {

class ReduxDevice : public MMIODevice {
public:
    ReduxDevice(Bus* bus) : m_bus(bus) {}

    u32 read(u32 offset) override {
        switch (offset) {
            case 0:
                return m_port_id;
        }

        throw Panic("Illegal read to ReduxDevice");
    }

    void write(u32 offset, u32 val) override {
        switch (offset) {
            case 0:
                m_port_putchar = val;
                m_bus->putchar(static_cast<char>(m_port_putchar));
                return;
        }
        
        throw Panic("Illegal write to ReduxDevice");
    }

private:
    u32 m_port_id{0x50435358};
    u8 m_port_putchar;
    [[maybe_unused]] u8 m_port_dbg_break;
    [[maybe_unused]] u8 m_port_exit_code;
    [[maybe_unused]] u32 m_port_message_ptr;

    Bus* m_bus;
};

}
