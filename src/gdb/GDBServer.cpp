#include <numeric>
#include <poll.h>
#include <sys/socket.h>

#include "strUtil.hpp"
#include "logging.hpp"
#include "GDBServer.hpp"

namespace pse {

GDBServer::RSPHandler::RSPHandler(int sock) :
    m_s(SockIO(sock)), m_ack(true) {}

void GDBServer::RSPHandler::set_ack(bool ack){
    m_ack = ack;
}

void GDBServer::on_connect(int  sock){
    RSPHandler s(sock);

    handle_rsp(s);
}

std::optional<std::string_view> GDBServer::RSPHandler::next(){
    // assume, we are out of a packet already. We trash these
    ssize len1 = m_s.read_to(m_buf, '$');
    // ensure read to was successful i.e. no overflow
    // by checking last character is indeed the char read to
    if (len1 < 0 || m_buf[len1 - 1] != '$'){
        LOG_DBG("failure seeking to start of packet");
        return std::nullopt;
    }

    ssize len2 = m_s.read_to(m_buf, '#');
    if (len2 < 0 || m_buf[len2 - 1] != '#'){
        LOG_DBG("failure reading packet contents");
        return std::nullopt;
    }

    char checksum_str[2];
    if (m_s.read(checksum_str) < 0){
        LOG_DBG("failure reading checksum");
        return std::nullopt;
    }

    // process checksum
    u8 checksum = s2i(std::string_view(checksum_str, 2));
    u8 acc = 0;
    for (int i=0; i<len2-1; i++){ //since [len2-1] is '#'
        acc += m_buf[i];
    }
    if (acc != checksum){
        LOG_DBG("{}", std::string_view(m_buf, len2));
        LOG_DBG("Checksum mismatch (given: {}, actual: {})", checksum, acc);
        LOG_DBG("Requesting retransmission");
        if (m_ack){
            m_s.write("-");
        }
        return std::nullopt;
    }

    // perform escaping
    usize len = 0;
    for (int i=0; i<len2-1; i++){
        if (m_buf[i] == 0x7d){
            // write the escaped char
            m_buf[len] = m_buf[i+1] ^ static_cast<char>(0x20);
            i += 1; // already processed next char, so skip
        } else {
            m_buf[len] = m_buf[i];
        }

        len += 1;
    }

    if (m_ack){
        m_s.write("+");
    }
    return std::string_view(m_buf, len);
}

ssize GDBServer::RSPHandler::write(std::string_view buf){
#ifdef GDB_DEBUG
    LOG_DBG("Writing: {}", buf);
#endif
    usize i=0;
    char esc_chars[] = { '$', '#', 0x7d };

    StrBuilder<2 * PACK_SIZE> res;
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

    const std::string_view sp = res.to_sv();
    u8 checksum = std::accumulate(sp.begin() + 1, sp.end(), 0);

    res.push("#");
    res.push_int_pad(checksum, 2);
    
    ssize len = m_s.write(res.to_sv());

    if (m_ack){
        char resp[1];
        if (m_s.read(resp) < 0){
            LOG_DBG("error reading ack");
            return -1;
        }

        if (resp[0] != '+'){
            LOG_DBG("no ack received");
            return -1;
        }
    }

    return len;
}

bool GDBServer::RSPHandler::pending_int(){
    struct pollfd pfd{};
    int sock = m_s.get_sock();
    pfd.fd = sock;
    pfd.events = POLLIN;

    int poll_result = poll(&pfd, 1, 0);
    if (poll_result > 0 && (pfd.revents & POLLIN)){
        char next;

        ssize num_peeked = recv(sock, &next, 1, MSG_PEEK | MSG_DONTWAIT);
        if (num_peeked == 1 && next == 0x03){
            return true;
        }
    }

    return false;
}

}

