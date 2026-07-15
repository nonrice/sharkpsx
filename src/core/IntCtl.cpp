#include "IntCtl.hpp"
#include "Panic.hpp"
#include "logging.hpp"

namespace pse {

u32 IntCtl::read(u32 offset){
    switch (offset){
        case 0x00:
            return m_stat.val;
        case 0x04:
            return m_mask.val;
        default:
            Panic("Unkown interrupt controll offset");
    }
    return 0xFFFFFFFF;
}

void IntCtl::write(u32 offset, u32 val){
    switch (offset){
        case 0x00:
            m_stat.val &= val;
            break;
        case 0x04:
            m_mask.val = val;
            break;
        default:
            Panic("Unknown interrupt controll write offset");
    }

}

void IntCtl::set_interrupt(Interrupt i){
    // LOG_DBG("Set interrupt! {}", static_cast<usize>(i));
    m_stat.val |= (1 << i);
}

bool IntCtl::pending(){
    return m_stat.val & m_mask.val;
}

};
