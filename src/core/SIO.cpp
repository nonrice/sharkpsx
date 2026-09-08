#include <algorithm>

#include "SIO.hpp"
#include "logging.hpp"

namespace pse {

SIO::SIO(EventSched* sched, IntCtl* intc) : m_sched(sched), m_intc(intc) {
    m_stat.tx_notfull = true; 
    m_stat.tx_idle = true; 
    m_ctrl.tx_en = true;
}

u32 SIO::read(u32 offset) {
    switch (offset){
        case 0x00:
            return rd_rx();
            break;
        case 0x04:
            return m_stat.val;
            break;
        case 0x08:
            return m_mode.val;
            break;
        case 0x0A:
            return m_ctrl.val;
            break;
        default:
            LOG_DBG("Unkown sio read offset {}", offset);
    }

    return 0;
}

void SIO::write(u32 offset, u32 val){
    switch (offset){
        case 0x00:
            wr_tx(val);
            break;
        case 0x04:
            m_stat.val = val;
            break;
        case 0x08:
            wr_mode(val);
            break;
        case 0x0A:
            wr_ctrl(val);
            break;
        default:
            LOG_DBG("Unkown sio write offset {}", offset);
    }
}

u32 SIO::rd_rx(){
    u32 res = 0;
    for (usize i=0;
            i < std::min(m_rx.size(), static_cast<usize>(4));
            i++){
        res <<= 8;
        res |= m_rx.peek(i);
    }
    LOG_DBG("rd_rx result: " HEX32, res);

    if (!m_rx.empty()){
        // danger: diff pop behavior depending on read size
        m_rx.pop();
    }

    if (m_rx.empty()){
        m_stat.rx_notempty = false;
    }

    return res;
}

void SIO::set_dev(SIODev* p, usize i){
    assert(i <= 1);
    if (p == nullptr && m_devs[i] != nullptr){
        return;
    }
    
    m_devs[i] = p;
}

void SIO::wr_tx(u8 val){
    if (!m_ctrl.tx_en){
        return;
    }

    if (!m_ctrl.rx_en && !m_ctrl.dtr_out_lvl){
        return;
    }

    SIODev* sel = m_devs[m_ctrl.port];
    m_stat.tx_notfull = false; 
    
    m_sched->add([=,this](){
        if (!m_ctrl.tx_en){
            return;
        }

        u8 ret = (sel == nullptr) ? 0xFF : sel->exch(val);

        if (m_rx.size() == RX_SZ){
            m_rx.pop();
        }
        m_rx.push(ret);
        m_stat.rx_notempty = true;
        m_stat.tx_notfull = true;
    }, 100);

    m_sched->add([=,this](){
        if (!m_ctrl.tx_en){
            return;
        }

        if (m_ctrl.dsrint_en && sel != nullptr){
            m_intc->set_interrupt(IntCtl::CTRL);
            m_stat.irq = true;
        }
    }, 250);
}

void SIO::wr_mode(u16 val){
    m_mode.val = val;
}

void SIO::wr_ctrl(u16 val){
    // LOG_DBG("SIO_CTRL write " HEX16, val);
    m_ctrl.val = val;

    if (m_ctrl.reset){
        m_ctrl.val = 0;
        m_mode.val = 0;
        m_ctrl.tx_en = true;
        return;
    }

    if (m_ctrl.ack){
        m_stat.rx_par_err = false;
        m_stat.rx_fifo_overrun = false;
        m_stat.rx_fifo_badstp = false;
        m_stat.irq = false;
    }

    SIODev* sel = m_devs[m_ctrl.port];
    if (sel != nullptr){
        sel->set_cs(!m_ctrl.dtr_out_lvl);
    }
}



}
