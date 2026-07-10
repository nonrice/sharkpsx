#include "Controller.hpp"
#include "logging.hpp"

#include <cassert>

namespace pse {

void Controller::set_cs(bool val){
    LOG_DBG("Controller cs set {}", val);
    if (val == true){
        m_state = 0;
    }
}

u8 Controller::exch(u8 val){
    u8 res = 0;
    switch (m_state){
        case 0:
            assert(val == 0x01);
            res = 0xFF;
            break;
        case 1:
            assert(val == 0x42);
            res = ID & 0xFF;
            break;
        case 2:
            res = ((ID >> 8) & 0xFF);
            break;
        case 3:
            res = m_switches & 0xFF;
            break;
        case 4:
            res = ((m_switches >> 8) & 0xFF);
            break;
    }
    
    m_state += 1;
    if (m_state == 5){
        m_state = 1;
    }

    LOG_DBG("Controller exhcnaged: " HEX8 " in, " HEX8 " out", val, res);

    return res;
}

bool Controller::get_ack(){
    return true;
}

void Controller::set_switches(u16 s){
    m_switches = s;
}


}
