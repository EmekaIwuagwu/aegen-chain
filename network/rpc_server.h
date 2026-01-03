#pragma once
#include <string>
#include <functional>
#include <map>
#include <thread>
#include <atomic>

namespace aegen {

class RPCServer {
public:
    using Handler = std::function<std::string(const std::string&)>;

    RPCServer();
    ~RPCServer();

    void start(int port);
    void stop();
    void registerEndpoint(const std::string& name, Handler handler);

private:
    void listenLoop(int port);
    void handleClient(uintptr_t clientSocket);

    std::map<std::string, Handler> handlers;
    std::atomic<bool> running;
    std::thread serverThread;
};

}
