#pragma once

#include "BitField.hpp"
#include "Fifo.hpp"
#include "MMIODevice.hpp"
#include "SIODev.hpp"
#include "IntCtl.hpp"

namespace pse {

class SIO : public MMIODevice {
public:
    SIO(Bus* bus, IntCtl* intc);

    u32 read(u32 offset) override;
    void write(u32 offset, u32 val) override;
    
    void set_dev(SIODev* p, usize i);
private:
    Bus* m_bus;
    IntCtl* m_intc;

    SIODev* m_devs[2]{};

    static constexpr usize RX_SZ = 8;
    Fifo<u8, RX_SZ> m_rx;

    union Mode {
        u16 val;

        bf16<0, 1> baudr_factor;
        bf16<2, 3> charlen;
        b16<4> par_en;
        b16<5> par_type;
        bf16<6, 7> stoplen;
        b16<8> clk_pol;
    };
    Mode m_mode;

    union Control {
        u16 val;

        b16<0> tx_en;
        b16<1> dtr_out_lvl;
        b16<2> rx_en;
        b16<3> tx_out_lvl;
        b16<4> ack;
        b16<5> rts_out_lvl;
        b16<6> reset;

        bf16<8, 9> rx_int_mode;
        b16<10> txint_en;
        b16<11> rxint_en;
        b16<12> dsrint_en;
        b16<13> port;
    };
    Control m_ctrl;

    union Stat {
        u32 val;

        b32<0> tx_notfull;
        b32<1> rx_notempty;
        b32<2> tx_idle;
        b32<3> rx_par_err;
        b32<4> rx_fifo_overrun; // sio1
        b32<5> rx_fifo_badstp; // ..
        b32<6> rx_input_level; //  l..
        b32<7> dsr_input_level;
        b32<8> cts_input_level;
        b32<9> irq;
        bf32<11, 31> baud;
    };
    Stat m_stat;

    u32 rd_rx();
    void wr_tx(u8 val);

    void wr_mode(u16 val);
    void wr_ctrl(u16 val);

    u32 rd_stat();
};

}
