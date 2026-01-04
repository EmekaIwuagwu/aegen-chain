#pragma once
#include <string>
#include <functional>
#include <map>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

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
    void workerThread(); // Thread pool worker

    std::map<std::string, Handler> handlers;
    std::atomic<bool> running;
    std::thread serverThread;
    
    // Thread pool to prevent DoS via thread exhaustion
    std::vector<std::thread> workerPool;
    std::queue<uintptr_t> taskQueue;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    static constexpr size_t THREAD_POOL_SIZE = 16;
};

}
