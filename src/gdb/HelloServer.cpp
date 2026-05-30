#include "Server.hpp"

class HelloServer : public pse::Server {
public:
    using pse::Server::Server;

protected:
    void on_connect(pse::Net::socket_t conn){
        send(conn, "Hello world!\n", 13, 0);
    }
};

int main(){
    pse::Net::init();

    HelloServer s(8012);
    s.init();
    s.run();
}
