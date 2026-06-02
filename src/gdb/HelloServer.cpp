// TESTING!

#include "Server.hpp"
#include "SockIO.hpp"
#include "logging.hpp"

class HelloServer : public pse::Server {
public:
    using pse::Server::Server;

protected:
    void on_connect(pse::Net::socket_t conn){
        pse::SockIO in(conn);

        while (true){
            char buf[100]; // +1 for null term
            pse::ssize num_read = in.read_to(buf, '\n');
            if (num_read < 0){
                LOG("Read error");
                return;
            }

            LOG("You said: {}", std::string(buf, num_read - 1));
        }
    }
};

int main(){
    pse::Net::init();

    HelloServer s(8012);
    if (s.init() < 0){
        LOG("Failed to initialize server. Is the port in use?");
        return 1;
    }
    s.run();

    pse::Net::shutdown();
}
