#pragma once

#include "MMIODevice.hpp"
#include "BitField.hpp"

namespace pse {

class IntCtl : public MMIODevice {
public:
    u32 read(u32 offset) override;
    void write(u32 offset, u32 val) override;

    enum Interrupt : usize {
        VBLANK = 0,
        GPU = 1,
        CDROM = 2,
        DMA = 3,
        TMR0 = 4,
        TMR1 = 5,
        TMR2 = 6,
        CTRL = 7,
        SIO = 8,
        SPU = 9,
        LIGHTPEN = 10
    };
    void set_interrupt(Interrupt i);
    bool pending();
private:
    union IRQ {
        u16 val;
        
        b16<VBLANK> vblank;
        b16<GPU> gpu;
        b16<CDROM> cdrom;
        b16<DMA> dma;
        b16<TMR0> tmr0;
        b16<TMR1> tmr1;
        b16<TMR2> tmr2;
        b16<CTRL> ctrl_recv;
        b16<SIO> sio;
        b16<SPU> spu;
        b16<LIGHTPEN> lightpen;
    };
    IRQ m_stat{};
    IRQ m_mask{};
};

}
