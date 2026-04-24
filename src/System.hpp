#include "CPU.hpp"
#include "Bus.hpp"
#include "RAM.hpp"
#include "BIOSROM.hpp"

namespace pse {

class System {
public:
    System();

    void tick();

private:
    friend class Debugger;

    CPU m_cpu;
    Bus m_bus;
    RAM m_ram;
    BIOSROM m_bios_rom;

};

};
