#pragma once

#include "types.hpp"

#include "System.hpp"

namespace pse {

class Debugger {
public:
    Debugger(System& system);

    void run();

private:
    System& m_system;
};
    
};
