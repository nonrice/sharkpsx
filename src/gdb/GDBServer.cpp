#include <numeric>

#include "strUtil.hpp"
#include "logging.hpp"
#include "GDBServer.hpp"

namespace pse {


void GDBServer::on_connect(Net::socket_t sock){
    SockIO sio(sock);
    char pack[1000];

    next_packet(pack, sio); // qsupported
    write_packet("PacketSize=1000;QStartNoAckMode+", sio);

    next_packet(pack, sio); // vcont
    write_packet("vCont;c;s;C;S", sio); 

    next_packet(pack, sio); // mustreplyempty
    write_packet("", sio);


}

ssize GDBServer::next_packet(std::span<char> buf, SockIO s){
    // assume, we are out of a packet already. We trash these
    ssize len1 = s.read_to(buf, '$');
    // ensure read to was successful i.e. no overflow
    // by checking last character is indeed the char read to
    if (len1 < 0 || buf[len1 - 1] != '$'){
        LOG_DBG("failure seeking to start of packet");
        return -1;
    }

    ssize len2 = s.read_to(buf, '#');
    if (len2 < 0 || buf[len2 - 1] != '#'){
        LOG_DBG("failure reading packet contents");
        return -1;
    }

    char checksum_str[2];
    if (s.read(checksum_str) < 0){
        LOG_DBG("failure reading checksum");
        return -1;
    }

    // process checksum
    u8 checksum = s2i(std::string_view(checksum_str, 2));
    u8 acc = 0;
    for (int i=0; i<len2-1; i++){ //since [len2-1] is '#'
        acc += buf[i];
    }
    if (acc != checksum){
        LOG_DBG("{}", std::string(buf.data(), len2));
        LOG_DBG("Checksum mismatch (given: {}, actual: {})", checksum, acc);
        LOG_DBG("Requesting retransmission");
        s.write("-");
        return -1;
    }

    // perform escaping
    usize len = 0;
    for (int i=0; i<len2-1; i++){
        if (buf[i] == 0x7d){
            // write the escaped char
            buf[len] = buf[i+1] ^ static_cast<char>(0x20);
            i += 1; // already processed next char, so skip
        } else {
            buf[len] = buf[i];
        }

        len += 1;
    }

    s.write("+");
    return len;
}

ssize GDBServer::write_packet(std::string_view buf, SockIO s){
    usize i=0;
    char esc_chars[] = { '$', '#', 0x7d };

    StrBuilder<400> res;
    res.push("$");

    while (i < buf.size()){
        usize nxt_esc = buf.size();

        for (auto ch : esc_chars){
            // find is -1 -> usize max on failure so this is perfect!
            nxt_esc = std::min(nxt_esc, static_cast<usize>(
                        find(buf.substr(i), ch)));
        }

        res.push(buf.substr(i, nxt_esc));
        if (nxt_esc == buf.size()){
            break;
        }

        res.push("\x7d");
        char esc_val = buf[nxt_esc] ^ 0x20;
        // dangerous technically! But push just copies, so it's fine
        res.push(std::string_view(&esc_val, 1)); 

        i = nxt_esc+1;
    }

    const std::string_view sp = res.to_span();
    u8 checksum = std::accumulate(sp.begin() + 1, sp.end(), 0);

    res.push("#");
    res.push_int_pad(checksum, 2);
    
    return s.write(res.to_span());
}


}
