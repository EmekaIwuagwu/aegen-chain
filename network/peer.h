#pragma once
#include <string>

namespace aegen {

class Peer {
public:
    std::string ip;
    int port;
    
    void connect();
    void disconnect();
};

}
