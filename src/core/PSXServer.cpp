#include "PSXServer.hpp"
#include "strUtil.hpp"
#include "logging.hpp"

namespace pse {

PSXServer::PSXServer(BasicDebug& d, u16 port)
    : GDBServer(port), m_dbg(d)
{}

u32 to_le(u32 x){
    u8 b0 = x & 0xFF;
    u8 b1 = (x >> 8) & 0xFF;
    u8 b2 = (x >> 16) & 0xFF;
    u8 b3 = (x >> 24) & 0xFF;

    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

void PSXServer::handle_rsp(RSPHandler& h){
    while (true){
        auto sv_opt = h.next();
        if (!sv_opt){
            LOG_DBG("Couldn't read next packet");
            return;
        }

        auto sv = *sv_opt;

#ifdef GDB_DEBUG
        LOG_DBG("Recieved: {}", sv);
#endif

        switch (sv[0]){
            case 'g': {
                StrBuilder<2000> r;
                BasicDebug::RegDump rs = m_dbg.dump_regs();

                for (usize i=0; i<32; i++){
                    r.push_int_pad(to_le(rs.gp[i]), 8, 16);
                }
                r.push_int_pad(to_le(rs.sr), 8, 16);
                r.push_int_pad(to_le(rs.lo), 8, 16);
                r.push_int_pad(to_le(rs.hi), 8, 16);
                r.push_int_pad(to_le(rs.bad_vaddr), 8, 16);
                r.push_int_pad(to_le(rs.cause), 8, 16);
                r.push_int_pad(to_le(rs.pc), 8, 16);

                // fake FPU regs (neccssary)
                for (usize i=0; i<32; i++){
                    r.push_int_pad(0, 8, 16);
                }

                h.write(r.to_sv());
                break;
            }
            case 's': {
                m_dbg.step();
                h.write("S05");
                break;
            }
            case 'c': {
                // since we are in the server, sigcapture already enabled
                m_dbg.cont([&h]{
                        return h.pending_int();
                        });
                h.write("S05");
                break;
            }
            case 'Z': {
                char type = sv[1];
                sv = sv.substr(3);
                u32 addr = s2i(next_tok(sv, ','));
                LOG_DBG("Bkpt at " HEX32, addr);
                if (type == '0'){
                    m_dbg.set_breakpoint(addr);
                    h.write("OK");
                } else if (type == '2') {
                    m_dbg.set_watchpoint_write(addr);
                    h.write("OK");
                } else if (type == '3') {
                    m_dbg.set_watchpoint_read(addr);
                    h.write("OK");
                } else if (type == '4') {
                    m_dbg.set_watchpoint_read(addr);
                    m_dbg.set_watchpoint_write(addr);
                    h.write("OK");
                } else {
                    h.write("");
                }
                break;
            }
            case 'z': {
                char type = sv[1];
                sv = sv.substr(2);
                u32 addr = s2i(next_tok(sv, ','));
                if (type == '0'){
                    m_dbg.remove_breakpoint(addr);
                    h.write("OK");
                } else if (type == '2') {
                    m_dbg.remove_watchpoint_write(addr);
                    h.write("OK");
                } else if (type == '3') {
                    m_dbg.remove_watchpoint_read(addr);
                    h.write("OK");
                } else if (type == '4') {
                    m_dbg.remove_watchpoint_read(addr);
                    m_dbg.remove_watchpoint_write(addr);
                    h.write("OK");
                } else {
                    h.write("");
                }
                break;
            }
            case 'm': {
                sv = sv.substr(1);
                u32 addr = s2i(next_tok(sv, ','));
                usize num_bytes = s2i(next_tok(sv, ','));

                StrBuilder<128> r;
                for (usize offset=0; offset<num_bytes; offset++){
                    r.push_int_pad(m_dbg.read8(addr + offset), 2, 16);
                }
                h.write(r.to_sv());
                break;
            }
            case '?': {
                StrBuilder<64> r;
                r.push("S");
                r.push_int_pad(0x5, 2, 16);
                // r.push("thread:1;");
                h.write(r.to_sv());
                break;
            }
            case 'T':
                // h.write("OK");
                h.write("");
                break;
            case 'v':
                if (sv.starts_with("vCont")){
                    // h.write("vCont;c;C;s;S");
                    h.write("");
                } else if (sv.starts_with("vMustReplyEmpty")){
                    h.write("");
                }
                break;
            case 'H':
                // h.write("OK");
                h.write("OK");
                break;
            case 'q':
                if (sv.starts_with("qSupported")){
                    StrBuilder<512> r;
                    r.push("PacketSize=");
                    r.push_int(RSPHandler::PACK_SIZE, 16);
                    r.push(";QStartNoAckMode+");
                    // r.push(";qXfer:features:read+");
                    // r.push(";multiprocess+");
                    //TODO add custom conf?
                    h.write(r.to_sv());
                } else if (sv.starts_with("qfThreadInfo")){
                    h.write("");
                    // h.write("m1");
                } else if (sv.starts_with("qsThreadInfo")){
                    h.write("");
                    // h.write("l");
                } else if (sv.starts_with("qC")){
                    h.write("");
                    // h.write("QC1");
                } else if (sv.starts_with("qAttached")) {
                    h.write("");
                    // h.write("1");
                } else if (sv.starts_with("qSymbol::")) {
                    h.write("OK");
                } else { 
                    h.write("");
                }
                break;
            case 'Q':
                if (sv.starts_with("QStartNoAckMode")){
                    h.write("OK");
                    h.set_ack(false);
                }
                break;
            case 'D':
                return;
            default:
                LOG_DBG("Unknown command!!!");
                h.write("");
        }
    }
}



}
