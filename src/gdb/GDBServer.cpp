#include "logging.hpp"
#include "GDBServer.hpp"

namespace pse {

static u64 hs2i(std::span<const char> s){
    u64 total = 0;
    u64 base = 1;
    for (ssize i=s.size()-1; i>=0; i--){
        if (s[i] >= '0' && s[i] <= '9'){
            total += base * (s[i] - '0');
        } else if (s[i] >= 'A' && s[i] <= 'F'){
            total += base * ((s[i] - 'A') + 10);
        } else {
            break;
        }

        base *= 16;
    }

    return total;
}

void GDBServer::on_connect(Net::socket_t sock){
    SockIO sio(sock);
    char pack[1000];

    while (true){
        ssize len = next_packet(pack, sio);

        if (len < 0){
            LOG_DBG("failure parsing next packet");
            return;
        }

        LOG("Packet: {}", std::string(pack, len));
    }

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
    u8 checksum = hs2i(checksum_str);
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
            buf[len] = buf[i+1] ^ 0x20;
            i += 1; // already processed next char, so skip
        } else {
            buf[len] = buf[i];
        }

        len += 1;
    }

    s.write("+");
    return len;
}

}
